#include <Arduino.h>
#include <math.h>

#define GPIO_LED1   18
#define GPIO_LED2   19
#define GPIO_LED3   21

#define GPIO_BTN1   27
#define GPIO_BTN2   26
#define GPIO_BTN3   25

#define N_BUTTONS   3

#define BNT_MASK    0B0111
#define BNT1_MASK   0B0001
#define BNT2_MASK   0B0010
#define BNT3_MASK   0B0100

static void IRAM_ATTR isr_gpio1();
static void IRAM_ATTR isr_gpio2();
static void IRAM_ATTR isr_gpio3();

typedef void (*isr_t)();

static struct s_button
{
  int       btn_gpio;
  int       led_gpio;
  uint32_t  mask;
  isr_t     isr;
} buttons[N_BUTTONS] = {
  { GPIO_BTN1, GPIO_LED1, BNT1_MASK, isr_gpio1 },
  { GPIO_BTN2, GPIO_LED2, BNT2_MASK, isr_gpio2 },
  { GPIO_BTN3, GPIO_LED3, BNT3_MASK, isr_gpio3 }
};

static TaskHandle_t htask1;

static void task1(void *arg) {
  uint32_t rv;
  BaseType_t rc;

  for (;;) {
    rc = xTaskNotifyWait(0, BNT_MASK, &rv, portMAX_DELAY);
    assert( rc == pdPASS );

    printf("Task notified: rv=%u\n", unsigned(rv));
    
    for( auto& button: buttons) {
      if ( rv & button.mask ) {
        printf(" Button %u notified, reads %d\n",
        unsigned(log2(button.mask)+1),digitalRead(button.btn_gpio));
        digitalWrite(button.led_gpio,
          digitalRead(button.btn_gpio));
      }
    }
  }
}

inline static BaseType_t IRAM_ATTR isr_gpiox(uint8_t gpiox) {
  s_button& button = buttons[gpiox];
  BaseType_t woken = pdFALSE;

  xTaskNotifyFromISR(htask1, button.mask, eSetBits, &woken);
  return woken;
}

static void IRAM_ATTR isr_gpio1() {
  if ( isr_gpiox(0) )
    portYIELD_FROM_ISR();
}

static void IRAM_ATTR isr_gpio2() {
  if ( isr_gpiox(1) )
    portYIELD_FROM_ISR();
}

static void IRAM_ATTR isr_gpio3() {
  if ( isr_gpiox(2) )
    portYIELD_FROM_ISR();
}

void setup() {
  int app_cpu = xPortGetCoreID();
  BaseType_t rc;

  for( auto& button: buttons) {
    
    pinMode(button.led_gpio,OUTPUT);
    digitalWrite(button.led_gpio,HIGH);

    pinMode(button.btn_gpio,INPUT_PULLUP);

    attachInterrupt(button.btn_gpio, button.isr, CHANGE);
  }

  delay(2000);
  printf("tasknfy5.ino\n");

  // Start event task
  rc = xTaskCreatePinnedToCore(
    task1,       // Function
    "evtask",     // Name
    4096,         // Stak Size
    nullptr,      // Argument
    1,            // Priority
    &htask1,      // Handle ptr
    app_cpu       // CPU
  );
  assert( rc == pdPASS );
}

// Not used
void loop() {
  vTaskDelete(nullptr);
}