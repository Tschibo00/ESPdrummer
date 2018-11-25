#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"
#include "DigiEnc.h"
#include "math.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"

SSD1306Wire  display(0x3c, 22,21);

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


#define LEDC_CHAN(g,c) LEDC.channel_group[(g)].channel[(c)]
void my_ledcWrite(uint8_t chan, uint32_t duty)
{
    uint8_t group=(chan/8), channel=(chan%8);
//    LEDC_MUTEX_LOCK();
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
  //  LEDC_MUTEX_UNLOCK();
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

#define OLED_I2C_ADDRESS   0x3C
#define OLED_CONTROL_BYTE_CMD_SINGLE  0x80
#define OLED_CONTROL_BYTE_CMD_STREAM  0x00
#define OLED_CONTROL_BYTE_DATA_STREAM 0x40
#define OLED_CONTROL_BYTE_DATA_SINGLE   0xc0
#define OLED_CMD_SET_CONTRAST     0x81  // follow with 0x7F
#define OLED_CMD_DISPLAY_RAM      0xA4
#define OLED_CMD_DISPLAY_ALLON      0xA5
#define OLED_CMD_DISPLAY_NORMAL     0xA6
#define OLED_CMD_DISPLAY_INVERTED     0xA7
#define OLED_CMD_DISPLAY_OFF      0xAE
#define OLED_CMD_DISPLAY_ON       0xAF
#define OLED_CMD_SET_MEMORY_ADDR_MODE 0x20  // follow with 0x00 = HORZ mode = Behave like a KS108 graphic LCD
#define OLED_CMD_SET_COLUMN_RANGE   0x21  // can be used only in HORZ/VERT mode - follow with 0x00 + 0x7F = COL127
#define OLED_CMD_SET_PAGE_RANGE     0x22  // can be used only in HORZ/VERT mode - follow with 0x00 + 0x07 = PAGE7
#define OLED_CMD_SET_COL_NIBBLE_LO            0x00
#define OLED_CMD_SET_COL_NIBBLE_HI            0x10
#define OLED_CMD_SET_PAGE_START                0xb0
#define OLED_CMD_SET_DISPLAY_START_LINE 0x40
#define OLED_CMD_SET_SEGMENT_REMAP    0xA1  
#define OLED_CMD_SET_MUX_RATIO      0xA8  // follow with 0x3F = 64 MUX
#define OLED_CMD_SET_COM_SCAN_MODE    0xC8  
#define OLED_CMD_SET_DISPLAY_OFFSET   0xD3  // follow with 0x00
#define OLED_CMD_SET_COM_PIN_MAP    0xDA  // follow with 0x12
#define OLED_CMD_NOP          0xE3  // NOP
#define OLED_CMD_SET_DISPLAY_CLK_DIV  0xD5  // follow with 0x80
#define OLED_CMD_SET_PRECHARGE      0xD9  // follow with 0x22
#define OLED_CMD_SET_VCOMH_DESELCT    0xDB  // follow with 0x30
#define OLED_CMD_SET_CHARGE_PUMP  0x8D  // follow with 0x14void oled_init() {
void oled_init(){
      Wire.begin(22,21);
      Wire.setClock(400000);
//  Wire.begin();            // Init the I2C interface (pins A4 and A5 on the Arduino Uno board) in Master Mode.
//  TWBR=0;           // Set the I2C to HS mode - 400KHz! TWBR = (CPU_CLK / I2C_CLK) -16 /2. Some report that even 0 is working. **** test it out ****
  Wire.beginTransmission(OLED_I2C_ADDRESS);   // Begin the I2C comm with SSD1306's address (SLA+Write)
  Wire.write(OLED_CONTROL_BYTE_CMD_STREAM);   // Tell the SSD1306 that a command stream is incoming
  Wire.write(OLED_CMD_DISPLAY_OFF);     // Turn the Display OFF
  Wire.write(OLED_CMD_SET_CONTRAST);      // set contrast
  Wire.write(0xff);
  Wire.write(OLED_CMD_SET_VCOMH_DESELCT);   // Set the V_COMH deselect volatage to max (0,83 x Vcc)
  Wire.write(0x30);
  Wire.write(OLED_CMD_SET_MEMORY_ADDR_MODE);    // vertical addressing mode
  Wire.write(0x01);
  Wire.write(OLED_CMD_SET_CHARGE_PUMP);     // Enable the charge pump
  Wire.write(0x14);
  Wire.write(OLED_CMD_DISPLAY_ON);      // Turn the Display ON
  Wire.write(OLED_CMD_SET_PAGE_RANGE);                  // use the current page
  Wire.write(0);
  Wire.write(7);
  Wire.write(OLED_CMD_SET_COLUMN_RANGE);                // use all columns
  Wire.write(0);
  Wire.write(127);
  Wire.endTransmission();
}

void taskDisplay( void * parameter ){
}

void setup() {
  Serial.begin(115200);
  
  display.init();
  display.flipScreenVertically(); 

//  oled_init();


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
  while(1){
    display.clear();                                // just pump out the osci to the display
  //  for (uint8_t i=0;i<128;i++)
    //  display.setPixel(i,osciBuffer[i<<1]>>2);
/*    display.drawString(0,0,String(d1->val));
    display.drawString(0,10,String(d2->val));
    display.drawString(0,20,String(d3->val));
    display.drawString(0,30,String(d4->val));
    display.drawString(0,56,"Hello World");
*/
/*
  uint8_t sample, sampleLo, sampleHi;
  uint8_t dispII, page;
    for (dispII=0; dispII<128; dispII++) {
      Wire.beginTransmission(OLED_I2C_ADDRESS);
      Wire.write(OLED_CONTROL_BYTE_DATA_STREAM);
      sample=(osciBuffer[127-dispII])>>2;
  sample=38;
      sampleLo=1<<(sample&7);
      sampleHi=sample>>3;
      for (page=0; page<8; page++)
        Wire.write(page==sampleHi?sampleLo:0);
      Wire.endTransmission();   
    }
*/
    
    display.display();
  }



//  while (audioISRrunning){}
  
//  delay(100);

}
