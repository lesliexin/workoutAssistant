#include<Wire.h>

const int interuptPin = 2;        //the pin receiving the input signal to be measured
volatile int edge_counter = 0;

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

int minVal=265;
int maxVal=402;

double x;
double y;
double z;

int num_of_reds = 0;
bool already_red = true;
long rep_time_events = 0.00;
long rep_time_seconds = 0.00;
bool rep_already_counted = false;
int num_of_reps = 0;
bool weight_up;
bool weight_down;
 
int wristTilted[4] = {1, 1, 1, 1};
bool weight_up_history[3] = {false, false, false};
float weight_history_time[3] = {0.00, 0.00, 0.00};

//setting up button pins 
const int resetPin = 9;
const int doneWorkoutPin = 10;
const int doneSetPin = 11; 

// variables to read button state 
volatile int resetButtonState = 0; 
volatile int pauseButtonState = 0; 
volatile int doneWorkoutButtonState = 0; 
volatile int doneSetButtonState = 0; 

// variables to change in interrupt handler if pressed is true for respective button 
volatile bool reset = false; 
volatile bool pause = false; 
volatile bool already_paused = false;
volatile bool doneWorkout = false; 
volatile bool doneSet = false; 

// stores events after each overflow occurs 
volatile long timerOverflow = 0; 

// stores overflow events from pre-pausing or pre-doneSet period 
long prevOverflow = 0; 

// Variable to store instantenous events completed from the timer 
volatile long currentTime = 0; 

void setup(){
 
  // setup MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  pinMode(interuptPin, INPUT);
  Serial.begin(9600);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  DDRB |= (1 << 5); //enable LED port for writing
    
  //setting up timer 1 for: done set, 
  TCCR1A=0;//resetTimer1controlregisters
  TCCR1B=0;//setWGM_2:0=000
  TIMSK1 = 0x1;
  TCCR1B |= (1 << CS12);  
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  //setting up buttons as input 
  pinMode(resetPin, INPUT);
  pinMode(doneWorkoutPin, INPUT);
  pinMode(doneSetPin, INPUT);

  //setup the interrupt for buttons 
  attachInterrupt(0, pin_ISR, RISING);
  sei(); //enable interrupts 

}

void check_consecutive_tilt(int wristTilted[]){
  int rightTiltCounter = 0;
  for(int i=0; i<4; i++){
    if(wristTilted[i] == 1){
      digitalWrite(4, LOW);
      digitalWrite(6, LOW);
      digitalWrite(5, HIGH);
      already_red = false;
      return; 
    }
    else if (wristTilted[i] == 2){
      rightTiltCounter++;
    }
  }

  digitalWrite(5, LOW);
  if (!already_red){
    num_of_reds++;
  }
  
  if (rightTiltCounter == 4){
//    Serial.println("RIGHT LED");
    digitalWrite(6, HIGH);
    already_red = true;
    return;
  }
  
  else{
//    Serial.println("LEFT LED");
    digitalWrite(4, HIGH);
    already_red = true;
    return;
  }

}

int checkX_out_of_range(double x){
  if (x > 20.00 && x <= 180.00){
     return 2; 
  }
  else if (x > 180.00 && x < 340.00){
     return 0; 
  }
  else {
    return 1;
  }
}

double get_time(long of, long ct){
  double totalTime = 0; 
  totalTime = (of+ct)/((double)15625);
  return totalTime; 
}

void breakTime(){
  TCNT1 = 0; 
  timerOverflow = 0; 
  Serial.println("Take a break for 15 seconds!");
  bool firstAlreadyOutputted = false;
  bool midAlreadyOutputted = false; 
  bool lastAlreadyOutputted = false; 
  
  // timer for 15 seconds
  while ((timerOverflow + TCNT1) <= 78125){
    if(firstAlreadyOutputted){}
    else{
      Serial.println("15 seconds left"); 
      Serial.print("");
      firstAlreadyOutputted = true; 
    }
  }
      
  TCNT1 = 0; 
  timerOverflow = 0; 
    
  while ((timerOverflow + TCNT1) <= 78125){
    if(midAlreadyOutputted){
      //nothing
      }
    else{
      Serial.println("10 seconds left"); 
      Serial.print("");
      midAlreadyOutputted = true; 
    }
  }

  TCNT1 = 0; 
  timerOverflow = 0;

  while ((timerOverflow + TCNT1) <= 78125){
    if(lastAlreadyOutputted){}
    else{
      Serial.println("5 seconds left"); 
      Serial.print("");
      lastAlreadyOutputted = true; 
    }     
  }
  
  Serial.println("Start next set!");
}

void output_status(){
  if(reset){
    double resetTime = get_time(timerOverflow, currentTime);
    Serial.println("\n-----------------------------------------");    
    Serial.print("RESET");
    Serial.println(" ");
    Serial.print("Time Elapsed: ");
    Serial.println(resetTime); //INSERT TIME HERE 
//    Serial.print("Reps Completed: ");
//    Serial.println(num_of_reps); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE 
    Serial.println("-----------------------------------------");  
    Serial.println("WORKOUT STARTED");  
    Serial.println("-----------------------------------------\n");   
    timerOverflow = 0; //overflow value is now zero 
    currentTime = 0; 
    num_of_reds = 0;
    reset = false; 
  } 
  else if(pause){
    double pauseTime = get_time(prevOverflow, currentTime);  
    Serial.println("\n-----------------------------------------");
    Serial.print("WORKOUT PAUSED");
    Serial.println(" ");
    Serial.print("Time Elapsed: ");
    Serial.println(pauseTime); //INSERT TIME HERE 
//    Serial.print("Reps Completed: ");
//    Serial.println(num_of_reps); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE 
    Serial.println("-----------------------------------------\n");  
  }
  else if(doneWorkout){
    double workoutTime = get_time(timerOverflow, currentTime);  
    Serial.println("\n-----------------------------------------");
    Serial.print("WORKOUT COMPLETED");
    Serial.println(" ");
    Serial.print("Time Elapsed: ");
    Serial.println(workoutTime); //INSERT TIME HERE 
//    Serial.print("Reps Completed: ");
//    Serial.println(num_of_reps); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE
    Serial.println("-----------------------------------------\n");
    Serial.println("Press RESET to start again!");
    timerOverflow = 0; //overflow value is now zero  
    currentTime = 0; 
    num_of_reds = 0; 
    doneWorkout = false; 
  }
  else if(doneSet){
    double setTime = get_time(timerOverflow, currentTime);  
    Serial.println("\n-----------------------------------------");
    Serial.print("SET COMPLETED");
    Serial.println(" ");  
    Serial.print("Time Elapsed");
    Serial.println(setTime); //INSERT TIME HERE 
//    Serial.print("Reps Completed");
//    Serial.println(num_of_reps); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE
    Serial.println("-----------------------------------------\n");
    breakTime();  
    doneSet = false; 
  }
  
}

void print_accelerometer_values(){
  Serial.print("AngleX= ");
  Serial.println(x);
  Serial.print("AngleY= ");
  Serial.println(y);
  Serial.print("AngleZ= ");
  Serial.println(z);
  Serial.println("-----------------------------------------");
}

bool set_weight_status(){

  if (y > 60 && y < 90){
    weight_up = true;
    weight_down = false;
  }
  else if (y > 275 && y < 300){
    weight_down = true;
    weight_up = false;
  }
  else{
    weight_down = false;
    weight_up = false;
  }
}

bool check_rep_completed(){
  if (weight_up_history[0] == true && weight_up_history[2] == true){
    if (weight_up_history[1] == false && rep_already_counted == false){
      rep_time_events = weight_history_time[2] - weight_history_time[0];
      rep_time_seconds = rep_time_events / 15625.00;
      num_of_reps++;
      Serial.print("Rep: ");
      Serial.println(num_of_reps);
//      Serial.print("Seconds: ");
//      Serial.println(rep_time_seconds);
      rep_already_counted = true;
    }
  }
  else if (weight_up_history[0] == false){
    rep_already_counted = false;
  }
}

void record_rep_history(){
  if (weight_up && !weight_up_history[0]){
    
    for (int i = 3; i > 0; i--){
      weight_up_history[i] = weight_up_history[i-1];
    }

    for (int i = 3; i > 0; i--){
      weight_history_time[i] = weight_history_time[i-1];
    }
    
    weight_up_history[0] = true;
    weight_history_time[0] = timerOverflow + TCNT1;
  }

  else if (weight_down && weight_up_history[0]){
    
    for (int i = 3; i > 0; i--){
      weight_up_history[i] = weight_up_history[i-1];
    }

    for (int i = 3; i > 0; i--){
      weight_history_time[i] = weight_history_time[i-1];
    }
    
    weight_up_history[0] = false;
    weight_history_time[0] = timerOverflow + TCNT1;
  }
}


void loop(){

  // set up MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);

  // reading accelerometer
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);

  // converting to degrees
  x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);


//  print_accelerometer_values();
  
  // record new values
  for (int i = 3; i > 0; i--){
    wristTilted[i] = wristTilted[i-1];
  }

  wristTilted[0] = checkX_out_of_range(x);
   
  check_consecutive_tilt(wristTilted);
//
//  set_weight_status();
//  record_rep_history();
//  check_rep_completed();
  
  resetButtonState = digitalRead(resetPin);
  doneWorkoutButtonState = digitalRead(doneWorkoutPin);
  doneSetButtonState = digitalRead(doneSetPin);

//  for (int i = 0; i < 3; i++){
//    Serial.print(weight_up_history[i]); 
//  }
//  Serial.println();
//  
  
  // polling for reset, doneworkout and done set 
  if(resetButtonState == HIGH){
    reset = true;
    currentTime = TCNT1; 
    // outputting status based on pressed button 
    output_status();  
    TCNT1 = 0; // reset timer 
  }
  
  else if(doneWorkoutButtonState == HIGH){
    doneWorkout = true; 
    currentTime = TCNT1;  
    output_status();
    TCNT1 = 0; // reset timer 
  }
  
  else if(doneSetButtonState == HIGH){
    doneSet = true; 
    currentTime = TCNT1;
    prevOverflow = timerOverflow;   
    output_status();
    TCNT1 = currentTime; 
    timerOverflow = prevOverflow; 
  }

  else if(pause && !already_paused){
    output_status(); 
    already_paused = true;
  }
  
   delay(100);
}

// handles a press of pause button 
void pin_ISR()
{
  if (pause == false){
    currentTime = TCNT1;
    prevOverflow = timerOverflow;   
  }
  else if (pause == true) {
    already_paused = false;
    Serial.println("\n-----------------------------------------");
    Serial.println("WORKOUT UNPAUSED");
    Serial.println("-----------------------------------------\n");
    TCNT1 = currentTime; //start from last value of outputTime  
    timerOverflow = prevOverflow; 
  } 
  pause = !pause;
}

ISR (TIMER1_OVF_vect){
  timerOverflow += 65536; 
} 


