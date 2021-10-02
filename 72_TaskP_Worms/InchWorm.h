#ifndef InchWorm_h
#define InchWorm_h

#include "Arduino.h"
#define LILYGO_WATCH_2020_V1
#include <LilyGoWatch.h>



class InchWorm
{
  public:
    InchWorm(int iWorm);
    void draw(TFT_eSPI *tft);
  private:
  static const int iSegw = 9, iSegsw = 4, iSegh = 3;
    int iWorm;
    int iX, iY;
    int iWormw = 30;
    int iWormh = 10;
    int iDir = 1;
    int iState = 0;
};

#endif
