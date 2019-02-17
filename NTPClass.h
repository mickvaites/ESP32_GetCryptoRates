#include  <WiFiUdp.h>
#include <TimeLib.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef  NTPClass_h
#define  NTPClass_h

#define   NTP_PACKET_SIZE 48
#define   localTimeOffset 0UL      // your localtime offset from UCT

class NTPTime {
  public:
  
    bool          NTP_time_syncronised = false;
    byte          rtc_hour, rtc_minute, rtc_second, rtc_day, rtc_month, rtc_year, rtc_dow;
  
    NTPTime( int  interval, char *server_name );
    void          begin();
    void          update();

  private:
    WiFiUDP       udp;
    
    int           update_interval;
    int           update_interval2;
    unsigned long last_update;
    unsigned long last_update2;

    unsigned long NTP_last_request_millis;
    int           NTP_request_wait_interval;
    int           NTP_last_request_count;
  
    time_t        last_NTP_update_time = 0;
    bool          NTP_time_update_needed = true;

    unsigned int  local_port = 2390;      // local port to listen for UDP packets
    const char*   NTP_server_name = "pool.ntp.org";

    byte          NTP_buffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
 
    char          *dow_str[9] = { 0, "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0 };

    int           dst( time_t t );
    time_t        lastSunday(time_t);

};

#endif
