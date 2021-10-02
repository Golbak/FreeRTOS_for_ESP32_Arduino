#include <Arduino.h>
#include <FreeRTOS.h>

// LED is active high
#define ON  HIGH
#define OFF LOW

//GPIO definitions:
#define GPIO_LED      12
#define GPIO_TRIGGER  25
#define GPIO_ECHO     26

typedef unsigned long usec_t;

static SemaphoreHandle_t shBarrier;
static TickType_t ttRepeatTicks = 1000;

//
//  Report the distance in CM
//
static void report_cm(usec_t usecs) {
  unsigned uCm, uTenths;

  uCm = usecs * 10ul / 58ul;
  uTenths = uCm % 10;
  uCm /= 10;

  printf("Distance %u.%u cm, %u usecs\n",
    uCm, uTenths, usecs);
}

//
// Range Finder Task
//
static void range_task(void *argp) {
  BaseType_t rc;
  usec_t usecs;

  for(;;){
    

    rc = xSemaphoreTake(shBarrier, portMAX_DELAY);
    assert(rc == pdPASS);

    // Send ping:
    digitalWrite(GPIO_LED, ON);
    digitalWrite(GPIO_TRIGGER, ON);
    delayMicroseconds(10);
    digitalWrite(GPIO_TRIGGER, OFF);

    // Listen for echo
    usecs = pulseInLong(GPIO_ECHO, ON, 50000);
    digitalWrite(GPIO_LED, OFF);

    if ( (usecs > 0) && (usecs < 50000ul) ) {
      report_cm(usecs);
    } else {
      printf("No echo\n");
    }
  }
}

//
// Send sync to range_task every 1 sec
//
static void sync_task(void *argp) {
  BaseType_t rc;
  TickType_t ttTicktime;

  delay(1000);

  ttTicktime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&ttTicktime, ttRepeatTicks);
    rc = xSemaphoreGive(shBarrier);
    // assert(pdPass);
  }
}

//
// Program Initialization
//
void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = xPortGetCoreID();
  TaskHandle_t th;

  shBarrier = xSemaphoreCreateBinary();
  assert(shBarrier);

  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, OFF);
  pinMode(GPIO_TRIGGER, OUTPUT);
  digitalWrite(GPIO_LED, OFF);
  pinMode(GPIO_ECHO, INPUT_PULLUP);

  delay(2000);

  printf("\nHSCR04\n");

  xTaskCreatePinnedToCore(
    range_task,
    "range_task",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);
  

  xTaskCreatePinnedToCore(
    sync_task,
    "sync_task",
    2048,
    nullptr,
    1,
    &th,
    iApp_cpu
  );
  assert(th);
  
  
}

void loop() {
  vTaskDelete(nullptr);
}