#include <Arduino.h>

#define GPIO_LED  21

static TaskHandle_t htask1;

static void task1 (void *argv) {
  uint32_t rv;

  for (;;) {
    rv = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(GPIO_LED,!digitalRead(GPIO_LED));
    printf("Task notificed: rv=%u\n",unsigned(rv));
  }
}

void setup() {
  int app_cpu = 0;
  BaseType_t rc;

  app_cpu = xPortGetCoreID();
  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);

  delay(2000);
  printf("tasknfy1.ino");

  rc = xTaskCreatePinnedToCore(
    task1,
    "task1",
    3000,
    nullptr,
    1,
    &htask1,
    app_cpu
  );
  assert(rc);
}

void loop() {
  delay(1000);
  xTaskNotifyGive(htask1);
}