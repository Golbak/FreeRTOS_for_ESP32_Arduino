#ifndef AlertLED_h
#define AlertLED_h

#include <Arduino.h>
#include <FreeRTOS.h>

#define ON  HIGH
#define OFF LOW

class AlertLED {
public:
  AlertLED(int iGpio, unsigned uPeriod_ms=1000);
  void alert();
  void cancel();

  static void callback(TimerHandle_t th);

private:
  TimerHandle_t thandle = nullptr;
  volatile bool bState;
  volatile unsigned uCount;
  unsigned uPeriod_ms;
  int iGpio;

  void reset(bool bS);
};

#endif // AlertLED_h
