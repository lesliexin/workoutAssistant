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

int num_of_red = 0;

bool wristTilted[4] = {false, false, false, false}; //stores checks of tilted wrist 

//setting up button ports 
const int resetPin = 8;
const int pausePin = 9;
const int doneWorkoutPin = 10;
const int doneSetPin = 11; 

// variables to read button state 
int resetButtonState = 0; 
int pauseButtonState = 0; 
int doneWorkoutButtonState = 0; 
int doneSetButtonState = 0; 

// variables to change in interrupt handler if pressed is true for respective button 
volatile bool reset = false; 
volatile bool pause = false; 
volatile bool doneWorkout = false; 
volatile bool doneSet = false; 


void setup(){
//  cli(); //disable interrupts 
 
//   setup MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  pinMode(interuptPin, INPUT);
  Serial.begin(9600);

  DDRB |= (1 << 5); //enable LED port for writing
  attachInterrupt(0, pin_ISR, RISING);
  sei(); //enable interrupts 

  //setting up buttons as input 
  pinMode(resetPin, INPUT_PULLUP);
  pinMode(pausePin, INPUT_PULLUP);
  pinMode(doneWorkoutPin, INPUT_PULLUP);
  pinMode(doneSetPin, INPUT_PULLUP);
}


bool check_consecutive_tilt(bool wristTilted[]){
  for(int i=0; i<4; i++){
    if(wristTilted[i] == false){
      return false; 
    }
  }
  return true; 
}

bool check_out_of_range(double x){
  if (x > 30.00 && x < 330.00){
     return true; 
  }
  else{
    return false;
  }
}

void output_status(){
  if(reset){
    Serial.print("RESET");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(x); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes: ");
    Serial.println(z); //INSERT MISTAKES HERE 
  } 
  else if(pause){
    Serial.print("WORKOUT PAUSED");
    Serial.println(" ");
  }
  else if(doneWorkout){
    Serial.print("WORKOUT COMPLETED");
    Serial.println(" ");
  }
  else if(doneSet){
    Serial.print("SET COMPLETED");
    Serial.println(" ");  
  }
  
}

void loop(){
  Serial.println("wirubwru");

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

  wristTilted[0] = check_out_of_range(x);

  // checking for tilted wrists 
  if(check_consecutive_tilt(wristTilted)){
    PORTB |= (1 << 5);
    delay (300);  
  }  
  else {
     PORTB &= ~(1 << 5);  
  }

  // dealing with pressed buttons 
  output_status(); 
  reset = false; 
  pause = false; 
  doneWorkout = false; 
  doneSet = false; 
  
  delay(500);
}

// handles a press of button (triggers on press) 
void pin_ISR()
{
  // checks which button is pressed 
  resetButtonState = digitalRead(resetPin);
  pauseButtonState = digitalRead(pausePin);
  doneWorkoutButtonState = digitalRead(doneWorkoutPin);
  doneSetButtonState = digitalRead(doneSetPin);

  // changes volatile buttons appropriately 
  if (resetButtonState == HIGH){
    reset = true; 
  }
  else if(pauseButtonState == HIGH){
    pause = true; 
  }
  else if(doneWorkoutButtonState == HIGH){
    doneWorkout = true; 
  }
  else if(doneSetButtonState == HIGH){
    doneSet = true; 
  }
}


