#include "DigiEnc.h"

void DigiEnc::process() {
  bool _a=digitalRead(26);
  bool _b=digitalRead(27);
  // clockwise
  if (_lastA&&_lastB&&_a&&!_b)  _valQuad++;
  if (!_lastA&&_lastB&&_a&&_b)  _valQuad++;
  if (!_lastA&&!_lastB&&!_a&&_b)  _valQuad++;
  if (_lastA&&!_lastB&&!_a&&!_b)  _valQuad++;
  // counter-clockwise
  if (_lastA&&_lastB&&!_a&&_b)  _valQuad--;
  if (!_lastA&&_lastB&&!_a&&!_b)  _valQuad--;
  if (!_lastA&&!_lastB&&_a&&!_b)  _valQuad--;
  if (_lastA&&!_lastB&&_a&&_b)  _valQuad--;
  
  _lastA=_a;
  _lastB=_b;
  if (_a&&_b) {
    if (_valQuad<0)
      val--;
    if (_valQuad>0)
      val++;
    if (val>_max){
      if (_wrapping)
        val=_min;
      else
        val=_max;
    }
    if (val<_min){
      if (_wrapping)
        val=_max;
      else
        val=_min;
    }
    _valQuad=0;
  }
}
