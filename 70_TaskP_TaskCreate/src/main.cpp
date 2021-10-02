#include <Arduino.h>

static int iApp_cpu = 0;

void task2(void *argp) {
  printf("Task2 executing, priority %u.\n",
    (unsigned)uxTaskPriorityGet(nullptr));
  vTaskDelete(nullptr);
}

void task1(void *argp) {
  BaseType_t rc;
  TaskHandle_t th;

  printf("Task1 executing, priority %u.\n",
  (unsigned)uxTaskPriorityGet(nullptr));
  
  rc = xTaskCreatePinnedToCore(
    task2,
    "task2",
    2000,
    nullptr,
    4,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);

  printf("Task2 create\n");
  
  vTaskDelete(nullptr);
}

void setup() {
  BaseType_t rc;
  unsigned uPriority = 0;
  TaskHandle_t th;

  iApp_cpu = xPortGetCoreID();

  delay(2000); // Allow USB init time

  vTaskPrioritySet(nullptr,3);
  uPriority = uxTaskPriorityGet(nullptr);
  assert(uPriority == 3);

  printf("\nTaskCreate.ino\n");
  printf("loopTask executing, priority %u.\n",
    uPriority);

  rc = xTaskCreatePinnedToCore(
    task1,
    "task1",
    2000,
    nullptr,
    2,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);
  //delay(1);
  
  printf("Task1 create\n");
}

void loop() {
  vTaskDelete(nullptr);
}