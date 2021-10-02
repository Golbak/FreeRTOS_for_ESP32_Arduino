
#include <FreeRTOS.h>

#define GPIO_LED1     12
#define GPIO_LED2     13

#define ON  HIGH
#define OFF LOW

static volatile bool bStartf = false;
static TickType_t ttPeriod = 250;

static void big_think() {
  for ( int i=0; i<40000; ++i) {
    __asm__ __volatile__ ("nop");
  }
}

//
//  Delay
//
static void task_led1(void *argp) {

  while( !bStartf )
    ;
  
  for(;;){
    digitalWrite(GPIO_LED1, !digitalRead(GPIO_LED1));
    big_think();
    delay(ttPeriod);
  }
}

//
//  vTaskDelayUntil
//
static void task_led2(void *argp) {

  while( !bStartf )
    ;

  TickType_t ttTime = xTaskGetTickCount();
  
  for(;;){
    digitalWrite(GPIO_LED2, !digitalRead(GPIO_LED2));
    big_think();
    vTaskDelayUntil(&ttTime, ttPeriod);
  }
}


void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = xPortGetCoreID();
  TaskHandle_t th;
    
  delay(2000);

  pinMode(GPIO_LED1, OUTPUT);
  pinMode(GPIO_LED2, OUTPUT);
     
  xTaskCreatePinnedToCore(
    task_led1,
    "delay",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);

  xTaskCreatePinnedToCore(
    task_led2,
    "delay_until",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);

  delay(200);
  bStartf = true;
}

void loop() {
  vTaskDelete(xTaskGetCurrentTaskHandle());
}
