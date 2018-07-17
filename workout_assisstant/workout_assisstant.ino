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

// Incrementer for number of times overflow occurs 
int timerOverflow = 0; 

// Variable to store temporary time from timer 
volatile long outputTime = 0; 

// Checking if pause is already enabled 
volatile bool pauseAlreadypressed = false; 

void setup(){
  cli(); //disable interrupts 
 
  // setup MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  pinMode(interuptPin, INPUT);
  Serial.begin(9600);

  DDRB |= (1 << 5); //enable LED port for writing
  attachInterrupt(0, pin_ISR, RISING);
  
  //setting up timer 1 for: done set, 
  TCCR1A=0x0;//resetTimer1controlregisters
  TCCR1B=0x0;//setWGM_2:0=000
  TCCR1B=0x4;//setTimer1toclk/256
  TIMSK1=0x6;//enableOCRinterruptsbits
  OCR1A=10000;//setOutputCompareValueA
  OCR1B=50000;//setOutputCompareValueB

  TCCR0B |= _BV(CS02); // Set or Clear bit CS02 of register TCCROB, the most signifigant bit of Timer0's prescaler. See page 110 of the datasheet. TCCR0B |= (_BV(CS02) sets CS02 to HIGH, TCCR0B &= ~(_BV(CS02) sets CS02 to LOW.
  TCCR0B |= _BV(CS00); // Set or Clear bit CS00 of register TCCROB, the least signifigant bit of Timer0's prescaler. See page 110 of the datasheet. TCCR0B |= (_BV(CS00) sets CS00 to HIGH, TCCR0B &= ~(_BV(CS00) sets CS00 to LOW.


  sei(); //enable interrupts 

  //setting up buttons as input 
  pinMode(resetPin, INPUT);
  pinMode(pausePin, INPUT);
  pinMode(doneWorkoutPin, INPUT);
  pinMode(doneSetPin, INPUT);

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
    double resetTime = 1/((1/(outputTime*65536))*(16000000/1024));    
    Serial.print("RESET");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(resetTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes: ");
    Serial.println(z); //INSERT MISTAKES HERE 
    outputTime = 0; //overflow value is now zero 
    reset = false; 
  } 
  else if(pause){
    Serial.print("WORKOUT PAUSED");
    Serial.println(" ");
    Serial.print("Time Elapsed");
    Serial.println(resetTime); //INSERT TIME HERE 
    Serial.print("Reps Completed");
    Serial.println(y); //INSERT REPS HERE 
    Serial.print("Mistakes: ");
    Serial.println(z); //INSERT MISTAKES HERE 
    outputTime = 0; //overflow value is now zero 
    reset = false; 
    pause = false; 
  }
  else if(doneWorkout){
    Serial.print("WORKOUT COMPLETED");
    Serial.println(" ");
    doneWorkout = false; 
  }
  else if(doneSet){
    Serial.print("SET COMPLETED");
    Serial.println(" ");  
    doneSet = false; 
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

  // outputting status based on pressed button 
  output_status(); 

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
    outputTime = TCNT1; 
 //   TCNT1 = 0; // Start from beginning
  }
  else if(pauseButtonState == HIGH){
    if (pause == false ){
      outputTime = TCNT1;
      TCNTI = 0; //stop;  
    }
    else if (pause == true) {
      
    } 
    pause = !pause;

  }
  else if(doneWorkoutButtonState == HIGH){
    doneWorkout = true; 
  }
  else if(doneSetButtonState == HIGH){
    doneSet = true; 
  }
}

ISR (TIMER1_OVF_vect){
  TCNT1 = 42420; // count up to 0xFFFFF, overflow to 0, and then start again from 42420
  timerOverflow++; 
} 


