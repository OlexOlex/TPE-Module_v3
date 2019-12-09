/**
   Copyright Olex (c) 2017
*/

#ifndef SEQUENCEUTILITY
#define SEQUENCEUTILITY

#include <FS.h>

/* saves current step position of running sequences */
static unsigned int sequenceStep[NUM_OUTPUT_PINS];
/* keep track on how often this sequence was run (needed if 0 < repeat < ininite */
static int sequenceRuns[NUM_OUTPUT_PINS];
/** list of all available sequences */
static String* sequenceNames;
/** number of different sequences (including no sequence, "-") */
static byte sequenceCount = 1;


void initSequenceHandling() {
  // initialize fields
  for (byte i = 0; i < NUM_OUTPUT_PINS; i++) {
    sequenceStep[i] = 0;
    sequenceRuns[i] = 0;
  }
  sequenceCount = 0;
  Dir dir = SPIFFS.openDir(FILE_DIR_NAME);
  while (dir.next()) {
    // open current file
    File f = dir.openFile("r");
    // set timeout in case the file is a large binary file/etc (picture)
    f.setTimeout(20);
    // check if the first line starts with the sequence identifier string
    String firstLine = f.readStringUntil('\n');
    if (firstLine.startsWith(SEQUENCEIDENTIFIER)) {
      // count how many sequences we found
      sequenceCount++;
    }
    f.close();
  }
  // create list of sequences from files
  sequenceNames = new String[sequenceCount + 1];
  sequenceCount = 1;
  sequenceNames[0] = getNoSequenceIdentifier();
  dir = SPIFFS.openDir(FILE_DIR_NAME);
  while (dir.next()) {
    // open current file
    File f = dir.openFile("r");
    // set timeout in case the file is a large binary file/etc (picture)
    f.setTimeout(30);
    // check if the first line starts with the sequence identifier string
    String firstLine = f.readStringUntil('\n');
    if (firstLine.startsWith(SEQUENCEIDENTIFIER)) {
      // save the filename (without the first '/')
      sequenceNames[sequenceCount] = String(f.name()).substring(1, String(f.name()).length());
      sequenceCount++;
    }
    f.close();
  }

}


/**
   Saves that the given sequence now runs on the givenconnector
   and starts the sequence
   0 < connector <= NUM_OUTPUT_PINS
*/
void startSequence(byte connector, String sequenceName) {
  // if this is "no sequence"
  if(sequenceName.equals(getNoSequenceIdentifier())){
    // end possibly scheduled stuff of running sequence
    stopSequenceTicker(connector);
    // save this option
    setSelectedOption(connector, sequenceName);
    // set 0 as the current pwm value
    setOutputPWM(connector, "0");
  // if sequence not allready running, set it and start it
  }else if ((sequenceName != getSelectedOption(connector))) {
    // end possibly scheduled stuff of running sequence
    stopSequenceTicker(connector);
    // save this option
    setSelectedOption(connector, sequenceName);
    // set 0 as the current pwm value
    pinValues[connector - 1] = 0;
    // reset step counter for this connector
    sequenceStep[connector - 1] = 0;
    // reset repeated run counter for this connector
    sequenceRuns[connector - 1] = 0;
    // run first step of the sequence
    nextSequenceStep(connector);

    DEBUGOUT.println("started sequence " + sequenceName);

  } else {
    DEBUGOUT.println("sequence already active: " + sequenceName);
  }
}

/**
   stops and resets everything regardig the sequence of the specified connector
*/
void stopSequence(byte connector) {
  // turn off output
  setOutputPWM(connector, "0");
  // deactivate ticker
  stopSequenceTicker(connector);
  // reset configuration of active sequence to no sequence
  setSelectedOption(connector, getNoSequenceIdentifier());
}

/**
   reads next sequence step and configures everything accordingly
*/
void nextSequenceStep(byte connector) {

  // index in storage fields:
  byte connIndex = connector - 1;

  // generate file name of sequence file
  String sequenceFileName = "/";
  sequenceFileName += getSelectedOption(connector);

  // open sequence file
  File sequenceFile = SPIFFS.open(sequenceFileName, "r");
  if (sequenceFile) {
    // set timeout in case the last line does not terminate by '\n'
    sequenceFile.setTimeout(50);

    // read until you get to the position of the beginning of the [step]th line
    // for this read first line ("pwmsequence") and forget it, then [step] lines
    for (unsigned int i = 0; i <= sequenceStep[connIndex]; i++) {
      // read line in file (read until /n or /r)
      sequenceFile.find('\n');
    }
    // now read the line we are interested in
    String line = sequenceFile.readStringUntil('\n');

    // split line "[value to set], [ms to wait to next step]" or "repeat, [#repetitions/infinite]"
    String lineParts[2];// = line.split(", ");
    unsigned int commaIndex = line.indexOf(',');
    // if there is a comma in the line and at least one character before
    if (commaIndex > 0) {
      lineParts[0] = line.substring(0, commaIndex);
      // if there are at least two characters behind the comma (should be a space and the second number)
      if (line.length() < commaIndex + 2) {
        // no proper format, stop
        stopSequence(connector);
        DEBUGOUT.println("Possibly broken sequence file " + sequenceFileName + " line " + sequenceStep[connector - 1]);
        return;
      } else {
        lineParts[1] = line.substring(commaIndex + 2, line.length());
        //DEBUGOUT.println("part 1: " + lineParts[0] + " part 2: " + lineParts[1]);
        if (lineParts[0] == REPEATIDENTIFIER) {
          if (lineParts[1] == INFINITEIDENTIFIER) {
            // reset sequence to beginning
            sequenceStep[connIndex] = 0;
            // close file since it needs to be opened in the next step as well
            sequenceFile.close();
            // and do the next step (which will set the next execution as well)
            nextSequenceStep(connector);
            // don't close the file a second time
            return;
          } else {
            // add 1 to the repeat counter and check, if this was the last run
            sequenceRuns[connIndex]++;
            // if there were enough runs stop it (e.g. "repeat, 5" and the sequence would be run a 6th time)
            if (sequenceRuns[connIndex] > lineParts[1].toInt()) {
              // we're done
              stopSequence(connector);
            } else {
              // reset sequence to beginning
              sequenceStep[connIndex] = 0;
              // close file since it needs to be opened in the next step as well
              sequenceFile.close();
              // and do the next step (step 0, which will set the next execution as well)
              nextSequenceStep(connector);
              return;
            }
          }
        } else {
          // read how long to wait
          unsigned long millisValue = lineParts[1].toInt();
          //DEBUGOUT.println("Wait ms: " + String(millisValue));
          // if the file contains bullshit (why run something for 0ms?) or is broken (conversion returns 0 if non-integer number),
          if (millisValue < 1) {
            // probably broken file - stop sequence
            stopSequence(connector);
            DEBUGOUT.println("ERROR: running step for 0ms - stopping sequence " + getSelectedOption(connector));
          } else {
            // run this step and prepare everything for the next step
            // set output
            analogWrite(getConnectorPin(connector), lineParts[0].toInt());
            // old alternative: setOutputPWM(connector, lineParts[0]);
            // increment step counter
            sequenceStep[connIndex]++;
            // set next call: schedulerID, delay to next execution, (fire now, then wait? - no)
            setNextSequenceExecution(connector, millisValue);

          }
        }
      }
    }
    // close file
    sequenceFile.close();
    //DEBUGOUT.println("Next sequence step done of " + getSelectedOption(connector));
  } else {
    DEBUGOUT.println("Error opening sequence file " + sequenceFileName);
    stopSequence(connector);
  }
}

#endif
