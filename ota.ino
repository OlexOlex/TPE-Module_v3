#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <StreamString.h>


// the magic number any firmware (for the ESP8266) needs to start with
#define FIRMWAREHEADERMAGICNUMBERBYTES 4
#define FIRMWAREHEADERMAGICNUMBER 0xE9

/* updates sketch to one having been uploaded to the file systeem
 * returns 0 if everything is okay, else error code:
 * 1 -> File does not exist or error opening file
 * 2 -> not enough space in firmware part of the flash
 * 3 -> file too short
 * 4 -> file does not start with magic number. Is no compatible firmware file
 * 5 -> begin() failed
 * 6 -> setMD5() failed
 * 7 -> writeStream() failed
 * 8 -> end() failed
 */
byte doUpdate(String updateFileName, String md5sum){
  // check if file exists
  if(!SPIFFS.exists(updateFileName)){
    return 1;
  }
  File updateFile = SPIFFS.open(updateFileName, "r");
  // check if file opening worked
  if(!updateFile){
    return 1;
  }
  // get file size
  int fileSize = updateFile.size();
  if(((int)ESP.getFreeSketchSpace()) < fileSize){
    return 2;
  }

  // check if at least 4 Bytes are available
  if(updateFile.available() < FIRMWAREHEADERMAGICNUMBERBYTES){
    // file does not even contain the magic number header any firmware needs to start with
    return 3;
  }
  // read first 4 Bytes in a buffer
  char buf[FIRMWAREHEADERMAGICNUMBERBYTES];
  updateFile.readBytes(buf, FIRMWAREHEADERMAGICNUMBERBYTES);
  // check first bytes for
  if(buf[0] != FIRMWAREHEADERMAGICNUMBER) {
    Serial.println("[fileUpdate] First Bytes of firmware file are not 0xE9. No compatible firmware.");
    return 4;
  }

  //updateFile.seek(0, SeekSet);
  updateFile.close();

  updateFile = SPIFFS.open(updateFileName, "r");

  StreamString error;

  if(!Update.begin(fileSize, U_FLASH)) {
      Update.printError(error);
      error.trim(); // remove line ending
      DEBUGOUT.println("[fileUpdate]: Update.begin failed! " + error);
      return 5;
  }

  // set md5 checksum
  if(!Update.setMD5(md5sum.c_str())) {
      DEBUGOUT.println("[fileUpdate]: Update.setMD5 failed! " + md5sum);
      return 6;
  }

  if(Update.write(updateFile) != fileSize) {
      Update.printError(error);
      error.trim(); // remove line ending
      DEBUGOUT.println("[fileUpdate]: Update.writeStream failed! " + error);
      // TODO: enable WIFI stuff rather than rebooting
      return 7;
  }

  if(!Update.end()){
      Update.printError(error);
      error.trim(); // remove line ending
      DEBUGOUT.println("[fileUpdate]: Update.end failed! " + error);
      // TODO: enable WIFI stuff rather than rebooting
      return 8;
  }

  Serial.println("update done");
  // close file
  updateFile.close();

  // TODO: really reboot?
  ESP.restart();

  return 0;
}
