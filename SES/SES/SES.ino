#include <avr\wdt.h>

const int UNIT_NO = 1; //unit 2 waits for watchdog to run out, unit 1 operates immediately
const int WATCHDOG_MS = 8000;//miliseconds after which watchdog times out and changes active units
int currentMilis = 0;//current time value for active watchdog timer

//start as inactive
bool active = false;

//inactive = 700, active = 200
int ledTime = 700;

//LDR read-outs
int ldrValues[5];//changes between frames for 5 data frames
int ldrChangeThreshold = 100;//if change exceeds this, consider the motor moving
int ldrValuesIndex = 0;//current index to be filled next loop

int lastLdrRead = 0;
int currentLdrRead;

//light level changing check
bool currentlyMoving = false;
bool movementDetected = false;


void setup() {

	//setting serial to baudrate of 9600
	Serial.begin(9600);
	Serial1.begin(1200);//inter-mcu code

	//notifying host machine which unit this is
	char printable[50];
	sprintf(printable, "Initializing Unit No. %d", UNIT_NO);
	Serial.println(printable);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(LED_BUILTIN, OUTPUT);

	//timer setup
	noInterrupts();           // disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	OCR1A = 31250;            // compare match register 16MHz/256/2Hz
	TCCR1B |= (1 << WGM12);   // CTC mode
	TCCR1B |= (1 << CS12);    // 256 prescaler 
	TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
	interrupts();             // enable all interrupts

	//if unit 1 start as active
	if (UNIT_NO == 1)
		active = true;

	//set hw watchdog
	wdt_enable(WDTO_4S);

	//analog ldr setup
	pinMode(A0, INPUT_PULLUP);//pullup so uses internal resistor because goddamn

	//initialize unit 1 time out
	currentMilis = 0;
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
	ReceiveCommand();//receive command to move motor
	MotorDriving();//send commands to motor

	WatchdogReset();//stave off watchdog reset
	ldrReadOut();//process current light sensor data

	detectMovement();//detect movement when necessary
}

void detectMovement() {
	if (currentlyMoving)
	{
		for (int i = 0; i < sizeof(ldrValues) / sizeof(int); i++)
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
}

void ldrReadOut()
{
	currentLdrRead = analogRead(A0);
	Serial.print("LDR read-out: ");
	Serial.println(currentLdrRead);

	//calculate differences
	ldrValues[ldrValuesIndex] = lastLdrRead - currentLdrRead;

	//go through list of values
	ldrValuesIndex++;
	if (ldrValuesIndex > sizeof(ldrValues) / sizeof(int))
	{
		ldrValuesIndex = 0;
	}

	lastLdrRead = currentLdrRead;
}

void WatchdogReset()//if active and unit 1, alert unit 2 while alive
{
	if (UNIT_NO == 1 && active)
	{
		char printable[50];
		sprintf(printable, "Resetting Unit 2 Watchdog; Time = %d s", millis() / 1000);
		Serial.println(printable);
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
void serialEvent1()//currently set to serial1
{
	if (!active)
	{
		int read = Serial1.read();
		Serial1.println(read);
		if (read == 120)
		{
			//reset
			currentMilis = 0;
			Serial.print("Watchdog reset received, value: ");
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

long oldMilis;

ISR(TIMER1_COMPA_vect) {//every 500ms
	if (!active)//only pay attention to the timer if inactive
	{
		char printable[60];
		sprintf(printable, "Current seconds: %d s", currentMilis / 1000);
		Serial.println(printable);
		if (currentMilis > WATCHDOG_MS)
		{
			Serial.println("Watchdog activated");
			Serial.println("Activating this unit");
			active = true;
			return;
		}
		else
		{
			currentMilis += millis() - oldMilis;
		}
		oldMilis = millis();
		sprintf(printable, "Total seconds: %d s", millis() / 1000);
		Serial.println(printable);
	}
}

void MotorDriving()
{

}

void ReceiveCommand()
{
}