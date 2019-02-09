#ifndef INSTRUMENT
#define INSTRUMENT

#include <Arduino.h>
#include "InstParam.h"
#include "DigiEnc.h"

class Instrument {
  protected:
    char _name[4];
    InstParam* _params[4]={NULL,NULL,NULL,NULL};
    DigiEnc* _encs[4];
  
  public:
    Instrument(){
      strncpy(_name, "NUL", 3);
      for (uint8_t i=0; i<4; i++)
        _encs[i]=new DigiEnc(DigiEnc::getPinA(i), DigiEnc::getPinB(i), 0, 255, false, true);
    }

    char* getName(){return _name;}
    void setName(char name[4]){strncpy(_name, name, 3);}
    InstParam** getParams(){return _params;}
    InstParam* getParam(char index){return _params[index];} //TODO should add a range check of index here
    void processEncoders();
    virtual int16_t getNextSample()=0;
};

#endif
