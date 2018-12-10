#include "myoled.h"
#include "myfont.h"
#include <Arduino.h>        // only required for serial monitor output


void oled_init(){
  Wire.begin(22,21);
  Wire.setClock(800000);
  Wire.beginTransmission(OLED_I2C_ADDRESS);     // Begin the I2C comm with SSD1306's address (SLA+Write)
  Wire.write(OLED_CONTROL_BYTE_CMD_STREAM);     // Tell the SSD1306 that a command stream is incoming
  Wire.write(OLED_CMD_DISPLAY_OFF);             // Turn the Display OFF
  Wire.write(OLED_CMD_SET_CONTRAST);            // set contrast
  Wire.write(0xff);
  Wire.write(OLED_CMD_SET_VCOMH_DESELCT);       // Set the V_COMH deselect volatage to max (0,83 x Vcc)
  Wire.write(0x30);
  Wire.write(OLED_CMD_SET_MEMORY_ADDR_MODE);    // horizontal addressing mode
  Wire.write(0x00);
  Wire.write(OLED_CMD_SET_CHARGE_PUMP);         // Enable the charge pump
  Wire.write(0x14);
  Wire.write(OLED_CMD_DISPLAY_ON);              // Turn the Display ON
  Wire.write(OLED_CMD_SET_PAGE_RANGE);          // use the current page
  Wire.write(0);
  Wire.write(7);
  Wire.write(OLED_CMD_SET_COLUMN_RANGE);        // use all columns
  Wire.write(0);
  Wire.write(127);
  Wire.write(OLED_CMD_DISPLAY_NORMAL);          // non-inverted
  Wire.write(OLED_CMD_SET_SEGMENT_REMAP);       // rotate screen by 180 degrees
  Wire.write(OLED_CMD_SET_COM_SCAN_MODE);
  Wire.endTransmission(true);
}


void display(char *screen){
  display(screen, NULL,-1);
}


void display(char *screen, uint8_t *mask, int trackPos){
  uint8_t x,y;
  for (y=0;y<8;y++){
    Wire.beginTransmission(OLED_I2C_ADDRESS);
    Wire.write(OLED_CONTROL_BYTE_CMD_STREAM);
    Wire.write(OLED_CMD_SET_PAGE_RANGE);
    Wire.write(y);
    Wire.write(y);
    Wire.write(OLED_CMD_SET_COLUMN_RANGE);        // used to reset the pointer to 0,y before pushing out the next row
    Wire.write(0);
    Wire.write(127);
    Wire.endTransmission(true);
    Wire.beginTransmission(OLED_I2C_ADDRESS);
    Wire.write(OLED_CONTROL_BYTE_DATA_STREAM);
    if (y==4){                                        // in line 4 add the top line to seperate menu from pattern view
      for (x=0;x<16;x++)
        Wire.write(myfont+screen[y*16+x]*8+14*8+((x%4)==3?7*8:0)+(x==trackPos?24:0)+(mask==NULL?0:(mask[y*16+x]==32?0:1024)),8);
    } else {
      if (y>4){                                       // in the pattern part (lower 4 rows) add separator where required
        for (x=0;x<16;x++)
          Wire.write(myfont+screen[y*16+x]*8+((x%4)==3?7*8:0)+(x==trackPos?24:0)+(mask==NULL?0:(mask[y*16+x]==32?0:1024)),8);
      }else{
        for (x=0;x<16;x++)
          Wire.write(myfont+screen[y*16+x]*8+(mask==NULL?0:(mask[y*16+x]==32?0:1024)),8);
      }
    }
    Wire.endTransmission(true);
  }
}


void convert_font_to_SSD1306(){
  uint8_t tar[8];
  uint8_t b[8]={1,2,4,8,16,32,64,128};
  uint8_t c,i,t;
  for (c=0;c<128;c++){
    for (i=0;i<8;i++)
      tar[i]=0;
    for (i=0;i<8;i++){
      for (t=0;t<8;t++){
        if (myfont[c*8+t]&b[7-i])
          tar[i]+=b[t];
      }
    }
    for (i=0;i<8;i++){
      myfont[c*8+i]=tar[i];
      myfont[c*8+i+1024]=255^tar[i];
    }
  }
  for (i=0;i<7*8;i++){          // first make a plain copy of the pattern dots
    myfont[i+7*8]=myfont[i];
    myfont[i+14*8]=myfont[i];
    myfont[i+21*8]=myfont[i];
  }
  for (i=0;i<7;i++){            // add separator line on the right side of char 7-13 and 21-27
    myfont[i*8+7*8+7]=255;
    myfont[i*8+21*8+7]=255;
  }
  for (i=0;i<14*8;i++)          // add top line on char 14-27
    myfont[i+14*8]=myfont[i+14*8]|1;
}


// my own version of the the ledcwrite library function, because it crashes in combination with I2C communication when using the mutex
#define LEDC_CHAN(g,c) LEDC.channel_group[(g)].channel[(c)]
void my_ledcWrite(uint8_t chan, uint32_t duty){
    uint8_t group=(chan/8), channel=(chan%8);
//    LEDC_MUTEX_LOCK();          // commented out to avoid crashing when I2C is written to
    LEDC_CHAN(group, channel).duty.duty = duty << 4;//25 bit (21.4)
    if(duty) {
        LEDC_CHAN(group, channel).conf0.sig_out_en = 1;//This is the output enable control bit for channel
        LEDC_CHAN(group, channel).conf1.duty_start = 1;//When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware.
        if(group) {
            LEDC_CHAN(group, channel).conf0.val |= BIT(4);
        } else {
            LEDC_CHAN(group, channel).conf0.clk_en = 1;
        }
    } else {
        LEDC_CHAN(group, channel).conf0.sig_out_en = 0;//This is the output enable control bit for channel
        LEDC_CHAN(group, channel).conf1.duty_start = 0;//When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware.
        if(group) {
            LEDC_CHAN(group, channel).conf0.val &= ~BIT(4);
        } else {
            LEDC_CHAN(group, channel).conf0.clk_en = 0;
        }
    }
  //  LEDC_MUTEX_UNLOCK();          // commented out to avoid crashing when I2C is written to
}
