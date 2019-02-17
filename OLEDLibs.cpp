#include <U8x8lib.h>
#include "OLEDLibs.h"
#include "SDLibs.h"

#define OLED_SCL      15
#define OLED_SDA      4
#define OLED_RESET    16

#define OLED_ROW0     0
#define OLED_ROW1     1
#define OLED_ROW2     2
#define OLED_ROW3     3     // Not normally used
#define OLED_ROW4     4
#define OLED_ROW5     5
#define OLED_ROW6     6
#define OLED_ROW7     7
#define OLED_INVERTED 0
#define OLED_SHOWPSW  1

//U8G2_SSD1306_128X64_NONAME_F_HW_I2C   u8g2(U8G2_R0, 16, 15, 4);

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ OLED_RESET);   // OLEDs without Reset of the Display


////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t  pound_tile[8] = {

0b10010000,
0b11010110,
0b10111010,
0b10010010,
0b10010010,
0b10000100,
0b10000000,
0b00000000,

};

uint8_t  euro_tile[8] = {

0b00101000,
0b01111100,
0b10101010,
0b10101010,
0b10101010,
0b10000010,
0b01000100,
0b00000000,

};

uint8_t  copyright_tile[8] = {

0b01111100,
0b10000010,
0b10111010,
0b10101010,
0b10101010,
0b10000010,
0b01111100,
0b00000000,

};

uint8_t  signal_tile[8] = {

0b10011111,
0b11111111,  
0b10001111,  
0b11111111,
0b10000111,
0b11111111,
0b10000011,
0b11111111, 

};

uint8_t  no_signal_tile[8] = {

0b10011010,
0b11111101,  
0b10001010,  
0b11111111,
0b10000111,
0b11111111,
0b10000011,
0b11111111, 

};


////////////////////////////////////////////////////////////////////////////////////////////////////

void  initOLED() {
  u8x8.begin();

  u8x8.setFlipMode(OLED_INVERTED);

  //u8x8.clearDisplay();
  
  writeSDAuditLog("OLED Initialised");

  u8x8.setFont(u8x8_font_chroma48medium8_r);
//  u8x8.setFont(u8x8_font_artossans8_r);
//  u8x8.setFont(u8x8_font_victoriamedium8_r);
//  u8x8.setFont(u8x8_font_artosserif8_r);

}

static bool OLEDSemaphore = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
void  writeTextToOLED( bool inverted, int x, int y, char *format, ... ) {

  char msgbuf[1025];
  va_list msgargs;
  va_start (msgargs, format);
  vsnprintf (msgbuf, 1024, format, msgargs);

  if( OLEDSemaphore == true ) {
    return;
  } else {
    OLEDSemaphore = true;
    if( inverted ) {
        u8x8.inverse();
    } else {
        u8x8.noInverse();
    }
    u8x8.drawString(x, y, msgbuf );
    OLEDSemaphore = false;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  clearDisplayOLED() {
  
  u8x8.clearDisplay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  drawTileOnOLED( uint8_t x, uint8_t y, uint8_t cnt, uint8_t tile_id ) {

  switch ( tile_id ) {
    
    case  OLED_POUND_TILE:
      u8x8.drawTile( x, y, cnt, pound_tile );
      break;
    case  OLED_EURO_TILE:
      u8x8.drawTile( x, y, cnt, euro_tile );
      break;
    case OLED_COPYRIGHT_TILE:
      u8x8.drawTile( x, y, cnt, copyright_tile );
      break;
    case OLED_SIGNAL_TILE:
      u8x8.drawTile( x, y, cnt, signal_tile );
      break;
    case OLED_NO_SIGNAL_TILE:
      u8x8.drawTile( x, y, cnt, no_signal_tile );
      break;
    
  }
}
