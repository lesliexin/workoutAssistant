//Written by Ahmet Burkay KIRNIK
//TR_CapaFenLisesi
//Measure Angle with a MPU-6050(GY-521)

#include<Wire.h>

const int inputPin = 2;        //the pin receiving the input signal to be measured
volatile int edge_counter = 0;

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

int minVal=265;
int maxVal=402;

double x;
double y;
double z;

void setup(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(9600);
/*
  cli(); //disable interrupts 
  DORB |= (1<<5); //enable LED port for writing 
  TCCR1A = 0x0; // reset Timer1 control registers 
  TCCR1B = 0x0; // set WGM_2:0 = 000 
  TCCR1B = 0x4; // set Timer1 to clk/256 
  TIMSK1 = 0x6; // enable OCR interrupts bits 
  OCR1A = 2000; // set output compare value A 
  OCR1B = 50000; // set output compare value B 
  sei(); //enable interrupts
*/ 

  bool wristTilted[4] = {false, false, false, false}; //stores checks of tilted wrist  
}


bool check_consecutive_tilt(bool wristTilted[]){
  for(int i=0; i<4; i++){
    if(wristTilted[i] == false){
      return false; 
    }
  }
  return true; 
}

void loop(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);

  x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);

  Serial.print("AngleX= ");
  Serial.println(x);

  Serial.print("AngleY= ");
  Serial.println(y);

  Serial.print("AngleZ= ");
  Serial.println(z);
  Serial.println("-----------------------------------------");
  delay(500);

   
  if(check_consecutive_tilt(wristTilted[])){
     // alerts user that they are tilting their wrist (turn on red LED)
  }

     

}
