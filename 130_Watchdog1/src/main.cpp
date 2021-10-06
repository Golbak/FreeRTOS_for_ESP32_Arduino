#include <Arduino.h>
#include <esp_task_wdt.h>

extern bool loopTaskWDTEnabled;
static TaskHandle_t htask;

void setup() {
  esp_err_t er;

  htask = xTaskGetCurrentTaskHandle();
  loopTaskWDTEnabled = true;
  
  delay(2000);
  printf("\nwatchdog1.ino\n");

  er = esp_task_wdt_status(htask);
  assert(er == ESP_ERR_NOT_FOUND);

  if( er == ESP_ERR_NOT_FOUND ) {
    er = esp_task_wdt_init(5 ,true); // WD set to 5 seconds
    assert(er == ESP_OK);
    er = esp_task_wdt_add(htask);
    assert(er == ESP_OK);
    printf("Task is subscribed to TWDT.\n");
  }
}

static int dly = 1000;

// Watchdog is reset each loop iteration in loopTask(),
// if loopTaskWDTEnabled is set as true
void loop() {
  esp_err_t er;

  printf("loop(dly=%d)..\n", dly);
  er = esp_task_wdt_status(htask);
  assert(er == ESP_OK);
  delay(dly);
  dly += 1000;
}