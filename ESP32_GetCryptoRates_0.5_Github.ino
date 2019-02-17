#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Ticker.h>

#include  "NTPClass.h"
#include  "SDLibs.h"
#include  "OLEDLibs.h"

#define DEBUG 1
#define PG_VERSION      0.5

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

NTPTime         Ntp1( 3600000, "pool.ntp.org" );        // Once an hour adjust the time
Ticker          UpdateTheScreen;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define UPDATE_CURRENCY_PERIOD      10
#define CC_HOST               "min-api.cryptocompare.com"
#define CC_API_URL            "/data/price"
#define TARGET_CURRENCIES     "BTC,BCH,LTC,ETH,ETC,XRP,NEO"

#define BTC                   0
#define BCH                   1
#define LTC                   2
#define ETH                   3
#define ETC                   4
#define XRP                   5
#define NEO                   6

static  bool  currency_update_needed = true;
static  char  *base_currencies[] = { "GBP", "EUR", "USD" };
static char   selected_base_currency = 0;

#define CURRENCY_SMA_SIZE   6 * 24
#define MAX_CURRENCIES      7

char  *currency_names[] = { "BTC", "BCH", "LTC", "ETH", "ETC", "XRP", "NEO" };

struct  _historic_values {
  float     value[MAX_CURRENCIES];
  float     gradient[MAX_CURRENCIES];
}historic_values[CURRENCY_SMA_SIZE] = {0};

struct  _currencies{
  float         value;
  float         sma;
  float         gra;
  unsigned int  historic_count;
  char          change;
  bool          updated;
}currencies[MAX_CURRENCIES] = { 0 };

/*
float   latest_currency_values[MAX_CURRENCIES] = {0};
float   currency_sma[MAX_CURRENCIES] = {0};
float   currency_gra[MAX_CURRENCIES] = {0};
static unsigned int currency_sma_count[MAX_CURRENCIES] = {0};
*/
const int API_TIMEOUT = 8000;

const char* host = CC_HOST;
const int httpsPort = 443;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void  CalculateSMA( int currency ) {

  float avg = {0}, gra = {0};

  for( int a = CURRENCY_SMA_SIZE - 1; a > 0; --a ) {
    historic_values[a].value[currency] = historic_values[a - 1].value[currency];
    avg += historic_values[a].value[currency];
    historic_values[a].gradient[currency] = historic_values[a - 1].gradient[currency];
    gra += historic_values[a].gradient[currency];
  }
  if( currencies[currency].historic_count < CURRENCY_SMA_SIZE ) {
    currencies[currency].historic_count ++;
  }
  historic_values[0].value[currency] = currencies[currency].value;
  historic_values[0].gradient[currency] = historic_values[0].value[currency] - historic_values[1].value[currency];
  
  avg += currencies[currency].value;
  currencies[currency].sma = avg / currencies[currency].historic_count;
  currencies[currency].gra = gra / currencies[currency].historic_count;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void  UpdateScreenCb() {

  static char   currency_update_counter = UPDATE_CURRENCY_PERIOD;
  char   *months_str[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }; 

  writeTextToOLED(1, 0, 0, "%2d%3s %02d:%02d:%02d ",
                    day(), months_str[month() - 1], hour(), minute(), second() );
  if( WiFi.status() == WL_CONNECTED ) {
    drawTileOnOLED( 15, 0, 1, OLED_SIGNAL_TILE );
  } else {
    drawTileOnOLED( 15, 0, 1, OLED_NO_SIGNAL_TILE );
  }
  currency_update_counter --;

  for( int c = 0; c < MAX_CURRENCIES; c++ ) {
    if( currencies[c].updated == true ) {
      currencies[c].updated = false;
      writeTextToOLED( 0, 0, c + 1, "%-4s ", currency_names[c] );
      if( selected_base_currency == 1 ) {
        drawTileOnOLED( 5, c + 1, 1, OLED_EURO_TILE );
      } else {
        drawTileOnOLED( 5, c + 1, 1, OLED_POUND_TILE );        
      }
      writeTextToOLED( 0, 6, c + 1, "%9.2f%c", 1/currencies[c].value, currencies[c].change );
    }
  }

  if( currency_update_counter == 0 ) {
    currency_update_counter = UPDATE_CURRENCY_PERIOD;
    currency_update_needed = true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void  GetCurrencies() {
  
  String line;
  char  json[500], dspbuf[20];
  char  urlbuf[1024];
  char  *cp, *cp2, *fp, *pp, *tp, *ep;
  int   len, c = 0;

  writeSDAuditLog("Called GetCurrencies()");
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  
  client.setTimeout(API_TIMEOUT);
  //client.setInsecure();
  
  writeSDAuditLog("connecting to %s\n", host);
  if (!client.connect(host, httpsPort)) {
    writeSDAuditLog("connection failed\n");
    return;
  }

  sprintf( urlbuf, "%s?fsym=%s&tsyms=%s", CC_API_URL, base_currencies[selected_base_currency], TARGET_CURRENCIES );
  
  writeSDAuditLog("requesting URL: %s?fsym=%s&tsyms=%s", CC_API_URL, base_currencies[selected_base_currency], TARGET_CURRENCIES );  

  client.print(String("GET ") + String(urlbuf) + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  writeSDAuditLog("request sent" );
  
  unsigned long timeout = millis();
  while (client.available() == 0 ) {
    line = client.readStringUntil('\n');
    if (millis() - timeout > 5000) {
      writeSDAuditLog("Request timeout !!!!!\n" );
      client.stop();
      return;
    }
  }
  while( client.connected()) {
    line = client.readStringUntil('\n');
    if (line.startsWith("Content-Length: ")) {
      len = line.substring(15).toInt();
      writeSDAuditLog( "Got content length = %d", len );
    }
    if (line.startsWith("Transfer-Encoding: chunked")) {
      writeSDAuditLog( "Encoding chunked ... length after headers" );
    }
    if( line == "\r" ) {
      writeSDAuditLog("Received Headers");
      break;
    }
    // Serial.println(line);
  }
  
  if( client.available()) {
    line = client.readStringUntil('\n');

    char  tmp[10];
    strcpy( tmp, line.c_str());
    len = strtol(tmp, 0, 16);

    line = client.readStringUntil('\n');

    writeSDAuditLog("%s\n", line.c_str());
  }
  client.stop();

  strcpy(json, line.c_str());
  json[len] = '\0';

  if( cp = strchr(json,'{')) {
    cp++;
    if( cp2 = strchr(cp, '}')) {
      *cp2 = '\0';
      if( fp = strtok(cp, ",")) {
        c = 1;
        do {
          tp = fp;
          while( *tp != '\0' && (*tp == ' ' || *tp == '"' )) {
            tp ++;
          }
          if( *tp ) {
            if( pp = strchr( tp, (char)':')) {
              if( ep = strchr( tp, (char)'"')) {
                *ep = '\0';
              }
              pp++;         // Step over the ':'
              while( *pp != '\0' && (*pp == ' ' || *pp == '"' )) {
                pp ++;
              }
              if( ep = strchr( pp, (char)'"')) {
                *ep = '\0';
              }
              if( strncasecmp( "BTC", tp, 3 ) == 0 ) {
                c = BTC;
              } else if( strncasecmp( "BCH", tp, 3 ) == 0 ) {
                c = BCH;
              } else if( strncasecmp( "LTC", tp, 3 ) == 0 ) {
                c = LTC;
              } else if( strncasecmp( "ETH", tp, 3 ) == 0 ) {
                c = ETH;
              } else if( strncasecmp( "ETC", tp, 3 ) == 0 ) {
                c = ETC;
              } else if( strncasecmp( "BCH", tp, 3 ) == 0 ) {
                c = BCH;
              } else if( strncasecmp( "XRP", tp, 3 ) == 0 ) {
                c = XRP;
              } else if( strncasecmp( "NEO", tp, 3 ) == 0 ) {
                c = NEO;
              } else {
                writeSDAuditLog( "Invalid currency found %s\n", tp );
                return;
              }
              currencies[c].value = atof(pp);
              CalculateSMA( c );
              float l1 = 1/currencies[c].value;
              float l2 = 1/(currencies[c].sma);

              int a1 = (int)(l1 * 100);
              int a2 = (int)(l2 * 100);

              //writeSDAuditLog("l1 = %f, l2 = %f, a1 = %d, a2 = %d", l1, l2, a1, a2 );

              if( a1 > a2 ) {
                currencies[c].change = '+';
              } else if( a1 < a2 ) {
                currencies[c].change = '-';
              } else {
                currencies[c].change = ' ';
              }

              currencies[c].updated = true;
              
              writeSDAuditLog("1 %3s = %10.8f %s -- 1 %4s = %8.2f %s (SMA = %10.8f, GRA = %10.8f)[%c]",
                  base_currencies[selected_base_currency], currencies[c].value, currency_names[c],
                  currency_names[c], 1/currencies[c].value, base_currencies[selected_base_currency],
                  currencies[c].sma, currencies[c].gra, currencies[c].change );
              

            }
          }        
        }while( fp = strtok( NULL, "," ));  
      }    
    } else {
      writeSDAuditLog("unrecognised json %s no trailing }", json );
    }
  } else {
    writeSDAuditLog("unrecognised json %s no leading {", json );
  }
  writeSDAuditLog("Exit GetCurrencies()");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void  startWifi() {

  int   len=0;
  int   count = 0;

  WiFi.persistent( false );
  WiFi.disconnect();
  delay(1000);

  WiFi.mode(WIFI_OFF);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin("WIFISSID", "WIFIPASSWD");
  delay(1000);

  writeSDAuditLog( "Connecting to wifi" );
  writeTextToOLED(1, 0, 7, "Connecting wifi" );

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    count ++;
    if( count > 10 ) {
      writeTextToOLED(1, 0, 7, "Rebooting ..." );
      delay(5000);
      ESP.restart();
    }
  }
  Serial.println();
  delay(1000);
  
  writeSDAuditLog( "Connected to %s", WiFi.SSID().c_str() );
  writeSDAuditLog( "IP = %s", WiFi.localIP().toString().c_str() );
  
  writeTextToOLED(1, 0, 7, "%16s", WiFi.localIP().toString().c_str() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  initSDCard();
  initOLED();
  
  writeTextToOLED(1, 0, 0, " GetCryptoRates " );
  writeTextToOLED(0, 0, 1, "    v%3.2f .      ", PG_VERSION );
  
  drawTileOnOLED( 2, 2, 1, OLED_COPYRIGHT_TILE );
  writeTextToOLED(0, 3, 2,"Mick Vaites",0);

  delay(3000);      // Leave copyright messages on screen for 3 seconds

  clearDisplayOLED();
  Ntp1.begin();
  UpdateTheScreen.attach_ms( 1000, UpdateScreenCb );
  startWifi();

  writeSDAuditLog("Startup complete\n");

}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    writeSDAuditLog( "Lost Wifi connection ... reconnecting ..." );
    delay(5000);
    startWifi();
  } else {
    Ntp1.update();
    if( currency_update_needed == true ) {
      currency_update_needed = false;
      GetCurrencies() ;    
    }
  }
}
