#include <avr\wdt.h>

const int UNIT_NO = 2; //unit 2 waits for watchdog to run out, unit 1 operates immediately
const int WATCHDOG_MS = 8000;//miliseconds after which watchdog times out and changes active units
int currentMilis = 0;//current time value for active watchdog timer

//start as inactive
bool active = false;


void setup() {

	//setting serial to baudrate of 9600
	Serial.begin(9600);

	//notifying host machine which unit this is
	char printable [50];
	sprintf(printable, "Initializing Unit No. %d", UNIT_NO);
	Serial.println(printable);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(LED_BUILTIN, OUTPUT);

	//timer setup
 // initialize timer1 

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
}

void loop() {
	//debug blinking LED to make sure it hasn't crashed
	digitalWrite(LED_BUILTIN, HIGH);   
	delay(300);                       
	digitalWrite(LED_BUILTIN, LOW);  
	delay(300);    

	//if the current active unit
	if (active)
	{
		ReceiveCommand();
		MotorDriving();
		WatchdogReset();
	}
}

void WatchdogReset()//if active and unit 1, alert unit 2 while alive
{
	if (UNIT_NO == 1 && active)
	{
		Serial.write('a');
	}
}

//receives data at all over serial0
void serialEvent()//currently set to usb
{
	if ((char)Serial.read() == 'a')
	{
		//reset
		currentMilis = 0;
		Serial.println("Watchdog reset received");
	}
	else
	{
		Serial.println("Incorrect watchdog message received");
	}
}

long oldMilis;

ISR(TIMER1_COMPA_vect) {//every 500ms
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