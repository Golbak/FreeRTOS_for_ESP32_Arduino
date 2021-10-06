#include <Arduino.h>
#include <esp_task_wdt.h>

#define GPIO_LED  19

extern bool loopTaskWDTEnabled;
static TaskHandle_t htask;

static void task2(void *arg) {
  esp_err_t er;
  unsigned seed = hallRead();

  er = esp_task_wdt_add(nullptr);
  assert(er == ESP_OK);

  srand(seed);

  int dly;

  for (;;) {
    digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    esp_task_wdt_reset();
    dly = (rand()%7) * 1000;
    printf("task2(dly=%d)..\n", dly);
    delay( dly );
  }
}

void setup() {
  int app_cpu = xPortGetCoreID();
  BaseType_t rc;
  esp_err_t er;

  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);

  htask = xTaskGetCurrentTaskHandle();
  loopTaskWDTEnabled = true;
  
  delay(2000);
  printf("\nwatchdog2.ino\n");

  er = esp_task_wdt_status(htask);
  assert(er == ESP_ERR_NOT_FOUND);
  
  if ( er == ESP_ERR_NOT_FOUND ) {
    er = esp_task_wdt_init(5, true); // WD set to 5 seconds
    assert(er == ESP_OK);
    er = esp_task_wdt_add(htask);
    assert(er == ESP_OK);
  }

  rc = xTaskCreatePinnedToCore(
    task2,
    "task2",
    2000,
    nullptr,
    1,
    nullptr,
    app_cpu
  );
  assert(rc == pdPASS);
}

static int dly = 1000;

void loop() {
  esp_err_t er;

  printf("loop(dly=%d)..\n", dly);
  er = esp_task_wdt_status(htask);
  assert(er == ESP_OK);
  delay(dly);
  dly += 1000;
}