#include <FreeRTOS.h>

#define LED1  12
#define LED2  13
#define LED3  15

struct s_led {
  byte          bGpio; 
  byte          bState;
  unsigned      uNapms;  // Delay in ms
  TaskHandle_t  thTaskh;
};

static s_led sLeds[3] = {
  { LED1, 0,  50, 0 },
  { LED2, 0, 200, 0 },
  { LED3, 0, 750, 0 }  
};

static void led_task_func(void *argp) {
  s_led *spLed = (s_led*)argp;
  unsigned uStack_hwm = 0, uTemp; 

  vTaskDelay(1000);

  for(;;){
    digitalWrite(spLed->bGpio, spLed->bState ^= 1);
    uTemp = uxTaskGetStackHighWaterMark(nullptr);
    if( !uStack_hwm || uTemp < uStack_hwm) {
      uStack_hwm = uTemp;
      printf("Task for gpio %d has stack hwm %u, heap %u bytes\n",
        spLed->bGpio, uStack_hwm,
        unsigned(xPortGetFreeHeapSize()));
    }
    vTaskDelay(spLed->uNapms);
  }
}

void setup() {
  // put your setup code here, to run once:
  int iApp_cpu = 0;

  delay(500);

  iApp_cpu = xPortGetCoreID();
  printf("app_cpu is %d (%s core)\n",
    iApp_cpu,
    iApp_cpu > 0 ? "Dual" : "Single");

  printf("LEDs on gpios: ");
  for ( auto& sLed: sLeds ) {
    pinMode(sLed.bGpio, OUTPUT);
    digitalWrite(sLed.bGpio, LOW);
    xTaskCreatePinnedToCore(
      led_task_func,
      "led_task",
      2048,
      &sLed,
      1,
      &sLed.thTaskh,
      iApp_cpu
      );
      printf("%d ",sLed.bGpio);
  }
  putchar('\n');
  printf("There are %u heap bytes avaible. \n",
    unsigned(xPortGetFreeHeapSize()));
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(5000);
  printf("Suspending middle LED task.\n");
  vTaskSuspend(sLeds[1].thTaskh);

  delay(5000);
  printf("Resuming middle LED task.\n");
  vTaskResume(sLeds[1].thTaskh);
}
