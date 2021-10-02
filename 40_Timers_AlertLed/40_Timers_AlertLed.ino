#include <FreeRTOS.h>
#include "AlertLED.h"

#define GPIO_LED      12

static AlertLED alert1(GPIO_LED,1000);
static unsigned uLoop_count = 0;

void setup() {
  //delay(2000);  
  alert1.alert();
}

void loop() {
  if( uLoop_count >= 70 ) {
    alert1.alert();
    uLoop_count = 0;
  }

  delay(100);

  if( ++uLoop_count >= 50 )
    alert1.cancel();
}
