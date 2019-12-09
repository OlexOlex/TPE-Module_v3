/**
   contains all the wifi functionality
   Copyright Olex (c) 2017
 */
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>

#define DNS_PORT 53          // DNS requests will be received on port 53

#ifdef DEBUGOUT_CUSTOM
// TODO: needed? remove/remove from ifdef
WiFiEventHandler gotIpEventHandler;
#endif


// tells whether to show debug information about the module IP and WIFI (client mode) - used to show it only once
// TODO:uncomment #ifdef DEBUGOUT_CUSTOM
boolean showWifiInfosOnce = false;
//#endif


// tells whether there is a scan in progress or not
boolean scanningWifis = false;
// in case some other wifi uses the same SSID but a different password => enable looking for other known networks
int lastConnectionScanIndex = 0;
// pretty high number, since int max is 4,294,967,295 and 0 (or any value of millis() sshortly after bootup) - this > threashold
unsigned long lastConnMillis = 42948000;
int numWifisFound = 0;
boolean initializedWifi = false;


/**
   reads all neccesary values from wifi config file
*/
void startWifi() {


  // configure dns server - we don't have errors! :P
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  if(SPIFFS.exists(WIFICONFIGFILENAME)){

    // configure module to not save any SSID/password combinations used when connecting to a wifi network in station/ station + ap mode
    WiFi.persistent(false);
    // turn wifi off for proper reconfiguration (TODO: does this help?)
    WiFi.mode(WIFI_OFF);

    File wificonf = SPIFFS.open(WIFICONFIGFILENAME, "r");
    wificonf.setTimeout(50);

    String line;
    while(wificonf.available()){
      line = wificonf.readStringUntil('\n');

      // check if wifi mode config
      if(line.startsWith("wifimode=") && (line.length() > 9)){
        // read WiFi option in format "WiFiMode=[mode 1/2/3]"
        wifiMode = String(line.charAt(9)).toInt();
        // if the read value is invalid, activate both ap and station mode
        if((wifiMode < 1) || (wifiMode > MAXWIFIMODE)){
          wifiMode = 3;
        }
      }else if(line.startsWith("apssid=") && (line.length() > 7)){
        // read ap SSID
        apSsid = line.substring(7);
        // write info on serial output to recover access to module if passwords got forgotten (need to access TXD + GND for this - aka pry module open)
        BACKUPOUT.println("AP SSID: " + apSsid);
      }else if(line.startsWith("appwd=")){
        // read ap password
        apPwd = line.substring(6);
        // write info on serial output to recover access to module if passwords got forgotten (need to access TXD + GND for this - aka pry module open)
        BACKUPOUT.println("AP password: " + apPwd);
      }else if(line.startsWith("hostname=")){
        // read hostname
        line.replace("hostname=", "");
        // save hostname
        hostName = line;
        // set hostname
        WiFi.hostname(hostName);
        // else assume this to be a SSID of a known wifi and the next line is the according password. if this line is not empty
      }else{
        // get copy of the string to perform conversions for checks
        String linecopy = line;
        // check if line is not empty or contains only spaces and/or tabs
        linecopy.replace(" ", "");
        linecopy.replace("\t", "");
        linecopy.replace("\n", "");
        // if line is not "empty" as defined above, assume it to be the SSID of a known wifi
        // and assume the next line to be the according password
        if(!linecopy.equals("")){
          // if there is another line to read, read password
          if(wificonf.available()){
            String pass = wificonf.readStringUntil('\n');
            // add line as SSID and the next read line as password
            addWifi(line, pass);
            // write info on serial output to recover access to module if passwords got forgotten (need to access TXD + GND for this - aka pry module open)
            BACKUPOUT.println("Will connect to SSID: " + line + " password: " + pass);
          }
        }
      }

    }
    // done, close file
    wificonf.close();

    // create backup AP credentials and configuration if no WifiNetwork known _and_ no AP configured (WiFi mode 1)
    if((numKnownWifis == 0) && (wifiMode == 1)){
      wifiMode = 3;
      apSsid = BACKUPAPSSID;
      apPwd = "";
    }
    // if no ap SSID was configured, configure it
    if(apSsid.equals("")){
      apSsid = BACKUPAPSSID;
    }
  }


  #ifdef DEBUGOUT_CUSTOM
  // print IP to serial output when connected - only when debug output active
  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    DEBUGOUT.print("Station connected, IP: ");
    DEBUGOUT.println(WiFi.localIP());
  });
  #endif

  // connecting is done in main loops call to "checkWifiStatus()"
  DEBUGOUT.println("Wifi config read");
}


/**
 * check if current wifi mode is the one desired, if not, configure it accordingly
 * if wifi mode is 1 or 3 (station / sta+ap mode) check if still connected
 * if not connected to any WiFi, but scan data available, try to connect to first known network. Else start scan (again)
 * on scan complete, call check for known SSID + tries to connect if known SSID exists
 * TODO: turn on ap if in mode 4 and no wifi connection is established, else turn off app
 */
void checkWifiStatus(){

  // TODO: uncomment ifdef/endif
  //#ifdef DEBUGOUT_CUSTOM
  if(showWifiInfosOnce){
    if(WiFi.status() == WL_CONNECTED){
      DEBUGOUT.print("Connected - Wifi: ");
      DEBUGOUT.print(WiFi.SSID());
      DEBUGOUT.print(" IP: ");
      DEBUGOUT.println(WiFi.localIP()) ;
      // don't show this information again (until reconnection was initialized)
      showWifiInfosOnce = false;
    }
  }
  //#endif

  // check if wifi is running in configured state, if not, reconfigure it and if needed start scanning
  switch(getDesiredWifiMode()){
    // client only mode:
    case 1 :{
      // check for current configuration and reconfigure WiFi if needed
      // keep in mind, that for scanning station mode needs to be active
      if((WiFi.getMode() != WIFI_STA) || !initializedWifi){
        WiFi.mode(WIFI_STA);
        // scanning is done after switch statment
        // start DNS-Server for captive portal (return local IP on any DNS request)
        if(dnsRunning){
          dnsServer.stop();
          dnsRunning = false;
        }
        initializedWifi = true;
      }
      break;
    }
    // ap only mode:
    case 2 : {
      // check for current configuration and reconfigure WiFi if needed
      if((WiFi.getMode() != WIFI_AP) || !initializedWifi){
        WiFi.mode(WIFI_AP);

        // set subnet and gateway addresses
        IPAddress localIP(192,168,1,1);
        IPAddress gateway(192,168,1,1);
        IPAddress subnet(255,255,255,0);
        WiFi.softAPConfig(localIP, gateway, subnet);
        // start DNS-Server for captive portal (return local IP on any DNS request)
        if(!dnsRunning){
          dnsServer.start(DNS_PORT, "*", localIP);
          dnsRunning = true;

        }

        #ifdef BULLSHITSERIALAPINTERFERENCE
        if(getGpsEnabled()){
          Serial.println("disabling GPS to mitigate bullshit interference with starting an AP");
          disableGps();
        }
        #endif

        // start ap with given configuration
        //WiFi.softAP(apSsid.c_str(), apPwd.c_str(), apChannel, apHidden);;
        boolean result = WiFi.softAP(apSsid.c_str(), apPwd.c_str());

        #ifdef BULLSHITSERIALAPINTERFERENCE
        if(getGpsEnabled()){
          enableGps();
          Serial.println("enabling GPS...");
        }
        #endif

        // configure AP and station after switch statement
        #ifdef DEBUGOUT_CUSTOM
        if(result == true){
          DEBUGOUT.println("AP ready");
        }else{
          DEBUGOUT.println("AP setup failed!");
        }
        #endif
        initializedWifi = true;
      }
      break;
    }
    // sta + ap mode:
    case 3 : {
      // check for current configuration and reconfigure WiFi if needed
      // keep in mind, that for scanning station mode needs to be active
      if((WiFi.getMode() != WIFI_AP_STA) || !initializedWifi){
        WiFi.mode(WIFI_AP_STA);
        // scan for networks starts after this switch statement

        // set subnet and gateway addresses
        IPAddress localIP(192,168,1,1);
        IPAddress gateway(192,168,1,1);
        IPAddress subnet(255,255,255,0);
        WiFi.softAPConfig(localIP, gateway, subnet);
        // start DNS-Server for captive portal (return local IP on any DNS request)
        if(!dnsRunning){
          dnsServer.start(DNS_PORT, "*", localIP);
          dnsRunning = true;
        }

        #ifdef BULLSHITSERIALAPINTERFERENCE
        if(getGpsEnabled()){
          Serial.println("disabling GPS to mitigate bullshit interference with starting an AP");
          disableGps();
        }
        #endif

        // start ap with given configuration
        //WiFi.softAP(apSsid.c_str(), apPwd.c_str(), apChannel, apHidden);;
        boolean result = WiFi.softAP(apSsid.c_str(), apPwd.c_str());

        #ifdef BULLSHITSERIALAPINTERFERENCE
        if(getGpsEnabled()){
          enableGps();
          Serial.println("enabling GPS...");
        }
        #endif

        #ifdef DEBUGOUT_CUSTOM
        if(result == true){
          DEBUGOUT.println("Ready");
        }else{
          DEBUGOUT.println("Failed!");
        }
        #endif
        initializedWifi = true;
      }
      break;
    }
    // sta + ap only when not connected to wifi:
    case 4 : {
      // check for current configuration and reconfigure WiFi if needed
      // if not connected to a WiFi network, start scan and start accesspoint if not allready running
      if(((WiFi.status() != WL_CONNECTED) && (WiFi.status() != WL_IDLE_STATUS)) || !initializedWifi){
        // set mode if not allready active
        if(WiFi.getMode() != WIFI_AP_STA){
          WiFi.mode(WIFI_AP_STA);

          #ifdef BULLSHITSERIALAPINTERFERENCE
          if(getGpsEnabled()){
            Serial.println("disabling GPS to mitigate bullshit interference with starting AP");
            disableGps();
          }
          #endif

          // start ap with given configuration
          //WiFi.softAP(apSsid.c_str(), apPwd.c_str(), apChannel, apHidden);;
          boolean result = WiFi.softAP(apSsid.c_str(), apPwd.c_str());

          #ifdef BULLSHITSERIALAPINTERFERENCE
          if(getGpsEnabled()){
            enableGps();
            Serial.println("enabling GPS...");
          }
          #endif

          #ifdef DEBUGOUT_CUSTOM
          if(result == true){
            DEBUGOUT.println("Ready");
          }else{
            DEBUGOUT.println("Failed!");
          }
          #endif
          // configure station later, after switch statement
        }
      // turn off ap if it is active and the module is connected to a WiFi network
      }else if(WiFi.status() == WL_CONNECTED){
        if(WiFi.getMode() != WIFI_STA){
          WiFi.mode(WIFI_STA);
        }
      }
      initializedWifi = true;
      break;
    }
  }


  // if in Mode 1/3/4 scan/connect if not connected, else check if connected to known network
  if(getDesiredWifiMode() != 2){
    // if the wifi is not connected and timeout for connection not done yet [DOES NOT WORK: and not changing states (e.g. connecting) right now], scan for networks
    if((WiFi.status() != WL_CONNECTED) && ((millis() - lastConnMillis) > WIFICONNECTIONTIMEOUT ) ){
      // TODO: && (WiFi.status() != WL_IDLE_STATUS)){

      boolean connecting = false;
      // if connecting failed, try to connect to another network found in the scan
      if((WiFi.status() == WL_CONNECT_FAILED) && (!scanningWifis)){
        // try to find another known SSID among the networks left (save index of last SSID where connecting failed?)

        // search for any other known ssid left
        for(int foundIndex = lastConnectionScanIndex; foundIndex < WiFi.scanComplete(); foundIndex++){
          for(byte knownIndex = 0; knownIndex < numKnownWifis; knownIndex++){
            if(WiFi.SSID(foundIndex).equals(knownWifis[knownIndex])) {//, WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i));
              // configure module to connect to network with provided SSID using the provided password
              WiFi.begin(knownWifis[knownIndex].c_str(), knownWifiPasswords[knownIndex].c_str());
              // debug output
              #ifdef DEBUGOUT_CUSTOM
              DEBUGOUT.println("Found Wifi " + WiFi.SSID(foundIndex) + " - connecting");
              #endif
              // set connecting to true to make outer loop break as well
              connecting = true;
              break;
            }
          }
          // break if network found to connect to
          if(connecting){
            break;
          }
        }
      }
      // kind of "else", but gets executed as well if no further (in case of same SSID but different password) known network was found to connect to
      if(!connecting){
        // if not allready scanning for networks, start scan
        if(!scanningWifis){

          // prepare everything for scanning
          // remove old scan results
          WiFi.scanDelete();
          // start scanning: on scan complete call onWifiScanComplete, scan for hidden networks as well (to be used later to be recognized by BSSID)
          WiFi.scanNetworks(true, true);
          scanningWifis = true;

          // prepare everyting for the next connection attempt
          // reset index of the scanned network which was last attempted to connect to
          lastConnectionScanIndex = 0;

          #ifdef DEBUGOUT_CUSTOM
          DEBUGOUT.println("Started to scan for WiFi networks");
          #endif
        }else{
          // get scanning progress result (or -1/-2 if not finished/triggered
          int foundWifis = WiFi.scanComplete();
          // if finished (0..n results)
          if(foundWifis >= 0){
            onWifiScanComplete(foundWifis);
            scanningWifis = false;
          }
        }
      }
      // reset variable to show debug output on reconnect
      // TODO: uncomment#ifdef DEBUGOUT_CUSTOM
      showWifiInfosOnce = true;
      //#endif
    }

    // check whether WiFi client is connected to known network if configured to run
    // (network credentials might have been deleted)
    if(WiFi.status() == WL_CONNECTED){
      // check if SSID is known (has not been deleted from the list of known networks)
      bool wifiUnknown = true;
      for(byte knownWifiIndex = 0; knownWifiIndex < numKnownWifis; knownWifiIndex++){
        if(WiFi.SSID().equals(knownWifis[knownWifiIndex])){
          wifiUnknown = false;
          // end search
          break;
        }
      }
      // if SSID is not known (anymore) disconnect
      if(wifiUnknown){
        // disconnect
        WiFi.disconnect();
      }
    }
  }


}


// restart wifi AP, e.g. when setting new login credentials
/*void restartWifiAp(){
  boolean result = WiFi.softAP(apSsid.c_str(), apPwd.c_str());
  #ifdef DEBUGOUT_CUSTOM
  if(result == true){
    DEBUGOUT.println("Ready");
  }else{
    DEBUGOUT.println("Failed!");
  }
  #endif
}*/


/**
 * add a new wifi to the list of known wifis (if it does not yet exist, else update the entry)
 * call save routine afterwards!
 */
void addWifi(String ssid, String password){
  boolean wifiKnown = false;
  // check if the given ssid is known anyway
  for(byte i = 0; i < numKnownWifis; i++){
    // if this ssid is allready known
    if(ssid.equals(knownWifis[i])){
      // update password;
      knownWifiPasswords[i] = password;
      // no need to iterate over the other wifis, return
      return;
    }
  }

  // create new array of Strings for SSIDs and passwords
  String* newKnownWifis = new String[numKnownWifis + 1];
  String* newKnownPasswords = new String[numKnownWifis + 1];

  // copy previously known WiFi login credentials to new arrays
  for(byte i = 0; i < numKnownWifis; i++){
    newKnownWifis[i] = knownWifis[i];
    newKnownPasswords[i] = knownWifiPasswords[i];
  }

  // save new number of known wifis
  numKnownWifis++;
  newKnownWifis[numKnownWifis - 1] = ssid;
  newKnownPasswords[numKnownWifis - 1] = password;


  // free current arrays to prevent memory leaks
  delete[]knownWifis;
  delete[]knownWifiPasswords;

  // use new arrays instead of old ones
  knownWifis = newKnownWifis;
  knownWifiPasswords = newKnownPasswords;
}

/**
 * forget a wifi from the list of known wifis
 * call save routine afterward
 */
void forgetWifi(String ssid){
  // TODO: remove debug output
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("Removing WiFi login credentials from known WiFis: " + ssid);
  #endif

  // check if the given ssid is known anyway
  for(byte i = 0; i < numKnownWifis; i++){
    // if we found the ssid
    if(ssid.equals(knownWifis[i])){
      // create new array of Strings for SSIDs and passwords
      String* newKnownWifis = new String[numKnownWifis - 1];
      String* newKnownPasswords = new String[numKnownWifis - 1];

      // copy previously known WiFi login credentials to new arrays
      // but only until we reach the to be deleted one on position i
      for(byte j = 0; j < i; j++){
        newKnownWifis[j] = knownWifis[j];
        newKnownPasswords[j] = knownWifiPasswords[j];
      }
      // copy all other previously known WiFi login credentials to new arrays, but not the one to be deleted
      for(byte j = i+1; j < numKnownWifis; j++){
        newKnownWifis[j -1] = knownWifis[j];
        newKnownPasswords[j-1] = knownWifiPasswords[j];
      }

      // save new number of known wifis
      numKnownWifis--;

      // free current arrays to prevent memory leaks
      delete[]knownWifis;
      delete[]knownWifiPasswords;

      // use new arrays instead of old ones
      knownWifis = newKnownWifis;
      knownWifiPasswords = newKnownPasswords;

      // no need to iterate over the other wifis
      break;
    }
  }


  // create backup AP credentials and configuration if no WiFi network is known anymore _and_ no AP configured (WiFi mode 1)
  // state change will be handled in main loop
  if((numKnownWifis == 0) && (wifiMode == 1)){
    wifiMode = 3;
    apSsid = BACKUPAPSSID;
    apPwd = "";
  }
}


/**
 * search for first known WiFi network SSID and configure station to connect to this one
 * save index of this SSID so if connection fails, the search can continue from there
 */
void onWifiScanComplete(int num){
  numWifisFound = num;
  // debug output
  #ifdef DEBUGOUT_CUSTOM
  for(int i = 0; i < numWifisFound; i++){
    DEBUGOUT.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  #endif

  // iterate over all known wifi networks and see if one of them was found when scanning
  boolean connecting = false;
  for(byte knownIndex = 0; knownIndex < numKnownWifis; knownIndex++){
    for(int foundIndex = lastConnectionScanIndex; foundIndex < numWifisFound; foundIndex++){
      if(WiFi.SSID(foundIndex).equals(knownWifis[knownIndex])) {//, WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i));
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("Found Wifi " + WiFi.SSID(foundIndex) + " - connecting");
        #endif
        // configure module to connect to network with provided SSID using the provided password
        WiFi.begin(knownWifis[knownIndex].c_str(), knownWifiPasswords[knownIndex].c_str());
        // set hostname (seems to be forgotten between disconnects)
        WiFi.hostname(hostName);

        // save index in list of scanned WiFis
        lastConnectionScanIndex = foundIndex;

        // save timestamp (additionally simulates kind of a recent connection fail to prevent the system from starting a new scan
        // without providing time for a wifi connection to be established
        lastConnMillis = millis();

        // set connecting to true to make outer loop break as well
        connecting = true;
        break;
      }
      // break this loop as well if connecting procedure was initialized
      if(connecting){
        break;
      }
    }
  }
  // tell that scanning is done
  scanningWifis = false;
}



/**
 * saves WiFi config to WiFi config file
 * IMPORTANT:
 */
void saveWifiConfig(){

  // remove config file if existing
  if(SPIFFS.exists(WIFICONFIGFILENAME)){
    SPIFFS.remove(WIFICONFIGFILENAME);
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println("Removed wifi config file");
    #endif
  }

  // create+open config file and write everything to it
  File conf = SPIFFS.open(WIFICONFIGFILENAME, "w");

  // save WiFi option in format "WiFiMode=[mode 1/2/3]"
  conf.print("wifimode=" + String(wifiMode));
  // save acesspoint SSID + password
  conf.print("\napssid=" + String(apSsid));
  conf.print("\nappwd=" + String(apPwd));
  // save hostname
  conf.print("\nhostname=" + String(hostName));

  /*
     save SSID and password of known WiFis in followung format:
     [SSID]
     [Password]
   */
  for(byte i = 0; i < numKnownWifis; i++){
    conf.print("\n");
    conf.print(knownWifis[i]);
    conf.print("\n");
    conf.print(knownWifiPasswords[i]);
  }
  conf.print("\n");
  // close file
  conf.close();
}
