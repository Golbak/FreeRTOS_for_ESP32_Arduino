#include <FreeRTOS.h>

#define GPIO_LED    12
#define GPIO_BUTTON 25

#define DEBOUNCE_MASK 0x7FFFFFFF

static QueueHandle_t qhQueue;

//
//  BUtton Debouncing task;
//
static void debounce_task(void *argp) {
  uint32_t u32Level, u32State = 0, u32Last = 0xFFFFFFFF;
  bool bEvent;
  for(;;){
    u32Level = !!digitalRead(GPIO_BUTTON);
    u32State = (u32State << 1) | u32Level ;
        
    if ( (u32State & DEBOUNCE_MASK) == DEBOUNCE_MASK
      || (u32State & DEBOUNCE_MASK) == 0) {
        if ( u32Level != u32Last) {
          bEvent = !!u32Level;
          if ( xQueueSendToBack(qhQueue, &bEvent, 1) == pdPASS )
            u32Last = u32Level;
        }
      }
    
    taskYIELD();
  }
}

//
//
//
static void led_task(void *argp) {
  BaseType_t btS;
  bool bEvent, bLed = false;
  
  for (;;) {
    btS = xQueueReceive(
      qhQueue,
      &bEvent,
      portMAX_DELAY
    );
    assert(btS == pdPASS);
        
    if ( bEvent ) {
      // Button Press:
      // Toggle Led
      bLed ^= true;
      digitalWrite(GPIO_LED, bLed);
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = xPortGetCoreID();
  TaskHandle_t th;
  BaseType_t rc;
  
  delay(2000);

  qhQueue = xQueueCreate(40,sizeof(bool));
  assert(qhQueue);

  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
     
  xTaskCreatePinnedToCore(
    debounce_task,
    "debounce",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);
  
  
  xTaskCreatePinnedToCore(
    led_task,
    "led",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);
}

void loop() {
  vTaskDelete(xTaskGetCurrentTaskHandle());
}
