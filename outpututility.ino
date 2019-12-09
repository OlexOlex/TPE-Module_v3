/**
    contains all functions regarding input/output via pins
    Copyright Olex (c) 2017
 */


#ifndef OUTPUTUTILITY
#define OUTPUTUTILITY


/**
 * initialization routine for configuring output pins according to the desired functionality
 */
void initOutputs() {
  // set slower PWM frequency(default: 1kHz)
  analogWriteFreq(500);


  // initialize digital pins as output
  for(byte connId = 1; connId <= NUM_OUTPUT_PINS; connId++) {
    if((getConnectorFunction(connId) == OUTPUTFUNCTION_TOGGLE) || (getConnectorFunction(connId) == OUTPUTFUNCTION_PULSE)){
      pinMode(getConnectorPin(connId), OUTPUT);
    }
    // not needed and causes troubles
    else{
      // make sure potential previous configuration does not disturb pwm (when executed due to reconfiguration of output pins)
      // careful: resets chip when setting the wrong pins to input (e.g. SPI/SDIO to flash chip or primary serial pins)
      pinMode(getConnectorPin(connId), INPUT);
      digitalWrite(getConnectorPin(connId), 0);
    }
  }
}



/**
   sets fixed pwm value for the given output (if the String is no valid number, a 0 is set)
   additionally saves the current value to pinValues[connector-1]
   0 < connector <= NUM_OUTPUT_PINS
*/
void setOutputPWM(byte connector, String value) {
  short val = value.toInt();
  //DEBUGOUT.println("Setting output PWM of connector "+ String(connector) + " to "+ String(val));
  analogWrite(getConnectorPin(connector), val);
  pinValues[connector - 1] = val;
}


/**
   sets the given output to high (1) or low (0) according to the given value
*/
void setOutput(byte connector, String onOrOff) {
  byte state = onOrOff.toInt();
  if (state > 1) {
    DEBUGOUT.println("WARNING: setOutput received value " + onOrOff + " - setting to 0 (off)");
    state = 0;
  }
  //DEBUGOUT.println("setOutput received value " + String(state) + " for connector " + String(connector) + " aka pin " +String(getConnectorPin(connector)));
  digitalWrite(getConnectorPin(connector), state);
  pinValues[connector - 1] = state;
}


/**
   sets the given output to high (1) for a short time
*/
void pulseOutput(byte connector) {
  // turn connector on
  digitalWrite(getConnectorPin(connector), 1);
  // wait approx 20ms (or a bit longer)
  yieldWaitMs(20);
  // turn connector off again
  digitalWrite(getConnectorPin(connector), 0);
  // make sure the Value of the connector stays 0
  pinValues[connector - 1] = 0;
}


#endif
