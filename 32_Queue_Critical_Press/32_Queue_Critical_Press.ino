#include <FreeRTOS.h>

#define GPIO_LED      12
#define GPIO_BUTTONL  25
#define GPIO_BUTTONR  26

#define ON  HIGH
#define OFF LOW

#define ENABLE      (uint32_t)((1 << GPIO_BUTTONL) | (1 << GPIO_BUTTONR))
static const int sciResetPress = 0xFFFFFFF9;

#define DEBOUNCE_MASK 0x7FFFFFFF

static QueueHandle_t qhQueue;


//
//  BUtton Debouncing task;
//
static void debounce_task(void *argp) {
  unsigned uButtonGpio = *(unsigned*)argp;
  uint32_t u32Level, u32State = 0;
  int iEvent, iLast=-999;
  
  for(;;){
    u32Level = !digitalRead(uButtonGpio);
    u32State = (u32State << 1) | u32Level ;

    
        
    if ( (u32State & DEBOUNCE_MASK) == DEBOUNCE_MASK ) {
      iEvent = uButtonGpio;  
    } else {
      iEvent = -uButtonGpio;
    }
  
    if ( iEvent != iLast) {
      if ( xQueueSendToBack(qhQueue, &iEvent, 0) == pdPASS ) {
        iLast = iEvent;
      } else if ( iEvent < 0 ) {
        do{
          xQueueReset(qhQueue);
        }while( xQueueSendToBack(qhQueue, &sciResetPress, 0) != pdPASS );
        iLast = iEvent;
      }
    }
    
    /*
    printf("Led %u has state: %u and event: %i\n",
      uButtonGpio, u32State, iEvent);
    */
    
    taskYIELD();
  }
}

//
//
//
static void press_task(void *argp) {
  //static const uint32_t u32Enable = (1 << GPIO_BUTTONL) | (1 << GPIO_BUTTONR);
  BaseType_t btS;
  int iEvent;
  uint32_t u32State = 0;

  digitalWrite(GPIO_LED, OFF);
  
  for (;;) {
    btS = xQueueReceive(
      qhQueue,
      &iEvent,
      portMAX_DELAY
    );
    assert(btS == pdPASS);

    if ( iEvent == sciResetPress ) {
      printf("Reset Queue for unpressed button\n");
      digitalWrite(GPIO_LED, OFF);
      u32State = 0;
      continue;
    }
        
    if ( iEvent >= 0 ) {
      u32State |= 1 << iEvent;      
    } else {
      u32State &= ~(1 << -iEvent);
    }

    /*
    printf("Task press has state: %u and event: %i\n Compare with %u\n",
      u32State, iEvent, ENABLE);
    */
      
    if( u32State == ENABLE ) {
      digitalWrite(GPIO_LED, ON);
    } else {
      digitalWrite(GPIO_LED, OFF);
    }

    delay(10);
  }
}


void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = xPortGetCoreID();
  static int iLeft = GPIO_BUTTONL;
  static int iRight = GPIO_BUTTONR;
  TaskHandle_t th;
  BaseType_t rc;
  
  delay(2000);

  qhQueue = xQueueCreate(1,sizeof(int));
  assert(qhQueue);

  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_BUTTONL, INPUT_PULLUP);
  pinMode(GPIO_BUTTONR, INPUT_PULLUP);
     
  xTaskCreatePinnedToCore(
    debounce_task,
    "debounceL",
    2048,
    &iLeft,
    1,
    &th,
    iApp_cpu
  );
  assert(th);

  xTaskCreatePinnedToCore(
    debounce_task,
    "debounceR",
    2048,
    &iRight,
    1,
    &th,
    iApp_cpu
  );
  assert(th);
  
  
  xTaskCreatePinnedToCore(
    press_task,
    "press",
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
