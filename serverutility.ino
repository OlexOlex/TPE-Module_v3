/**
    contains all functions needed for serving web page requests
    Copyright Olex (c) 2017
 */

#ifndef SERVERUTILITY
#define SERVERUTILITY


#include <ESP8266WebServer.h>
#include <FS.h>

#define HTTPOK 200 // okay, there you go with my answer
#define HTTPCREATED 201 // user/rule/etc. created
#define HTTPOKNOCONTENT 204 // Everything OK, but no content in answer
#define HTTPBADREQUEST 400 //  DON'T SEND ME SHIT!
#define HTTPUNAUTHORIZED 401 // e.g. no authentification/autorization info provided
#define HTTPFORBIDDEN 403 // e.g. admin request by basic/viewer user
#define HTTPNOTFOUND 404 // classic...
#define HTTPCONFLICT 409 // e.g. requested to remove last admin, requested to create first user w/o admin rights
#define HTTPINSUFFICIENTSTORAGE 507 // e.g. if no more space available for a file upload


#define WEBAPPFILENAME "/webApp.html"

// for showing upload sucess feedback
boolean uploadAttempted = false;
boolean uploadSuccess = false;




// #############################################################
// # general support functions for the webserver
// #############################################################


/**
   returns the html content type String for a file according to its file name ending
   to be used when sending data
*/
String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  // if you have absolutely no idea, assume it's a text file
  return "text/plain";
}


/**
  Checks String for forbidden characters (that make trouble in json, etc.)
*/
String formatForJson(String input){
  input.replace("\"", "\\\"");
  return input;
}

/**
   returns key Strings for the selected pwm/binary value of the connectors for POST and GET
   use 1 to NUM_OUTPUT_PINS
   No check on the range of connector is done, simple concatenation of "pin" + String(connector) !
*/
String getConnectorPwmIdentifier(byte connector) {
  return "pwm" + String(connector);
}

/**
   returns key Strings for the selected pwm/binary value of the connectors for POST and GET
   use 1 to NUM_OUTPUT_PINS
   No check on the range of connector is done, simple concatenation of "pin" + String(connector) !
*/
String getOutputFunctionPinIdentifier(byte connector) {
  return "pinfunction" + String(connector);
}

/**
   returns key Strings for the selected pwm/binary value of the connectors for POST and GET
   use 1 to NUM_OUTPUT_PINS
   No check on the range of connector is done, simple concatenation of "pin" + String(connector) !
*/
String getOutputNameIdentifier(byte connector) {
  return "pinname" + String(connector);
}



// #####################################################
// Webserver functions
// #####################################################

/**
   initializes everything
*/
void initServer() {


  // define on which request the server should execute which function
  // on "" or "/" path, check whether there should be an answer, and whether this should be a dummy page or not
  server.on("/", handleRootRequest);
  server.on("", handleRootRequest);
  // always send webbApp on this path
  server.on("/webApp", sendWebApp);
  server.on("/info", handleInfoRequest);
  // REST API paths
  server.on("/api/control", handleControlRequest);
  server.on("/api/status", handleStatusRequest);
  server.on("/api/gps", handleGpsRequest);
  server.on("/api/login", handleLoginRequest);
  server.on("/api/logout", handleLogoutRequest);
  server.on("/api/updatepwd", handlePwdUpdate);
  server.on("/api/adminstatus", handleAdminStatusRequest);
  server.on("/api/adminoutputs", handleAdminOutputsRequest);
  server.on("/api/adminmodule", handleAdminModuleRequest);
  server.on("/api/adminwifi", handleAdminWifiRequest);
  server.on("/api/adminrules", handleAdminRules);
  server.on("/api/adminusers", handleAdminUsers);
  // show files (only if logged in)
  server.on("/api/files", HTTP_GET, handleAdminFilesRequest);
  // delete files (only if logged in)
  server.on("/api/files", HTTP_PUT, handleAdminFilesRequest);
  // handle file upload (only if logged in)
  server.on("/api/files", HTTP_POST, [](){handleAdminFilesRequest();}, [](){handleFileUpload();});

  // for downloading public files, accessing any nonexisting file/path or any file when logged in.
  server.onNotFound(handleNotFound);


  // now that the server knows how to react on which request, start the server
  server.begin();

  DEBUGOUT.println("Server started");
}




// check if password is right or header is present and contains the ID of an authentified session
// returns index of user in loggedInUsers or -1 if not logged in
signed short checkIsLoggedIn() {

  // if token and user name have been provided, check them
  if (server.hasArg("token") && server.hasArg("user")){
    return checkGetLoggedinUserIndex(decodePercentString(server.arg("user")), server.arg("token"));
  }
  // "else" if password and user name are given, check them
  if (server.hasArg("pwd") && (server.hasArg("user"))) {
    return tryLogin(decodePercentString(server.arg("user")), decodePercentString(server.arg("pwd")));
  }

  // "else" if no login attempt or session token available
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("No auth");
  #endif
  // => not logged in => -1
  return -1;
}

// returns true if admin,
// answers to client with HTTPFORBIDDEN and returns false if no admin,
// answers with HTTPUNAUTHORIZED (if not hideSite) and returns false if not logged in
boolean checkAndAnswerIsLoggedInAdmin(){
    signed short userid = checkIsLoggedIn();
    if ( userid > -1){
      char role = getUserRole(userid);
      if(role != USERADMIN){
        server.send(HTTPFORBIDDEN, "text/json", "{\"error\":\"forbidden\"}");
        return false;
      }else{
        return true;
      }
    // else ignore every input (e.g. for user with role USERVIEWER or everyone else)
    }else{
      if(!hideSite){
        server.send(HTTPUNAUTHORIZED, "text/json", "{\"error\":\"unauthorized\"}");
      }
      // don't answer if the whole thing should be hidden
      return false;
    }
}


/**
    takes care of output setting requests by a client (on/off, pulse, pwm/sequence)
    NOTE: make sure to check for login before calling this
*/
void handleOutputSettings() {

  // turn everything off
  if(server.hasArg("alloff")){
    if(server.arg("alloff").equals("1")){
      // turn everything off
      for(byte i = 1; i <= NUM_OUTPUT_PINS; i++){
        // turn off pwm and sequences or just binary turn off to 0
        if(getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
          setOutputPWM(i, "0");
          stopSequence(i);
        }else if((getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) || (getConnectorFunction(i) == OUTPUTFUNCTION_PULSE)) {
          setOutput(i, "0");
        }
      }
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("turned all off");
      #endif

      return;
    }
  }

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("received arguments:");
  #endif

  for (uint8_t argID = 0; argID < server.args(); argID++) {
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println(server.argName(argID) + ": " + server.arg(argID));
    #endif


    // check for sequence or binary pin settings changes

    // iterate over all output identifiers (1-NUM_OUTPUT_PINS)
    for (byte connID = 1; connID <= NUM_OUTPUT_PINS ; connID++) {

      // if this pin is configured for PWM, set the new sequence name
      if (server.argName(argID).equals("pin" + String(connID))) {
        if (getConnectorFunction(connID) == OUTPUTFUNCTION_PWM) {
          // only start a new sequence if it is a different sequence
          if (!decodePercentString(server.arg(argID)).equals(getSelectedOption(connID))) {
            // start the actual sequence
            startSequence(connID, decodePercentString(server.arg(argID)));
          }
        } else if (getConnectorFunction(connID) == OUTPUTFUNCTION_TOGGLE) {
          // set output to given value
          setOutput(connID, server.arg(argID));

        } else if (getConnectorFunction(connID) == OUTPUTFUNCTION_PULSE) {
          // if the value is 1
          if (server.arg(argID).equals("1")) {
            // pulse output connID
            pulseOutput(connID);
          }
        }
      }
    }
  }

  // set given PWM value if output is configured as pwm and no sequence is running on it
  for (byte connID = 1; connID <= NUM_OUTPUT_PINS ; connID++) {
    // if the parameter is relevant at all (pin configured as pwm + no sequence running + not just set to no sequence running),
    // set the pwm rate
    if ((getConnectorFunction(connID) == OUTPUTFUNCTION_PWM) && (getSelectedOption(connID).equals(getNoSequenceIdentifier()))){ // && (!newSequenceSet[connID - 1])) {
      // if a pwm rate is to be set, set it
      if (server.hasArg(getConnectorPwmIdentifier(connID))) {
        if(server.arg(getConnectorPwmIdentifier(connID)) != ""){
          // sets the PWM and saves the value to pinValues[connID-1]
          setOutputPWM(connID, server.arg(getConnectorPwmIdentifier(connID)));
        }
      }
    }
  }

  return;
}


/**
   show either the main page or a dummy page, or ignore requests to hide the server on non-configured paths
*/
void handleRootRequest() {
  // don't answer if the site should be hidden
  if (!hideSite) {
    if ( showDummyFile ) {
      sendDummyPage();
    } else {
      sendWebApp();
    }
  }
}



/**
  Send static web page, javascript of that page will do the rest
*/
void sendWebApp(){
  File returnFile = SPIFFS.open(WEBAPPFILENAME, "r");
  if(returnFile){
    // stream file
    if (server.streamFile(returnFile, getContentType(WEBAPPFILENAME)) != returnFile.size()) {
      DEBUGOUT.println("Sent less data than expected!");
    }
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println("sent file " + server.uri());
    #endif
    // close file
    returnFile.close();
    // done, further requests will be to API or
    return;
  }else{
    server.send(
      HTTPNOTFOUND,
      "text/html",
      "<html><body><h1>404 Page not found</h1><br><br>Upload site files (html, JS and CSS) to fix this problem.</body></html>"
    );
  }
}

/**
   handle for "/control" path requests and if no dummy page is set, "" and "/" paths as well
   returns current settings for the outputs and sequence options in format
      {
        "outputs": ["name": "[name]", "id": [1-6], "type": '[p/t/b/n]' (,"value": [value] (, "seq": "[seq]"))}, {...}...],
        "sequences": ["sequencename1", ... "sequencenameN"]
      }
*/
void handleControlRequest() {

  signed short userid = checkIsLoggedIn();
  if ( userid > -1){
    char role = getUserRole(userid);
    if((role == USERADMIN) || (role == USERBASIC)){
      // check for parameters controlling outputs
      handleOutputSettings();
    }
  // else ignore every input (e.g. for user with role USERVIEWER or everyone else)
  }else if(hideSite){
    // don't answer if the whole thing should be hidden
    return;
  }

  // start main object
  String message = "{";

  message += "\"outputs\":[";
  for (byte i = 1; i <= NUM_OUTPUT_PINS; i++) {
     // construct {"outputs": [{"name": ["name"], "id": [1-6], "type": '[p/t/b/n]' (,"value": [value] (, "seq": "[seq]"))}, {...}...]}
    message += "{\"name\":\"";
    message += formatForJson(getConnectorName(i));
    message += "\",\"id\":";
    message += String(i);
    message += ",\"type\":\"";
    message += getConnectorFunction(i);
    message += "\"";

    // if connector one is active (pwm, binary toggle or binary pulse), show according options, else ignore it:
    if (getConnectorFunction(i) == OUTPUTFUNCTION_PWM) {
      message += ",\"value\":";
      // current PWM value (if no sequence running)
      message += String(pinValues[i - 1]);

      message += ",\"seq\":\"";
      // generate javascript for live update
      message += getSelectedOption(i);
      message += "\"";
    // if connector is in binary toggle mode
    } else if (getConnectorFunction(i) == OUTPUTFUNCTION_TOGGLE) {
      message += ",\"value\":";
      message += pinValues[i - 1];
    }
    // pulse and none do not need any further data
    // close output object
    message += "}";
    // add object separating comma after every but the last object (remember, we started with 1!)
    if(i<NUM_OUTPUT_PINS){
      message += ",";
    }

  }

  // add all available sequence options
  message += "],\"sequences\":[";

  boolean firstSequence = true;
  // iterate over all known options (aka files)
  for (byte optionId = 0; optionId < sequenceCount; optionId++){
      if(firstSequence){
        firstSequence = false;
      }else{
        message += ",";
      }
      message += "\"";
      message += formatForJson(sequenceNames[optionId]);
      message += "\"";
  }
  // close sequence array and main json object
  message += "]}";
  // send data
  server.send(HTTPOK, "text/json", message);

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("sent answer for control request");
  #endif
}



/**
   handles requests for path "/status". Returns::
    * WiFi information strings (apssid, apip, ssid, ip, hostname, gatewayip, dnsip)
    * voltage reading in mV ("vbatmv":[value])
    * time + date of last gps contact as strings (lastutctime, lastutcdate)
 */
void handleStatusRequest() {
  signed short userid = checkIsLoggedIn();
  if ( userid == -1){
    // send dummy page if neccessary
    if(hideSite){
      return;
    }
  }


  String message = "{\"vbatmv\":";
  // read the external voltage on ADC pin using 1.5k to GND and 4.7k to VBat (1 Cell LiPo)
  // (better is 1,5k to GND and 15k to VBat, the multiply the value by 11)
  // and calculate battery voltage in mV)
  // read voltage from ADC-pin
  message += analogRead(A0) * ADCVOLTAGEMULTIPLIER;
  // if connected to a WiFi, show the SSID and device IP as well
  if (WiFi.status() == WL_CONNECTED) {
    message += ",\"ssid\":\"";
    message += formatForJson(WiFi.SSID());
    message += "\",\"ip\":\"";
    // why the fuck does String(WiFi.localIP()) generate only weird numbers, but using Serial.println(WiFi.localIP()) works?!?!?
    // working workaround:
    message += String(WiFi.localIP()[0]);
    message += ".";
    message += String(WiFi.localIP()[1]);
    message += ".";
    message += String(WiFi.localIP()[2]);
    message += ".";
    message += String(WiFi.localIP()[3]);
    message += "\",\"hostname\":\"";
    message += formatForJson(WiFi.hostname());
    // show gateway IP
    message += "\",\"gatewayip\":\"";
    message += String(WiFi.gatewayIP()[0]);
    message += ".";
    message += String(WiFi.gatewayIP()[1]);
    message += ".";
    message += String(WiFi.gatewayIP()[2]);
    message += ".";
    message += String(WiFi.gatewayIP()[3]);
    // show dns IP
    message += "\",\"dnsip\":\"";
    message += String(WiFi.dnsIP()[0]);
    message += ".";
    message += String(WiFi.dnsIP()[1]);
    message += ".";
    message += String(WiFi.dnsIP()[2]);
    message += ".";
    message += String(WiFi.dnsIP()[3]);
    message += "\"";
  }

  // if WiFi AP is opened, show SSID and IP of it
  if(getDesiredWifiMode() != 1){
    message += ",\"apssid\":\"";
    message += formatForJson(apSsid);
    message += "\",\"apip\":\"";
    message += String(WiFi.softAPIP()[0]);
    message += ".";
    message += String(WiFi.softAPIP()[1]);
    message += ".";
    message += String(WiFi.softAPIP()[2]);
    message += ".";
    message += String(WiFi.softAPIP()[3]);
    message += "\"";
  }

  // if GPS is enabled
  if(getGpsEnabled()){
    message += ",\"gpsstate\":\"enabled\",\"lastgpsdate\":\"";
    message += lastValidGpsUtcDate;
    message += "\",\"lastgpstime\":\"";
    message += lastValidGpsUtcTime;
    message += "\",\"posfix\":";
    message += posFix;
  }else{
    message += ",\"gpsstate\":\"disabled\"";
  }

  // show free heap space
  message += ",\"freeheap\":";
  message += ESP.getFreeHeap();
  // UTC timestamp of current time
  message += ",\"moduletime\":";
  message += timeClient.getEpochTime();
  message += "}";

  server.send(HTTPOK, "text/json", message);

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handled status request");
  #endif
}

/**
   handles the request for path "/info"
*/
void handleInfoRequest() {
  signed short userid = checkIsLoggedIn();
  if ( (userid < 0) && hideSite){
    // don't answer if the whole thing should be hidden
    return;
  }

  File returnFile = SPIFFS.open(slaveInfoFileName, "r");
    if(returnFile){
      // stream file
      if (server.streamFile(returnFile, getContentType(server.uri())) != returnFile.size()) {
        DEBUGOUT.println("Sent less data than expected!");
      }
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("sent file " + server.uri());
      #endif
      // close file
      returnFile.close();
      return;
    }
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handled info page request");
  #endif
}


/**
   handles the request for path "/gps"
   returns GPS data and time
           lastgpsdate (of last valid position), lastgpstime (o.l.v.p.), lastutcdate, lastutctime, lat, lon, elev, gndspeed, course,h ordil, numsat, posfix
*/
void handleGpsRequest() {

  signed short userid = checkIsLoggedIn();
  if ( (userid < 0) && hideSite){
    // don't answer if the whole thing should be hidden
    return;
  }


  String message = "{";

  // if GPS is enabled
  if(getGpsEnabled()){

    message += "\"gpsstate\":\"enabled\",\"lastgpstime\":\"";
    // timestamp of last valid GPS position - time part
    message += lastValidGpsUtcTime;
    message += "\",\"lastgpsdate\":\"";
    // date part
    message += lastValidGpsUtcDate;
    message += "\",\"lon\":\"";
    // longitude
    message += lonPos;
    message += "\",\"lat\":\"";
    // latitude
    message += latPos;
    message += "\",\"elev\":\"";
    // elevation above sea (m)
    message += elevation;
    message += "\",\"gndspeed\":\"";
    // speed on ground
    message += groundSpeed;
    message += "\",\"course\":\"";
    // coursse on ground (degree)
    message += course;
    message += "\",\"hordil\":\"";
    // horizontal dillution
    message += horDil;
    message += "\",\"numsat\":\"";
    // #satellites used for position fix
    message += numSatUsed;
    message += "\",\"posfix\":\"";
    // type of position fix (none, 2D/3D, differential, estimate (0, 1, 2, 6))
    message += posFix;
    message += "\"";

  }else{
    message += "\"gpsstate\":\"disabled\"";
  }
  message += "}";

  // send answer
  server.send(HTTPOK, "text/json", message);

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handled gps page request");
  #endif
}

// used for login
void handleLoginRequest(){
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("received logoin request");
  #endif

  if(server.hasArg("user") && server.hasArg("pwd")){

    signed short userId = checkIsLoggedIn();
    if(userId > -1){
      String message = "{\"token\":\"";
      message += loggedInUsersVector.at(userId)->token;
      message +="\",\"role\":\"";
      message += loggedInUsersVector.at(userId)->role;
      message +="\",\"title\":\"";
      message += pageTitle;
      message += "\"}";
      // send answer to successful login
      server.send(HTTPOK, "text/json", message);
      return;
    }
  }

  if(!hideSite){
    // send answer to unauthorized request only if site should not be hidden
    server.send(HTTPUNAUTHORIZED, "text/json", "{\"error\":\"unauthorized\"}");
  }
  // if site hidden, send no content
  return;
}


/**
   handles the request for path "/api/logout"
   returns JSON object containing "success" (1 successful logout, 0 logout failed)
                                  "msg" when logout attempt unsuccessful
*/
void handleLogoutRequest() {
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("received logout request");
  #endif

  if(server.hasArg("user") && server.hasArg("token")){

    if(checkIsLoggedIn() > -1){
      logout(server.arg("user"), server.arg("token"));
      // send answer to logout
      server.send(HTTPOKNOCONTENT, "text/json", "{}");
      return;
    }
  }

  if(!hideSite){
    // send answer to unauthorized request only if site should not be hidden
    server.send(HTTPUNAUTHORIZED, "text/json", "{\"error\":\"unauthorized\"}");
  }
  return;


}



/**
 * provides wifi configuration
 * add wifi
 * show known wifis
 * delete wifi
 * activate WPS listening
 * TODO: enable ap SSID hiding option (use WiFi.softAP(ssid, password, channel, hidden); )
 * TODO: if AP running, show in addition to how many devices are connected by WiFi.softAPgetStationNum()
 *        the IP/MAC of those devices
 * TODO: encode the SSIDs - they might contain " character
 */
void handleAdminWifiRequest(){
  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }

  String message = "{";
  bool changesDone = false;


  // check for a wifi config of the client to be removed
  if(server.hasArg("forgetwifi")){
    if(server.arg("forgetwifi") != ""){
      // remove wifi - but don't forget to decode the potentially percent-encoded string!
      forgetWifi(decodePercentString(server.arg("forgetwifi")));
      changesDone = true;
    }
  }


  // check for accesspoint config data
  if(server.hasArg("apssid")){
    if(server.hasArg("appwd")){
      String apSsidCp = decodePercentString(server.arg("apssid"));
      // if SSID is not an empty string (any bitstring is a valid SSID - sadly)
      // and if it actually is different to the current SSID
      if(!apSsidCp.equals("") && !apSsidCp.equals(apSsid)){
        // set SSID and password
        apSsid = apSsidCp;
        apPwd = decodePercentString(server.arg("appwd"));
        // restart accesspoint (if active aka wifi mode is 2 or 3 )
        if(getDesiredWifiMode()!=WIFI_STA){
          //restartWifiAp();
          // make the
          initializedWifi = false;
        }
        changesDone = true;
      }
    }
  }

  // check for accesspoint enable/disable request
  if(server.hasArg("enwifiap")){
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println(server.arg("enwifiap"));
    #endif

    if(server.arg("enwifiap").equals("1")){
      // if station mode is active, set mode to station + ap
      // else the mode is allready ap only (where would the request come from?)
      if(getDesiredWifiMode() == 1){
        setDesiredWifiMode(3);
        changesDone = true;
      }
    }else if(getDesiredWifiMode() == 3){
      // disable ap mode, but only if client/station is active, as well! (prevents lockout!)
      setDesiredWifiMode(1);
    }
  }

  // check whether the wifi client has to be enabled
  if(server.hasArg("enwific")){
    // enable wifi client/station only if ap mode is active (otherwise the station mode is active, anyway)
    if((getDesiredWifiMode() == 2) && (server.arg("enwific").equals("1"))){
      setDesiredWifiMode(3);
      changesDone = true;
    }else if((getDesiredWifiMode() == 3) && (server.arg("enwific").equals("0"))){
      // disable wifi station/client only if the accesspoint is still active! (prevents lockout)
      setDesiredWifiMode(2);
      changesDone = true;
    }
  }

  // check data of a new wifi for the client to add - empty password okay, empty SSID not
  if(server.hasArg("ssid")){
    if(server.hasArg("wifipwd")){
      if(server.arg("ssid") != ""){
        // add wifi data - but don't forget to decode the potentially percent-encoded strings!
        addWifi(decodePercentString(server.arg("ssid")), decodePercentString(server.arg("wifipwd")));
        changesDone = true;
      }
    }
  }

  // check for a new hostname to be set
  if(server.hasArg("hname")){
    if(server.arg("hname") != ""){
      // set hostname - but don't forget to decode the potentially percent-encoded string!
      String tempHostName = decodePercentString(server.arg("hname"));
      // check for max length of hostname
      if(tempHostName.length() <= 32){
        // save hostname
        hostName = tempHostName;
        // set hostname
        WiFi.hostname(hostName);
        changesDone = true;
      }
    }
  }

  // check for wps request
  if(server.hasArg("activateWps")){
    // try to get WiFi login credentials via WPS
    if(WiFi.beginWPSConfig()){
      // if successful, save wifi credentials
      addWifi(WiFi.SSID(), WiFi.psk());
      // tell SSID of wifi connected via WPS in answer (don't forget the comma in the end!)
      message += "\"wpswifi\":\"" + WiFi.SSID() + "\",";
      changesDone = true;
    }
  }

  // send wifi ap configuration (except for the password)
  message += "\"apssid\":\"";
  message += formatForJson(String(apSsid));
  // send wifi mode
  message += "\",\"wifimode\":";
  message += getDesiredWifiMode();


  // if WiFi is configured to be AP or AP+STA, show how many devices are connected to the ap
  if(getDesiredWifiMode() != WIFI_STA){
    message += ",\"numwificlients\":" + String(WiFi.softAPgetStationNum());
  }

  message += ",\"hostname\":\"";
  message += formatForJson(hostName);
  message += "\",\"knownWifis\":[";

  //TODO: encode the SSIDs - they might contain " character
  for(byte n=0; n < numKnownWifis; n++){
    // concatenate using a comma
    if(n > 0){
      message += ",";
    }
    message += "\"";
    message += knownWifis[n];
    message += "\"";
  }
  message += "]}";

  // send answer
  server.send(HTTPOK, "text/json", message);

  // save configuration to file. main loop calls function to react on changes
  if(changesDone){
    saveWifiConfig();
  }
}


/**
 * sends detailed information for users with admin permissions
 * sends JSON object containing
 *          WiFi information (wifiname, wifiip, hostname, gateway, dns, apssid, apip)
 *          module information (freeram, totaldisk, freedisk)
 *          battery voltage (vbatmv)
 */
void handleAdminStatusRequest(){

  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }

  String message = "{\"vbatmv\":";
  message += analogRead(A0) * ADCVOLTAGEMULTIPLIER;
  // if module connects to Wifi networks, show information
  if (WiFi.status() == WL_CONNECTED) {
    message += ",\"ssid\":\"";
    message += formatForJson(WiFi.SSID());
    message += "\",\"ip\":\"";
    // why the fuck does String(WiFi.localIP()) generate only weird numbers, but using Serial.println(WiFi.localIP()) works?!?!?
    // working workaround:
    message += String(WiFi.localIP()[0]);
    message += ".";
    message += String(WiFi.localIP()[1]);
    message += ".";
    message += String(WiFi.localIP()[2]);
    message += ".";
    message += String(WiFi.localIP()[3]);
    message += "\",\"hostname\":\"";
    message += formatForJson(WiFi.hostname());
    // show gateway IP
    message += "\",\"gatewayip\":\"";
    message += String(WiFi.gatewayIP()[0]);
    message += ".";
    message += String(WiFi.gatewayIP()[1]);
    message += ".";
    message += String(WiFi.gatewayIP()[2]);
    message += ".";
    message += String(WiFi.gatewayIP()[3]);
    // show dns IP
    message += "\",\"dnsip\":\"";
    message += String(WiFi.dnsIP()[0]);
    message += ".";
    message += String(WiFi.dnsIP()[1]);
    message += ".";
    message += String(WiFi.dnsIP()[2]);
    message += ".";
    message += String(WiFi.dnsIP()[3]);
    message += "\"";
  }

  // if WiFi AP is opened, show SSID and IP of it
  if(getDesiredWifiMode() != 1){
    message += ",\"apssid\":\"";
    message += formatForJson(apSsid);
    message += "\",\"apip\":\"";
    message += String(WiFi.softAPIP()[0]);
    message += ".";
    message += String(WiFi.softAPIP()[1]);
    message += ".";
    message += String(WiFi.softAPIP()[2]);
    message += ".";
    message += String(WiFi.softAPIP()[3]);
    message += "\"";
  }

  // show chip ID, free heap space, file system usage
  message += ",\"chipid\":\"";
  message += ESP.getChipId();
  message += "\",\"freeheap\":";
  message += ESP.getFreeHeap();
  message += ",\"useddisk\":";
  // get filesystem info to tell details about its usage
  FSInfo fsInfo;
  SPIFFS.info(fsInfo);
  // calculate percentage of used space
  message += fsInfo.usedBytes;
  // show flash size
  message += ",\"totaldisk\":";
  message += fsInfo.totalBytes;

  // if GPS is enabled
  if(getGpsEnabled()){
    message += ",\"gpsstate\":\"enabled\",\"lastgpsdate\":\"";
    message += lastValidGpsUtcDate;
    message += "\",\"lastgpstime\":\"";
    message += lastValidGpsUtcTime;
    message += "\",\"posfix\":";
    message += posFix;
  }else{
    message += ",\"gpsstate\":\"disabled\"";
  }

  // UTC timestamp of current time
  message += ",\"moduletime\":";
  message += timeClient.getEpochTime();
  message += ",\"fwversion\":\"" + String(VERSION) + "\"";
  message += "}";

  // send answer
  server.send(HTTPOK, "text/json", message);

}


/**
 * Master config page for outputs
 * configures outputs (name, function)
 */
void handleAdminOutputsRequest(){

  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }

  bool changesDone = false;

  // check for rename or reconfigure of any output functions:
  for(byte pinNum = 1; pinNum <= NUM_OUTPUT_PINS; pinNum++){
    // set new settings
    if(server.hasArg("pinname" + String(pinNum))){
      if((!server.arg("pinname" + String(pinNum)).equals(""))
          && (!server.arg("pinname" + String(pinNum)).equals(getConnectorName(pinNum)))){
        // save pin name if new name is neither empty string nor old name
        setConnectorName(pinNum, decodePercentString(server.arg("pinname" + String(pinNum))));
        changesDone = true;
      }
    }
    if(server.hasArg("pinfunction" + String(pinNum))){
      // only initialize pin when its Mode changed
      if((!server.arg("pinfunction" + String(pinNum)).equals(""))
          && (!server.arg("pinfunction" + String(pinNum)).startsWith(String(getConnectorFunction(pinNum)))) ){
        // set pin function (wants only a char - which is the first (and hopefully only) char in the given String
        setConnectorFunction(pinNum, server.arg("pinfunction" + String(pinNum)).charAt(0));
        changesDone = true;
      }
    }
  }

  // start main object
  String message = "{";

  message += "\"outputs\":[";
  for (byte i = 1; i <= NUM_OUTPUT_PINS; i++) {
     // construct {"outputs": [{"name": ["name"], "id": [1-6], "type": '[p/t/b/n]' (,"value": [value] (, "seq": "[seq]"))}, {...}...]}
    message += "{\"name\":\"";
    message += formatForJson(getConnectorName(i));
    message += "\",\"id\":";
    message += String(i);
    message += ",\"type\":\"";
    message += getConnectorFunction(i);
    // close output object
    message += "\"}";
    // add object separating comma after every but the last object
    if(i<NUM_OUTPUT_PINS){
      message += ",";
    }

  }

  // close sequence array and main json object
  message += "]}";
  // send data
  server.send(HTTPOK, "text/json", message);

  // save configuration to file
  if(changesDone){
    saveConfig();
  }
}

/**
 * List rules and rule options, add/remove/modifie rules on request
 */
void handleAdminRules(){

  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }
  // TODO: Check for requests for new rules, rule deletion and rule enabling/disabling
  // TODO: Return list of all rules



  DEBUGOUT.println("NIY! handleAdminRules()");
}

/**
 * List rules and rule options, add/remove/modify rules on request
 */
void handleAdminModuleRequest(){
  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }

  // only add one if there really was something changed! (to reduce flash wearing)
  byte changesDone =  0;
    String message = "{";

  // if the server credentials should be set, set them
  if(server.hasArg("serverurl") && (server.hasArg("serverport"))){
    int port = server.arg("serverport").toInt();
    String newServerUrl = decodePercentString(server.arg("serverurl"));
    if((port >= 0)
        && ((port != serverPort) || (!serverUrl.equals(newServerUrl)))
        && (port < 65353)){
      // if ethe port is in range, save the data
      serverUrl = newServerUrl;
      serverPort = port;
      // adeactivate connecting  to an external server
      if(serverUrl.equals("")){
        connectToServer = false;
      }
      changesDone++;
    }
  }

  // if the module should update its firmware to the content of a file in SPIFFS
  if(server.hasArg("fwupdatefile") && server.hasArg("fwmd5")){
    String updateFileName = decodePercentString(server.arg("fwupdatefile"));
    String updateFileMD5 = server.arg("fwmd5");
    Serial.println("md5: " + updateFileMD5 + " length: " + String(updateFileMD5.length()));
    // only (try to) update if the file name is not empty
    if(!updateFileName.equals("") && (updateFileMD5.length() == 32)){
      if(!updateFileName.startsWith("/")){
        updateFileName = "/" + updateFileName;
      }
      byte status = doUpdate(updateFileName, updateFileMD5);
      message += "\"updateStatus\":" + String(status) + ",";
      changesDone ++;
    }
  }

  // if the server certificate fingerprint should be set, set it
  if(server.hasArg("certfingerprint")){
    serverCertFingerprint = decodePercentString(server.arg("certfingerprint"));
    changesDone++;
  }

  // if there is a value telling whether a remote server should be used and for what functionality
  if(server.hasArg("servermode")){
    // in control mode, connect to server and let server control module
    if(server.arg("servermode").equals("c")
        && ((!serverRemoteControlMode) || (!connectToServer))){
      serverRemoteControlMode = true;
      connectToServer = true;
      changesDone++;
    }else{
      // if in supervision mode, only send data to server
      if(server.arg("servermode").equals("s") && (!connectToServer || serverRemoteControlMode)){
        connectToServer = true;
        serverRemoteControlMode = false;
        changesDone++;
      // else turn off connecting to a server, but only if needed
      // (otherwise the configuration would be saved every time, wearing the flash)
      }else if(connectToServer && (server.arg("servermode").equals("n"))){
        connectToServer = false;
        serverRemoteControlMode = false;
        changesDone++;
      }
    }
  }
  // if there is an argument telling us the interval (in seconds!)
  // between requests to the server, save the value
  if(server.hasArg("serverinterval")){
    // unsigned, we do not want any negative numbers,
    // and we want to avoid overflows when multiplying by 1000 (for millisecond value)
    unsigned int interval = server.arg("serverinterval").toInt();
    interval = interval * 1000;
    if((interval > 0) && (interval != serverContactInterval)){
      serverContactInterval = interval;
      changesDone++;
    }
  }
  // if there is an argument telling us the name of the file to be used as slaveinfo file, save the value
  if(server.hasArg("infofile")){
    if(!server.arg("infofile").equals(slaveInfoFileName)){
      slaveInfoFileName = decodePercentString(server.arg("infofile"));
      changesDone++;
    }
  }
  // if there is an argument telling us that the web page should be hidden or not, save the value
  if(server.hasArg("hidesite")){
    if((server.arg("hidesite").equals("true")) && (hideSite == false)){
      hideSite = true;
      changesDone++;
    }else if(hideSite && (server.arg("hidesite").equals("false"))){
      hideSite =  false;
      changesDone++;
    }
  }
  // if there is an argument telling us the file to use as a dummy web page, save the value
  if(server.hasArg("dummyfile")){
    if(!server.arg("dummyfile").equals(dummyFileName)){
      dummyFileName = decodePercentString(server.arg("dummyfile"));
      changesDone++;
    }
  }
  // if there is an argument telling us the file to use as a dummy web page, save the value
  if(server.hasArg("showdummyfile")){
    if(server.arg("showdummyfile").equals("true") && (!showDummyFile)){
      showDummyFile = true;
      changesDone++;
    }else if(showDummyFile && (server.arg("showdummyfile").equals("false"))){
      showDummyFile = false;
      changesDone++;
    }
  }
  // if there is an argument telling us the file to use as a dummy web page, save the value
  if(server.hasArg("pagetitle")){
    if(!server.arg("pagetitle").equals(dummyFileName)){
      pageTitle = decodePercentString(server.arg("pagetitle"));
      changesDone++;
    }
  }

  // sets GPS mode 0 = off, 1 = on, 2 = 1x daily for time sync
  if(server.hasArg("gpsmode")){
    setGpsUsage(server.arg("gpsmode").toInt());
    changesDone++;
  }

  // sets GPS mode 0 = off, 1 = on, 2 = 1x daily for time sync
  if(server.hasArg("ntpserver")){
    ntpServerName = decodePercentString(server.arg("ntpserver"));
    // set new ntp server url
    timeClient.setPoolServerName(ntpServerName.c_str());
    changesDone++;
  }


  // add all information to response
  message += "\"serverrcmode\":";
  message += serverRemoteControlMode;
  message += ",\"connect\":";
  message += connectToServer;
  message += ",\"serverurl\":\"";
  message += serverUrl;
  message += "\",\"serverport\":";
  message += String(serverPort);
  message += ",\"intervalms\":";
  message += String(serverContactInterval);
  message += ",\"certfingerprint\":\"";
  message += serverCertFingerprint;
  message += "\",\"infofile\":\"";
  message += slaveInfoFileName;
  message += "\",\"hidesite\":";
  message += hideSite;
  message += ",\"dummyfile\":\"";
  message += dummyFileName;
  message += "\",\"showdummyfile\":";
  message += showDummyFile;
  message += ",\"title\":\"";
  message += pageTitle;
  message += "\",\"ntpserver\":\"";
  message += ntpServerName;
  message += "\",\"gpsUsage\":";
  message += String(gpsMode); // 0, 1 or 2 - don't forget to convert to String

  // if there were changes done, add number of changes to message
  if(changesDone > 0){
    message += ",\"changesapplied\":";
    message += String(changesDone);
  }

  // if the module should restart, restart it
  if(server.hasArg("reset")){
    if(server.arg("reset").equals("1")){
      // close JSON object and send answer
      message += ",\"action\":\"restart\"}";
      server.send(HTTPOK, "text/json", message);
      ESP.restart();
      return;
    }
  }

  // if the module should be turned off, turn it off
  if(server.hasArg("moduleoff")){
    if(server.arg("moduleoff").equals("1")){
      DEBUGOUT.println("turning off");
      // close JSON object and send answer
      message += ",\"action\":\"off\"}";
      server.send(HTTPOK, "text/json", message);
      yield();
      delay(1000); // wait 1 sec before turning off (serial needs time, etc.)
      powerOff();
      return;
    }
  }

  // close JSON object and send answer
  message += "}";
  server.send(HTTPOK, "text/json", message);


  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handleAdminModuleRequest() done");
  #endif

  // save config if changes were done
  if(changesDone > 0){
    saveConfig();

  }
}


void handlePwdUpdate(){
  signed short userid = checkIsLoggedIn();
  if ( userid == -1){
    // TODO: return HTTPUNAUTHORIZED
    server.send(HTTPUNAUTHORIZED, "text/json", "{\"error\":\"unauthorized\"}");
  // else ignore every input (e.g. for user with role USERVIEWER or everyone else)
  }else if(hideSite){
    // don't answer if the whole thing should be hidden
    return;
  }

  String message = "{\"success\":";

  // handle request to change user password
  if(server.hasArg("pwd") && server.hasArg("newPwd") && server.hasArg("user")){
    if((server.arg("pwd") != "") && (server.arg("newPwd") != "") && (server.arg("user") != "")){
      // TODO! Handle password change request
      message += changePassword(server.arg("user"), server.arg("pwd"), server.arg("newPwd"));
      // close message and send answer
      message += "}";
      server.send(HTTPOK, "text/json", message);
      return;
    }else{
      message += "false, \"reason\":\"emptyString\"}";
      server.send(HTTPBADREQUEST, "text/json", message);
      return;
    }
  }
  // "else"
  message += "false, \"reason\":\"argMissing\"}";
  server.send(HTTPBADREQUEST, "text/json", message);
}

/**
 * List Users, add/remove/modify Users on request
 */
void handleAdminUsers(){
  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }

  byte addState = 0; // 0 => no request, 1 => request but add failed, 2 => request done
  byte removeState = 0; // 0 => no request, 1 => request but remove failed, 2 => request done

  // handle request to add a user
  if(server.hasArg("newUser") && server.hasArg("userPwd") && server.hasArg("userRole")){
    addState = 1;
    if((server.arg("newUser") != "") && (server.arg("userPwd") != "") && (server.arg("userRole") != "")){
      // for some reason this might not work => then leave state = 1
      if(addUser(decodePercentString(server.arg("newUser")), server.arg("userPwd"), server.arg("userRole").charAt(0))){
        // if successful, update state
        addState = 2;
      }
    }
  }

  // handle request to remove a user
  if(server.hasArg("delUser")){
    removeState = 1;
    // if argument not empty and the user is not trying to remove itself,
    // then try to remove the user with the given name
    if((server.arg("delUser") != "") && (server.arg("delUser") != server.arg("user"))){
      // for some reason (User allready removed, etc.) this might not work => then leave state = 1
      if(deleteUser(server.arg("delUser"))){
        // if successful, update state
        removeState = 2;
      }
    }
  }


  // handle request to update a user (format: updateUsers: "Name1 Name2 Name3" roles: "[role char 1][role char 2][role char 3]")
  if(server.hasArg("updateUser") && server.hasArg("role")){
    if((server.arg("updateUser") != "") && (server.arg("role") != "")){
      updateUserRole(server.arg("updateUser"), server.arg("role").charAt(0));
    }
  }


  // send list of users with their role
  String message = "{\"users\":";
  message += getUsersAsJsonArray();


  int httpState = HTTPOK;
  if((removeState == 1) || (addState == 1)){
    // something did not work
    httpState = HTTPCONFLICT;
  }

  // close message and send answer
  message += "}";
  server.send(httpState, "text/json", message);


  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handled handleAdminUsers()");
  #endif
}


/**
 * Lists existing Files with download option
 * provides file upload possibility
 */
void handleAdminFilesRequest(){
  if(!checkAndAnswerIsLoggedInAdmin()){
    // don't do anything if user is not admin
    // answer was allready sent to client
    return;
  }


  // remove files if requested - and build string (json list entries) for feedback
  boolean wereFilesDeleted = false;
  String deletedFiles = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    if(server.argName(i).startsWith("del_")){
      String fileName = server.arg(i);
      // reformat (e.g. %2F to / )
      fileName = decodePercentString(fileName);

      // remove file if it exists
      if(SPIFFS.exists(fileName)){
        SPIFFS.remove(fileName);
        // save name for feedback
        // prepend a comma if there were other files deleted before
        if(wereFilesDeleted){
          deletedFiles += ",";
        }
        deletedFiles += "\"";
        deletedFiles += formatForJson(fileName);
        deletedFiles += "\"";
        wereFilesDeleted = true;

        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("removed file " + fileName);
        #endif
      }
    }
  }

  FSInfo fsInfo;
  SPIFFS.info(fsInfo);

  // if there is a request to upload a file, check its size!
  if(server.hasArg("filesize")){
    DEBUGOUT.println("file size: " + server.arg("filesize"));
    if((fsInfo.totalBytes - fsInfo.usedBytes) < server.arg("filesize").toInt()){
      // reset upload attempt information, so no one can send further upload-write requests
      uploadAttempted = false;
      String message = "{\"freedisk\":" + String(fsInfo.totalBytes - fsInfo.usedBytes) +"}";
      server.send(HTTPINSUFFICIENTSTORAGE, "text/json", message);
      DEBUGOUT.println("ERROR: not enough space on flash for file to be uploaded");
      return;
    }
  }

  // begin object
  String message = "{";
  message += "\"totaldisk\":";
  message += String(fsInfo.totalBytes);
  message += ",\"useddisk\":";
  message += String(fsInfo.usedBytes);
  // add list of deleted files if neccessary
  if(deletedFiles != ""){
    message += ",\"deletedfiles\":[";
    message += deletedFiles;
    message += "]";
  }
  message += ",\"files\":[";

  // list existing files with link to download and option to delete ("del[\filename]"):
  Dir dir = SPIFFS.openDir(FILE_DIR_NAME);
  boolean firstFile = true;
  while (dir.next()) {
    File f = dir.openFile("r");
    if(!String(f.name()).startsWith(MODULEFILEPREFIX)){
      if(!firstFile){
        // when concatenating to previous files, separate by comma
        message += ",";
      }
      // file name
      message += "{\"name\":\"";
      message += formatForJson(f.name());
      message += "\",\"size\":\"";
      // file size
      message += String(f.size());
      message += "\"}";
      f.close();
      firstFile = false;
    }
  }

  // close array, close object
  message += "]}";
  // send answer
  server.send(HTTPOK, "text/json", message);
}




// ########################################################################
// other request handlers (favicon, upload, download, etc)
// ########################################################################

/**
   handles unknown requested paths. Might be files as well
   removed favicon-file to be served always, since this might make this visible as some kind of web server
*/
void handleNotFound() {

  signed short userid = checkIsLoggedIn();
  if ( userid > -1){
    char role = getUserRole(userid);
    if((role == USERADMIN) || (role == USERBASIC)){

    }
  // else ignore every input (e.g. for user with role USERVIEWER or everyone else)
  }
  // if user is logged in or the requested file is public or the favicon file, stream the file
  if((userid > -1) || (server.uri().startsWith(PUBLICFILEPREFIX))){ // || (server.uri() == FAVICON_FILE)){
    File returnFile = SPIFFS.open(server.uri(), "r");
    if(returnFile){
      // stream file
      if (server.streamFile(returnFile, getContentType(server.uri())) != returnFile.size()) {
        DEBUGOUT.println("Sent less data than expected!");
      }
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("sent file " + server.uri());
      #endif
      // close file
      returnFile.close();
      return;
    }
  }else if(hideSite){
    // don't answer if the whole thing should be hidden
    return;
  }

  // if there was nothing to send (no rights to get file, file not existing, etc.)
  // send error message
  String message = "404 File or Handle Not Found";

  // only show further information in debug mode
  #ifdef DEBUGOUT_CUSTOM
  message += "\n\nURI: " + server.uri() + "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " ";
    message += server.argName(i);
    message += ": ";
    message += server.arg(i);
    message += "\n";
  }
  #endif

  server.send(HTTPNOTFOUND, "text/plain", message);

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handle not found, returned:");
  DEBUGOUT.println(message);
  DEBUGOUT.println();
  #endif

}


void sendDummyPage() {
  if (SPIFFS.exists(dummyFileName)) {
    File file = SPIFFS.open(dummyFileName, "r");
    if(server.streamFile(file, getContentType(dummyFileName)) != file.size()){
      DEBUGOUT.println("ERROR sending dummy file, sent data != file data");
    }
    DEBUGOUT.println("sent dummy page");
  } else {
    server.send(HTTPNOTFOUND, "text/html", "<!DOCTYPE html><html><head><meta charset='ASCII'><title>404 Page not found</title></head><body>404 Page not found</body>");
    DEBUGOUT.println("ERROR - could not find dummy page file: " + dummyFileName);
  }
}



/**
   handles file upload requests on /api/files (server needs to be logged in)
   (second function to be passed to "server.on([path], [HTTP_METHOD], []()[main function for html], []()[upload handler function])" )
*/
void handleFileUpload() {

  /* TODO: protection? if(!checkAndAnswerIsLoggedInAdmin()){
    // not admin? Return. Answer was allready sent if neccessary
    return;
  }*/

  // for showing upload status feedback only after an attempt to upload a file
  uploadAttempted = true;

  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){

    // get filesystem info to tell details about its usage
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    String uploadFileName = String(upload.filename.c_str());
    uploadFileName = decodePercentString(uploadFileName);

    // append / to the start of the filename, if not existing
    if(!uploadFileName.startsWith("/")){
      uploadFileName = "/" + uploadFileName;
    }

    // TODO:check for possible further "/" in the file name
    // remove old file if existing - TODO: refuse upload if file allready existing?
    if(SPIFFS.exists(uploadFileName)){
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("removing file prior to upload " + uploadFileName);
      #endif
      SPIFFS.remove(uploadFileName);
    }
    // if file size is no integer, tell requester to go fix his shit
    /* TODO: make this work!
    if((!server.hasArg("filesize") || (server.arg("filesize").toInt() < 1))){
      // reset upload attempt information, so no one can send further upload-write requests
      uploadAttempted = false;
      server.send(HTTPBADREQUEST, "text/json", "{}");
      DEBUGOUT.println("ERROR uploading file. File size not mentioned / less than one Byte.");
      return;
    }*/
    // ignore "warning: comparison between signed and unsigned integer expression"
    // if "filesize" is smaller than 0, the check above will handle this
    // TODO: arg "filesize is not sent by curl!"
    DEBUGOUT.println("filesize: " + server.arg("filesize"));
    if((fsInfo.totalBytes - fsInfo.usedBytes) < server.arg("filesize").toInt()){
      // reset upload attempt information, so no one can send further upload-write requests
      uploadAttempted = false;
      String message = "{\"freedisk\":" + String(fsInfo.totalBytes - fsInfo.usedBytes) +"}";
      server.send(HTTPINSUFFICIENTSTORAGE, "text/json", message);
      DEBUGOUT.println("ERROR: not enough space on flash for file to be uploaded");
      return;
    }

  } else if((upload.status == UPLOAD_FILE_WRITE) && uploadAttempted){
    String uploadFileName = String(upload.filename.c_str());

    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println("UPLOAD_FILE_WRITE - filename: " + uploadFileName);
    DEBUGOUT.println("Upload size: " + String(upload.currentSize));
    #endif

    // don't write files with empty name or contain forbidden/troubling characters
    if(uploadFileName.equals("") || uploadFileName.equals("\\") || (uploadFileName.indexOf('\"') > -1) || (uploadFileName.indexOf('\\') > -1)){
      return;
    }

    // append / to the start of the filename, if not existing
    if(!uploadFileName.startsWith("/")){
      uploadFileName = "/" + uploadFileName;
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.print("added / to filename ");
      DEBUGOUT.println(uploadFileName);
      #endif
    }

    // get filesystem info to tell details about its usage
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    uint32_t freeDiskSpace = fsInfo.totalBytes - fsInfo.usedBytes;

    // only write if the file name is valid and there is enough space left
    if(!uploadFileName.equals("") && (upload.currentSize <= freeDiskSpace)){
      // open file and APPEND! this part
      // (multiple packets needed for uploading large files, so each packet is one part that needs to be appended - TCP is FIFO)
      File uploadFile = SPIFFS.open(uploadFileName, "a+");
      if(!uploadFile){
        // if creating + opening the file failed
        uploadSuccess = false;
      }else{
        // if writing to file is successfull, save state, else set to false
        // TODO: save to temporary file and remove original+move new one at end if everything went well
        if(uploadFile.write(upload.buf, upload.currentSize) == upload.currentSize){
          uploadSuccess = true;
        }else{
          uploadSuccess = false;
        }

        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.print("current fileSize: ");
        DEBUGOUT.println(String(uploadFile.size()));
        #endif
        Serial.print("~"); // 126 in ASCII - hopefully useful as "busy"-signal via the boards TX-LED


        // close file again
        uploadFile.close();
      }

    }else{
      DEBUGOUT.println("ERROR: File name empty");
      uploadSuccess = false;
    }

  } else if(upload.status == UPLOAD_FILE_END){
    // TODO: when upload was written in temporary file, "move" it (aka rename it)
    //#ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println("Upload file end");
    //#endif
  }

  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("handled upload request");
  #endif
}





// #################################################################
// helper functions (header and menu generation, etc)
// #################################################################



/**
 * sends html page which should be sent before turning off
 */
void sendPowerOffConfirm(){
  String message = "{\"poweroff\":1}";
  server.send(HTTPOK, "text/json", message);
}




#endif
