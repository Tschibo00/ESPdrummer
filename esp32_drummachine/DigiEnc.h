#ifndef DIGIENC
#define DIGIENC

#include <Arduino.h>

class DigiEnc {
  private:
    uint8_t _pinA;
    uint8_t _pinB;
    int32_t _min;
    int32_t _max;
    bool _wrapping=false;
    bool _dynamic=true;

    bool _lastA=true;
    bool _lastB=true;
    int8_t _valQuad=0;
    unsigned long _lastUpdate=0;
    unsigned long _deltaLastUpdate=0;

//    static constexpr uint8_t _pinAConfig[]{33,27,18,13};
//    static constexpr uint8_t _pinBConfig[]{32,26,19,12};

  public:
    int32_t val=0;
  
    DigiEnc(uint8_t _pinA=26, uint8_t _pinB=27, int32_t _min=-128, int32_t _max=127, bool _wrapping=false, bool _dynamic=true){
      this->_pinA=_pinA;
      this->_pinB=_pinB;
      this->_min=_min;
      this->_max=_max;
      this->_wrapping=_wrapping;
      this->_dynamic=_dynamic;
      pinMode(_pinA, INPUT_PULLUP);
      pinMode(_pinB, INPUT_PULLUP);
      pinMode(23, INPUT_PULLUP);    // set up the button input pins (every time the constructor is called, but that doesnt hurt too much)
      pinMode(5, INPUT_PULLUP);
      pinMode(4, INPUT_PULLUP);
      pinMode(14, INPUT_PULLUP);
    }

    static uint8_t getPinA(uint8_t index){
      switch(index){
        case 0: return 33; break;     // using a static array would be better, but wont compile
        case 1: return 27; break;
        case 2: return 18; break;
        case 3: return 13; break;
        default: return 0; break;
      }
    }
    static uint8_t getPinB(uint8_t index){
      switch(index){
        case 0: return 32; break;
        case 1: return 26; break;
        case 2: return 19; break;
        case 3: return 12; break;
        default: return 0; break;
      }
    }
    void process();     // should be called at least 250 times/second to ensure a errorfree processing even for fast turning

    static bool getButton(uint8_t index){
      switch(index){
        case 0: return !digitalRead(23); break;
        case 1: return !digitalRead(5); break;
        case 2: return !digitalRead(4); break;
        case 3: return !digitalRead(14); break;
        default: return false;
      }
    }
};

#endif
