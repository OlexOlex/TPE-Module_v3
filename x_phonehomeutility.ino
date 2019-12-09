/**
   contains all the functionality for "calling home" toa specified server for tracking and/or remote controlling
   Copyright Olex (c) 2017
 */

#ifndef PHONEHOMEUTILITY
#define PHONEHOMEUTILITY


//#define SHOWSERVERANSWER 1


// Use WiFiClient class to create TCP connections
WiFiClientSecure client;;

unsigned long lastPhoneHomeMillis = 0;


void initPhoneHome(){
  return;
}


/* establish connection to home server in appropriate intervals
 * if configured for remote control via home server, handle response message
 */
void phoneHome(){

  // if not configured to phone home OR if no wifi connection is established,
  // there is no need to phone home
  if(!connectToServer){
    return;
  }

  /* response:
   *  alloff=1 => "emergency off"
   *  pin[1-6]=[sequencename/1/ß] => if pwm => activate [sequencename] on pin
   *                                 if toggle, set pin to [1/0]
   *                                 if pulse, pulse pin if [1]
   *  pwm[1-6]=[0-1023] => if pwm => set pin to [value] (stop sequence if running)
   *  turnoff=1 => turn off module
   *
   */

  // react to response if configured to do so - really wait for response?
  if(client.connected()){
    boolean gotAnswer = false;
    // If there is data (aka a reply) available, ead all the lines of it and print them to serial port
    while (client.available()) {
      gotAnswer = true;
      String line = client.readStringUntil('\n');
      #ifdef SHOWSERVERANSWER
      DEBUGOUT.println(line);
      #endif
      // analyze only if remote control mode is enabled
      if(serverRemoteControlMode){
        // analyze request string

        // turn all outputs off if requested
        if(line.startsWith("alloff=1")){
          for(byte i = 1; i <= NUM_OUTPUT_PINS; i++){
            // turn off pwm and sequences or just binary turn off to 0
            if(getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
              stopSequence(i);
              setOutputPWM(i, "0");
            }else if((getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) || (getConnectorFunction(i) == OUTPUTFUNCTION_PULSE)) {
              setOutput(i, "0");
            }
          }
          #ifdef DEBUGOUT_CUSTOM
          DEBUGOUT.println("turned all off");
          #endif
          // turn off module if requested
        }else if(line.startsWith("pwm")){
          // check for pwm pin settings changes
          // remove "pwm" leaving [ID]=[value]"
          line = line.substring(3);
          // iterate over all output identifiers (1-NUM_OUTPUT_PINS)
          for (byte connID = 1; connID <= NUM_OUTPUT_PINS ; connID++) {
            // if this pin is configured for PWM, set the new sequence name
            if (line.startsWith(String(connID))) {
              line = line.substring(2); // remove "[ID]="
              if (getConnectorFunction(connID) == OUTPUTFUNCTION_PWM) {
                // strip line off "[ID]=" and set this pwm value
                setOutputPWM(connID, line);
              }
            }
          }
        // pulse/toggle pin or set sequence, if requested
        }else if(line.startsWith("pin")){
          // check for sequence or binary pin settings changes
          // remove "pin" leaving [ID]=[value]"
          line = line.substring(3);
          // iterate over all output identifiers (1-NUM_OUTPUT_PINS)
          for (byte connID = 1; connID <= NUM_OUTPUT_PINS ; connID++) {
            // if this pin is configured for PWM, set the new sequence name
            if (line.startsWith(String(connID))) {
              line = line.substring(2); // remove "[ID]="
              if (getConnectorFunction(connID) == OUTPUTFUNCTION_PWM) {
                // only start a new sequence if it is a different sequence
                if (!line.equals(getSelectedOption(connID))) {
                  // start the given sequence
                  startSequence(connID, line);
                }
              } else if (getConnectorFunction(connID) == OUTPUTFUNCTION_TOGGLE) {
                // set output to given value
                setOutput(connID, line);

              } else if (getConnectorFunction(connID) == OUTPUTFUNCTION_PULSE) {
                // if the value is 1
                if (line.equals("1")) {
                  // pulse output connID
                  pulseOutput(connID);
                }
              }
            }
          }
        // turn off module if requested
        }else if(line.startsWith("turnoff=1")){
          for(byte i = 1; i <= NUM_OUTPUT_PINS; i++){
            // turn off pwm and sequences or just binary turn off to 0
            // then turn off module
            if(getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
              setOutputPWM(i, "0");
              stopSequence(i);
            }else if((getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) || (getConnectorFunction(i) == OUTPUTFUNCTION_PULSE)) {
              setOutput(i, "0");
            }
          }
          // turn off power
          powerOff();
        // restart module if requested
        }else if(line.startsWith("restart=1")){
            for(byte i = 1; i <= NUM_OUTPUT_PINS; i++){
              // turn off pwm and sequences or just binary turn off to 0
              // then turn off module
              if(getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
                setOutputPWM(i, "0");
                stopSequence(i);
              }else if((getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) || (getConnectorFunction(i) == OUTPUTFUNCTION_PULSE)) {
                setOutput(i, "0");
              }
            }
            // restart module
            ESP.restart();
          // set gps usage if requested
        }else if(line.startsWith("gps=")){
          // remove "gps=" leaving [ID]=[value]"
          line = line.substring(4);
          setGpsUsage(line.toInt());
        }else if(line.startsWith("addwifi=")){
          // remove "addwifi=" leaving "[ssid][' '][password]"
          line = line.substring(8);
          int spacepos = line.indexOf(" ");
          // if line contains at least one space character which is not the first character
          if(spacepos>0){
            // split line
            addWifi(line.substring(0,spacepos), line.substring(spacepos + 1));
            saveWifiConfig();
          }

        }else if(line.startsWith("delwifi=")){
          // remove "delwifi=" leaving "[ssid][' '][password]"
          line = line.substring(8);
          forgetWifi(line);
          saveWifiConfig();
        }// "else" ignore line
      }// "else" if not in remote control mode, do nothing
    }
    if(gotAnswer){
      // stop client when an answer was received,
      // so there will be no further aiting for an answer
      client.stop();
    }
  }else{
    DEBUGOUT.println("client not (yet?) connected");
  }

  // if there is no connection to any wifi netork, dont attempt to connect
  if(WiFi.status() != WL_CONNECTED){
    return;
  }

  // if it's time to send data to the server
  // send current status and configuration as request
  if((millis() - lastPhoneHomeMillis) > serverContactInterval){
    lastPhoneHomeMillis = millis();
    // break connection if no answer was received
    if(client.connected()){
      client.stop();
    }

    // delete client and initiate new one
    //~client;

    // send request containing information

    String data = "{";

    data += "\"outputs\":[";
    for (byte i = 1; i <= NUM_OUTPUT_PINS; i++) {
       // construct {"outputs": [{"name": ["name"], "id": [1-6], "type": '[p/t/b/n]' (,"value": [value] (, "seq": "[seq]"))}, {...}...]}
      data += "{\"name\":\"";
      data += getConnectorName(i);
      data += "\",\"id\":";
      data += String(i);
      data += ",\"type\":\"";
      data += getConnectorFunction(i);
      data += "\"";

      // if connector one is active (pwm, binary toggle or binary pulse), show according options, else ignore it:
      if (getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
        data += ",\"value\":";
        // current PWM value (if no sequence running)
        data += String(pinValues[i - 1]);

        data += ",\"seq\":\"";
        // generate javascript for live update
        data += getSelectedOption(i);
        data += "\"";
      // if connector is in binary toggle mode
      } else if (getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) {
        data += ",\"value\":";
        data += pinValues[i - 1];
      }
      // pulse and none do not need any further data
      // close output object
      data += "}";
      // add object separating comma after every but the last object (remember, we started with 1!)
      if(i<NUM_OUTPUT_PINS){
        data += ",";
      }

    }

    // add all available sequence options
    data += "],\"sequences\":[";
    // iterate over all known sequence options
    for (byte optionId = 0; optionId < sequenceCount; optionId++){
        data += "\"";
        data += sequenceNames[optionId];
        data += "\"";
        // add comma after every but the last sequence (remember, we started with 0!)
        if(optionId < sequenceCount -1){
          data += ",";
        }
    }
    // close sequence array
    data += "],\"vbatmv\":";
    // read the external voltage on ADC pin using 1.5k to GND and 4.7k to VBat (1 Cell LiPo)
    // (better is 1,5k to GND and 15k to VBat, the multiply the value by 11)
    // and calculate battery voltage in mV)
    // read voltage from ADC-pin
    data += analogRead(A0) * ADCVOLTAGEMULTIPLIER;
    // if connected to a WiFi, show the SSID and device IP as well
    if (WiFi.status() == WL_CONNECTED) {
      data += ",\"ssid\":\"";
      // TODO: encode '"' for proper JSON format!
      data += WiFi.SSID();
      data += "\",\"ip\":\"";
      // why the fuck does String(WiFi.localIP()) generate only weird numbers, but using System.println(WiFi.localIP()) works?!?!?
      // working workaround:
      data += String(WiFi.localIP()[0]);
      data += ".";
      data += String(WiFi.localIP()[1]);
      data += ".";
      data += String(WiFi.localIP()[2]);
      data += ".";
      data += String(WiFi.localIP()[3]);
      data += "\",\"hostname\":\"";
      data += WiFi.hostname();
      // show gateway IP
      data += "\",\"gatewayip\":\"";
      data += String(WiFi.gatewayIP()[0]);
      data += ".";
      data += String(WiFi.gatewayIP()[1]);
      data += ".";
      data += String(WiFi.gatewayIP()[2]);
      data += ".";
      data += String(WiFi.gatewayIP()[3]);
      // show dns IP
      data += "\",\"dnsip\":\"";
      data += String(WiFi.dnsIP()[0]);
      data += ".";
      data += String(WiFi.dnsIP()[1]);
      data += ".";
      data += String(WiFi.dnsIP()[2]);
      data += ".";
      data += String(WiFi.dnsIP()[3]);
      data += "\"";
    }

    // if WiFi AP is opened, show SSID and IP of it
    if(getDesiredWifiMode() != 1){
      data += ",\"apssid\":\"";
      data += apSsid;
      data += "\",\"apip\":\"";
      data += String(WiFi.softAPIP()[0]);
      data += ".";
      data += String(WiFi.softAPIP()[1]);
      data += ".";
      data += String(WiFi.softAPIP()[2]);
      data += ".";
      data += String(WiFi.softAPIP()[3]);
      data += "\"";
    }

    // show chip ID, free heap space, file system usage
    data += ",\"chipid\":\"";
    data += ESP.getChipId();
    data += "\",\"freeheap\":";
    data += ESP.getFreeHeap();
    data += ",\"usedflash\":";
    // get filesystem info to tell details about its usage
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    // calculate percentage of used space
    data += fsInfo.usedBytes;
    // show flash size
    data += ",\"flashsize\":";
    data += fsInfo.totalBytes;

    // UTC timestamp of current time
    data += ",\"moduleutctime\":";
    data += timeClient.getEpochTime();

    // if GPS is enabled
    if(getGpsEnabled()){

      data += ",\"gpsstate\":\"enabled\",\"lastgpsdate\":\"";
      data += lastValidGpsUtcDate;
      data += "\",\"lastgpstime\":\"";
      data += lastValidGpsUtcTime;
      data += "\",\"lon\":\"";
      // longitude
      data += lonPos;
      data += "\",\"lat\":\"";
      // latitude
      data += latPos;
      data += "\",\"elev\":\"";
      // elevation above sea (m)
      data += elevation;
      data += "\",\"gndspeed\":\"";
      // speed on ground
      data += groundSpeed;
      data += "\",\"course\":\"";
      // coursse on ground (degree)
      data += course;
      data += "\",\"hordil\":\"";
      // horizontal dillution
      data += horDil;
      data += "\",\"numsat\":\"";
      // #satellites used for position fix
      data += numSatUsed;
      data += "\",\"posfix\":\"";
      // type of position fix (none, 2D/3D, differential, estimate (0, 1, 2, 6))
      data += posFix;

    }else{
      data += ",\"gpsstate\":\"disabled";
    }


    // close json object
    data += "\"}";

    // split serverUrl to serverName and serverPath,
    // so subdomain.domain.end/path/to/somewhere becomes
    // subdomain.domain.end and /path/to/somewhere
    String serverName = serverUrl;
    String serverPath = "/";
    int slashIndex = serverUrl.indexOf("/");
    if(slashIndex > 3){
      serverName = serverUrl.substring(0, slashIndex);
      serverPath = serverUrl.substring(slashIndex);
    }
    Serial.println("Server: " + serverName);
    Serial.println("Path: " + serverPath);
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()));


    // try to connect and send request
    if (!client.connect(serverName, serverPort)) {
      DEBUGOUT.println("Connection to " + serverName + " on port " + String(serverPort) + " failed"); // TODO: log if connect failed? Show in web interface?
      return;
    }
    DEBUGOUT.println("Connection established");


    if (!client.verify(serverCertFingerprint.c_str(), serverName.c_str())) {
      DEBUGOUT.println("certificate doesn't match fingerprint");
      // close and return, nothing to do
      client.stop();
      return;
    } /*else {
      Serial.println("certificate matches fingerprint");
    }*/
    // Send request to the server:
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName); // TODO: server name instead???
    // request for plaintext response, else (quality < 1.0) different text stuff
    //      (html, etc. - might be use for hiding the server's functionality)
    client.println("Accept: text/plain,text/*;q=0.1");
    client.println("Content-Type: application/json");
    client.println("User-Agent: TPE-Module");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);

    // reset time from last connections
    lastPhoneHomeMillis = millis();
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()));

  }

}

#endif
