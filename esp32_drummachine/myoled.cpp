#include "myoled.h"

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
  Wire.endTransmission();
}