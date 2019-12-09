/**
    contains all functions regarding input/output via pins
    Copyright Olex (c) 2017
 */

// timestamp in ms when to fire the next time
static volatile unsigned long nextTaskExec[NUM_OUTPUT_PINS];

// timestamp in ms of last time firing (used for handling overflow)
static volatile unsigned long taskInitTime[NUM_OUTPUT_PINS];

// tells if a task is active
static volatile boolean taskActive[NUM_OUTPUT_PINS];


/**
 *  init stuff used for sequences
 */
void initSequenceTicker(){

  for(byte i = 0; i< NUM_OUTPUT_PINS; i++){
    taskActive[i] = false;
    nextTaskExec[i] = 0;
  }

}

/**
 * run next sequence steps if neccessary
 */
void updateSequenceTicker(){
  // iterate over all tasks
  for(byte i = 0; i < NUM_OUTPUT_PINS; i++){
    // if  a tak is active, check if the execution time has passed
    if(taskActive[i] == true){
      unsigned long currTime = millis();
      // if no overflow happens between initializing and executing
      if(taskInitTime[i] < nextTaskExec[i]){
        // if the task should be executed since millis() have passed it's scheduled execution time
        // (potential overflow AFTER passing this time is regarded as well)
        if((nextTaskExec[i] < currTime) || (currTime < taskInitTime[i])){
          // stop this ticker (will be started again when the next step is set)
          taskActive[i] = false;
          // set next sequence step - careful! connectors ID is 1 to X, not 0 to X-1
          nextSequenceStep(i+1);
        }
      // else if an overflow happens between setting the execution time and desired execution
      }else{
        if((currTime < taskInitTime[i]) && (currTime >= nextTaskExec[i])){
          // stop this ticker (will be started again when the next step is set)
          taskActive[i] = false;
          // set next sequence step - careful! connectors ID is 1 to X, not 0 to X-1
          nextSequenceStep(i+1);
        }
      }
    }
  }
}

/**
 * set ticker to execute the next sequence step
 * 0 < connector <= NUM_OUTPUT_PINS
 */
void setNextSequenceExecution(byte connector, unsigned long delayMs){
  byte ticker = connector - 1;
  // save task initialization timestamp to handle possible overflows until the execution time
  taskInitTime[ticker] = millis();
  // set next excecution time
  nextTaskExec[ticker] = taskInitTime[ticker] + delayMs;
  // activate ticker
  taskActive[ticker] = true;
}

/**
 * stop ticker from executing the next sequence step
 * 0 < connector <= NUM_OUTPUT_PINS
 */
void stopSequenceTicker(byte connector){
  byte ticker = connector - 1;
  taskActive[ticker] = false;
}
