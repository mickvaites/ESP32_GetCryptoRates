#include <TimeLib.h>
#include <HardwareSerial.h>

#include  <stdarg.h>

#include "SDLibs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
#define           SD_MISO         19
#define           SD_SCLK         18
#define           SD_MOSI         23
#define           SD_CS           5

#define   FILENAME_LEN    30

static  bool      SDCardAccessSemaphore = false;

static  bool      SD_ok;
static  uint8_t   SD_card_type;
static  uint64_t  SD_card_size;

////////////////////////////////////////////////////////////////////////////////////////////////////
bool initSDCard() {

  Serial.begin(115200);
  
  SD_ok = false;
  
  Serial.println("No SD card attached");
  
  return SD_ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool  writeSDAuditLog( char *format, ... ) {

  char  msgbuf[1025];

  va_list msgargs;
  va_start (msgargs, format);
  vsnprintf (msgbuf, 1024, format, msgargs);

  Serial.println( msgbuf );

  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
