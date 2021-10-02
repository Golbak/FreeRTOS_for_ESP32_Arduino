#include <FreeRTOS.h>

#define GPIO  12

static void gpio_high(void *argp) {
  for(;;){
    for(short x=0; x<1000; ++x)
      digitalWrite(GPIO, HIGH);
    taskYIELD();
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

  TaskHandle_t thH1,thH2;
  
  xTaskCreatePinnedToCore(
    gpio_high,
    "gpio_high",
    2048,
    nullptr,
    1,
    &thH1,
    iApp_cpu
  );
  assert(thH1 != nullptr);
  
  
  xTaskCreatePinnedToCore(
    gpio_low,
    "gpio_low",
    2048,
    nullptr,
    1,
    &thH2,
    iApp_cpu
  );
  assert(thH2 != nullptr);
}

void loop() {
  delay(10000);
  assert(1==0);
}
