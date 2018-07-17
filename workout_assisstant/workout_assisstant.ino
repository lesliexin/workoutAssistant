//Written by Ahmet Burkay KIRNIK
//TR_CapaFenLisesi
//Measure Angle with a MPU-6050(GY-521)

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
 

int wristTilted[4] = {1, 1, 1, 1};

//setting up button pins 
const int resetPin = 8;
const int pausePin = 9;
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
volatile bool doneWorkout = false; 
volatile bool doneSet = false; 

// stores events after each overflow occurs 
long timerOverflow = 0; 

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
  TCCR1B |= (1 << CS12);  
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  //setting up buttons as input 
  pinMode(resetPin, INPUT);
  pinMode(pausePin, INPUT);
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
    Serial.println("RIGHT LED");
    digitalWrite(6, HIGH);
    already_red = true;
    return;
  }
  
  else{
    Serial.println("LEFT LED");
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
  totalTime = 1/((1/(of+ct))*(16000000/1024)); 
  
  return totalTime; 
}

void breakTime(){
  TCNT1 = 0; 
  timerOverflow = 0; 
  Serial.print("Take a break for 15 seconds!");
  bool firstAlreadyOutputted = false;
  bool midAlreadyOutputted = false; 
  bool lastAlreadyOutputted = false; 
  
  // timer for 15 seconds
  while (TCNT1 <= 234375){
    if(TCNT1 < 15625){ //less than 1 second has passed 
      if(firstAlreadyOutputted){
        //nothing
        }
      else{
        Serial.print("15 seconds left"); 
        Serial.println("");
        firstAlreadyOutputted = true; 
        }
      }
      
    else if((TCNT1 > 62500) && (TCNT1 < 93750)){ //more than 4 seconds and less than 6 has passed 
      if(midAlreadyOutputted){
        //nothing
        }
      else{
        Serial.print("10 seconds left"); 
        Serial.println(""); 
        midAlreadyOutputted = true; 
        }
      }
    
    else if(TCNT1 > 156250){ //if more than 10 seconds have passed 
      if(lastAlreadyOutputted){
        //nothing
        }
      else{
        Serial.print("5 seconds left"); 
        Serial.println(""); 
        lastAlreadyOutputted = true; 
        }
      }
    }

}

void output_status(){
  if(reset){
    double resetTime = get_time(timeOverflow, currentTime);    
    Serial.print("RESET");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(resetTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE 
    timerOverflow = 0; //overflow value is now zero 
    currentTime = 0; 
    num_of_reds = 0;
    reset = false; 
  } 
  else if(pause){
    double pauseTime = 1/((1/(prevOverflow+currentTime))*(16000000/1024));  
    Serial.print("WORKOUT PAUSED");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(pauseTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE 
  }
  else if(doneWorkout){
    double workoutTime = get_time(timeOverflow, currentTime);  
    Serial.print("WORKOUT COMPLETED");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(workoutTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE
    timerOverflow = 0; //overflow value is now zero  
    currentTime = 0; 
    num_of_reds = 0; 
    doneWorkout = false; 
  }
  else if(doneSet){
    double setTime = get_time(timeOverflow, currentTime);  
    Serial.print("SET COMPLETED");
    Serial.println(" ");  
    Serial.print("Time Elapsed");
    Serial.println(setTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes in workout: ");
    Serial.println(num_of_reds); //INSERT MISTAKES HERE
    breakTime();  
    doneSet = false; 
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

  // printing values
  Serial.print("AngleX= ");
  Serial.println(x);
  Serial.print("AngleY= ");
  Serial.println(y);
  Serial.print("AngleZ= ");
  Serial.println(z);
  Serial.println("-----------------------------------------");

  // record new values
  for (int i = 3; i > 0; i--){
    wristTilted[i] = wristTilted[i-1];
  }

  wristTilted[0] = checkX_out_of_range(x);
   
  check_consecutive_tilt(wristTilted);
  
  resetButtonState = digitalRead(resetPin);
  doneWorkoutButtonState = digitalRead(doneWorkoutPin);
  doneSetButtonState = digitalRead(doneSetPin);

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
    prevOverflow = timeOverflow;   
    output_status();
    TCNT1 = currentTime; 
    timerOverflow = prevOverflow; 
  }
  
   delay(500);
}

// handles a press of pause button 
void pin_ISR()
{
  if (pause == false){
    currentTime = TCNT1;
    prevOverflow = timerOverflow;   
  }
  else if (pause == true) {
    TCNT1 = currentTime; //start from last value of outputTime  
    timerOverflow = prevOverflow; 
  } 
  pause = !pause;
}

ISR (TIMER1_OVF_vect){
  timerOverflow += 65536; 
} 


