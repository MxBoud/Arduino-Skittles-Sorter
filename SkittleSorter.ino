


// Code by Maxime Boudreau - December 2017

#include <SoftwareSerial.h>   //From https://github.com/kroimon/Arduino-SerialCommand
#include <SerialCommand.h> //From https://github.com/kroimon/Arduino-SerialCommand
#include <AccelStepper.h>  //From http://www.airspayce.com/mikem/arduino/AccelStepper/
#include <Servo.h>

#include "Inventory.h" //Little class I made just for practice. 

Servo myServo;  // create servo object to control a servo



///COLOR
#define S0 6
#define S1 7
#define S2 8
#define S3 9
#define sensorOut 10

SerialCommand SCmd;   // The demo SerialCommand object (from the example file). 
AccelStepper stepper_MyStepper(4, 2, 4, 3, 5 );

bool autoMode = HIGH; //Does the arduino works in auto mode or in 
int numStepsAutoMode = -171; //Number of steps to go until next slot. NOTE: Since the stepper motor 
//Have 2048 for one rotation, this is the closest number so that numStempsAutoMode*12 is close to 2048. 
//It does not fall exactly at 2048, so I had to implement some corrections in the function MoveStepperToNextSlot 
int frequency;

//For rotation correction
int lastAmountOfRotation = 0 ; //See function MoveStepperToNextSlot()


// Stored value for every color. Note : This reading is strongly affected by ambiant light. 
float orange[3] = { 11017,22602, 18189};
float yellow[3] = {10823, 16072, 15191};
float green[3] = { 11458, 17365, 17279};
float red[3] = { 13893, 27045, 19511};
float purple[3] = { 16384, 26586, 19053};
float none[3] = {16036, 30211, 21701};
float none2[3] = {16036, 30211, 21701};


//Object to store the colors of the skittles in the incoming slots. 
Inventory inventory;


void setup()
{
  //SERVO MOTOR 
  myServo.attach(11);
  myServo.write(0);
  stepper_MyStepper.setMaxSpeed(800);
  stepper_MyStepper.setAcceleration(800);

 

 //COLOR SENSOR (https://www.osepp.com/electronic-modules/sensor-modules/58-color-sensor-module)
 // Setting frequency-scaling to 20%
  pinMode(S0,OUTPUT);
  pinMode(S1,OUTPUT);
  pinMode(A0,OUTPUT);
  digitalWrite(S0,LOW);
  digitalWrite(S1,HIGH);
  digitalWrite(A0,LOW);
  pinMode(S2,OUTPUT);
  pinMode(S3,OUTPUT);
  pinMode(sensorOut,INPUT);
  
  //SERIAL COMMUNICATION
  Serial.begin(9600);
  // Setup callbacks for SerialCommand commands
  SCmd.addCommand("MoveStepper",MoveStepper);     
  SCmd.addCommand("NextSlot",MoveStepperToNextSlot); 
  SCmd.addCommand("MoveServo",MoveServo);
  SCmd.addCommand("ToggleAuto",ToggleAuto);
  SCmd.addCommand("AjustNumStepsAuto",AjustNumStepsAuto);
  SCmd.addCommand("RC",ReadColorSerial);
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?")


  //Initial calibration (gives the user some time to make sure the stepper finishes 
  // so that the slots are aligned).
  int counter = 5;
  while(counter >1) {
    MoveStepperToNextSlot(); 
    delay(1000); 
    counter+= -1; 
     
  }  
  Serial.println("Ready");
}

void loop(){

  if (autoMode) {//Auto mode, the machine is sorting the incoming skittles
    inventory.MeasureAndPush(ReadColor());
    DecideServoLocation(inventory.items[1]); 
    MoveStepperToNextSlot();
  }
  //If autoMode == LOW ( 
  SCmd.readSerial();     //Processing serial commands (if sent)
}

void DecideServoLocation(String input) {// Input a color to decide where to align the servo
  if (input == "Red") {
    myServo.write(0);
  }
  else if (input == "Yellow") {
    myServo.write(28);
  }
  else if (input == "Green") {
    myServo.write(55);
  }
  else if (input == "Purple") {
    myServo.write(82);
  }
  else if (input == "Orange") {
    myServo.write(100);
  }
  else {
    myServo.write(0); 
  }
  
}

void MoveStepperToNextSlot() {//Trivial 
  //First part of is to see if a correction is needed in order to make sure that after one 
  //full rotation of the skittles wheel, it comes back to the same place.
  int amountOfRotation = stepper_MyStepper.currentPosition()/2048;
  if (amountOfRotation != lastAmountOfRotation) {
    stepper_MyStepper.runToNewPosition(stepper_MyStepper.currentPosition()+4);
    lastAmountOfRotation = amountOfRotation;
  }
  Serial.print("Amount of rotations since beginning = "); 
  Serial.println(amountOfRotation); 
  stepper_MyStepper.runToNewPosition(stepper_MyStepper.currentPosition() + numStepsAutoMode);
}

void MoveStepper() {//For Serial Debugging.
  int numSteps = 170;
  char *arg;
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL)      // As long as it existed, take it
  {
    numSteps = atoi(arg);
  }
 Serial.println(numSteps);
  stepper_MyStepper.runToNewPosition(stepper_MyStepper.currentPosition() + numSteps);
}

void AjustNumStepsAuto() {// For fine tuning the number of steps necessary to get to next slot

  char *arg;
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL)      // As long as it existed, take it
  {
    numStepsAutoMode = atoi(arg);
  }
 Serial.println(numStepsAutoMode);

}

void MoveServo() {// Useffull for debugging with serial monitor
  char *arg;
  arg = SCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL)      // As long as it existed, take it
  {
     int pos = atoi(arg);
     Serial.print("Servo at : ");
     Serial.println(pos);
     myServo.write(pos);
     inventory.MeasureAndPush(String(pos));
  }
  else {
    Serial.println("Invalid command");
  }
}

String FindClosestColor(float detect[3]) { //Apply a normalized scalar product between the  
  //RGB reading from the color sensor and the stored value for each possible skittles colors.
  //find wich one return the biggest value.
  String colorResult = "None";
  float value = 0;
  float test;

  test = CompareColors(detect, none);
    if (test >value ){
    value = test;
    colorResult = "None";
  }
  
  test = CompareColors(detect,yellow);
  if (test >value ){
    value = test;
    colorResult = "Yellow";
  }

  test = CompareColors(detect,red);
  if (test >value ){
    value = test;
    colorResult = "Red";
  }

  test = CompareColors(detect,orange);
  if (test >value ){
    value = test;
    colorResult = "Orange";
  }

  test = CompareColors(detect,purple);
  if (test >value ){
    value = test;
    colorResult = "Purple";
  }

  test = CompareColors(detect,green);
  if (test >value ){
    value = test;
    colorResult = "Green";
  }

  Serial.println(value);
  return colorResult;
}

String ReadColor() { //Read color with the color sensor.
  int R = ReadR();
  int G = ReadG();
  int B = ReadB();
  Serial.print(" R : ");
  Serial.print(R);
  Serial.print(" G : ");
  Serial.print(G);
  Serial.print(" B : ");
  Serial.print(B);
  Serial.println("");
  float det[3] = {  (float)R, (float)G, (float)B };
  String result = FindClosestColor(det);
  Serial.println(result);
  return result; 

}

void ReadColorSerial() { // Same function thant previous one, but return void (for Serial debugging). 
  int R = ReadR();
  int G = ReadG();
  int B = ReadB();
  Serial.print(" RGB : ");
  Serial.print(R);
  Serial.print(", ");
  Serial.print(G);
  Serial.print(", ");
  Serial.print(B);
  Serial.println("");
  float det[3] = {  (float)R, (float)G, (float)B };
  String result = FindClosestColor(det);
  Serial.println(result);
  }

int ReadR() {
  int color;
  // Setting red filtered photodiodes to be read
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  // Reading the output frequency
  frequency = 0;
  for (int i = 0;i<10;i++) {
    frequency += pulseIn(sensorOut, LOW);
  }

  return frequency;
  delay(50);
}

int ReadG() {
  int color;
  // Setting Green filtered photodiodes to be read
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  // Reading the output frequency
  frequency = 0;
  for (int i = 0;i<10;i++) {
    frequency += pulseIn(sensorOut, LOW);
  }
  return frequency;
  delay(50);
}

int ReadB() {
  int color;
  // Setting Blue filtered photodiodes to be read
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  // Reading the output frequency
  frequency = 0;
  for (int i = 0;i<10;i++) {
    frequency += pulseIn(sensorOut, LOW);
  }
  return frequency;
  delay(50);
}

int ReadW() {//Unused in current project. 
  int color;
  // Setting Blue filtered photodiodes to be read
  digitalWrite(S2, HIGH);
  digitalWrite(S3, LOW);
  // Reading the output frequency
  frequency = 0;
  for (int i = 0;i<10;i++) {
    frequency += pulseIn(sensorOut, LOW);
  }
  return frequency;
  delay(50);
}

float scalar( float col1[3], float col2[3]) { //Vector 3 scalar product.
  float result =(col1[0]*col2[0]+col1[1]*col2[1]+col1[2]*col2[2]);
  return result;

}

float CompareColors( float c1[3], float c2[3]) { //Comparing colors with the scalar product.
  return scalar(c1,c2)*pow(scalar(c2,c2)*scalar(c1,c1),-0.5);
}


void ToggleAuto() {// For Serial Debugging. 
  autoMode = !autoMode;
  if (autoMode) {
    Serial.println("Auto mode on");
  }
  else {
    Serial.println("Auto mode off");
  }

}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized()
{
  Serial.println("What?");
}


