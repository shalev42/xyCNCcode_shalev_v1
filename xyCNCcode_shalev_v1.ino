#include <Bounce2.h>


#define X_STEP_PIN         5
#define X_DIR_PIN          2
#define Y_STEP_PIN         6
#define Y_DIR_PIN          3
#define ENCODER_A_BIT_1 (10) // input IO for gray code bit 0 
#define ENCODER_A_BIT_0 (11) // input IO for gray code bit 1
#define ENCODER_B_BIT_1 (A3) // input IO for gray code bit 0
#define ENCODER_B_BIT_0 (A2) // input IO for gray code bit 1
// cross bits(0-1) for change counting direction (CW<>CCW)
//pins:
const int homingSwitchXPin = A4; // Pin for X-axis homing switch
const int homingSwitchYPin = A5; // Pin for Y-axis homing switch
int buttonPin1 = 9; 
int buttonPin2 = A0; 

// set a offset for the printer
int destPosition1 = 1; // Set your desired X-axis destination for start of the machine
int destPosition2 = 1; // Set your desired Y-axis destination for start of the machine

//encoders:
byte Old_Encoder_A_Read = 0; // old 2 bits read from first encoder 
byte New_Encoder_A_Read = 0; // new 2 bits read from first encoder 
int16_t Current_Encoder_A_Count = 0;// serial_printed and/or display 
int16_t Old_Encoder_A_Count = 0;// for later use

byte Old_Encoder_B_Read = 0; // old 2 bits read from second encoder 
byte New_Encoder_B_Read = 0; // new 2 bits read from second encoder 
int16_t Current_Encoder_B_Count = 0;// serial_printed and/or display 
int16_t Old_Encoder_B_Count = 0;// for later use
uint32_t Current_Time = 0   ;

//---------------------------------------------------------------------------------------------------------
//things you can change to controll the machine:
const int SpeedOFMotors = 1100; // Adjust the value as needed for motor speed
const int NumOfSteps = 5; //steps multiplayer
// Bounding box limits
int max_y = 3800;  // Adjust the maximum Y-axis coordinate
int max_x = 3500;  // Adjust the maximum X-axis coordinate
const unsigned long timerDuration = 1000 *(10000000000); // 1 minute in milliseconds delay between random drawings

//---------------------------------------------------------------------------------------------------------

// dont touch this stuff: 
int current_x = 0; // Current X-axis position
int current_y = 0; // Current Y-axis position
int counter_x = 0; // Counter for X-axis movement
int counter_y = 0; // Counter for Y-axis movement
const int min_x = 0;     // Adjust the minimum X-axis coordinate
const int min_y = 0;     // Adjust the minimum Y-axis coordinate
Bounce debouncerButton1 = Bounce(); // Debouncer instance for button 1
Bounce debouncerButton2 = Bounce(); // Debouncer instance for button 2
unsigned long timerStart = 0;
int c = 0;

//END OF SETTINGS:
//---------------------------------------------------------------------------------------------------------
int Read_Encoder_A(){
     Old_Encoder_A_Read = New_Encoder_A_Read ;
     New_Encoder_A_Read = (((digitalRead(ENCODER_A_BIT_1)) << 1) + (digitalRead(ENCODER_A_BIT_0))) ;
     byte Check_Direction  = (Old_Encoder_A_Read << 2) + New_Encoder_A_Read  ; // x4 = 2 rotate left 
     switch (Check_Direction)
     {
      case 1: case 7: case 8: case 14:
      return 1 ;
      case 2: case 4: case 11: case 13:
      return -1 ;
      case 0: case 5: case 10: case 15:
      return 0 ;
      default: // need to avoide delay in return 
      return 0 ; // 
    }
}
//---------------------------------------------------------------------------------------------------------
int Read_Encoder_B(){
     Old_Encoder_B_Read = New_Encoder_B_Read ;
     New_Encoder_B_Read = (((digitalRead(ENCODER_B_BIT_1)) << 1) + (digitalRead(ENCODER_B_BIT_0))) ;
     byte Check_Direction  = (Old_Encoder_B_Read << 2) + New_Encoder_B_Read  ; // x4 = 2 rotate left 
     switch (Check_Direction)
     {
      case 1: case 7: case 8: case 14:
      return 1 ;
      case 2: case 4: case 11: case 13:
      return -1 ;
      case 0: case 5: case 10: case 15:
      return 0 ;
      default: // need to avoide delay in return 
      return 0 ; // 
    }
}

//---------------------------------
void Update_Encoder_A_Count(){
  int temp_move = Read_Encoder_A();
  Current_Encoder_A_Count += temp_move;
}

//---------------------------------
void Update_Encoder_B_Count(){
  int temp_move = Read_Encoder_B();
  Current_Encoder_B_Count += temp_move;
}

//---------------------------------------------------------------------------------------------------------
void moveMotor(int dirPin, int stepPin, int steps) {
  digitalWrite(dirPin, steps > 0 ? HIGH : LOW);
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(SpeedOFMotors);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(SpeedOFMotors);
  }
}
//---------------------------------------------------------------------------------------------------------
void homing(int dirPin, int homingSwitchPin, int stepPin, int &currentPos) {
  while (digitalRead(homingSwitchPin) == LOW) {
    moveMotor(dirPin, stepPin, 1);
    currentPos = 0; // Update current position
  }
  moveMotor(dirPin, stepPin, 5); // Move slightly away from the switch
  moveMotor(dirPin, stepPin, -50); // Move back to home position
  currentPos = 0; // Update current position
}
//---------------------------------------------------------------------------------------------------------
void move(int new_x, int new_y) {
  // Calculate the difference between the current and new positions
  int x_diff = new_x - counter_x;
  int y_diff = new_y - counter_y;

  // Calculate the movement direction for each axis
  int x_dir = (x_diff > 0) ? 1 : -1; // Positive direction for X motor
  int y_dir = (y_diff > 0) ? 1 : -1; // Positive direction for Y motor

  // Calculate the number of steps for each axis
  int x_steps = abs(x_diff);
  int y_steps = abs(y_diff);
  
  // Determine the maximum number of steps
  int max_steps = max(x_steps, y_steps);

  // Move the motors incrementally to reach the target position
  for (int step = 0; step < max_steps; step++) {
    // Move X-axis if there's remaining distance
    if (step < x_steps) {
      moveMotor(X_DIR_PIN, X_STEP_PIN, -x_dir);
      counter_x += x_dir;
    }

    // Move Y-axis if there's remaining distance
    if (step < y_steps) {
      moveMotor(Y_DIR_PIN, Y_STEP_PIN, -y_dir);
      counter_y += y_dir;
    }

    // Check if current position is equal to the destination
    if (counter_x == new_x && counter_y == new_y) {
      break; // Exit the loop if destination reached
    }
  }
}


//---------------------------------------------------------------------------------------------------------
void randomDrawing() {
  // Code to be executed after one minute
  if (c == 0) {
    move(0 ,0);
    move(300 ,0);
    move(300 ,300);
    move(-300 ,300);
    move(-300 ,-300);
    move(300 ,-300);
    move(300 ,0);
    move(0 ,0);
    c = 1;  
  }
  else if (c == 1) {
    move(10 ,10);
    move(200, 0);
    move(400, 140.64);
    move(400, 340.64);
    move(200, 490.29);
    move(10, 340.64);
    move(10, 140.64);
    move(200, 0);
    move(10 ,10);
    c = 2;
   }

  else if (c == 2) {
    move(10, 10);       // Start point from (0, 0)
    move(40.14, 700.36);
    move(90.39, 140.14);
    move(150.36, 190.39);
    move(210.64, 220.64);
    move(270.86, 230.85);
    move(330.57, 220.83);
    move(380.38, 190.57);
    move(410.96, 140.35);
    move(440.12, 70.58);
    move(440.68, 10.77);
    move(430.60, -50.10);
    move(410.00, -100.31);
    move(360.99, -150.00);
    move(310.81, -180.67);
    move(250.76, -200.95);
    move(190.27, -210.58);
    move(120.84, -200.47);
    move(70.00, -170.74);
    move(20.28, -130.60);
    move(10, 10);       // Return to the start point
    c = 0;
   }

   
}               
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  pinMode(X_STEP_PIN, OUTPUT);
  pinMode(X_DIR_PIN, OUTPUT);
  pinMode(Y_STEP_PIN, OUTPUT);
  pinMode(Y_DIR_PIN, OUTPUT);
  pinMode(homingSwitchXPin, INPUT_PULLUP);
  pinMode(homingSwitchYPin, INPUT_PULLUP);
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
    // make encoder IO pulsedup inputs:
  pinMode(ENCODER_A_BIT_0, INPUT_PULLUP);  
  pinMode(ENCODER_A_BIT_1, INPUT_PULLUP);  
  pinMode(ENCODER_B_BIT_0, INPUT_PULLUP);  
  pinMode(ENCODER_B_BIT_1, INPUT_PULLUP);  
  Current_Time = millis();
  
  // Initialize button debouncers
  debouncerButton1.attach(buttonPin1);
  debouncerButton1.interval(50); // Debounce interval in milliseconds
  debouncerButton2.attach(buttonPin2);
  debouncerButton2.interval(50); // Debounce interval in milliseconds
  
  // Homing switches
  homing(X_DIR_PIN, homingSwitchXPin, X_STEP_PIN, current_x);
  homing(Y_DIR_PIN, homingSwitchYPin, Y_STEP_PIN, current_y);
  
  //move(100,300);
  //move(200,100);

}
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
void loop() {
  Update_Encoder_A_Count();
  Update_Encoder_B_Count();
  
  // Update destPosition1 and destPosition2 with current encoder counts
  destPosition1 = Current_Encoder_A_Count;
  destPosition2 = Current_Encoder_B_Count;

  // Calculate the new positions based on the encoder counts
  int new_x = destPosition1;
  int new_y = destPosition2;

  // Check if the new positions are within the bounding box
  if (new_x < min_x) new_x = min_x;
  if (new_x > max_x) new_x = max_x;
  if (new_y < min_y) new_y = min_y;
  if (new_y > max_y) new_y = max_y;

  // Check if the encoder counts have reached the maximum values
  if (Current_Encoder_A_Count >= max_x) {
    Current_Encoder_A_Count = max_x;
  }
  if (Current_Encoder_B_Count >= max_y) {
    Current_Encoder_B_Count = max_y;
  }
  if (Current_Encoder_A_Count <= min_x) {
    Current_Encoder_A_Count = min_x;
  }
  if (Current_Encoder_B_Count <= min_y) {
    Current_Encoder_B_Count = min_y;
  }  


  // Update button debouncers
  debouncerButton1.update();
  debouncerButton2.update();
  
//  // Check if button 1 is pressed
//  if (debouncerButton1.fell()|| debouncerButton2.fell()) {
//    Serial.println("Button pressed!");
//  }


         //checks for inactivity = button pressing if not - start a timer of 1 minute 
    if (millis() - timerStart >= timerDuration) {
    // Timer duration reached, execute the code
    randomDrawing();
    // Reset the timer
    timerStart = millis();
  }

  // Example usage: Move to new position if not reached yet
  if (counter_x != new_x || counter_y != new_y) {
    move(new_x * NumOfSteps, new_y * NumOfSteps);
    timerStart = millis(); // reset timer
  }
}
