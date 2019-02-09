#include "Instrument.h"

void Instrument::processEncoders(){
  if (DigiEnc::getButton(3)){                        // only change parameters if param button is pressed
    for (char i=0;i<4;i++){
      _encs[i]->process();
      _params[i]->setValue(_encs[i]->val);
    }
  }
}
