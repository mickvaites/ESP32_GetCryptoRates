#include  <WiFiUdp.h>
#include  <TimeLib.h>
#include  <DS3231.h>
#include  <Wire.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
#include  "NTPClass.h"
#include  "SDLibs.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
NTPTime::NTPTime( int interval, char *server_name ) {
    
  update_interval = interval;
  update_interval2 = 1000;    // One second
  last_update = millis();
  last_update2 = millis();

}
  
////////////////////////////////////////////////////////////////////////////////////////////////////
void  NTPTime::begin() {

  local_port = 2390;      // local port to listen for UDP packets
  NTP_time_update_needed = true;
  NTP_time_syncronised = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  NTPTime::update() {
    
  time_t      epoch = 0UL;
  int         cb = 0;
  char        timestr[17] = {0};
  char        tmpstr[500];

  if( NTP_time_update_needed == true ) {
    udp.begin(local_port);
    writeSDAuditLog( "sending NTP packet to %s", NTP_server_name);
      
    // set all bytes in the buffer to 0
    memset(NTP_buffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    NTP_buffer[0] = 0b11100011;   // LI, Version, Mode
    NTP_buffer[1] = 0;     // Stratum, or type of clock
    NTP_buffer[2] = 6;     // Polling Interval
    NTP_buffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    NTP_buffer[12]  = 49;
    NTP_buffer[13]  = 0x4E;
    NTP_buffer[14]  = 49;
    NTP_buffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(NTP_server_name, 123); //NTP requests are to port 123
    udp.write(NTP_buffer, NTP_PACKET_SIZE);
    udp.endPacket();
    NTP_last_request_millis = millis();
    NTP_last_request_count = 0;
    NTP_time_syncronised = false;
    NTP_time_update_needed = false;
    NTP_request_wait_interval = 1000;
      
  } else if(( NTP_time_syncronised == false ) && (( millis() - NTP_last_request_millis) > NTP_request_wait_interval )){

    cb = udp.parsePacket();
    if (!cb) {
      writeSDAuditLog( "no NTP packet received");
      NTP_last_request_millis = millis();
      NTP_request_wait_interval = 1000;
      NTP_last_request_count ++;
      if( NTP_last_request_count > 5 ) {      // more that five goes and we send request again
        NTP_time_update_needed = true; 
        writeSDAuditLog( "resending NTP request" );
      }
    } else {
      writeSDAuditLog( "packet received, length=%d", cb );
      // We've received a packet, read the data from it
      udp.read(NTP_buffer, NTP_PACKET_SIZE); // read the packet into the buffer

      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, extract the two words:

      unsigned long highWord = word(NTP_buffer[40], NTP_buffer[41]);
      unsigned long lowWord = word(NTP_buffer[42], NTP_buffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      udp.stop();

      epoch = highWord << 16 | lowWord;

      epoch -= 2208988800UL + localTimeOffset;
      epoch += dst(epoch);
      setTime(epoch);
      writeSDAuditLog( "epoch = %d, last_NTP_update_time = %d (difference = %d)", epoch, last_NTP_update_time, epoch - last_NTP_update_time );
      last_NTP_update_time = epoch;
         
      NTP_time_syncronised = true;
      NTP_time_update_needed = false;
    }

  } else if((millis() - last_update) > update_interval) {
    last_update = millis();
    NTP_time_update_needed = true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int NTPTime::dst (time_t t) { // calculate if summertime in Europe

  tmElements_t te;
  te.Year = year(t)-1970;
  te.Month =3;
  te.Day =1;
  te.Hour = 0;
  te.Minute = 0;
  te.Second = 0;
  time_t dstStart,dstEnd, current;
  dstStart = makeTime(te);
  dstStart = lastSunday(dstStart);
  dstStart += 2*SECS_PER_HOUR;  //2AM
  te.Month=10;
  dstEnd = makeTime(te);
  dstEnd = lastSunday(dstEnd);
  dstEnd += SECS_PER_HOUR;  //1AM
  if (t>=dstStart && t<dstEnd) {
    return (3600);  //Add back in one hours worth of seconds - DST in effect
  } else {
    return (0);  //NonDST
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
time_t NTPTime::lastSunday(time_t t) {

  t = nextSunday(t);  //Once, first Sunday
  if(day(t) < 4) {
    return t += 4 * SECS_PER_WEEK;
  } else {
    return t += 3 * SECS_PER_WEEK;
  }
}
