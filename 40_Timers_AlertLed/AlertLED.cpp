#include <Arduino.h>
#include "AlertLED.h"

AlertLED::AlertLED(int iGpio, unsigned uPeriod_ms) {
  this->iGpio = iGpio;
  this->uPeriod_ms = uPeriod_ms;
  pinMode(this->iGpio, OUTPUT);
  this->reset(false);
}

void AlertLED::reset(bool bS) {
  bState = bS;
  uCount = 0;
  digitalWrite(this->iGpio,bS?ON:OFF);
}

void AlertLED::alert() {
  if ( !thandle ) {
    thandle = xTimerCreate(
      "alert_tmr",
      pdMS_TO_TICKS(uPeriod_ms/20),
      pdTRUE,
      this,
      AlertLED::callback);
    assert(thandle);
  }

  reset(true);
  xTimerStart(thandle, portMAX_DELAY);
}

void AlertLED::cancel() {
  if ( thandle ) {
    xTimerStop(thandle, portMAX_DELAY);
    digitalWrite(this->iGpio,OFF);
  }
}

void AlertLED::callback(TimerHandle_t th) {
  AlertLED *obj = (AlertLED*)pvTimerGetTimerID(th);

  assert(obj->thandle == th);
  obj->bState ^= true;
  
  digitalWrite(obj->iGpio, obj->bState?ON:OFF);

  if ( ++obj->uCount >= (5 * 2) ) {
    obj->reset(true);
    xTimerChangePeriod(th, 
      pdMS_TO_TICKS(obj->uPeriod_ms/20), 
      portMAX_DELAY);
  } else if ( obj->uCount == ((5 * 2) - 1) ) {
    xTimerChangePeriod(th, 
      pdMS_TO_TICKS(obj->uPeriod_ms/20+obj->uPeriod_ms/2), 
      portMAX_DELAY);
    assert(!obj->bState);      
  }
}
