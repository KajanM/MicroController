#include <Keypad.h>
#include <Arduino.h>

enum SYSTEM_STATUS {
	LOCKED, // 0
	UNLOCKED, // 1
};

static SYSTEM_STATUS currentStatus = LOCKED;
const String password = "1968";
String input;

const byte ledPin = 12;
const byte ROWS = 4; // four rows
const byte COLS = 3; // three columns
char keys[ROWS][COLS] = {
	{ '1','2','3' },
	{ '4','5','6' },
	{ '7','8','9' },
	{ '*','0','#' }
};

byte rowPins[ROWS] = { 5, 4, 3, 2 }; // pins on Intel Galielo I/O
byte colPins[COLS] = { 8, 7, 6 }; // pins on Intel Galielo I/O

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
	Serial.begin(115200);
	// in case there is an LED CONNECTED
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin, HIGH); // The default is system locked..so, the LED must be HIGH
	keypad.addEventListener(handleKey); // this is the listener to handlethe keys
}

void loop() {
	// reading the keyboard
	char key = keypad.getKey();
	// if it’s a valid key  
	if (key) {
		if ((key != '#') && (key != '*'))
		{
			input += key;
		}
		Serial.print("key:");
		Serial.println(key);
	}
}

// this function is only called when the PIN code
// typed matches the secret PIN code and inverts
// the system logic. It means if the system was LOCKED
// it will be UNLOCKED and vice versa.

void updateLEDStatus() {
	if (currentStatus == LOCKED)
	{
		currentStatus = UNLOCKED;
		Serial.println("SYSTEM UNLOCKED");
		// turn OFF the LED
		digitalWrite(ledPin, LOW);

		system("curl 'http://api.pushingbox.com/pushingbox?devid=xxxxxxxxxxxxx' > /dev/ttyGS0");
	}
	else
	{
		currentStatus = LOCKED;
		Serial.println("SYSTEM LOCKED");
		// turn ON the LED
		digitalWrite(ledPin, HIGH);
	}
}

// this function is responsible to handle
// the keypad events
void handleKey(KeypadEvent key) {
	switch (keypad.getState())
	{
	case PRESSED:
		digitalWrite(ledPin, !digitalRead(ledPin));
		delay(500);
		digitalWrite(ledPin, !digitalRead(ledPin));

		// this is our ENTER
		if (key == '#') {
			Serial.println(input);
			if (input == password)
			{
				updateLEDStatus();
			}
			input = "";
		}
		break;

	case RELEASED:
		// this is our CLEAR
		if (key == '*') {
			input = "";
		}
		break;
	}
}

