#include <Arduino.h>

#define GPIO_LED    21

#define LOOP_MASK   0B0001
#define TASK2_MASK  0B0010

static TaskHandle_t htask1;

static void task1(void *arg) {
  uint32_t rv;
  BaseType_t rc;

  for (;;) {
    rc = xTaskNotifyWait(0, 0B0011, &rv, portMAX_DELAY);
    digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    printf("Task notified: rv=%u\n", unsigned(rv));

    if ( rv & LOOP_MASK ) {
      printf(" loop() notified this task.\n");
    }
    if ( rv & TASK2_MASK ) {
      printf(" task2() notified this task.\n");
    }
  }
}

static void task2(void *argv) {
  unsigned count;
  BaseType_t rc;

  for (;; count += 100u ) {
    delay(500+count);
    rc = xTaskNotify(htask1, TASK2_MASK, eSetBits);
    assert(rc == pdPASS);
  }
}

void setup() {
  int app_cpu = 0;
  BaseType_t rc;

  app_cpu = xPortGetCoreID();
  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);

  delay(2000);
  printf("tasknfy4.ino:\n");

  rc = xTaskCreatePinnedToCore(
    task1,
    "task1",
    3000,
    nullptr,
    1,
    &htask1,
    app_cpu
  );
  assert( rc == pdPASS );

  rc = xTaskCreatePinnedToCore(
    task2,
    "task2",
    3000,
    nullptr,
    1,
    nullptr,
    app_cpu
  );
  assert( rc == pdPASS );
}

void loop() {
  BaseType_t rc;

  delay(500);
  rc = xTaskNotify(htask1, LOOP_MASK, eSetBits);
  assert( rc == pdPASS );
}