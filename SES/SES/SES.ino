#include <Servo.h>
//#include <Servo.h>
#include <avr\wdt.h>


const int UNIT_NO = 1; //unit 2 waits for watchdog to run out, unit 1 operates immediately
const int WATCHDOG_MS = 8000;//miliseconds after which watchdog times out and changes active units
volatile int currentMillis = 0;//current time value for active watchdog timer

//start as inactive
volatile bool active = false;

//inactive = 700, active = 200
int ledTime = 700;

//LDR read-outs
int ldrValues[3];//changes between frames for 5 data frames
int ldrChangeThreshold = 100;//if change exceeds this, consider the motor moving
int ldrValuesIndex = 0;//current index to be filled next loop

int lastLdrRead = 0;
int currentLdrRead = 0;

//light level changing check
bool currentlyMoving = false;
bool movementDetected = false;

//motor control

//servo objects
Servo myservo;
Servo myservo1;

//previous motor position 
int prev_pos = 0;


void setup() {

	//setting serial to baudrate of 115200
	Serial.begin(115200);
	Serial1.begin(9600);//inter-mcu serial

	//notifying host machine which unit this is
	char printable[25];
	sprintf(printable, "Initializing Unit No. %d", UNIT_NO);
	Serial.println(printable);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(LED_BUILTIN, OUTPUT);

	//timer setup
	noInterrupts();           // disable all interrupts
	TCCR3A = 0;
	TCCR3B = 0;
	TCNT3 = 0;

	OCR3B = 31250;            // compare match register 16MHz/256/2Hz
	//TCCR3B |= (1 << WGM32);   // CTC mode
	TCCR3B |= (1 << CS32);    // 256 prescaler 
	TIMSK3 |= (1 << OCIE3B);  // enable timer compare interrupt
	interrupts();             // enable all interrupts

	//if unit 1 start as active
	if (UNIT_NO == 1)
		active = true;

	//set hw watchdog
	wdt_enable(WDTO_2S + WDTO_1S);

	//analog ldr setup
	pinMode(A0, INPUT_PULLUP);//pullup so uses internal resistor because goddamn

	//initialize unit 1 time out
	currentMillis = 0;

	//motor initialization
	myservo.attach(9);  // attaches the servo on pin 9 to the servo object
	myservo1.attach(8);
	myservo.write(0);
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
		activeTasks();
	}

	wdt_reset();
}

void activeTasks()
{
	//round robin
	//ReceiveCommand();//receive command to move motor
	//MotorDriving();//send commands to motor

	WatchdogReset();//stave off watchdog reset
	ldrReadOut();//process current light sensor data

	detectMovement();//detect movement when necessary
}

void detectMovement() {
	if (currentlyMoving)
	{
		for (int i = 0; i < 3; i++)//sizeof(ldrValues) / sizeof(int)
		{
			if (abs(ldrValues[i]) > abs(ldrChangeThreshold))
			{
				movementDetected = true;
			}
		}
	}
}

void movementEnded()
{
	currentlyMoving = false;
	if (!movementDetected)//if it didn't move during the motor rotation, give up on this unit
	{
		Serial.println("No movement/light change detected, deactivating this unit");
		active = false;
	}
	else
	{
		Serial.println("Successfully moved motor");
	}
	//reset regardless
	movementDetected = false;
}

void ldrReadOut()//there is a bug in here
{
	currentLdrRead = analogRead(A0);
	Serial.print("LDR read-out: ");
	Serial.println(currentLdrRead);

	//calculate differences
	ldrValues[ldrValuesIndex] = lastLdrRead - currentLdrRead;

	Serial.print("LDR change value ");
	Serial.print(ldrValuesIndex);
	Serial.print(" = ");
	Serial.println(ldrValues[ldrValuesIndex]);

	///between here and
	//go through list of values
	ldrValuesIndex++;
	if (ldrValuesIndex > 2)//sizeof(ldrValues) / sizeof(int)
	{
		ldrValuesIndex = 0;
	}
	///here
	//Serial.println("Reset counter");
	lastLdrRead = currentLdrRead;
}

void WatchdogReset()//if active and unit 1, alert unit 2 while alive
{
	if (UNIT_NO == 1 && active)
	{
		Serial.print("Unit 2 Watchdog Reset, time = ");
		Serial.print(millis() / 1000);
		Serial.println(" s");
		Serial1.write(120);
	}
}

void serial1Flush() {
	Serial.print("Flushing: ");
	while (Serial1.available() > 0) {
		char t = Serial1.read();
		Serial.print(t);
	}
	Serial.println();
}

//receives data at all over serial1
void serialEvent1()//currently set to serial1 (pin 18/19)
{
	if (!active)
	{
		int read = Serial1.read();
		Serial1.println(read);
		if (read == 120)
		{
			//reset
			currentMillis = 0;
			Serial.print("Watchdog reset received, value: ");
			Serial.println(read);
		}
		else
		{
			Serial.print("Incorrect watchdog message received, value: ");
			Serial.println(read);
		}
		Serial1.flush();
		serial1Flush();
	}
}

void serialEvent()
{
	int plus_pos, pos;   // variable to store the servo position
	char rx_byte = 0, sign;
	char buff[3];
	int count = 3, i = 0, k, j;
	bool flag = "TRUE";
	//initialize position of motor
   //initialize buffer for receiving commands from keyboard
	for (j = 0; j < 3; j++)
	{
		buff[j] = 0;
	}
	while (count > 0) // read up to 360 degrees 
	{
		if (Serial.available() > 0)
		{                                // is a character available?
			rx_byte = Serial.read();       // get the character
			// check if a number was received
			if ((rx_byte >= '0') && (rx_byte <= '9'))
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
	sign = buff[0] + 48; //sign received
	//sign= buff[0];
	Serial.println(sign);
	Serial.print("Angle received:");
	count = 3;  //2 digits + 1 sign
	i--;
	for (k = 1; k <= i; k++)
	{
		buff[k] = buff[k] + 48;
		Serial.print(buff[k]);
		buff[k] = buff[k] - 48;
	}
	i = 0;
	//}
  //CHECK IF DEGREES ARE BELOW 180 AND ADJUST-----
	plus_pos = buff[1] * 10 + buff[2];
	pos = prev_pos + plus_pos;
	if (sign == '1')
	{
		pos = prev_pos - plus_pos;
		Serial.println("SUBSTARCTION1");
	}

	if (pos > 180 || (pos < 0))
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
				for (j = prev_pos; j >= pos; j--)
				{
					Serial.println("SUBSTARCTION");
					myservo.write(j);              // tell servo to go to position in variable 'j'
					Serial.println(j);
					delay(15);                       // waits 15ms for the servo to reach the position
				}
			}
			else
			{
				for (j = prev_pos; j <= pos; j++)
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

long oldMilis;

ISR(TIMER3_COMPB_vect) {//every 500ms
	if (!active)//only pay attention to the timer if inactive
	{
		Serial.print("Current seconds: ");
		Serial.print(currentMillis / 1000);
		Serial.println(" s");
		if (currentMillis > WATCHDOG_MS)
		{
			Serial.println("Watchdog activated");
			Serial.println("Activating this unit");
			active = true;
			return;
		}
		else
		{
			currentMillis += millis() - oldMilis;
		}
		oldMilis = millis();
		Serial.print("Total seconds: ");
		Serial.print(millis() / 1000);
		Serial.println(" s");
	}


	TCNT3 = 0;
}

void MotorDriving()
{

}

void ReceiveCommand()
{
}