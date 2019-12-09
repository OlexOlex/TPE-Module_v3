/**
   contains all config values and functions
   Copyright Olex (c) 2017


   Pin status must be at boot:
   GPIO0   HIGH (LOW for flashing)
   GPIO2   HIGH @boot (internal pullup active, ignore)
   GPIO15  LOW @boot

   TODO; serverCertFingerprint - change magic strings accordingly
 */

#ifndef MYCONFIG
#define MYCONFIG

// careful! might make the WatchDog bite you!
//#define DEBUGOUT_CUSTOM

#define DEBUGOUT Serial
#define GPSSERIAL Serial
#define BACKUPOUT Serial

// define for mitigating weird behaviour when turned on GPS makes wifi.softAP(...) freeze
// result: if active, GPS gets disabled before the calls of wifi.softAP(..) in mywifiutility
#define BULLSHITSERIALAPINTERFERENCE

#define CPU80MHZ 80
#define CPU160MHZ 160


// comment for "production"
//#define DELETEUSERSFILEONBOOT


/* INFO: following pins will cause trouble: 0,9,10 (when set to OUTPUT value 0), 6,7,8,11 (when set to INPUT),
   they will reset the module!

   Pin 1 is standard TX, pin 3 standard RX, don't block them! (pin 3 somehow breaks the system when set to output)
   Pin 2 controls module power
   Pin 3 controls GPS power (inverted!)
*/
#ifndef LEGACYMODULECOMPATIBILITY
const byte pins[NUM_OUTPUT_PINS] = {14, 13, 12, 4, 5, 15};
#else
const byte pins[NUM_OUTPUT_PINS] = {14, 13, 12, 4, 5};
#endif
// Cookie name:
//#define COOKIE_NAME "TPESESSIONCOOKIE"

#define FILE_DIR_NAME "/"
#define PUBLICFILEPREFIX "/public"
#define MODULEFILEPREFIX "/module"

// "hidden" main config file
#define CONFIGFILENAME "/module/conf"
// "hidden" wifi config file
#define WIFICONFIGFILENAME "/module/wifi"
// "hidden" user file name
#define USERSFILENAME "/module/users"
// "hidden" rules file name
#define RULESFILENAME "/module/rules"
// "hidden" user file name
#define AREASFILENAME "/module/areas"

// role abbreviations
#define USERADMIN 'a'
#define USERBASIC 'b'
#define USERVIEWER 'v'
#define USERADMINSTRING "a" // for comparison to Strings

// timeout in milliseconds until a wifi connection should be established - never use less than 10 sec (10 000 ms), better > 15 sec
#define WIFICONNECTIONTIMEOUT 20000
#define WIFISCANTIMEOUT 20000

// single exception to nonprivate files
#define FAVICON_FILE "/favicon.ico"

#define SEQUENCEIDENTIFIER "pwmsequence"
#define REPEATIDENTIFIER "repeat"
#define INFINITEIDENTIFIER "infinite"

#define OUTPUTFUNCTION_OFF 'n'
#define OUTPUTFUNCTION_TOGGLE 't'
#define OUTPUTFUNCTION_PULSE 'b'
#define OUTPUTFUNCTION_PWM 'p'



// TODO?: use progmem for strings occouring often, if the firmware gets too big
//const char HTTPHEADER[] PROGMEM = "<http><head></head>";
//const char HTTPLIGHTBODY[] PROGMEM = "<body><bgcolor=#000000>";

#define ADCVOLTAGEMULTIPLIER 11


//  WiFi constants:
// BACKUP accesspoint SSID if not configured
#define BACKUPAPSSID "TPE_Module_" + String(ESP.getChipId())
// maximum value of wifi mode (1..MAXWIFIMODE) - 1 = sta/client, 2 = ap, 3 = both, 4 will be sta, but ap if no wifi available
#define MAXWIFIMODE 3

#define DEFAULTSERVERPORT 80


// whether GPS should be enabled or not
static byte gpsMode = 0;
// per default use the closest ntp server
static String ntpServerName = "pool.ntp.org";

static String slaveInfoFileName = "";
static String dummyFileName = "";
// SAVESPACE: if global variables take too much space, these two can be compined to one byte with state 0-3
static boolean showDummyFile = false;
static boolean hideSite = false;

// WIFI config variables:

// desired wifi mode
//  (1 = client, 2 = acesspoint, 3 = client + accesspoint, future: 4 = client, only open accesspoint if no known wifi available)
//  default for shipping: 3
static byte wifiMode = 3;
// acesspoint credentials + config stuff, default for shipping: Backup SSID and no password protection
static String apSsid = BACKUPAPSSID;
static String apPwd = "";
// not used yet:
//static byte apChannel = 1;
//static boolean apHidden  = false;

// stores information about all known wifis
static String* knownWifis;
static String* knownWifiPasswords;
static byte numKnownWifis = 0;

// set backup hostname if none is configured in the config file
static String hostName = BACKUPAPSSID;

// output pins config
static volatile char pinFunctionalities[NUM_OUTPUT_PINS];
static String* pinNames = new String[NUM_OUTPUT_PINS];

static String pageTitle = "TPE Module v" + String(VERSION);

// data for connecting to a remote server
// the URL or IP of the server to connect to
static String serverUrl = "";
static unsigned int serverPort = DEFAULTSERVERPORT;
static String serverCertFingerprint = "";
static boolean connectToServer = false;
static boolean serverRemoteControlMode = false;
// connect all X milliseconds to the server for sending status data and if configured, retrieve information
static unsigned int serverContactInterval = 10000;

/**
    space for saving set pwm and binary values of the outputs
    should be in utility, but then the compiler does not find it fast enough and complains in serverutility...
*/
static volatile short pinValues[NUM_OUTPUT_PINS];

/**
   returns the pin for the connector
   use values 1-7, for others it will return 255
*/
byte getConnectorPin(byte conn) {
  if((conn - 1 ) < NUM_OUTPUT_PINS){
    return pins[conn-1];
  }
  // if out of range: return bullshit (way too high number), so any setOutput([pinNumber]) etc. will exit without setting any pin
  return 255;
}


/**
   returns the desired functionality of the connector
   return values:
        "t" = binary, toggles on or off
        "b" = binary, only pulse (few ms)
        "p" = pwm output
        "n" = not connected, deactivated
*/
char getConnectorFunction(byte conn) {
  // return saved values
  if((0 < conn) && (conn <= NUM_OUTPUT_PINS)){
    return pinFunctionalities[conn - 1];
  }
  // if something went wrong or the ID was not configured (e.g. targets for rules)
  // "if in doubt, turn it off"
  return OUTPUTFUNCTION_OFF;
}

/**
   sets the desired functionality of the connector
   values:
        "t" = binary, toggles on or off
        "b" = binary, only pulse (few ms)
        "p" = pwm output
        "n" = not connected, deactivated (will be set on any value not corresponding the others)
   IMPORTANT: call saveConfig() afterwards to save status in config file
*/
void setConnectorFunction(byte conn, char func) {
  if((0 < conn) && (conn <= NUM_OUTPUT_PINS)){
    switch (func){
      case OUTPUTFUNCTION_TOGGLE:{
        // save config
        pinFunctionalities[conn - 1] = OUTPUTFUNCTION_TOGGLE;
        // configure pin as output
        pinMode(getConnectorPin(conn), OUTPUT);
        // turn off pin
        digitalWrite(getConnectorPin(conn), 0);
        break;
      }
      case OUTPUTFUNCTION_PULSE:{
        pinFunctionalities[conn - 1] = OUTPUTFUNCTION_PULSE;
        // configure pin as output
        pinMode(getConnectorPin(conn), OUTPUT);
        // turn off pin
        digitalWrite(getConnectorPin(conn), 0);
        break;
      }
      case OUTPUTFUNCTION_PWM:{
        pinFunctionalities[conn - 1] = OUTPUTFUNCTION_PWM;
        // configure pin as input (aka no digital output)
        pinMode(getConnectorPin(conn), INPUT);
        // turn off pwm
        analogWrite(getConnectorPin(conn), 0);
        break;
      }
      default:{
        pinFunctionalities[conn - 1] = OUTPUTFUNCTION_OFF;
        // configure pin as input (aka no output)
        pinMode(getConnectorPin(conn), INPUT);
        // turn off pin pullup resistor
        digitalWrite(getConnectorPin(conn), 0);
        break;
      }
    }
  }
}


/**
   sets the desired name of the connector
   IMPORTANT: call saveConfig() afterwards to save status in config file
*/
void setConnectorName(byte conn, String connName){
  if((0 < conn) && (conn <= NUM_OUTPUT_PINS)){
    /*if(pinNames[conn]){
      // TODO: delete previous String to avoid memory leaks?
      [_delete](pinNames[conn-1]);
    }*/
    // save name
    pinNames[conn-1] = connName;
  }
}



/**
   returns the name of the given connector
*/
String getConnectorName(byte conn) {

  if((0 < conn) && (conn <= NUM_OUTPUT_PINS)){
    // return name
    return pinNames[conn-1];
  }
  return "undefined";
}




// #############################################################
// # WiFi and GPS stuff
// #############################################################

/**
   1 => WiFi_STA
   2 => WiFi_AP
   3 => WIFI_AP_STA
   4 => WiFi_STA but if no known WiFi network is available, make it WiFi_AP_STA
// #############################################################
// # Output definitions
// #############################################################

*/
byte getDesiredWifiMode() {
  return wifiMode;
}

/**
   set desired wifi mode
   1 => WiFi_STA
   2 => WiFi_AP
   3 => WIFI_AP_STA
   4 => WiFi_STA but if no known WiFi network is available, make it WiFi_AP_STA
   IMPORTANT: call saveConfig() afterwards to save status in config file
 * configuration routines trigger when "checkWifi()" is called from main loop
 */
void setDesiredWifiMode(byte newMode){
  // needs to be 1, 2, 3 or 4 (sta, ap, both, or only ap if no wifi is available. Do not disable both!)
  if((newMode > 0) && (newMode <= MAXWIFIMODE)){
    wifiMode = newMode;
  }else{
    // in case of wrong value: enable both acesspoint and station mode
    wifiMode = 3;
  }
}

/**
 * returns desired GPS usage
 * 0 = off, 1 = on, 2 = 1x daily for time sync
 */
byte getGpsEnabled(){
  return gpsMode;
}


/**
   sets desired GPS usage
   0 = off, 1 = on, 2 = 1x daily for time sync
   IMPORTANT: call saveConfig() afterwards to save status in config file
 */
void setGpsUsage(byte use){
  gpsMode = use;
  switch(use){
    // GPS off
    case 0:{
      disableGps();
      gpsMode = 0;
      break;
    }
    // GPS on
    case 1: {
      enableGps();
      gpsMode = 1;
      break;
    }
    // GPS on 1x daily for time sync, then off
    case 2: {
      disableGps();
      gpsMode = 2;
      break;
    }
    // anything else
    default: {
      disableGps();
      gpsMode = 0;
      DEBUGOUT.println("got " + String(use) + " as GPS mode - diasabled GPS");
      break;
    }
  }
}




// #############################################################
// # Web page strings
// #############################################################



/**
 * initializes configuration
 * * initializes values
 * * searches language files
 * * reads saved configuration values from files for config, WiFi and GPS
 */
void initConfig() {

  #ifdef DELETEUSERSFILEONBOOT
    if(SPIFFS.exists(USERSFILENAME)){
      SPIFFS.remove(USERSFILENAME);
    }
  #endif

  // initialize GPS pin
  pinMode(GPSPWRPIN, OUTPUT);
  // turn off GPS (if the hardware did not turn it off in initialized state)
  digitalWrite(GPSPWRPIN, HIGH);


  // before reading stuff from config files:
  // initialize all selected sequences with the "no sequence" identifier, all pin values with 0 and all functionalities
  for (byte i = 1; i <= NUM_OUTPUT_PINS; i++) {
    // no sequences are running
    setSelectedOption(i, getNoSequenceIdentifier());
    // binary outouts off/ pwm value is 0
    pinValues[i-1] = 0;
    // in case no configuration is found later, specify every pin as deactivated
    pinFunctionalities[i-1] = OUTPUTFUNCTION_OFF;
  }

  // read module config from file
  if(SPIFFS.exists(CONFIGFILENAME)){
    // open config file in read mode
    File conf = SPIFFS.open(CONFIGFILENAME, "r");
    conf.setTimeout(50);
    // read each line and see, which configuration it is
    while(conf.available()){
      String line = conf.readStringUntil('\n');
      // check if line matches "pin[ID (1-9)]=[mode (char)],[name (String)]"
      if(line.startsWith("pin") && (line.indexOf('=') == 4) && (line.indexOf(',') == 6)){
        // read pin config
        // happy typecasting/converting
        // id is at index 3 (4th char)
        byte pinId = String(line.charAt(3)).toInt();
        // mode is at index 5 (6th char)
        char pinFunc = line.charAt(5);
        // name is from index 7 (8th char) on
        String pinName = line.substring(7);

        // if valid pin ID
        if((0 < pinId) && (pinId <= NUM_OUTPUT_PINS)){
          pinFunctionalities[pinId - 1] = pinFunc;
          pinNames[pinId - 1] = pinName;
        }
      }else if(line.startsWith("GPSmode=") && (line.length() > 8)){
        // read GPS mode
        line.replace("GPSmode=", "");
        byte gpsMode = String(line.charAt(0)).toInt();
        if(gpsMode > 2){
          // if invalid value => turn off GPS
          gpsMode = 0;
        }
        // load GPS mode
        setGpsUsage(gpsMode);
      }else if(line.startsWith("ntpServer=") && (line.length() > 8)){
        // read desired ntp server url
        line.replace("ntpServer=", "");
        ntpServerName = line;
        timeClient.setPoolServerName(ntpServerName.c_str());
      }else if(line.startsWith("serverPort=")){
        // read port
        line.replace("serverPort=", "");
        // if not in the range (signed int can't exceed max port), set to default
        serverPort = line.toInt();
        if(serverPort < 0){
          serverPort = DEFAULTSERVERPORT;
        }

      }else if(line.startsWith("server=")){
        // read server URL or IP as String
        line.replace("server=", "");
        serverUrl = line;

      }else if(line.startsWith("certFingerprint=")){
        // read server certificate fingerprint as String
        line.replace("certFingerprint=", "");
        serverCertFingerprint = line;

      }else if(line.startsWith("serverConn=")){
        // read whether to connect to a remote server
        line.replace("serverConn=", "");
        connectToServer = line.equals("1");

      }else if(line.startsWith("serverContactInterval=")){
        // read whether to connect to a remote server
        line.replace("serverContactInterval=", "");
        serverContactInterval = line.toInt();

      }else if(line.startsWith("serverRC=")){
        // read server URL or IP as String
        line.replace("serverRC=", "");

        serverRemoteControlMode = line.equals("1");

      }else if(line.startsWith("hideSite=")){
        // read server URL or IP as String
        line.replace("hideSite=", "");

        hideSite = line.equals("1");

      }else if(line.startsWith("slaveinfofile=")){
        // read server URL or IP as String
        line.replace("slaveinfofile=", "");

        slaveInfoFileName = line;

      }else if(line.startsWith("dummyfile=")){
        // read server URL or IP as String
        line.replace("dummyfile=", "");

        dummyFileName = line;

      }else if(line.startsWith("pagetitle=")){
        // read server URL or IP as String
        line.replace("pagetitle=", "");

        pageTitle = line;

      }

    }
    // close config file
    conf.close();
  }
}



/**
 * saves config to config file
 */
void saveConfig(){

  // remove config file if existing
  if(SPIFFS.exists(CONFIGFILENAME)){
    SPIFFS.remove(CONFIGFILENAME);
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println("Removed config file");
    #endif
  }

  // create+open config file and write everything to it
  File conf = SPIFFS.open(CONFIGFILENAME, "w");

  // save output specific configuration stuff:
  for(byte connId = 1; connId <= NUM_OUTPUT_PINS; connId++){
    // save pin info in format "pin[number]=[function],[name]"
    conf.print("pin" + String(connId));
    conf.print("=");
    conf.print(pinFunctionalities[connId -1]);
    conf.print("," );
    conf.print(pinNames[connId - 1]);
    conf.print("\n");
  }

  // save whether any page will be shown at all for non-authorized users
  conf.print("hideSite=" + String(hideSite));

  // save GPS mode
  conf.print("\nGPSmode=" + String(gpsMode));
  // save ntp server name
  conf.print("\nntpServer=" + ntpServerName);
  // save server connection information
  conf.print("\nserver=" + serverUrl);
  conf.print("\nserverPort=" + String(serverPort));
  conf.print("\ncertFingerprint=" + serverCertFingerprint);
  conf.print("\nserverConn=");
  if(connectToServer){
    conf.print("1");
  }else{
    conf.print("0");
  }
  conf.print("\nserverRC=");
  if(serverRemoteControlMode){
    conf.print("1");
  }else{
    conf.print("0");
  }
  conf.print("\nserverContactInterval=");
  conf.print(String(serverContactInterval));
  conf.print("\nslaveinfofile=");
  conf.print(slaveInfoFileName);
  conf.print("\ndummyfile=");
  conf.print(dummyFileName);
  conf.print("\npagetitle=");
  conf.print(pageTitle);
  conf.print("\n");

  conf.close();
}

#endif
