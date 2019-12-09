/**
   Copyright Olex (c) 2019

   Uses Arduino standard libraries, ESP8266WiFi.h, ESP8266WebServer.h, FS.h
*/


/**
   Connections:
   Pin 1 is standard TX, pin 3 standard RX connected to GPS module
   Pin 2 controls module power (on version 1 it was pin 15)
   Pin 0 controls GPS power (inverted! low = on)
   Pins 14, 13, 12, 4, 5, 15 are Outputs 1-6 (15 has pullup on startup, remember!)

   Pin status must be at boot:
   GPIO0   HIGH (LOW for flashing)
   GPIO2   HIGH @boot (internal pullup active, ignore)
   GPIO15  LOW @boot
 */


/**
 * Config files:
 * /module/conf
 *      Structure: per line: name=value
 * /module/wifi
 *      Structure: line 1: SSID, line 2: password, line 3: SSID2, line 4: password2, ...
 * /module/users
 *      Structure per line: name [' '] role(char a/c/v) [' '] password
 *
 * /module/rules
 *      Structure per line: [name (at least 2 chars, no space/newline)] [' '] [priority] [' '] [inside/outside (0/1)] [' '] [area1] [' '] ... [areanN] [' '] ['t'] [' '] [action1] [' '] ... [actionN]
 *                Structure per area: [name (at least 2 chars, no space/newline)] [' '] [type (char b/w/p/a/t/s)] [' '] [type related information]
 *                    Type related:   battery('b'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [lower bound in mV] [' '] [upper bound in mV]
 *                                    time('t'): [from - d(0-7),hh(0-23),mm(0-59),ss(0-59)] [' '] [to(like "from")]
 *                                    wifi('w'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [SSID]
 *                                    position('p'):  [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [coordinate e/w] [' '] [coordinate n/s] [' '] [max distance/proximity in m]
 *                                    area('a'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [coordinate e/w1] [' '] [coordinate n/s1] [' '] [e/w2] [' '] [n/s2] ...
 *                                    special('s'):   'w' (connected to wifi) [' '] [in/out/enter/leave('i'/'o'/'e'/'l')]
 *                                                    TODO: 'g' (gps available/no satelites) [position/no pos/position gained/position lost (p/n/g/l)]
 *                action: [output (1-6, TODO:255 (Server message))] [' '] [value(0/1 / 0-1024)] [' '] [sequence name]
 * /module/rulesenabled
 *      Structure per line: [name of enabled rule]
 */

/**
 * Webapp:
 * /webapp
 *
 * API:
 * /api/login (pwd + user)
 *    => 200 OK json (token) / 401 UNAUTHORIZED
 * /api/logout (token)
 *    => 201 (OK)NOCONTENT / 401 UNAUTHORIZED
 * /api/control ( (nothing) / pin[1-6]=[0/1, 1 (toggle/pulse)] / pwm[1-6]=[0-1023 (pwm)] / seq[1-6]=[sequencename] / alloff=1 )
 *    => 200 OK JSON (outputs[], sequences[])
 * /api/status
 * /api/info
 * /api/gps
 * /api/adminstatus
 * /api/adminoutputs (pin[1-6]=[pinname], pinfunction[1-6]=[n/t/b/p])
 *
 * /api/adminmodule
 * /api/adminwifi
 * /api/admingps
 * /api/adminrules
 * /api/adminusers
 * /api/files GET
 * /api/files POST

 */

/**
   DONE:
   * PWM outputs
   * Binary outputs on/off
   * Pulse outputs
   * Outputs configurable
   * Naming of Outputs
   * Sequences
   * GPS
   * WiFi configuration
   * Saving configs (wifi, users, moduleconfig)
   * Users (create, login, change password, delete)
   * Login via webpage
   * File upload + deletion
   * live-control using javascript for config page after login (for sliders + dropdowns, toggle and on/off)
   * Info page
   * Custom favicon
   * use css file for web ui style
   * turning off and restart module functionality
   * DNS for clients of AP
   * connecting to home server, remote control via home server, webconfig of connection
   * use GPS and if not given ntp and time server for time sync
   * File download (download links need token)

   To be changed for new system

   In Progress
   * User: change role
   * Capture portal in accesspoint
   * WiFi mode 4: station mode, but turn to station + ap if no known network available
   * Connecting to home server - first memory leak, with updated libraries panic and restart
   * rules
   * battery areas
   * GPS proximity areas
   * reacting to in/outside of area (rule)
   * Config of areas + reacting (with priorities and enabling/disabling)

   TODO:
   * enable random waiting times in sequneces (with lower, upper bound) - random intensity, too?
   * enabling GPS only once a day for time sync if no ntp available
   * WiFi client areas (including fixed area "no WiFi", only applicable in station and sta+ap mode!)
   * saving and loading of areas
   * GPS areas
   * Time + Time areas
   * combining areas with time


   * improvements:
      * serial output of read adminpassword and both WiFi station + ap ssids + passwords). In case someone forgets them
              (Adminpw - better don't?, station DONE, ap DONE)
      * listen on special pin for high signal for factory default reset or on serial for reset
          config command (complex work since there is GPS as well... should be included there)
          -> opening the module enables recovery
      * proper UI design
 */

// well, we want to provide a webserver, don't we?
#include <ESP8266WebServer.h>
#include <FS.h>
// stuff for ntp-client
#include <NTPClient.h>
#include <WiFiUdp.h>
// stuff for responding to DNS-requests
#include <DNSServer.h>
// stuff for OTA capability
#include <ArduinoOTA.h>
// we want to answer ANY request from clients when they are in this modules network
#include <ESP8266mDNS.h>

// if defined, the output pins and power enable/disable pin are defined for version one modules (not tested yet!)
//#define LEGACYMODULECOMPATIBILITY

#ifndef LEGACYMODULECOMPATIBILITY
// software uses pin id = 1...NUM_OUTPUT_PINS (dev value= 7, whyever it crashes at 6...)
#define NUM_OUTPUT_PINS 6
// Pin used to turn power "on" (high, to keep power on after turning Module on) and off (low)
#define POWERPIN 2
#else
// software uses pin id = 1...NUM_OUTPUT_PINS (dev value= 7, whyever it crashes at 6...)
#define NUM_OUTPUT_PINS 5
// Pin used to turn power "on" (high, to keep power on after turning Module on) and off (low)
#define POWERPIN 15
#endif

// Pin used to turn the GPS Module on and off (inverted! on = low, off = high)
#define GPSPWRPIN 0
// the firmware version. increment by hand
#define VERSION "3.1.2"

// start an instance of the server listening on port 80
ESP8266WebServer server(80);

// use DNS-server for captive portal
DNSServer dnsServer;
boolean dnsRunning = false;

// stuff for ntp time updates
WiFiUDP ntpUDP;
// time server pool,
// offset due to time zone (in seconds, TODO: can be changed using setTimeOffset() - make configurable! 3600 made it be 1hr advanced in CET?!?!),
// update interval (in milliseconds, can be changed using setUpdateInterval() )
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// info of last area update to measure for interval
unsigned long lastRuleUpdate = 0;
// area update interval, not less than 1000ms
static unsigned long ruleUpdateInterval = 2000;



// setup routine
void setup() {
  // setup code, runs once on "boot"
  powerOn();


  // start serial connection for GPS input and debug information output
  Serial.begin(9600);
  // set timeout for read operations
  Serial.setTimeout(500);
  Serial.println("Serial running at 9600 baud for GPS, 500ms timeout");

  // mount SPI flash filesystem
  SPIFFS.begin();
  Serial.println("SPIFFS initialized");

  // read config files, etc
  initConfig();
  Serial.println("config initialized");

  // initialize outputs
  initOutputs();
  Serial.println("outputs initialized");

  // inititalize handling of sequences
  initSequenceHandling();
  Serial.println("sequences initialized");

  // initialize ticker
  initSequenceTicker();
  Serial.println("Ticker initialized");

  // start WiFi client and/or accesspoint
  startWifi();
  Serial.println("WiFi initialized");

  // TODO: set wifi light sleep mode

  // start server
  initServer();
  Serial.println("Server initialized");

  // start NTP time client and receive time from network
  timeClient.begin();
  Serial.println("NTP client initialized");

  // initialize phoning home
  initPhoneHome();
  Serial.println("Phoning home initialized");

  Serial.print("Free heap space: ");
  Serial.println(ESP.getFreeHeap());

  initRules();
  Serial.println("Rules initialized");

}


// loop routine, run permanently, only wifi stuff will be done between runs
void loop() {

  // check if still connected to a WiFi (if wifi mode is 1 or 3)
  checkWifiStatus();
  // yield to prevent watchdog timeouts - might take quite a while when scanning
  yield();


  // check repeatedly for incoming client connections:
  server.handleClient();
  // yield to prevent watchdog timeouts - might take quite a while when processing requests
  yield();

  // update scheduler
  updateSequenceTicker();
  // yield to prevent watchdog timeouts - might take quite a while when reading data from files
  yield();
  //Serial.println("3");

  // update time via ntp if neccessary (and if possible. A missing wifi connection leads to delays)
  if(WiFi.status() == WL_CONNECTED){
    timeClient.update();
    //yield();
  }

  // check for new GPS data
  checkGpsInput();
  // yield to prevent watchdog timeouts
  yield();
  //Serial.println("5");

  // "phone home" to tracking/control server
  phoneHome();
  //Serial.println("6");

  if(dnsRunning){
    yield();
    dnsServer.processNextRequest();
    //yield();
  }


  /* TODO
  yield();
  checkRules();
  */

}
