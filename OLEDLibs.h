////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef  OLEDLibs_h
#define  OLEDLibs_h

#define OLED_POUND_TILE               1
#define OLED_EURO_TILE                2
#define OLED_COPYRIGHT_TILE           3
#define OLED_SIGNAL_TILE              4
#define OLED_NO_SIGNAL_TILE           5

void  initOLED();
void  writeTextToOLED( bool, int , int , char *, ... );
void  clearDisplayOLED();
void  drawTileOnOLED( uint8_t, uint8_t, uint8_t, uint8_t );


#endif
