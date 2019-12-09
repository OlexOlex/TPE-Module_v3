/**
    area stuff - GPS and WiFi
    Copyright Olex (c) 2017

    Functionality:
    each rule defines "if in [Area] then do [X]"
    if multiple rules want to set the same output, the first sequence wins before the highest value
 */


/* area type identifiers:
 *  Battery voltage (upper, lower bound)
 *  WiFi connection (SSID, connected/disconnected)
 *  GPS proximity (lon, lat, distance, inside/outside)
 *  GPS area (inside/outside, [(lon, lat), ..])
 *  Time area (starttime, endtime, list of weekdays: active/deactivated)
 *  Time delay area (has to be last to be checked, since the first checking triggers the timer)
 */

#ifndef AREAUTILITY
#define AREAUTILITY

#include "math.h"

#define RULEAPPLY_INSIDE 0
#define RULEAPPLY_OUTSIDE 1
#define RULEAPPLY_STATECHANGE 2

#define AREATYPEBATT 'b'
#define AREATYPETIME 't'
#define AREATYPETIMEDELAY 'd'
#define AREATYPEWIFI 'w'
#define AREATYPEGPSPROX 'p'
#define AREATYPEGPSPOLY 'g'
#define AREATYPENONE 'n'

#define ACTIONSNEXTIDENTIFIER '-'

#define STATETRUE 't'
#define STATEFALSE 'f'
#define STATECHANGE 'c'


#define EARTHRADIUSM 6371000
#define PIBY180 0.0174532925199433 // pi/180




class Action {
  public:
    // Id of the pin to configure
    // TODO "connector" 255 defines a (short) message to the server. Message can be stored in sequence-string
    byte connectorId;
    // sequence to run (ignored if pin is not configured to be pwm / message to server)
    String sequence;
    // value/state of pin to set to, toggle: "0"/"1" (or higher)
    //                               pulse: "1"-"100" [number of times to pulse]
    //                               pwm: "0"-"1023"
    // (ignored if pin is configured to be pwm AND sequence string is not nosequencestring)
    String connectorValue;

    Action(){
      // "connector 0" does not exists and therefore permanently has the functionality "off"
      connectorId = 0;
      sequence = "";
      connectorValue = "0";
    }

    Action(byte targetPinId, String targetSequence, String targetconnectorValue){
      connectorId = targetPinId;
      sequence = targetSequence;
      connectorValue = targetconnectorValue;
      return;
    }

    byte getConnectorID(){
      return connectorId;
    }

    String getSequence(){
      return sequence;
    }

    String getconnectorValue(){
      return connectorValue;
    }

    // execute according to the output configuration
    void execute(){
      if(getConnectorFunction(connectorId) == OUTPUTFUNCTION_PWM){
        if(sequence == ""){
          setOutputPWM(connectorId, connectorValue);
        }else{
          startSequence(connectorId, sequence);
        }

      }else if(getConnectorFunction(connectorId) == OUTPUTFUNCTION_TOGGLE){
        if(connectorValue == 0){
          setOutput(connectorId, "0");
        }else{
          setOutput(connectorId, "1");
        }
      }else if(getConnectorFunction(connectorId) == OUTPUTFUNCTION_PULSE){
        pulseOutput(connectorId);
      }
      // if output was configured to be deactivated (e.g if Action was initialized but not configured)
      // do nothing
    }
};

// Parent Area class. Only used for deriving child classes
// always evaluates to false
class Area {
  public:
    // if inside/outside the given area
    boolean inside;
    // name for user to identify rule
    String areaName;
    // type of area
    char areaType;

    // evaluation function dummy -> to be overridden by derived classes
    virtual boolean evaluate(){
      return false;
    }

    Area(){
      areaName = "";
      inside = false;
      areaType = AREATYPENONE;
    }

    /** returns if the device state is inside this area.
      * Does not make anything go inside anything..
      */
    boolean getInside(){
      return inside;
    }

    /** returns the Area name */
    String getAreaName(){
      return areaName;
    }

    /** returns the type of the Area (used for typecaasting) */
    char getAreaType(){
      return areaType;
    }


    // TODO: destructor => delete AreaName? (String => pointer to '\0' terminated char array?)

};

/** Classic alarm, just with a duration like a date entry in your calendar. */
class TimeArea : public Area {
  public:

    // days of the week (0-6), true => active for this day
    boolean daysOfWeek[7] =  {false, false, false, false, false, false, false};
    // time start and time to end in ms
    long millisStart;
    long millisEnd;

    /** "days": array of boolean[7]! */
    TimeArea(String name, boolean newInside, long newMillisStart, long newMillisEnd){
      areaName = name;
      inside = newInside;
      millisStart = newMillisStart;
      millisEnd = newMillisEnd;
      areaType = AREATYPETIME;
      return;
    }

    /** returns array of the size 7 (0-6) */
    boolean* getDaysOfWeek(){
      return daysOfWeek;
    }

    long getMillisStart(){
      return millisStart;
    }

    long getMillisEnd(){
      return millisEnd;
    }

    boolean evaluate(){
      // TODO TODO TODO!!!
      return false;
    }
};

/** delay until the rule containing it gets active
  * - gets triggered when first evaluating it, so it needs to be the last area to be evaluted
  * - is active/evaluates to true at least once when the delay is up
  * - active as long as the duration specifies (in ms)
  */
class TimeDelayArea : public Area {
  private:

    // time start and time to end in ms
    unsigned long delayMillis;
    unsigned long startMillis;
    unsigned long endMillis;
    // Duration = 0 resets immediately
    unsigned long duration;
    boolean passed;

  public:

    TimeDelayArea(String name, boolean newInside, unsigned long newMillis, unsigned long newDuration){
      areaName = name;
      inside = newInside;
      delayMillis = newMillis;
      areaType = AREATYPETIMEDELAY;
      startMillis = 0;
      endMillis = 0;
      duration = newDuration;
      // to make sure, this does not happen multiple times since millis() has an overflow every ~50 days
      passed = false;
      return;
    }

    unsigned long getDelayMillis(){
      return delayMillis;
    }

    boolean evaluate(){

      // if it already passed, check if it is still active
      if(passed){
        unsigned long endTime = endMillis + duration;
        if(endTime > endMillis){
          if((millis() > endTime) || (millis() < endMillis)){
            reset();
            return !inside;
          }else{
            return inside;
          }
        }else{
          if((millis() > endTime) && (millis() < endMillis)){
            reset();
            return !inside;
          }else{
            return inside;
          }
        }
      }

      // if the variables have not yet been initialized, initialize them
      if((startMillis == 0) && (endMillis == 0)){
        startMillis = millis();
        endMillis = startMillis + delayMillis;
      }

      // normal case
      if(startMillis < endMillis){
        // if millis is larger than endmillis or, in case of an overflow,
        // if millis() is smaller than startMillis
        if((endMillis < millis()) || (startMillis > millis())){
          // if duration is zero, reset immediately
          if(duration == 0){
            reset();
          }else{
            // this rule is "active"
            passed = true;
          }
          // evaluate to true (this Area applies) but keep inside/outside in mind
          return inside;
        }else{
          // evaluate to false (not enough time passed)
          return !inside;
        }
      // in case of overflow:
      }else{
        if((millis() > endMillis) && (millis() < startMillis)){
          // if duration is zero, reset immediately
          if(duration == 0){
            reset();
          }else{
            // this rule is "active"
            passed = true;
          }
          return inside;
        }else{
          // time is not up
          return !inside;
        }
      }
    }

    // resets the "timer"
    void reset(){
      startMillis = 0;
      endMillis = 0;
      passed = false;
    }

};

/** area for checking if the battery voltage is inside specified values
  * lower and upper millivolt are given as bounds
  */
class BattArea : public Area {
  private:

    // lower bound and
    // upper bound in mV (minimium 0V - though the System will likely crash below 1,8V, maximum 6V)
    // protection circuits for LiPo cells will terminate at ~3V
    short lowerMv;
    short upperMv;

  public:

    BattArea(String name, boolean newInside, short newLower, short newUpper){
      areaName = name;
      inside = newInside;
      lowerMv = newLower;
      upperMv = newUpper;
      areaType = AREATYPEBATT;
      return;
    }

    short getUpperMv(){
      return upperMv;
    }

    short getLowerMv(){
      return lowerMv;
    }

    // if voltage is inside the boundaries, return inside, else not inside
    boolean evaluate(){
      short vbatMv = analogRead(A0) * ADCVOLTAGEMULTIPLIER;
      if((vbatMv < upperMv) && (vbatMv > lowerMv)){
        return inside;
      }else{
        return !inside;
      }
    }
};

/** area for checking if the module is connected to a wifi network with the specified SSID
  * if the module is configured to only open an accesspoint, this evaluates to !inside
  */
class WifiArea : public Area {
  private:
    // SSID of the wifi
    String ssid;

  public:

    WifiArea(String name, boolean newInside, String newSsid){
      areaName = name;
      inside = newInside;
      ssid = newSsid;
      areaType = AREATYPEWIFI;
      return;
    }

    String getSsid(){
      return ssid;
    }

    // evaluates ONLY to inside if connected to a WiFi network with the given name
    boolean evaluate(){
      // if wifi mode is in station only or ap+station mode
      if(wifiMode != 2 ){
        // if the module is connected to a wifi network
        if(WiFi.status() == WL_CONNECTED){
          // if the network name matches the name of this rule
          if(WiFi.SSID() == ssid){
            // return that the module is in this area
            return inside;
          }
        }
      }
      // in any other case return !inside
      return !inside;
    }
};

/** Checks for proximity, in meters to given coordinates in radian
  *
  */
class GpsProxArea : public Area {
  private:

    // latitude of the position
    String lat;
    // longitude of the position
    String lon;
    // latitude of the position
    double latRad;
    // longitude of the position
    double lonRad;
    // proximity in meters (don't use less than 10m in open areas and 15m in buildings)
    double proxM;

  public:
    GpsProxArea(String name, boolean newInside, String newLat, String newLon, double newLatRad, double newLonRad, double newProxM){
      areaName = name;
      inside = newInside;
      lat = newLat;
      lon = newLon;
      latRad = newLatRad;
      lonRad = newLonRad;
      proxM = newProxM;
      areaType = AREATYPEGPSPROX;
      return;
    }

    String getLat(){
      return lat;
    }

    String getLon(){
      return lon;
    }

    double getProxM(){
      return proxM;
    }

    // NOTE: uses last valid GPS position. Does not care for how old it is
    boolean evaluate(){
      if(!getGpsEnabled()){
        return !inside;
      }
      if((latPos == "") || (lonPos == "")){
        return !inside;
      }
      if(proxM < getGpsDistanceFromRad(getRadiansFromWGS85(latPos), getRadiansFromWGS85(lonPos), latRad, lonRad)){
        return inside;
      }
      // "else"
      return !inside;
    }
};

// NIY! TODO: evaluate, if inside a polygon of GPS points
/*class GpsArea : public Area {
  public:

    // number of points defining this area (and length of latlon-array)
    byte numAreaPoints;
    // lat+lon in radians of the boundary points
    double* latLonRads;

    GpsArea(String name, boolean newInside, byte numNewAreaPoints, double newLatLonRad[]){
      areaName = name;
      inside = newInside;
      latLonRads = newLatLonRad;
      areaType = AREATYPEGPSPOLY;
      numAreaPoints = numNewAreaPoints;
      return;
    }

    // destructor
    ~GpsArea(){
      delete[]latLonRads;
      return;
    }

    // NIY! evaluates to false
    boolean evaluate(){
      // TODO TODO TODO !!!
      return false;
    }
};*/

/** A Rule consists of
  * - areas, that can return true/false and need all to evaluate to true to be seen as "inside"
  * - onState, when to execute the actions (when "inside" is true, false, or changed since the last evaluation)
  * - actions, that are to be executed if all areas and the desired state evaluate to true
  *     (all areas true and desired state true,
  *      or at least one area false and desired state false,
  *      or the last state is not the current one but the other one [all/not all true])
  */
class Rule {

  private: // aka "don't fuck with these things"
    // rule name
    String name;
    // priority level. 0 = inactive, 1 highest priority, 255 lowest priority
    byte priority;
    // last state to detect state changes
    boolean lastState;
    // whether the areas should be evaluated to true, false or state change of the whole thing
    // STATETRUE, STATEFALSE or STATECHANGE
    char onState;
    // list of Area objects to consider
    Area* areas;
    byte numAreas = 0;
    // list of things to do when this rule applies
    Action* actions;
    byte numActions = 0;

  public:

    // constructor
    Rule(String newName,
          byte newPrio,
          char newOnState){
      onState = newOnState;
      priority = newPrio;
      name = newName;
      lastState = false;
      return;
    }

    // destructor
    ~Rule(){
      // TODO IMPORTANT: delete vector content!?!
      return;
    }

    void addArea(Area* newArea){
      //areas.push_back(newArea);
      // TODO TODO TODO TODO (actions as well!)
      Area* newAreas = new Area[numAreas + 1];
      for(byte index = 0; index < numAreas; index++){
        // TODO: copy areas by creating a ne one? (so the old one can be destroyed)
        newAreas[index] = areas[index];
      }
      numAreas++;
      // TODO: destroy old area pointer
      areas = newAreas;
      return;
    }

    void addAction(Action* newAction){
      actions.push_back(newAction);
      return;
    }

    char getOnState(){
      return onState;
    }

    byte getPriority(){
      return priority;
    }

    std::vector<Area*> getAreas(){
      return areas;
    }

    std::vector<Action*> getActions(){
      return actions;
    }

    // if the rule evaluates to true or false
    boolean evaluate(){
      // gets set to false as soon as any area does not apply
      boolean areasApply = true;
      // evaluate every Area. Break as soon as one does not apply.
      // Delay Areas are triggered, as long as they are the last in the area. They reset automatically.
      for(std::vector<Area*>::iterator it = areas.begin(); it != areas.end(); ++it){
        if(!(*it)->evaluate()){
          areasApply = false;
          break;
        }
      }

      // apply the desired state
      switch (onState) {
        case STATETRUE:{
          return areasApply;
          break;
        }case STATEFALSE:{
          return !areasApply;
          break;
        }case STATECHANGE:{
          if(areasApply != lastState){
            lastState = areasApply;
            return true;
          }
          break;
        }
      }
      // whatever might fail, in that case return false
      DEBUGOUT.println("ERROR evaluating Rule " + name + " - desired state: " + String(onState));
      return false;
    }

};




// list of rules
Rule* rules;
// length of this list
byte numOfRules = 0;

// timestamp of last rule check
unsigned long lastRuleCheckMillis = 613243;


// 0 = automatic mode / manually turned off, 1 = manually set
// if 1 => apply no rules to this output!
// TODO: use them - but how/when? byte pinAutoModes[NUM_OUTPUT_PINS] = {0, 0, 0, 0, 0, 0};


/**
 * converts from string in format "hh mm.mmmmm[E/W/N/S]" or "hhh mm.mmmmm[E/W/N/S]"] to radians (double)
 *
 * microdegees = (hh * 1000000) + (mm.mmmmm * 1000000 / 60)
 *
 *
 * TODO: can double even hold this large numbers? (at least 9 digits)
 *
double getMicroRadiansFromWGS85(String latOrLon){

  // 0.X hours, always
  long microdegrees;
  // read first 2 or 3 chars (hh or hhh, followed by ' ')
  byte spaceIndex = laTorLon.indexOf(' ');
  microdegrees = latOrLon.substring(0, spaceIndex).toInt() * (10000000);
  // read next 2 chars (mm), multiply by 100000 and divide by 60 (optimized: x*5000/3)
  microdegrees = milliminute + ((latOrLon.substring(spaceIndex, spaceIndex + 2).toInt() * 5000) /3);
  // skip '.' and read next 5 chars (.mmmmm), multiply by 60. Only E/W/N/S left on string
  microdegrees = microdegrees + (latOrLon.substring(spaceIndex + 3, spaceIndex + 5).toInt() / 60 );

  // TODO
  PIBY180;

  if(latOrLon.endsWith("W") || latOrLon.endsWith("S")){
    coord = -coord;
  }
}*/


/**
 * checks all rules
 * checks according to priority, reacts only to the one applying with the highest priority
 * TODO test
 * TODO: remove debug outputs
 */
void checkRules(){

  // if not enough time passed since last check, return
  if((millis() - lastRuleCheckMillis) < ruleUpdateInterval){
    return;
  }

  DEBUGOUT.println("A");
  // if multiple rules want to set the same output, the last sequence or the highest
  // constant value wins (sequences get set, might override other sequences, maxPinValues
  // get increased, highest value wins if no sequence was set)
  Action* mostPrioActions;
  byte numMostPrioActions = 0;

  // for making sure the highest priority rule is the last one to modify any port
  byte maxConnectorPriorities[NUM_OUTPUT_PINS] = {0, 0, 0, 0, 0, 0};

  DEBUGOUT.println("B");
  // check all rules by iterating over them
  for(byte ruleIndex = 0; ruleIndex < numOfRules; ruleIndex++){
    DEBUGOUT.println("CA");
    // if this rule evaluates to true
    if(rules[ruleIndex].evaluate()){
      DEBUGOUT.println("CB");
      // add all actions to the list of Actions to be executed
      // if the rule is the highest priority by now for this connector
      for(std::vector<Action*>::iterator actionsIt = (rules[ruleIndex].getActions()).begin();
            actionsIt != (rules[ruleIndex].getActions()).end();
            ++actionsIt){
        // if rule priority is higher than all previous rule priorities, execute this rule
        // TODO: check for dummy "connector" 255 to send a message to the server
        if(rules[ruleIndex].getPriority() > maxConnectorPriorities[(*actionsIt)->getConnectorID()-1]){
          // save pointer to this target to the according output in the array of most priority targets
          mostPrioActions.push_back((*actionsIt));
          // and save its rules priority to the array of highest priorities
          maxConnectorPriorities[(*actionsIt)->getConnectorID()-1] = rules[ruleIndex].getPriority();
        }
      }
    }
  }
  DEBUGOUT.println("D");
  // now only the targets of the rules with the highest priority remain
  // so execute these rules
  for(byte i=0; i<NUM_OUTPUT_PINS; i++){
    mostPrioActions[i]->execute();
  }
  DEBUGOUT.println("E");

}



/**
 * converts from string in format "hh mm.mmmmm[E/W/N/S]" or "hhh mm.mmmmm[E/W/N/S]"] to radians (double)
 *
 * degees = hh + (mm.mmmmm / 60)
 *
 *
 * TODO: can double even hold these large numbers? (at least 9 digits)
 */
double getRadiansFromWGS85(String latOrLon){

  long rad;
  // read first 2 or 3 chars (hh or hhh, followed by ' ')
  byte spaceIndex = latOrLon.indexOf(' ');
  rad = latOrLon.substring(0, spaceIndex).toInt();
  // read next 2 chars (mm), divide by 60 and add to the value
  rad = rad + ((double)(latOrLon.substring(spaceIndex, spaceIndex + 2).toInt()) / 60);
  // skip '.' and read next 5 chars (.mmmmm), divide by (60 * 10000). Only E/W/N/S left on string
  rad = rad + ((double)latOrLon.substring(spaceIndex + 3, spaceIndex + 7).toInt() / (60 * 100000) );

  // multiply by pi/180 to get final radian amount  [0.0174532925199433 = pi/180]
  rad = rad * PIBY180;

  if(latOrLon.endsWith("W") || latOrLon.endsWith("S")){
    rad = -rad;
  }

  return rad;
}

// returns rough distance in meters. does not consider slightly oval form of the planet
double getGpsDistanceFromRad(double latRad1, double lonRad1, double latRad2, double lonRad2){
  double dLat = latRad1 - latRad2;
  double dLon = lonRad1 - lonRad2;

  double a = sin(dLat/2) * sin(dLat/2) +
          sin(dLon/2) * sin(dLon/2) * cos(latRad1) * cos(latRad2);
  return (2 * atan2(sqrt(a), sqrt(1-a))) * EARTHRADIUSM;


}


/* /module/rules
*      Structure per line: [name (at least 2 chars, no space/newline)] [' '] [priority] [' '] [inside/outside (0/1)] [' '] [area1] [' '] ... [areanN] [' '] ['-'] [' '] [action1] [' '] ... [actionN]
*                Structure per area: [type (char b/w/p/a/t/s)] [' '] [inside (0/1)] [' '] [type related information]
*                    Type related:   battery('b'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [lower bound in mV] [' '] [upper bound in mV]
*                                    time('t'): [from - d(0-7),hh(0-23),mm(0-59),ss(0-59)] [' '] [to(like "from")]
*                                    wifi('w'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [SSID]
*                                    position('p'):  [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [coordinate e/w] [' '] [coordinate n/s] [' '] [max distance/proximity in m]
*                                    area('a'): [in/out/enter/leave('i'/'o'/'e'/'l')] [' '] [coordinate e/w1] [' '] [coordinate n/s1] [' '] [e/w2] [' '] [n/s2] ...
*                                    special('s'):   'w' (connected to wifi) [' '] [in/out/enter/leave('i'/'o'/'e'/'l')]
*                                                    TODO: 'g' (gps available/no satelites) [position/no pos/position gained/position lost (p/n/g/l)]
*                action: [output (1-6, TODO:255 (Server message))] [' '] [value(0/1 / 0-1024)] [' '] [sequence name]
*/
//loads rules from rule file
void initRules(){


  if(SPIFFS.exists(RULESFILENAME)){
    // open config file in read mode
    File ruleFile = SPIFFS.open(RULESFILENAME, "r");
    ruleFile.setTimeout(50);
    // read each line and see, which configuration it is
    while(ruleFile.available()){
      DEBUGOUT.println("2");
      boolean lineFinished = false;
      // read name
      String ruleName = ruleFile.readStringUntil(' ');
      if(ruleFile.available()){
        // read priority
        int priority =  ruleFile.readStringUntil(' ').toInt();
        // if priority parse successful (>=0)
        if(ruleFile.available() && (priority >-1)){
          // readinside/outside
          int inside = ruleFile.readStringUntil(' ').toInt();
          boolean actionsNext = false;
          Rule* newRule = new Rule(ruleName, priority, inside);

          // read areas until a new "area" dataset starts with "-" => actions to come
          while(ruleFile.available() && (!actionsNext)){
            // read type of area
            char areaType = ruleFile.readStringUntil(' ').charAt(0);
            // if we did not read an area type identifier, there is no area following,but actions.
            // so break this loop
            if(areaType == ACTIONSNEXTIDENTIFIER){
              actionsNext = true;
              continue;
            }
            if(ruleFile.available()){
              int inside = ruleFile.readStringUntil(' ').toInt();
              if(ruleFile.available())  {
                switch (areaType) {
                  case AREATYPEBATT: {
                    // TODO
                    break;
                  }
                  case AREATYPETIME: {
                    // TODO
                    break;
                  }
                  case AREATYPEWIFI: {
                    // TODO
                    break;
                  }
                  case AREATYPETIMEDELAY: {
                    // TODO
                    break;
                  }
                  case AREATYPEGPSPROX: {
                    // TODO
                    break;
                  }
                  case AREATYPEGPSPOLY: {
                    // TODO
                    break;
                  }
                  default:{
                    DEBUGOUT.println("ERROR reading rule file. Area of unknown type " + String(areaType));
                  }
                }
              }
            }
          }
          // TODO: read actions for this
        }
      }

      if(!lineFinished){
        // in case of fail, purge
        ruleFile.readStringUntil('\n');
      }
    }
  }

}

/** Saves Rules to rule file
  */
void saveRules(){
  // TODO TODO TODO

}


#endif
