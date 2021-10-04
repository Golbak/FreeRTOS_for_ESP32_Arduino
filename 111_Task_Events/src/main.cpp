#include <Arduino.h>

#define GPIO_LED 21

static TaskHandle_t htask1;

static void task1 (void *arg) {
  uint32_t rv;

  for (;;) {
    rv = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    printf("Task notified: rv=%u\n", rv);
  }
}

static void task2 (void *arg) {
  unsigned count = 0;

  for (;; count += 100) {
    delay(500+count);
    xTaskNotifyGive(htask1);
  }
}

void setup() {
  int app_cpu = 0;
  BaseType_t rc;

  app_cpu = xPortGetCoreID();
  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);

  delay(2000);
  printf("tasknfy2.ino: \n");

  rc = xTaskCreatePinnedToCore(
    task1,
    "task1",
    3000,
    nullptr,
    1,
    &htask1,
    app_cpu
  );
  assert(rc == pdPASS);

  rc = xTaskCreatePinnedToCore(
    task2,
    "task2",
    3000,
    nullptr,
    1,
    nullptr,
    app_cpu
  );
  assert(rc == pdPASS);
}

void loop() {
  delay(500);
  xTaskNotifyGive(htask1);
}