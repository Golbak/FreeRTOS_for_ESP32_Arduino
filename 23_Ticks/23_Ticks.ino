#include <FreeRTOS.h>

#define GPIO  12

static void gpio_high(void *argp) {
  for(;;){
    digitalWrite(GPIO, HIGH);
  }
}

static void gpio_low(void *argp) {
  for(;;){
    digitalWrite(GPIO, LOW);
  }
}

void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = xPortGetCoreID();

  pinMode(GPIO, OUTPUT);
  delay(1000);
  printf("Setup started..\n");
  
  xTaskCreatePinnedToCore(
    gpio_high,
    "gpio_high",
    2048,
    nullptr,
    1,
    nullptr,
    iApp_cpu
  );

  xTaskCreatePinnedToCore(
    gpio_low,
    "gpio_low",
    2048,
    nullptr,
    1,
    nullptr,
    iApp_cpu
  );
}

void loop() {
  vTaskDelete(xTaskGetCurrentTaskHandle());
}
