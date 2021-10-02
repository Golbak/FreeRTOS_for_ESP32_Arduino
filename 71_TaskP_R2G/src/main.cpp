#include <Arduino.h>

static int iApp_cpu = 0;

void task1(void *argp) {
  printf("Task2 executing, priority %u.\n",
    (unsigned)uxTaskPriorityGet(nullptr));
  vTaskDelete(nullptr);
}

void setup() {
  BaseType_t rc;
  TaskHandle_t th;

  iApp_cpu = xPortGetCoreID();

  delay(2000); // Allow USB init time

  printf("\nTaskCreate2.ino\n");
  printf("loopTask executing, priority %u.\n",
    uxTaskPriorityGet(nullptr));

  rc = xTaskCreatePinnedToCore(
    task1,
    "task1",
    2000,
    nullptr,
    0,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);
  
  printf("Task1 create\n");

  vTaskSuspend(th);
  vTaskPrioritySet(th,3);

  printf("Zzzz... 3 secs\n");
  delay(3000);

  vTaskResume(th);
}

void loop() {
  vTaskDelete(nullptr);
}