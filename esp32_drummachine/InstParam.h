#ifndef INSTPARAM
#define INSTPARAM

#include <Arduino.h>

class InstParam {
  private:
    char _name[4];
    uint8_t _value;
  
  public:
    InstParam(){
      strncpy(_name, "NUL", 3);
      _value=0;
    }
    InstParam(char name[4]){
      strncpy(_name, name, 3);
      _value=0;
    }
    InstParam(char name[4], uint8_t value){
      strncpy(_name, name, 3);
      _value=value;
    }

    char* getName(){return _name;}
    uint8_t getValue(){return _value;}
    void setName(char name[4]){strncpy(_name, name, 3);}
    void setValue(uint8_t value){_value=value;}
};

#endif
