#include <avr\wdt.h>
#include <Servo.h>

Servo myservo;  // create servo object to control a servo
Servo myservo1;
// twelve servo objects can be created on most boards
const int UNIT_NO = 1; //unit 2 waits for watchdog to run out, unit 1 operates immediately
const int WATCHDOG_MS = 8000;//miliseconds after which watchdog times out and changes active units
int currentMilis = 0;//current time value for active watchdog timer

//start as inactive
bool active = false;

//inactive = 700, active = 200
int ledTime = 700;
int prev_pos = 0;


void setup() {

  //setting serial to baudrate of 9600
  Serial.begin(9600);
  Serial1.begin(9600);
  
  //notifying host machine which unit this is
  char printable [50];
  sprintf(printable, "Initializing Unit No. %d", UNIT_NO);
  Serial.println(printable);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  //timer setup
  noInterrupts();           // disable all interrupts
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  OCR2A = 31250;            // compare match register 16MHz/256/2Hz
  TCCR2B |= (1 << WGM12);   // CTC mode
  TCCR2B |= (1 << CS12);    // 256 prescaler 
  TIMSK2 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts

  //if unit 1 start as active
  if (UNIT_NO == 1)
    active = true;

  //set hw watchdog
  wdt_enable(WDTO_4S);
  //-----
  Serial.println("Please insert: 0 for addition and 1 for substraction.\n");
  Serial.println("Then insert the desired angle.\n");
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo1.attach(8);
  myservo.write(0);

  //----
}

void loop() {
  //debug blinking LED to make sure it hasn't crashed
  digitalWrite(LED_BUILTIN, HIGH);   
  delay(ledTime);                       
  digitalWrite(LED_BUILTIN, LOW);  
  delay(ledTime);     

  //if the current active unit
  if (active)
  {
  ledTime = 200;

    //ReceiveCommand();
    //MotorDriving();
    WatchdogReset();
  }

  wdt_reset();
}

void WatchdogReset()//if active and unit 1, alert unit 2 while alive
{
  if (UNIT_NO == 1 && active)
  {
    char printable[50];
    sprintf(printable, "Resetting Unit 2 Watchdog; Time = %e", millis());
    Serial.println(printable);
    Serial1.write(120);
    
  }
}

//receives data at all over serial1
void serialEvent1()//currently set to serial1
{
  if (!active)
  {
    if ((char)Serial1.read() == 'a')
    {
      //reset
      currentMilis = 0;
      Serial.println("Watchdog reset received");
      Serial1.flush();
    }
    else
    {
      Serial.println("Incorrect watchdog message received");
    }
  }
}

long oldMilis;

//
void serialEvent()
{
  int plus_pos, pos;   // variable to store the servo position
  char rx_byte = 0, sign;
  char buff[3];
  int count = 3, i =0, k, j;
  bool flag = "TRUE";
   //initialize position of motor
  //initialize buffer for receiving commands from keyboard
    for (j=0;j<3;j++)
      { 
        buff[j] = 0;
      }
    while (count > 0) // read up to 360 degrees 
     {
      if (Serial.available() > 0) 
      {                                // is a character available?
      rx_byte = Serial.read();       // get the character
      // check if a number was received
      if ((rx_byte >='0') && (rx_byte <= '9')) 
          {        
          buff[i] = rx_byte - 48;
          count--;
          i++;
          }
          else 
          {
          Serial.println("Not correct, please type again.");
          }
       }
      }
          sign  = buff[0]+48; //sign received
          //sign= buff[0];
          Serial.println(sign);
          Serial.print("Angle received:");
          count = 3;  //2 digits + 1 sign
          i--;
          for (k=1;k<=i;k++)
            {
            buff[k]=buff[k]+48;
            Serial.print(buff[k]);
            buff[k]=buff[k]-48;
            }
          i = 0;
          //}
        //CHECK IF DEGREES ARE BELOW 180 AND ADJUST-----
        plus_pos = buff[1]*10+buff[2];
        pos = prev_pos + plus_pos;
        if (sign == '1')
        {
         pos = prev_pos - plus_pos; 
         Serial.println("SUBSTARCTION1");
        }
         
        if (pos>180 || (pos<0))
        {
          Serial.println("Incorrect angle");
          pos = prev_pos;
        }
        else
         {
            Serial.println("Plus position is:");
            Serial.println(plus_pos);
            if (flag)
            {
              // in steps of 1 degree
              if (sign == '1')
              {
                  for (j=prev_pos;j>=pos;j--)
                  {
                    Serial.println("SUBSTARCTION");
                    myservo.write(j);              // tell servo to go to position in variable 'j'
                    Serial.println(j);
                    delay(15);                       // waits 15ms for the servo to reach the position
                  }
              }
              else
              {
                for (j=prev_pos;j<=pos;j++)
                {
                  myservo.write(j);              // tell servo to go to position in variable 'j'
                  Serial.println(j);
                  delay(15);                       // waits 15ms for the servo to reach the position
                }
               }
              prev_pos = pos;
            }
        }

}


///



ISR(TIMER2_COMPA_vect) {//every 500ms
  if (!active)//only pay attention to the timer if inactive
  {
    char printable [60];
    sprintf(printable, "Current milliseconds: %d", currentMilis);
    Serial.println(printable);
    if (currentMilis > WATCHDOG_MS)
    {
      Serial.println("Watchdog activated");
      Serial.println("Activating this unit");
      active = true;
    }
    else
    {
      currentMilis += millis() - oldMilis;
    }
    oldMilis = millis();
    sprintf(printable, "Total milliseconds: %d", millis());
    Serial.println(printable);
  }
}

void MotorDriving()
{

}

void ReceiveCommand()
{
}

