#include <Servo.h>

Servo myservo;  // create servo object to control a servo
Servo myservo1;
// twelve servo objects can be created on most boards

void setup() {
  Serial.begin(9600);
  Serial.println("Please type desired angle\n");
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo1.attach(8);
  
}

char rx_byte = 0;
char buff[3];
int count = 3, i =0, k, j;

void loop() {
  //initialize buffer for communication
  for (j=0;j<3;j++)
    { 
      buff[j] = 0;
    }
  
  
  if (Serial.available() > 0) {    // is a character available?
    rx_byte = Serial.read();       // get the character
    if (count > 0) // read up to 360 degrees
    {
    // check if a number was received
      if ((rx_byte > '0') && (rx_byte <= '9')) {        
        buff[i] = rx_byte;
        count--;
        i++;
        //Serial.print("Angle received \n");
        //Serial.println(rx_byte);
        }
        else {
        Serial.println("Not correct, please type again.");
        }
    }
    else 
        {
        Serial.print("Angle received: \n");
        count = 3; 
        i--;
        for (k=0;k<=i;k++)
          {
          Serial.println(buff[k]);
          }
        i = 0;
        }
  }
  //control the motors
//  void control_motor(int buff[3]) 
//    {
      int plus_pos, pos, prev_pos = 10;   // variable to store the servo position
      plus_pos = buff[0]*100+buff[1]*10+buff[0]; 
      bool flag = "TRUE";
      if (flag){
        // in steps of 1 degree
        pos = prev_pos + plus_pos;
        for (j=0;j<=pos;j++)
        {
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
        }
      }
      }
    //return 1;
    //exit(0); 

  //}

//------------


