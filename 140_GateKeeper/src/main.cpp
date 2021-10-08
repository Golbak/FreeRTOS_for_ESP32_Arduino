#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

// Buttons:
#define BUTTON0     9   // PA0 on dev1
#define BUTTON1     8   // PB0 on dev0
#define BUTTON2     16  // PB1 on dev0
#define NB          3   // Number of Buttons

// LEDs:
#define LED0        1     // PA1 on dev0
#define LED1        2     // PA2 on dev0
#define LED2        3     // PA3 on dev0

#define LED3        27    // PB4 on dev1

// MCP23X17 addresses
#define DEV0        0x20
#define DEV1        0x21

#define N_DEV       2           // PCF8574
#define GATEKRDY    0b0001      // Gatekeeper ready

#define IO_RDY      0b0001      // Task notification
#define IO_ERROR    0b0010      // Task notification
#define IO_BIT      0b0100      // Task notification

#define STOP        int(1)      // Arduino I2C API

#define PCF_ERROR   -1

static struct s_gatekeeper {
  EventGroupHandle_t  grpevt;         // Group Event handle(*)
  QueueHandle_t       queue;          // Request queue handle
} gatekeeper = {
  nullptr, nullptr
};

// Message struct for Message/Response queue
struct s_ioport {
  uint8_t input : 1;  // 1=input else output
  uint8_t value : 1;  // Bit value
  uint8_t error : 1;  // Error bit, when used
  uint8_t port : 5;   // Port number
  TaskHandle_t htask; // Reply Task handle
};

// Gatekeeper task: Owns I2C bus operations and
// state management of the PCF8574P devices.

static void gatekeeper_task(void *arg) {
  static Adafruit_MCP23X17 mcp[N_DEV];
  uint8_t devx, pinx;         // Device index, pin index
  s_ioport ioport;            // Queue message pointer
  uint32_t notify;            // Task Notification word
  BaseType_t rc;              // Return code

  // Create API communitation queues
  gatekeeper.queue = xQueueCreate(8, sizeof ioport);
  assert(gatekeeper.queue);

  // Start I2C Bus Support
  Wire.begin();

  // Configure all GPIOs
  assert(mcp[0].begin_I2C(DEV0));  // I2C Fail?
  assert(mcp[1].begin_I2C(DEV1));  // I2C Fail?
  
  mcp[BUTTON0 / 16].pinMode(BUTTON0 % 16, INPUT_PULLUP);
  mcp[BUTTON1 / 16].pinMode(BUTTON1 % 16, INPUT_PULLUP);
  mcp[BUTTON2 / 16].pinMode(BUTTON2 % 16, INPUT_PULLUP);

  mcp[LED0 / 16].pinMode(LED0 % 16, OUTPUT);
  mcp[LED1 / 16].pinMode(LED1 % 16, OUTPUT);
  mcp[LED2 / 16].pinMode(LED2 % 16, OUTPUT);
  mcp[LED3 / 16].pinMode(LED3 % 16, OUTPUT);

  // Indicates gatekeeper ready for use:
  xEventGroupSetBits(gatekeeper.grpevt, GATEKRDY);

  // Event loop
  for (;;) {
    notify = 0;

    // Receive command:
    rc = xQueueReceive(gatekeeper.queue, &ioport, portMAX_DELAY);
    assert(rc == pdPASS);

    devx = ioport.port / 16;        // device index
    pinx = ioport.port % 16;        // pin index 
    assert(devx < N_DEV);

    if ( ioport.input ) {
      // COMMAND: Read a GPIO BIT:
      ioport.value = mcp[devx].digitalRead(pinx);
      ioport.error = false; // Successful
    } else {
      // COMMAND: WRITE A GPIO BIT
      mcp[devx].digitalWrite(pinx, (ioport.value&1));
      ioport.error = false; // Successful
    }

    notify = IO_RDY;
    if ( ioport.error )
      notify |= IO_ERROR;
    if ( ioport.value )
      notify |= IO_BIT;
    
    // Notify client about completion
    if ( ioport.htask ) {
      xTaskNotify(
        ioport.htask,
        notify,
        eSetValueWithOverwrite
      );
    }
  }
}

// Block caller until gatekeeper ready
static void pcf8574_wait_ready() {

  xEventGroupWaitBits(
    gatekeeper.grpevt,  // Event group handle
    GATEKRDY,           // Bits to wait for
    0,                  // Bits to clear
    pdFAIL,             // Wait for all bits
    portMAX_DELAY       // Wait forever
  );
}

// RETURNS:
// 0          - GPIO set/is low
// 1          - GPIO set/is high
// PCF_ERROR  - Failed operation
inline static short pcf8574_send(s_ioport* ioport) {
  BaseType_t rc;
  uint32_t notify;          // Returned notification word

  pcf8574_wait_ready(); // Block until ready

  rc = xQueueSendToBack(
    gatekeeper.queue,
    ioport,
    portMAX_DELAY
  );
  assert(rc == pdPASS);

  // Wait to be notified:
  rc = xTaskNotifyWait(
    0,                      // no clear on entry
    IO_RDY|IO_ERROR|IO_BIT, // clear on exit
    &notify,
    portMAX_DELAY
  );
  assert(rc == pdTRUE);

  return (notify & IO_ERROR) ? PCF_ERROR : !!(notify & IO_BIT);
}

// Get GPIO pin status:
// RETURNS:
// 0          - GPIO is low
// 1          - GPIO is high
// PCF_ERROR  - Failed to read GPIO
static short pcf8574_get(uint8_t port) {
  s_ioport ioport;    // Port pin (0..15)

  assert(port < 32);
  ioport.input = true;  // Read request
  ioport.port = port;   // 0..15 port pin
  ioport.htask = xTaskGetCurrentTaskHandle();

  return  pcf8574_send(&ioport);
}

// Write GPIO pin for a PCF8574 port:
// RETURNS:
// 0 or 1:    Succesful bit write
// PCF_ERROR  Failed GPIO write
static short pcf8574_put(uint8_t port, bool value) {
  s_ioport ioport;          // Port pin (0..15)
  
  assert(port < 32);
  ioport.input = false;   // Read request
  ioport.value = value;   // Bit value
  ioport.port = port;     // 0..15 port pin
  ioport.htask = xTaskGetCurrentTaskHandle();

  return  pcf8574_send(&ioport);
}

// User task: Uses gatekeeper task for
// reading/writing PCF8574 port pins.
//
// Pins:
// 0..7   Device 0 (address DEV0)
// 8..15  Device 1 (address DEV1)
//
// Detect button press, and then activate
// corresponding LED.
static void usr_task1(void *argp) {
  static const struct s_state {
    uint8_t button;
    uint8_t led;
  } states[3] = {
    { BUTTON0, LED0 },
    { BUTTON1, LED1 },
    { BUTTON2, LED2 }
  };
  short rc;

  // Initialize all LEDs high (inactive):
  for ( unsigned bx=0; bx<NB; ++bx ) {
    rc = pcf8574_put(states[bx].led, true);
    assert(rc != PCF_ERROR);
  }

  // Monitor push buttons:
  for (;;) {
    for ( unsigned bx=0; bx<NB; ++bx ) {
      rc = pcf8574_get(states[bx].button);
      assert(rc != PCF_ERROR);
      rc = pcf8574_put(states[bx].led, rc&1);
      assert(rc != PCF_ERROR);
    }
  }
}

static void task2(void* argp) {
  uint8_t state;
  short rc;

  for (;;) {
    delay(500);
    rc = pcf8574_get(LED3);
    assert(rc != PCF_ERROR);
    state = !(rc & 1);
    pcf8574_put(LED3, state);
  }
}


// Initialize Application
void setup() {
  int app_cpu = xPortGetCoreID();
  BaseType_t rc; // return code

  // Create Event Group for Gatekeeper
  // This must be created before any using
  // task execute.
  gatekeeper.grpevt = xEventGroupCreate();
  assert(gatekeeper.grpevt);

  delay(2000);
  printf("\nGatekeeper program\n");

  // Start the gatekeeper task
  rc = xTaskCreatePinnedToCore(
    gatekeeper_task,
    "gatekeeper",
    2000,
    nullptr,
    2,
    nullptr,
    app_cpu
  );
  assert(rc == pdPASS);
 
  // Start  user task 1
  rc = xTaskCreatePinnedToCore(
    usr_task1,
    "usr_task1",
    2000,
    nullptr,
    1,
    nullptr,
    app_cpu
  );
  assert(rc == pdPASS);
  
  // Start  user task 2
  rc = xTaskCreatePinnedToCore(
    task2,
    "task2",
    2000,
    nullptr,
    1,
    nullptr,
    app_cpu
  );
  assert(rc == pdPASS);
  
}

// Not used:
void loop() {
  vTaskDelete(nullptr);
}