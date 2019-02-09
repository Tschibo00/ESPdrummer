#ifndef INSTBDR
#define INSTBDR

#include "Instrument.h"

class InstBdr:public Instrument{
  public:
    InstBdr(){
      strncpy(_name, "Bdr", 3);
      _params[0]=new InstParam("Ptc",100);
      _params[1]=new InstParam("Att",255);
      _params[2]=new InstParam("Dec",80);
      _params[3]=new InstParam("Drv",128);
    }

    int16_t getNextSample() override;
};

#endif
