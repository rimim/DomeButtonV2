/*
  DOME BUMP CONTROLER v2 (modified)
  Original sketch:
  http://www.droidvillage.net/wp-content/uploads/2012/11/Dome_Bumps_Nov_16_Teeces1.txt

  TEECES REAR PSI CONTROL
  D3 = POWER DOWN / POWER UP

  Random Rear PSI Lighting when not in use

  Yellow and Green when both pressed

  All OFF once Enable Timer is exceeded

  Yellow for Left
  Green for Right

  Repeat Sequence after successful entry - then back to Random after 2 seconds

  An Error causes rapid blink for 1 seconds then reset

  **********************

  On initial Power Up - DO enable Lights
*/
#define USE_SERIAL
#define TEECES_PSI      1  /* 1: Rear PSI 2: Both PSI */
#ifdef TEECES_PSI
#include <LedControl.h>
#endif
#define TEECES_D_PIN    6
#define TEECES_C_PIN    7
#define TEECES_L_PIN    8

#define PSIstuck        15 //odds (in 100) that a PSI will get partially stuck between 2 colors
const int PSIpause[2] = { 3000, 6000 };
const byte PSIdelay[2] PROGMEM = { 25, 35 };

#define PIN_DISABLED            255
#define PIN_MAIN_POWER_RELAY    16
#define PIN_SOUND_TRIGGER       15

#define LEFT_BUTTON_MAX         4     //4; A, B, C, D
#define RIGHT_BUTTON_MAX        4     //4; 1, 2, 3  // this will be increased to 4 to allow the full 16 outputs (A1 to D4)

#define SizeOfArray(arr) (sizeof(arr)/sizeof((arr)[0]))

// GENERAL CONSTANT VARIABLES
#define ON              1
#define OFF             0

#define HIGH            1
#define LOW             0

// I/O Pin Asignment
#define LEFT            2    // Left Button Input
#define RIGHT           3    // Right Button Input

// REAR PSI LIGHTING MODES
#define YELLOW          1
#define GREEN           2
#define BOTH            3
#define ALTERNATE       4
#define RANDOM         -1

byte  LEDColor;
int   LEDRate;

byte  FirstTime                 = 1;
byte  PowerDown;
byte  PowerDownInitialize;
int   PowerDownTime;

int   PowerDownDelay            = 2000;     // time between Power Down Sound Trigger and Turning OFF Main Power Relay
int   PowerDownSoundDelay       = 500;      // time to pulse the Power Down Sound Trigger

byte Speed                      = 1;  // 1mS per loop iteration for main loop
byte PSIBrightness              = 15;  // Rear PSI Brightness (1 to 15 (15 is brightest))

byte IO[] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static byte PIN_MAP[] = {
  PIN_DISABLED, /* Dummy pin 0 */
  12,           /* Pin 1 */
  13,           /* Pin 2 */
#ifdef USE_SERIAL
  PIN_DISABLED, /* Pin 3 - disabled used for RX */
  PIN_DISABLED, /* Pin 4 - disabled used for TX */
#else
  1,            /* Pin 3 */
  0,            /* Pin 4 */
#endif
  A3,           /* Pin 5 */
  A2,           /* Pin 6 */
  A1,           /* Pin 7 */
  A0,           /* Pin 8 */
  11,           /* Pin 9 */
  10,           /* Pin 10 */
  9,            /* Pin 11 */
#ifdef TEECES_PSI
  PIN_DISABLED, /* Pin 12 - disabled */
  PIN_DISABLED, /* Pin 13 - disabled */
  PIN_DISABLED, /* Pin 14 - disabled */
#else
  TEECES_L_PIN, /* Pin 12 */
  TEECES_C_PIN, /* Pin 13 */
  TEECES_D_PIN, /* Pin 14 */
#endif
  5,            /* Pin 15 */
  4             /* Pin 16 */
};

/*
 * Create a new controller
 * Params :
 * int dataPin    The pin on the Arduino where data gets shifted out
 * int clockPin   The pin for the clock
 * int csPin      The pin for selecting the device when data is to be sent
 * int numDevices The maximum number of devices that can be controled
 LedControl(int dataPin, int clkPin, int csPin, int numDevices);
 */
#ifdef TEECES_PSI
LedControl lc=LedControl(TEECES_D_PIN,TEECES_C_PIN,TEECES_L_PIN,TEECES_PSI);   // Setup MAX7221 LED Drive Signals
#if TEECES_PSI == 2
 #define REAR_PSI 1
#else
 #define REAR_PSI 0
#endif
/*
   Each PSI has 7 states. For example on the front...
    0 = 0 columns Red, 6 columns Blue
    1 = 1 columns Red, 5 columns Blue (11)
    2 = 2 columns Red, 4 columns Blue (10)
    3 = 3 columns Red, 3 columns Blue  (9)
    4 = 4 columns Red, 2 columns Blue  (8)
    5 = 5 columns Red, 1 columns Blue  (7)
    6 = 6 columns Red, 0 columns Blue
*/
static byte sPSIState[TEECES_PSI][6];
void setPSIstate(bool frontRear, byte PSIstate)
{
  //set PSI (0 or 1) to a state between 0 (full green) and 6 (full orange)
  // states 7-11 are moving backwards
  if (PSIstate > 6)
    PSIstate = 12 - PSIstate;
  for (byte col = 0; col < 6; col++)
  {
    byte mask = (col < PSIstate) ? B10101010 : B01010101;
    mask = ((col & 1) == 0) ? mask : ~mask;
    if (sPSIState[frontRear][col] != mask)
    {
      sPSIState[frontRear][col] = mask;
      lc.setColumn(frontRear, col, mask);
    }
  }
}

void setPSISolidstate(bool frontRear, byte mask)
{
  for (byte col = 0; col < 6; col++)
  {
    if (sPSIState[frontRear][col] != mask)
    {
      sPSIState[frontRear][col] = mask;
      lc.setColumn(frontRear, col, mask);
    }
  }
}

byte PSIstates[2] = { 0, 0 };
unsigned long PSItimes[2] = { millis(), millis() };
unsigned int PSIpauses[2] = { 0, 0 };
#endif

void setup()
{
#ifdef USE_SERIAL
  Serial.begin(9600);
#endif
  Initialize();
}

void setDomeControllerOutputPut(int domePin, int state)
{
#ifdef USE_SERIAL
  Serial.print("PIN");
  Serial.print(domePin);
  Serial.print(" ");
  Serial.println(state);
#endif
  int pin = (domePin >= 1 && domePin < SizeOfArray(PIN_MAP)) ? PIN_MAP[domePin] : PIN_DISABLED;
  if (pin != PIN_DISABLED)
    digitalWrite(pin, state);
}

//*************************************************************************************//*************************************************************************************//*************************************************************************************
//*************************************************************************************//*************************************************************************************//*************************************************************************************
//*************************************************************************************//*************************************************************************************//*************************************************************************************

void loop()             // run once every "Speed" mS
{
  // State Variables
  static byte MainState;

  static unsigned long time;
  static unsigned long last_time;

  // Counters
  static unsigned int NONECounter;
  static unsigned int BOTHCounter;
  static unsigned int LEFTCounter;
  static unsigned int RIGHTCounter;

  static byte LeftButton;                   // counter that is incremented on each Left Button Press
  static byte RightButton;                  // counter that is incremented on each Right Button Press

  static byte InValidInput;                 // flag to indicate an invalid input from user
  static int InvalidTime;

  static int Address;
  static byte LedStatusState;
  static int YellowRate;
  static int GreenRate;
  static byte YellowFlash;
  static byte GreenFlash;

  static byte DisplaySequence;

  time = millis();              // get an updated time stamp
  if (time - last_time > Speed)     // check if time has passed to change states - used to slow down the main loop
  {
    last_time = time;           // reset the timer
    switch (MainState)
    {
       case 0:                                                                // BOTH INPUTS HIGH (NOT PRESSED)
       {  
          if(PowerDown)
          {
            LEDColor = OFF;
          }
          else
          {
            LEDColor        = ALTERNATE;
            LEDRate         = RANDOM;
          } 
         
          if ( (digitalRead(LEFT) == HIGH)&&(digitalRead(RIGHT) == HIGH) )      // wait for Extenal Trigger #1 and #2 to go HIGH before watching for external negative trigger
          {
            if (NONECounter++ > 100)                                // Both Buttons need to be Released for 100mS to prevent a false trigger
            {
              MainState++;                                                      // both external lines are not triggered - ready to watch for negative trigger (go to next state)
              NONECounter = 0;
            }
          }
          else
          {
            NONECounter = 0;
          }
          break;
       }

       case 1:                                                             // KeyPad Open - reserved for sound or light action
       {  
          MainState++;
          break;
       }

       case 2:                                                      // Watch for LEFT and RIGHT button press
       {  
          if ( (digitalRead(LEFT) == LOW)&&(digitalRead(RIGHT) == LOW) )                // wait for BOTH Extenal Trigger #1 and #2 to go LOW 
          {
            if (BOTHCounter++ > 20)                         // Both Buttons need to be Held Low to trigger Lights
            {
              LEDColor = BOTH;
              LEDRate = ON; 
            }
            if (BOTHCounter++ > 1000)                           // Both Buttons need to be Held Low for  100 second (100 x 10mS)
            {
              BOTHCounter = 0;
              MainState++;
            }
          }
          else    // BOTH buttons not pressed - continue Alternate Pattern
          {
            if(PowerDown)
            {
              LEDColor = OFF;
            }
            else
            {
              LEDColor        = ALTERNATE;
              LEDRate         = RANDOM;
            } 
            BOTHCounter = 0;    // turn on both Yellow and Green if buttons pressed
          }
          break;
        }

        case 3:                         // KeyPad Activated - reserved for sound or light action
        {
          LEDColor = OFF;                   // Turn off ALL LED's when KeyPad is Unlocked and ready for user input
          LEDRate = OFF; 
          MainState++;
          break;
        }

        case 4:                  // Wait for Both Lines to be Released before proceeding
        {  
          if ( (digitalRead(LEFT) == HIGH)&&(digitalRead(RIGHT) == HIGH) )     // wait for Extenal Trigger #1 and #2 to go HIGH before watching for external negative trigger
          {
            if (NONECounter++ > 20)          // Both Buttons need to be Held Low for 0.200 second (20 x 10mS)
            {
              MainState++;
              NONECounter = 0;
              LEDColor = OFF;                   // Turn off ALL LED's when KeyPad is Unlocked and ready for user input
              LEDRate = OFF; 
            }
          }
          else
          {
            NONECounter = 0;
          }
          break;
        }

        case 5:                     // reserved for sound or light action
        {   
          MainState++;
          break;
        }

        case 6:                      // Watch for LEFT, RIGHT, or BOTH button press
        {  
          //
          // Left Button(s) must be pressed first (before Right Button)
          //
          // Once Right Button(s) have been pressed (RightButton >0); pressing the left button again will abort the sequence
          //
          //
          // after a code has been entered - reduce the double press time requirementto 150mS
          // if no activity in 10 seconds - reset the 2 second double press time requirement
          if ( (digitalRead(LEFT) == LOW)&&(digitalRead(RIGHT) == HIGH) )      // LEFT button pressed
          {      
            RIGHTCounter = 0;
            BOTHCounter  = 0;
            NONECounter  = 0;

            if(LEFTCounter++ > 20)          // debounce the button press
            {
              LEDColor = YELLOW;
              LEDRate  = ON;
              LEFTCounter = 0;
              //check to see if Right Button has been pressed previously or if Left Button has been pressed too much
              // if so, SET ERROR FLAG
              
              LeftButton++;               // increment left button accumulator
              if ( (LeftButton > LEFT_BUTTON_MAX) || (RightButton) )
              {
                InValidInput = 1;
              }
              MainState   = 4;            // Look for next Button Press
            #ifdef USE_SERIAL
              Serial.println("LEFT");
            #endif
            }
          }
          else if ( (digitalRead(RIGHT) == LOW)&&(digitalRead(LEFT) == HIGH) )  // RIGHT button pressed
          {              
            LEFTCounter = 0;
            BOTHCounter = 0;
            NONECounter = 0;

            if(RIGHTCounter++ > 20)
            {
              LEDColor = GREEN;
              LEDRate  = ON;
              RIGHTCounter = 0;
              //check to see if Left Button has NOT been pressed previously or if Right Button has been pressed too much
              // if so, SET ERROR FLAG
              
              RightButton++;              // increment left button accumulator
              if ( (RightButton > RIGHT_BUTTON_MAX) || (!LeftButton) )
              {
                InValidInput = 1;
              }
              MainState   = 4;            // Look for next Button Press
            #ifdef USE_SERIAL
              Serial.println("RIGHT");
            #endif
            }
          }
          else if ( (digitalRead(RIGHT) == LOW)&&(digitalRead(LEFT) == LOW) )   // BOTH buttons pressed
          {
            LEFTCounter  = 0;
            RIGHTCounter = 0;
            NONECounter  = 0;
            
            if(BOTHCounter++ > 20)
            {
              LEDColor = BOTH;
              LEDRate  = ON;
              BOTHCounter = 0;
              NONECounter     = 0;
              
              if ( (!LeftButton) || (!RightButton) )
              {
                InValidInput = 1;
              }
              MainState = 7;  // process user inputs
            }
          }
          else                              // NO buttons pressed
          {                    
            LEFTCounter   = 0;
            RIGHTCounter  = 0;
            BOTHCounter   = 0;
            
            if(NONECounter++ > 4000)        // if nothing is pressed for 5 seconds reset system
            {
              NONECounter     = 0;
              LEDColor = OFF;
              LEDRate  = OFF;
              MainState      = 8;
              InValidInput   = 1;
            }
          }
          break;
        }

        case 7:                 // wait for Both Buttons to go HIGH 
        {  
          if ( (digitalRead(LEFT) == HIGH)&&(digitalRead(RIGHT) == HIGH) ) 
          {
            if (NONECounter++ > 20) 
            {
              LEDColor = OFF;
              LEDRate  = OFF;
              MainState = 9;                 
            }
          }   
          break;
        }

        case 8:                 // KEYPAD TIMED OUT - DO SOMETHING SPECIAL
        {  
          MainState++;
          break;
        }

        case 9:                 // PROCESS USER INPUTS
        {  
          LEDColor = OFF;
          LEDRate  = OFF;
        
          if(InValidInput)
            MainState = 13;
          else
            MainState = 11;
          break;
        }
   
        case 10: // not used                
        {   
          MainState++;
          break;
        }

        case 11:              // Process Valid Input Command:  LEFT = 1-4  RIGHT = 1-3
        {
          FirstTime = 0;    // Now that a Valid Input has been received - allow Rear PSI Lights to turn ON

          // produce a value from 1 to 16 (A1-D4)
          Address = (((LEFT_BUTTON_MAX * LeftButton) + RightButton) - LEFT_BUTTON_MAX);

      #ifdef PIN_MAIN_POWER_RELAY
          if (PowerDown)            // If The System was Previously Powered Down - Then ONLY a POWER UP CODE IS VALID - ALL OTHERS ARE INVALID
          {
            if(Address == PIN_MAIN_POWER_RELAY)
            { 
              PowerDown  = 0;
              /* TRIGGER MAIN POWER RELAY - TURN ON POWER*/
              setDomeControllerOutputPut(PIN_MAIN_POWER_RELAY, HIGH);
              /* Turn of fSound Trigger */
              setDomeControllerOutputPut(PIN_SOUND_TRIGGER, HIGH);
              MainState++;
              break;
            } 
            else
            {
              MainState = 13;    // Invalid Input
              break;
            }
          }
      #endif
/*
          A1  1:1  1
          A2  1:2  2
          A3  1:3  3
          A4  1:4  4
          B1  2:1  5
          B2  2:2  6 
          B3  2:3  7
          B4  2:4  8
          C1  3:1  9
          C2  3:2  10
          C3  3:3  11
          C4  3:4  12
          D1  4:1  13
          D2  4:2  14 
          D3  4:3  15       
          D4  4:4  16
*/
        // Toggle The IO Pin State

        if(IO[Address] == HIGH)
        {
          IO[Address] = LOW;
          setDomeControllerOutputPut(Address, LOW);
        #ifdef PIN_MAIN_POWER_RELAY
          if (Address == PIN_MAIN_POWER_RELAY)
          {
            // POWER DOWN SEQUENCE
            PowerDownInitialize   = 1;
            PowerDownTime = 0;
          }
        #endif
        }
        else
        {
          IO[Address] = HIGH;
        #ifdef PIN_MAIN_POWER_RELAY
          if (Address == PIN_MAIN_POWER_RELAY)
          {
            // POWER DOWN SEQUENCE
            PowerDownInitialize = 1;
            setDomeControllerOutputPut(Address, LOW);
            PowerDownTime = 0;
          }
          else
        #endif
          {
            setDomeControllerOutputPut(Address, HIGH);
          }
        }
        MainState++;
        break;
      }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 12:    // Display Valid Input
      {  

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// THIS CODE ALLOWS the USER INPUT LED FEEDBACK ROUTINE TO BE INTERRUPTED BY THE USER PRESSING BOTH BUTTONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if ( (digitalRead(LEFT) == LOW)&&(digitalRead(RIGHT) == LOW) )                // wait for BOTH Extenal Trigger #1 and #2 to go LOW 
        {
          if (BOTHCounter++ > 50)                         // Both Buttons are HELD - EXIT and Allow User to Enter a New Code
          {
            BOTHCounter    = 0;
            YellowRate     = 0;
            GreenRate      = 0;
            LeftButton     = 0;
            RightButton    = 0;
            InValidInput   = 0;
            LedStatusState = 0;
            MainState      = 3;                             // EXIT and Allow User to Enter a New Code (state 3)
          }
        }
        else    // BOTH buttons not pressed - continue FEEDBACK ROUTINE
        {
          BOTHCounter = 0;    
        }      

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////      
            
        // Flash Yellow for Left
        // Flash Green for Right
        switch (LedStatusState)
        {
          case 0:        //  TURN ALL LEDs OFF - BEFORE FLASHING STATE
          {
            LEDColor = OFF;
            LEDRate  = OFF;
            if (YellowRate++ > 300)
            {
              YellowRate = 0;  // clear flag
              LedStatusState++;
            }
            break;
          }
          case 1:        // YELLOW ON
          {
            LEDColor = YELLOW;
            LEDRate  = ON;
            //YellowFlash++;                            // increment the Yellow Flash Counter
            if (YellowRate++ > 75)
            {
              YellowRate = 0;  // clear flag
              LedStatusState++;
            }
            break;
          }
                 
          case 2:      // YELLOW OFF
          {
            LEDColor = OFF;

            if (YellowRate++ > 150)
            {
              YellowRate = 0;  // clear flag
              if(++YellowFlash >= LeftButton)            // check for the need to flash Yellow again (if Left was pressed more than once)
              {
                YellowFlash = 0;
                LedStatusState++;     
              }
              else
              {
                LedStatusState = 1;
              }
            }
            break;
          }             
                 
          case 3:        // GREEN ON
          {
            LEDColor = GREEN;
            LEDRate  = ON;
            //GreenFlash++;                            // increment the Yellow Flash Counter
          
            if (GreenRate++ > 75)
            {
              GreenRate = 0;  // clear flag
              LedStatusState++;
            }
            break;
          }
                 
          case 4:      // GREEN OFF
          {
            LEDColor = OFF;
            //LEDRate  = OFF;
            
            if (GreenRate++ > 150)
            {
              GreenRate = 0;  // clear flag
              if(++GreenFlash >= RightButton)
              {
                GreenFlash = 0;
                LedStatusState++;     
              }
              else
              {
                LedStatusState = 3;
              }
            }
            break;
          }
          case 5:      // ALL OFF
          {
            if (GreenRate++ > 300)
            {
              GreenRate = 0;
              LedStatusState++;                    
            }
            break;
          }   
          case 6:      // REPEAT 
          {
            if (++DisplaySequence > 1)                        // In Future and || (PowerDown) || (PowerDownInitialize) to prevent repeating the code sequence during a power down.
            {
              DisplaySequence = 0;
              LedStatusState++;                    
            }
            else
            {
              LedStatusState = 1;
            }               
            break;
          } 
          case 7:      // 
          {
            if (GreenRate++ > 100)                            // small delay before resuming psi routine
            {
              GreenRate         = 0;
              LeftButton        = 0;
              RightButton       = 0;
              InValidInput      = 0;
              LedStatusState    = 0;
              MainState         = 0;                     
            }
            break;
          }                  
        }
        break;            
      }        // end of case 12

      case 13:                // INVALID INPUT - DO SOMETHING SPECIAL
      {   
      #ifdef USE_SERIAL
        Serial.println("INVALID");
      #endif

// THIS CODE ALLOWS the INVALID INPUT LED ROUTINE TO BE INTERRUPTED BY THE USER PRESSING BOTH BUTTONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if ( (digitalRead(LEFT) == LOW)&&(digitalRead(RIGHT) == LOW) )                // wait for BOTH Extenal Trigger #1 and #2 to go LOW 
        {
          if (BOTHCounter++ > 50)                         // Both Buttons are HELD - EXIT and Allow User to Enter a New Code
          {
            BOTHCounter       = 0;
            YellowRate        = 0;
            GreenRate         = 0;
            LeftButton        = 0;
            RightButton       = 0;
            InValidInput      = 0;
            LedStatusState    = 0;
            MainState         = 3;                             // EXIT and Allow User to Enter a New Code (state 3)
          }
        }
        else    // BOTH buttons not pressed - continue FEEDBACK ROUTINE
        {
          BOTHCounter = 0;    
        }      
////////////////////////////////////////////////////////////////////////////////////////////////
  
        LEDColor        = ALTERNATE;
        LEDRate         = 100;
        LeftButton      = 0;
        RightButton     = 0;
        InValidInput    = 0;

        if(InvalidTime++ > 2000)
        {
          InvalidTime   = 0;
          MainState     = 0;        // RESTART ENTIRE SEQUENCE
        }
        break;
      }
    }// end of switch(MainState)
      
    FlashLED(LEDColor, LEDRate);    // update LEDs
      
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// create a power down initialization flag to begin counter - then clear a
// IF POWER DOWN COMMAND WAS ENTERED (D3) THEN SET TIMER FOR POWER DOWN.
    if (PowerDownInitialize)
    {
      if (PowerDownTime++ > PowerDownSoundDelay)
      {
        //PowerDownSoundDelay = 0;            // this function will still retrigger - but clearing counter will greatly reduce redundant calls
        setDomeControllerOutputPut(12, HIGH); // output pin 15 - Turn off Power Down Sound Tigger (so it doesn't repeat)  
        // no need to set IO[12] since it will never be blindly toggled.  
      }

      if (PowerDownTime > PowerDownDelay)
      {
        PowerDownTime = 0;      
        PowerDown     = 1;
        PowerDownInitialize = 0;
        
        // RESET ALL SETTINGS AND WAIT FOR POWER UP KEYPAD SEQUENCE
        for (int pin = 1; pin < SizeOfArray(PIN_MAP); pin++)
        {
          int p = PIN_MAP[pin];
          if (p != PIN_DISABLED)
            digitalWrite(p, LOW);
        }
        for(int n = 0; n < 17; n++)
        { 
          IO[n]=0;  // Set all outputs LOW
        } 
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void FlashLED(byte color, int rate)
{
#ifdef TEECES_PSI 
  if(!color)                    // LEDs OFF
  {
    setPSISolidstate(REAR_PSI, 0);
  }    
  else if (rate == ON)          // ALWAYS ON
  {
    if(color == YELLOW)
    {
      // ALL YELLOW
      setPSIstate(REAR_PSI, 6);
    }
    else if(color == GREEN)
    {
      // ALL GREEN
      setPSIstate(REAR_PSI, 0);
    }
    else if(color == BOTH)
    {
      setPSISolidstate(REAR_PSI, ~0);
    }
  }
  else
  {
    for (byte PSInum = 0; PSInum < TEECES_PSI; PSInum++) {
      if (millis() - PSItimes[PSInum] >= PSIpauses[PSInum]) {
        //time's up, do something...
        PSIstates[PSInum]++;
        if (PSIstates[PSInum] == 12) PSIstates[PSInum] = 0;
        if (PSIstates[PSInum] != 0 && PSIstates[PSInum] != 6) {
          //we're swiping...
          PSIpauses[PSInum] = pgm_read_byte(&PSIdelay[PSInum]);
        }
        else {
          //we're pausing
          PSIpauses[PSInum] = random(PSIpause[PSInum]);
          //decide if we're going to get 'stuck'
          if (random(100) <= PSIstuck) {
            if (PSIstates[PSInum] == 0) PSIstates[PSInum] = random(1, 3);
            else PSIstates[PSInum] = random(3, 5);
          }
        }
        setPSIstate(PSInum, PSIstates[PSInum]);
        PSItimes[PSInum] = millis();
      }
    }
  }
#endif
}

void Initialize()
{
  pinMode(LEFT, INPUT);               // Switch 3 - Power Up Trigger (LEFT)
  pinMode(RIGHT, INPUT);              // Switch 1 - Power Up Trigger (RIGHT)
  
  digitalWrite(LEFT, HIGH);           // turn on pullup resistors
  digitalWrite(RIGHT, HIGH);          // turn on pullup resistors
  
#ifdef TEECES_PSI
  lc.shutdown(0, false);
  lc.clearDisplay(0);
  lc.setIntensity(0,PSIBrightness);            // 0 = dim, 15 = full brightness
#if TEECES_PSI == 2
  lc.shutdown(1, false);
  lc.clearDisplay(1);
  lc.setIntensity(1,PSIBrightness);            // 0 = dim, 15 = full brightness
#endif
#endif

  for (int pin = 1; pin < SizeOfArray(PIN_MAP); pin++)
  {
    int p = PIN_MAP[pin];
    if (p != PIN_DISABLED)
    {
      pinMode(p, OUTPUT);
      digitalWrite(p, LOW);            // Pin 1 set Low on Power Up
    }
  }
  
//*************************************************************************************
//*************************************************************************************
//*************************************************************************************
//*************************************************************************************

  for(int n = 0; n < 17; n++)
  { 
    IO[n]=0;  // Set all outputs LOW
  }
  IO[12] = 1;  // set Power Down Extra pin HIGH
}
//*************************************************************************************
//*************************************************************************************
//*************************************************************************************
//*************************************************************************************
//*************************************************************************************

