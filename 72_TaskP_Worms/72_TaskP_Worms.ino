// MAX SIZE OF TFT 235 x 235

#define LILYGO_WATCH_2020_V1
#define TFT_WIDTH   235
#define TFT_HEIGHT  235

#include <LilyGoWatch.h>
#include "InchWorm.h"

#define WORM1_TASK_PRIORITY 9
#define WORM2_TASK_PRIORITY 8
#define WORM3_TASK_PRIORITY 7

#define MAIN_TASK_PRIORITY  10

static TTGOClass *ttgo;
static InchWorm worm1(1);
static InchWorm worm2(2);
static InchWorm worm3(3);

static QueueHandle_t qh = nullptr;
static int iApp_cpu = 0;

static void worm_task(void *argp) {
  InchWorm *worm = (InchWorm*) argp;
    
  for (;;) {
    for (int iX = 0; iX<800000 ; iX++) {
      __asm__ __volatile__("nop");
    }
    xQueueSendToBack(qh,&worm,0);
    vTaskDelay(10);
  }
}

void setup()
{
  TaskHandle_t th;
  BaseType_t rc;

  iApp_cpu = xPortGetCoreID();

    
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();

  delay(2000);

  printf("\nWorms.ino\n");

  vTaskPrioritySet(nullptr, MAIN_TASK_PRIORITY);
  
  qh = xQueueCreate(4,sizeof(InchWorm*));
  assert(qh);
  printf("Queue Time created\n");

  ttgo->tft->fillRect(0,0,TFT_WIDTH,TFT_HEIGHT,TFT_WHITE);
  worm1.draw(ttgo->tft);
  worm2.draw(ttgo->tft);
  worm3.draw(ttgo->tft);

  
  rc =  xTaskCreatePinnedToCore(
    worm_task,
    "worm1",
    3000,
    &worm1,
    WORM1_TASK_PRIORITY,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);
  printf("Task worm 1 create.\n");
  
  rc = xTaskCreatePinnedToCore(
    worm_task,
    "worm2",
    3000,
    &worm2,
    WORM2_TASK_PRIORITY,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);
  printf("Task worm 2 create.\n");
  
  rc = xTaskCreatePinnedToCore(
    worm_task,
    "worm3",
    3000,
    &worm3,
    WORM3_TASK_PRIORITY,
    &th,
    iApp_cpu
  );
  assert(rc == pdPASS);
  assert(th);
  printf("Task worm 3 create.\n");
  
}

void loop()
{
  InchWorm *worm = nullptr;

  if (xQueueReceive(qh,&worm,1) == pdPASS)
    worm->draw(ttgo->tft);
  else
    delay(1);
}
