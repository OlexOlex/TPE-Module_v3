/**
    general utility stuff often used - e.g. storing data, reading properties off a file
    Copyright Olex (c) 2017

    TODO MOSTMINOR: test for GLONASS-Messages (µBlox $GN... equal $GP... messages)
 */

#define BASEYEAR 1970
#define BASEYEARADAPTED 30
#define SECONDSPERDAY 86400

/** space for saving selected sequences - TODO: proper stuff working with an array!*/
String selectedOpt1 = "-";
String selectedOpt2 = "-";
String selectedOpt3 = "-";
String selectedOpt4 = "-";
String selectedOpt5 = "-";
String selectedOpt6 = "-";
//String selectedOpt7 = "-";
//String* selectedOptions;

//String utcTime = "-";
//String utcDate = "-";

// Information from the last valid GPS Data received
String lastValidGpsUtcTime = "-";
String lastValidGpsUtcDate = "-";
unsigned long lastValidTimeLocalMillis = 0;
String latPos = "";
String lonPos = "";
String elevation = "-";
String horDil = "-";
String numSatUsed = "-";
String groundSpeed = "-";
String course = "-";
String posFix = "0";
bool validGpsData = false;
// indicates whether currently there is a valid position fix
bool validGpsPos = false;

// used for taking gps string apart
int gpsCurrComma = 0;



/** string identifying no selected sequence */

String getNoSequenceIdentifier() {
  return "-";
}

/**
   turns off module rather safely
*/
void powerOff() {
  // unmount SPIFFS - not neccessary, but done anyway
  SPIFFS.end();
  // turn GPIO15 low for turning off power
  digitalWrite(POWERPIN, 0);
  // wait some time (1 sec) in case power does not run out instantly
  delay(1000);
  // restart module in case power off fails (or if running on development hardware)
  ESP.restart();
}



void powerOn() {
  // turn GPIO15 high for turning on power
  pinMode(POWERPIN, OUTPUT);
  digitalWrite(POWERPIN, 1);
}

/**
   returns the selected sequence option of the connectors 1-7
   other int values will result in "-"
*/
String getSelectedOption(byte connector) {
  switch (connector) {
    case 1 : {
        return selectedOpt1;
      } break;
    case 2 : {
        return selectedOpt2;
      } break;
    case 3 : {
        return selectedOpt3;
      } break;
    case 4 : {
        return selectedOpt4;
      } break;
    case 5 : {
        return selectedOpt5;
      } break;
    case 6 : {
        return selectedOpt6;
      } break;
    default : {
        DEBUGOUT.println("ERROR getSelectedOption: returning \"-\" as selected sequence for connector ID " + connector);
        return getNoSequenceIdentifier();
      }
  }
}



/**
   sets the selected sequence option of the connectors 1-7
   other int values will result in nothing but an error output via serial
*/
void setSelectedOption(byte connector, String sequenceName) {
  switch (connector) {
    case 1 : {
        selectedOpt1 = sequenceName;
      } break;
    case 2 : {
        selectedOpt2 = sequenceName;
      } break;
    case 3 : {
        selectedOpt3 = sequenceName;
      } break;
    case 4 : {
        selectedOpt4 = sequenceName;
      } break;
    case 5 : {
        selectedOpt5 = sequenceName;
      } break;
    case 6 : {
        selectedOpt6 = sequenceName;
      } break;
    default : {
        DEBUGOUT.println("ERROR setting selected option for connector " + String(connector) + " to " + sequenceName);
      }
  }
}



/**
   yield (nonbusy-wait) until the given amount of milliseconds passed
   might take longer than the given time interval
   will handle overflows as well (millis overflows approximatelyevery 50 days
*/
void yieldWaitMs(unsigned long ms) {
  if (ms == 0) {
    return;
  }
  // IMPORTANT: millis might overflow approximately every 50 Days, so might endTimeMillis!
  unsigned long startTimeMillis = millis();
  unsigned long endTimeMillis = startTimeMillis + ms;
  // if endTimeMillis had an overflow, wait until the overflow happens
  if (endTimeMillis < startTimeMillis) {
    // while no overflow happened
    while (millis() > startTimeMillis) {
      yield();
    }
  }

  // while time delay not passed
  while (endTimeMillis >= millis()) {
    // leave time to process other things
    yield();
  }
}


/**
 * checks Serial for new GPS data
 */
void checkGpsInput(){
   while(GPSSERIAL.available() > 0){
    // read line
    String msg = GPSSERIAL.readStringUntil('\n');
    #ifdef DEBUGOUT_CUSTOM
    DEBUGOUT.println(msg);
    #endif

    // only gets changed if something goes wrong
    validGpsData = true;
    gpsCurrComma = 0;

    // extract GGA Data (Time, Latitude, N/S, Longitude, E/W, PosFix Status (>0 ok), #Satellites used, horizontal dillution, altitude above sea, Unit (M = Meter), Rest unused
    // Format: "$G[P/N/L]GGA",hhmmss.ss,ddmm.mmmm,N/S,dddmm.mmmm,E/W,0/1/2/3 [int status],[double #SatUsed],[double horrdill], [double alt],M,
    //            [unused geoid separation], [unused unit Meter], [unused, age of DGPS correction],[unused DiffStationID],[unused Checksum]
    //            [GPS (US)/ GNSS (combined) / GLONASS (Russian)]
    if(msg.startsWith("$GPGGA") || msg.startsWith("$GNGGA") || msg.startsWith("$GLGGA")){
      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("Got GGA message");
      #endif

      // get time
      String utcTimeStr = getNextGpsPart(msg);
      if((utcTimeStr.length() == 9) || (utcTimeStr.length() == 8)){
        // save local timestamp to
        lastValidTimeLocalMillis = millis();
        //utcTime = timeStr;
      }else{
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid time string: " + utcTimeStr + " length not 10 but " + utcTimeStr.length());
        #endif
      }

      // get latitude
      String lat = getNextGpsPart(msg);
      if(lat.length() != 10){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid latitude string: " + lat + " length not 10 but " + lat.length());
        #endif
      }
      // get N/S
      String latns = getNextGpsPart(msg);
      if(latns.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid latitude N/S string: " + latns + " length not 1 but " + latns.length());
        #endif
      }

      // get longitude
      String lon = getNextGpsPart(msg);
      if(lon.length() != 11){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid longitude string: " + lon + " length not 11 but " + lon.length());
        #endif
      }
      // get E/W
      String lonew = getNextGpsPart(msg);
      if(lonew.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid longitude E/W string: " + lonew + " length not 1 but " + lonew.length());
        #endif
      }

      // get position fix value (0 = no Pos, 1 = 2D/3D, 2 = Differential (not used), 6 = Estimated ("DR") Fix
      String actPosFix = getNextGpsPart(msg);
      if(actPosFix.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid posFix string: " + actPosFix + " length not 1 but " + actPosFix.length());
        #endif
      }else{
        if(actPosFix == "0"){
          validGpsPos = false;
          DEBUGOUT.println("no position fix: " + actPosFix);
        }else{
          #ifdef DEBUG_CUSTOM
          DEBUGOUT.println("valid position " + actPosFix);
          #endif
          validGpsPos = true;
        }
      }

      // get num of satellites used
      String numSat = getNextGpsPart(msg);
      if((numSat.length() == 0) || (numSat.length() > 2)){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid numSat string: " + numSat + " length not 1 or 2 but " + numSat.length());
        #endif
      }

      // get horizontal dillution
      String hdil = getNextGpsPart(msg);
      if(hdil.length() == 0){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid horizontal dillution string: " + hdil + " length 0");
        #endif
      }

      // get elevation above sea level
      String elev = getNextGpsPart(msg);
      if(elev.length() == 0){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid elevation string: " + elev + " length 0 ");
        #endif
      }

      // only save data if it is valid
      if(validGpsData && validGpsPos){
        lastValidGpsUtcTime = utcTimeStr;
        // update time (so no NTP server needs to be used)
        setTimeFromGps();

        latPos = lat.substring(0,2) + " " +lat.substring(2) + latns;
        lonPos = lon.substring(0,3) + " "+lon.substring(3) + lonew;
        elevation = elev;
        horDil = hdil;
        numSatUsed = numSat;
        posFix = actPosFix;


        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("Valid GPS position:\nUTC:");
        DEBUGOUT.println(lastValidGpsUtcTime);
        DEBUGOUT.println("Position: " + latPos + ", " + lonPos);
        DEBUGOUT.println("Elevation above sea: " + elevation);
        DEBUGOUT.println("Horizontal dillution: " + horDil);
        DEBUGOUT.println("Number of satellites used: " + numSatUsed);
        #endif
      }

    }else if(msg.startsWith("$GPRMC") || msg.startsWith("$GNRMC") || msg.startsWith("$GLRMC")){
      // Structure:                                     [cog = ref to north?]
      // $GPRMC,hhmmss,status,latitude,N,longitude,E,spd,cog,ddmmyy,mv,mvE,mode*cs<CR><LF>

      #ifdef DEBUGOUT_CUSTOM
      DEBUGOUT.println("Got $GPRMC message");
      #endif

      // get time
      String utcTimeStr = getNextGpsPart(msg);
      if((utcTimeStr.length() == 9) || (utcTimeStr.length() == 8)){
        //utcTime = timeStr;
      }else{
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC time string: " + utcTimeStr + " length not 10 but " + utcTimeStr.length());
        #endif
      }

      // get status value (V = Nav receiver warning, A = OK)
      String stat = getNextGpsPart(msg);
      if(stat.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC status string: " + stat + " length not 1 but " + stat.length());
        #endif
      }else{
        if(stat != "A"){
          validGpsPos = false;
          #ifdef DEBUGOUT_CUSTOM
          DEBUGOUT.println("no valid data: " + stat);
          #endif
        }else{
          validGpsPos = true;
        }
      }

      // get latitude + N/S
      String lat = getNextGpsPart(msg);
      if(lat.length() != 10){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC latitude string: " + lat + " length not 10 but " + lat.length());
        #endif
      }
      // get N/S
      String latns = getNextGpsPart(msg);
      if(latns.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC latitude N/S string: " + latns + " length not 1 but " + latns.length());
        #endif
      }

      // get longitude + E/W
      String lon = getNextGpsPart(msg);
      if(lon.length() != 11){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC longitude string: " + lon + " length not 11 but " + lon.length());
        #endif
      }
      // get E/W
      String lonew = getNextGpsPart(msg);
      if(lonew.length() != 1){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC longitude E/W string: " + lonew + " length not 1 but " + lonew.length());
        #endif
      }

      // get ground speed in knots
      String gSpd = getNextGpsPart(msg);
      if(gSpd.length() == 0){
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC ground speed string: " + gSpd + " length 0");
        #endif
      }

      // get course over ground (horizontal direction of movement) in knots (whyever you need another message to get km/h...)
      String hCourse = getNextGpsPart(msg);
      if(hCourse.length() == 0){
        hCourse = "0";
      }

      // get date
      String utcDateStr = getNextGpsPart(msg);
      if(utcDateStr.length() == 6){
        //utcDate = dateStr.substring(0,6) + "." + dateStr.substring(2,4) + ".20"+ dateStr.substring(4,6);
      }else{
        validGpsData = false;
        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("invalid RMC date string: " + utcDateStr + " length not 10 but " + utcDateStr.length());
        #endif
      }

      // ignore rest of message

      // only save data if it is valid
      if(validGpsData && validGpsPos){
        lastValidGpsUtcTime = utcTimeStr;
        lastValidGpsUtcDate = utcDateStr;
        // update time (so no NTP server needs to be used)
        setTimeFromGps();

        latPos = lat.substring(0,2) + " " +lat.substring(2) + latns;
        lonPos = lon.substring(0,3) + " "+lon.substring(3) + lonew;
        groundSpeed = gSpd;
        course = hCourse;

        #ifdef DEBUGOUT_CUSTOM
        DEBUGOUT.println("Valid RMC GPS position:\nUTC:");
        DEBUGOUT.println(lastValidGpsUtcTime);
        DEBUGOUT.println(lastValidGpsUtcTime);
        DEBUGOUT.println("Ground speed: " + groundSpeed);
        DEBUGOUT.println("Course over ground: " + course);
        #endif
      }
    }
  }
}


/* return Sub-String between first and second comma from the given position
 * returns empty string if none found
 * IMPORTANT: updates startpos
 * TODO?: get only pointer to startpos
 */
String getNextGpsPart(String msg){
  gpsCurrComma = msg.indexOf(',', gpsCurrComma);
  if(gpsCurrComma> -1){
      int nextcomma = msg.indexOf(',', gpsCurrComma + 1);
      if(nextcomma != -1){
        String returnstring = msg.substring(gpsCurrComma +1, nextcomma);
        gpsCurrComma = nextcomma;

        return returnstring;
      }
  }
  return "";
}


/**
 * turn on GPS by hardware - switch on power
 */
void enableGps(){
  // set GPS power pin to low to turn on power for the GPS module (this FET should turn on on low gate level)
  digitalWrite(GPSPWRPIN, LOW);
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("GPS enabled");
  #endif
  return;
}
/**
 * turning off GPS via hardware
 */
void disableGps(){
  // set GPS power pin to high to turn off power for the GPS module (this FET should turn of on high gate level)
  digitalWrite(GPSPWRPIN, HIGH);
  #ifdef DEBUGOUT_CUSTOM
  DEBUGOUT.println("GPS disabled");
  #endif
  return;
}


void setTimeFromGps(){
  // days per month in "normal" years
  static const byte daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // dateStr.substring(0,2) + "." + dateStr.substring(2,4) + ".20"+ dateStr.substring(4,6);
  // results in dd.mm.yyyy
  unsigned int years = lastValidGpsUtcDate.substring(4,6).toInt();
  // years since 1970 - since we only get the last two digits ([20]XX) we have XX + 30 years
  years = years + BASEYEARADAPTED;
  // extra days due to leapyears since 1970 (ignore 2100 where there will be no leap year)
  unsigned int leapYearExtraDays = years/4;
  // days passed the last full years since 01.01.1970
  unsigned long days = (years*365) + leapYearExtraDays;
  // full months of this year
  byte month = lastValidGpsUtcDate.substring(2,4).toInt();
  // add days passed this month (minus one because the current day has not yet been finished)
  days = days + lastValidGpsUtcDate.substring(0,2).toInt() - 1 ;
  // add days passed the last months this year (subtract one for month IDs 0-11)
  for(byte i=0; i<month-1; i++){
    days = days + daysPerMonth[i];
  }
  // if in a leap year and past 29th of february, add one more day
  if(((years % 4) != 0) && (month > 2)){
    // insert 29th of february
    days = days + 1;
  }
  // Format of time String: hhmmss - convert this to seconds
  // get hours and convert to minutes
  unsigned long secsSinceseventies = lastValidGpsUtcTime.substring(0,2).toInt()*60;
  // add minutes and convert to seconds
  secsSinceseventies = (secsSinceseventies + lastValidGpsUtcTime.substring(2,4).toInt())*60;
  // add seconds passed since last full minute
  secsSinceseventies = secsSinceseventies + lastValidGpsUtcTime.substring(4,6).toInt();
  // add all seconds passed since 1.1.1970 0:00 UTC until 0:00 this day
  secsSinceseventies = secsSinceseventies + (days*SECONDSPERDAY);
  // update time
  timeClient.setCurrentEpocSecs(secsSinceseventies);
}

/**
 * decodes Strings encoded GET parameter alike
 */
String decodePercentString(String raw){

  raw.replace("%20"," ");
  raw.replace("%21","!");
  raw.replace("%22","\"");
  raw.replace("%23","#");
  raw.replace("%24","$");
  raw.replace("%26","&");
  raw.replace("%27","'");
  raw.replace("%28","(");
  raw.replace("%29",")");
  raw.replace("%2A","*");
  raw.replace("%2B","+");
  raw.replace("%2C",",");
  raw.replace("%2D","-");
  raw.replace("%2E",".");
  raw.replace("%2F","/");
  raw.replace("%3A",":");
  raw.replace("%3B",";");
  raw.replace("%3C","<");
  raw.replace("%3D","=");
  raw.replace("%3E",">");
  raw.replace("%3F","?");
  raw.replace("%40","@");
  raw.replace("%58","[");
  raw.replace("%5C","\\");
  raw.replace("%5D","]");
  raw.replace("%5E","^");
  raw.replace("%5F","_");
  raw.replace("%60","`");
  raw.replace("%7B","{");
  raw.replace("%7C","|");
  raw.replace("%7D","}");
  raw.replace("%7E","~");
  raw.replace("%0D%0A","\n");
  raw.replace("%0A","\n");
  // replace % last to avoid errors
  raw.replace("%25","%");

  // !     #     $     &     '     (     )     *     +     ,     /     :     ;     =     ?     @     [     ]
  // %21   %23   %24   %26   %27   %28   %29   %2A   %2B   %2C   %2F   %3A   %3B   %3D   %3F   %40   %5B   %5D

  // newline               space "     %     -     .     <     >     \     ^     _     `     {     |     }     ~
  // %0A or %0D or %0D%0A  %20   %22   %25   %2D   %2E   %3C   %3E   %5C   %5E   %5F   %60   %7B   %7C   %7D   %7E
  return raw;
}
