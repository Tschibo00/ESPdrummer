//#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"
#include "DigiEnc.h"
#include "math.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"
#include "myoled.h"

uint8_t screen[128*8];

uint8_t osciBuffer[256];
uint16_t osciPos;
hw_timer_t *timerSound=NULL;
hw_timer_t *timerSequencer=NULL;
hw_timer_t *timerDisplay=NULL;
int16_t oBdr, oHi, oSnr, o;
uint8_t sintab[256];
uint8_t metaltab[256];
uint8_t noisetab[256];
uint8_t voltab[256];

DigiEnc *d1, *d2, *d3, *d4;
uint8_t intSeqDivider=0;

volatile uint8_t seq_step;
volatile uint8_t seq_micron;
volatile uint8_t seq_total;

volatile int16_t instVolBdr=0;
volatile int16_t instTunBdr=0;
volatile uint16_t instPosBdr=0;
uint16_t bdrTunOffset=100;

volatile int16_t instVolHi=0;
volatile int16_t instTunHi=0;
volatile uint16_t instPosHi=0;
uint16_t hiTunOffset=100;

volatile int16_t instVolSnr=0;
volatile int16_t instTunSnr=0;
volatile uint16_t instPosSnr=0;

volatile int16_t instTemp;

#define OUTPIN 25
#define PWM_CHANNEL 1

uint8_t pattern[4][16]={
  {2,0,0,0,2,0,0,0,2,0,0,0,2,0,1,0},
  {0,0,0,0,2,0,0,0,0,0,0,0,2,0,0,1},
  {1,0,2,1,1,0,2,0,1,1,2,0,1,0,2,1},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// my own version of the the ledcwrite library function, because it crashes in combination with I2C communication when using the mutex
#define LEDC_CHAN(g,c) LEDC.channel_group[(g)].channel[(c)]
void my_ledcWrite(uint8_t chan, uint32_t duty)
{
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

void IRAM_ATTR onSound() {
  instTemp=voltab[instVolBdr];
  oBdr=sintab[instPosBdr>>8];
  if (oBdr>=128) {
    if (instTemp>(oBdr-128))
      oBdr=128;
    else
      oBdr-=instTemp;
  }else{
    if ((instTemp+oBdr)>128)
      oBdr=128;
    else
      oBdr+=instTemp;
  }
  instPosBdr+=instTunBdr;

  instTemp=voltab[instVolSnr];
  oSnr=noisetab[instPosSnr>>8];
  if (oSnr>=128) {
    if (instTemp>(oSnr-128))
      oSnr=128;
    else
      oSnr-=instTemp;
  }else{
    if ((instTemp+oSnr)>128)
      oSnr=128;
    else
      oSnr+=instTemp;
  }
  instPosSnr+=instTunSnr;

  instTemp=voltab[instVolHi];
  oHi=metaltab[instPosHi>>8];
  if (oHi>=128) {
    if (instTemp>(oHi-128))
      oHi=128;
    else
      oHi-=instTemp;
  }else{
    if ((instTemp+oHi)>128)
      oHi=128;
    else
      oHi+=instTemp;
  }
  instPosHi+=instTunHi;

  o=oBdr+oHi+oSnr-384;
  if (o<-1024) o=-1024;
  if (o>1023) o=1023;
  o+=1024;

  osciBuffer[osciPos>>2]=o>>3;
//  dacWrite(OUTPIN, o);        // Use 8bit DAC
  my_ledcWrite(PWM_CHANNEL, o);  // use 11bit PWM

  osciPos++;
  if (osciPos>1023) osciPos=0;
}

void IRAM_ATTR onSequencer() {
  // high-speed input processing
  d1->process();
  d2->process();
  d3->process();
  d4->process();
  hiTunOffset=d1->val;

  intSeqDivider=(intSeqDivider+1)&3;
  if (intSeqDivider!=0)
    return;
  
  seq_step=seq_total>>4;
  seq_micron=seq_total&15;

  instVolBdr+=5;
  instTunBdr-=6;
  if (instVolBdr>128) instVolBdr=128;
  if (instTunBdr<30) instTunBdr=30;
  if (pattern[0][seq_step]>0)
    if (seq_micron==0){
      if (pattern[0][seq_step]==2)
        instVolBdr=0;
      else
        instVolBdr=60;
      instTunBdr=bdrTunOffset;
      instPosBdr=0;
    }

  instVolSnr+=4;
  instTunSnr+=1;
  if (instVolSnr>128) instVolSnr=128;
  if (instTunSnr<30) instTunSnr=30;
  if (pattern[1][seq_step]>0)
    if (seq_micron==0){
      if (pattern[1][seq_step]==2)
        instVolSnr=0;
      else
        instVolSnr=70;
      instTunSnr=20;
      instPosSnr=0;
    }

  instVolHi+=4;
  instTunHi+=0;
  if (instVolHi>128) instVolHi=128;
  if (instTunHi<30) instTunHi=30;
  if (pattern[2][seq_step]>0)
    if (seq_micron==0){
      if (pattern[2][seq_step]==2)
        instVolHi=100;
      else
        instVolHi=115;
      instTunHi=hiTunOffset;
      instPosHi=0;
    }

  seq_total++;
}

void taskDisplay( void * parameter ){
}

void setup() {
  Serial.begin(115200);

  oled_init();

  int a1,a2,a3,a4;
  for (int i=0;i<256;i++){
    sintab[i]=(uint8_t)(sin(((float)i)/128.0*3.14159)*127.0+127.0);
    noisetab[i]=random(255);
    a1=((int)(0.0838f*i))%2;
    a2=((int)(0.12845f*i))%2;
    a3=((int)(0.2049f*i))%2;
    a4=((int)(0.37693f*i))%2;
    a1=a1+a2+a3+a4;
    if ((a1==0)||(a1==2))
      metaltab[i]=255;
    else
      metaltab[i]=0;
    if (i<128)
      voltab[i]=(pow(((float)i)/100.f, 1.75f)*83.7f);
    else
      voltab[i]=128;
  }

  d1=new DigiEnc(33,32,20,700,false);
  d2=new DigiEnc(27,26,20,700,false);
  d3=new DigiEnc(18,19,20,700,false);
  d4=new DigiEnc(13,12,20,700,false);

  ledcAttachPin(OUTPIN, PWM_CHANNEL);
  ledcSetup(PWM_CHANNEL, 39062, 11); // max frequency, 11 bit resolution, i.e. 39062,5hz
  
  timerSound=timerBegin(0, 64, true);
  timerAttachInterrupt(timerSound, &onSound, true);
  timerAlarmWrite(timerSound, 32, true);                    // sound loop runs at 39.062,5hz
  timerStart(timerSound);
  timerAlarmEnable(timerSound);
  
  timerSequencer=timerBegin(1, 80, true);
  timerAttachInterrupt(timerSequencer, &onSequencer, true);
  timerAlarmWrite(timerSequencer, 1750, true);              // test: run at about 60times/sec = 16 micron per step, ca. 120bpm. interrupt runs 240times/sec, but only every 4th time the sequencer is processed. the high frequency is required for processing digital encoders
  timerStart(timerSequencer);
  timerAlarmEnable(timerSequencer);

//  xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 10000, NULL,2, NULL, 0);
}

void loop() {
/*  uint8_t sample, sampleLo, sampleHi;
  uint8_t dispII, page;
  while(1){
    for (page=0; page<8; page++){
      Wire.beginTransmission(OLED_I2C_ADDRESS);
      Wire.write(OLED_CONTROL_BYTE_DATA_STREAM);
      for (dispII=0; dispII<128; dispII++) {
        sample=(osciBuffer[127-dispII])>>2;
        sampleLo=1<<(sample&7);
        sampleHi=sample>>3;
        Wire.write(page==sampleHi?sampleLo:0);
      }
      Wire.endTransmission();   
    }
  }
  */

  uint8_t x,y;
  for (y=0;y<128;y++)
    for (x=0;x<8;x++)
      screen[x*128+y]=y;


  
  for (y=0;y<8;y++){
    while(Wire.busy()){}
    Wire.beginTransmission(OLED_I2C_ADDRESS);
    Wire.write(OLED_CONTROL_BYTE_DATA_STREAM);
    Wire.write(&(screen[y*128]),128);
//    Wire.write(screen,128*8-1);
//    Wire.write(0);
    Wire.endTransmission(true);   
  }



  
}
