#include <LiquidCrystal.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

/*                  // don't use non-I2C Display
const int LCDEnable = 8;
const int LCDRS = 7;
*/

const int led = 5;             // LED output on PWM Pin
const int modeBt[2] = {4,0};           // Mode Change button on Pin 4, button # 0
const int actionBt[2] = {2,1};         // Action Button on pin 2, Button # 1
const int PushButton1 = 0; // push button indicator for debouncing
const int PushButton2 = 1; // Mode is button 1, action is button 2
const int Potpin =A0;

byte Mode = 1;
byte ModeChanged = 0;
byte ActionState = 0;

unsigned long On_time = 150;        // Time to keep LED on for basicFlash, in ms
unsigned long Cycle_time = 500;     // Total time of basicFlash cycle - must be >= On_time

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

int PotRaw = 0;
int PotPrev = -1;    // make initial value not a valid input, for comparing in FlashPWM 

int debounce[2] = {0,0};  // counter for debouncing sub., buttons #
//int debounce_thresh = 2;
unsigned long debounceLastMillis[2] = {0,0};    // debounce counter timer for buttons
byte debounceTime = 50;                 // # of ms before considering button active

String ModeName = "Flasher";    // set initial Mode Name

/*                  // don't use non-I2C Display
      // LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal LCD(LCDRS, LCDEnable, 9, 10, 11, 12);
*/

LiquidCrystal_I2C LCD_I2C(0x27,16,2);  // set the lcd_I2C address to 0x27 for a 16 chars and 2 line display


// the setup routine runs once when you press reset:
void setup() {
    LCD_I2C.init();                      // initialize the lcd_I2C 
    LCD_I2C.init();
    LCD_I2C.backlight();        // turn on backlight

    /*                  // don't use non-I2C Display
    LCD.begin(16,2);
    LCD.clear();
    */
    
  LCDLayout();    // setup static Display elements
  LCDUpdate();    // Send initial Data to LCD
  
  // initialize the pins
  pinMode(led, OUTPUT);
  pinMode(modeBt[0], INPUT_PULLUP);
  pinMode(actionBt[0], INPUT_PULLUP);
  if(Cycle_time < On_time)        // If Cycle time is shorter than On_Time, make it = On_time
    Cycle_time = On_time;     // prevents errors in Basic_flash routine
}


void loop() {
  GetMode();
  LCDUpdate();    // Send Data to LCD
  
  switch (Mode)
  {
    case 1:                         // Flashing based on On_Time & Cycle Time
      Basic_Flash();
          ModeName = "Flasher  ";
      break;
      
    case 2:
      Toggle_Latch();
          ModeName = "Toogle   ";
      break;
      
    case 3:
      Toggle_Momentary();
          ModeName = "Moment  ";
      break;
      
    case 4:
      FlashNoDelay();
          ModeName = "DutyCycl ";
      break;
      
    case 5:
      FlashPWM();
          ModeName = "Dimmer   ";
      break;
      
    default:
      Mode = 1;
            ModeName = "";
          ModeName = "Flasher ";
      PotPrev = -1;
  }
}




void GetMode() {
  boolean ModeButtonPressed;               // This simulator doesn't like the line: boolean button = Debounce(modeBt);"
  ModeButtonPressed = Debounce(modeBt[0],modeBt[1]);      // works fine in Arduino IDE
                        // send pin # and Button #
  if((ModeButtonPressed == true) && (ModeChanged == 0))   // if Button debouncing is finished
  { 
    ModeChanged++;        //  Set mode changed flag
    Mode++;           //  increment Mode
  }
  else if((ModeButtonPressed == true) && (ModeChanged == 1))   // If button STILL being pressed
  { // Do nothing
  }
  else
    ModeChanged = 0;           // Ensure 'mode changed' Flag is clear
  
}

void Basic_Flash() 
{
  PotCycle();   // determine duty cycle from pot position
  
  digitalWrite(led, HIGH);   // turn the LED on 
  delay(On_time);               // wait for 'On_time'
  digitalWrite(led, LOW);    // turn the LED off
  delay(Cycle_time - On_time);  // finsh cycle
}




void Toggle_Latch()
{
  boolean ActionButtonPressed;               // The simulator doesn't like the line: "boolean button = Debounce(modeBt, etc);"
  ActionButtonPressed = Debounce(actionBt[0],actionBt[1]);  // works fine in Arduino IDE
                        // send pin # and Button #  
  if((ActionButtonPressed == true) && (ActionState == 0)) // Action button being pressed
  { // and no change (action) has happened yet)
    digitalWrite(led,!digitalRead(led));  // Toggle LED State
    ActionState = 1;            // Flag an Action has happened, for this button push
  }
  else if(ActionButtonPressed == false) 
  {
    ActionState = 0;
  }
}



void Toggle_Momentary()
{
  boolean ActionButtonPressed;               // The simulator doesn't like the line: "boolean button = Debounce(modeBt, etc);"
  ActionButtonPressed = Debounce(actionBt[0],actionBt[1]);  // seems to works fine in Arduino IDE
  
  if(ActionButtonPressed == true)
  {
    digitalWrite(led,HIGH);
  }
  else 
  {
    digitalWrite(led,LOW);
  }
}



void FlashNoDelay()
{
  PotCycle();
  
  currentMillis = millis();
  int led_state = digitalRead(led);
  
  if ((currentMillis - previousMillis >= On_time) && (led_state == HIGH)) 
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    
    digitalWrite(led,!led_state); // if the LED is off turn it on and vice-versa
  }
  else if((currentMillis - previousMillis >= (Cycle_time - On_time)) && (led_state == LOW))
    //else if((currentMillis - previousMillis >= (Off_time)) && (led_state == LOW))
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    digitalWrite(led,!led_state); // if the LED is off turn it on and vice-versa
  }
}

void PotCycle(){             // Duty cycle controlled by Pot
  PotRaw = analogRead(Potpin);  
  if(PotRaw >= 5)             // Pot doesn't always read down to 0.
  {
    On_time = map(PotRaw,0,1023,0,Cycle_time);  //Make On_time proportional to pot position 
  }
  else On_time = Cycle_time / 2;
  }

void FlashPWM()
{
//  PotRaw = analogRead(Potpin);  
  if(PotRaw != PotPrev)
  {
    analogWrite(led,PotRaw / 4);
    PotPrev = PotRaw;
  }
}


boolean Debounce(int button_Pin, int button_Num)
  {
  boolean _debounced = false;
  
  if ((digitalRead(button_Pin) == !1) && (debounce[button_Num] == 0))  // Active low due to internal pull-up && on first loop
      {
      debounce[button_Num]++;                   // increment debounce loop counter for active button
      debounceLastMillis[button_Num] = millis();   // set base time for debounce count
      }
  else if ((digitalRead(button_Pin) == !1) && (debounce[button_Num] != 0))  // button active, not first loop
      if((millis() - debounceLastMillis[button_Num]) >= debounceTime)       // check if delay time reached
      {
            _debounced = true;                        // set value to be returned to TRUE
            debounce[button_Num]++;                   // increment debounce loop counter for active button, just for fun
            //debounceLastMillis[button_Num] = millis();
      }
  else                                 
  { // If button NOT being Pressed  
    debounce[button_Num] = 0;       // when button released, Reset Debounce counter for button  
  }
  
  return _debounced;
}



void LCDLayout()
{ 
/*                  // don't use non-I2C Display
  LCD.setCursor(0,1);       // move cursor to first position (0) of the Bottom (1) Line
  LCD.print("Pot%:");
  LCD.setCursor(9,0);
  LCD.print("Bt.1:");
  LCD.setCursor(9,1);
  LCD.print("Bt.2:");
*/
  LCD_I2C.setCursor(0,1);       // move cursor to first position (0) of the Bottom (1) Line
  LCD_I2C.print("Pot%:");
  LCD_I2C.setCursor(9,0);
  LCD_I2C.print("Bt.1:");
  LCD_I2C.setCursor(9,1);
  LCD_I2C.print("Bt.2:");
}

void LCDUpdate()
{
  /*                  // don't use non-I2C Display
  LCD.setCursor(0,0);
  LCD.print(ModeName);
  LCD.setCursor(15,0);
  LCD.print(digitalRead(modeBt[0]));
  LCD.setCursor(15,1);
  LCD.print(digitalRead(actionBt[0]));
  LCD.setCursor(5,1);
  LCD.print(map(PotRaw,0,1022,0,100));
  */
  
  LCD_I2C.setCursor(0,0);
  LCD_I2C.print(ModeName);
  LCD_I2C.setCursor(15,0);
  LCD_I2C.print(digitalRead(modeBt[0]));
  LCD_I2C.setCursor(15,1);
  LCD_I2C.print(digitalRead(actionBt[0]));
  LCD_I2C.setCursor(6,1);
  PotCycle();
  LCD_I2C.print(map(PotRaw,0,1023,0,99));
}
