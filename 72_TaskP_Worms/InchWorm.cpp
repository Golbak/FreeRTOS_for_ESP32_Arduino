#include "Arduino.h"
#include "InchWorm.h"

InchWorm::InchWorm(int iWorm) :
 iWorm(iWorm) {
}

void InchWorm::draw(TFT_eSPI *tft) {  
  int iPy = 7 + (iWorm-1) * 20;
  int iPx = 2 + iX;

  iPy += iWormh - 3;

  // Clear row for erase previous status
  tft->fillRect(iPx,iPy-2*iSegh,3*iSegw,3*iSegh,TFT_WHITE);

  // Actual status draw
  switch( iState ) {
    case 0: // _-_
      tft->fillRect(iPx,iPy,iSegw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw,iPy-iSegh,iSegsw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw+iSegsw,iPy,iSegw,iSegh,TFT_BLACK);
      break;
    case 1: // _^_
      tft->fillRect(iPx,iPy,iSegw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw,iPy-2*iSegh,iSegsw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw+iSegsw,iPy,iSegw,iSegh,TFT_BLACK);
      tft->drawLine(iPx+iSegw,iPy,iPx+iSegw,iPy-2*iSegh,TFT_BLACK);
      tft->drawLine(iPx+iSegw+iSegsw,iPy,iPx+iSegw+iSegsw,iPy-2*iSegh,TFT_BLACK);
      break;
    case 2: // _^^_
      if(iDir < 0)
        iPx -= iSegsw;
      tft->fillRect(iPx,iPy,iSegw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw,iPy-2*iSegh,iSegw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+2*iSegw,iPy,iSegw,iSegh,TFT_BLACK);
      tft->drawLine(iPx+iSegw,iPy,iPx+iSegw,iPy-2*iSegh,TFT_BLACK);
      tft->drawLine(iPx+2*iSegw,iPy,iPx+2*iSegw,iPy-2*iSegh,TFT_BLACK);
      break;
    case 3: // _-_ (moved)
      if(iDir < 0)
        iPx -= iSegsw;
      else
        iPx += iSegsw;
      tft->fillRect(iPx,iPy,iSegw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw,iPy-iSegh,iSegsw,iSegh,TFT_BLACK);
      tft->fillRect(iPx+iSegw+iSegsw,iPy,iSegw,iSegh,TFT_BLACK);
      break;
    default:
      ;
  } /* End of swith ( iState ) */
  
  iState = (iState+1) % 4;
  if ( !iState ) {
    iX += iDir * iSegsw;
    if ( iDir > 0 ) {
      if ( iX + 3*iSegw+iSegsw >= TFT_WIDTH ) {
        iDir = -1;
      } 
    } else if ( iX <= 2 ) {
        iDir = +1;
    }
  }
}
