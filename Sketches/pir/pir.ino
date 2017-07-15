
const byte ledPIRpin = 13; // LED pin for the LED
const byte sensorPIR_Pin = 9; // input pin
byte pirState = LOW; //

void setup() {
	pinMode(ledPIRpin, OUTPUT); // declare output
	pinMode(sensorPIR_Pin, INPUT); // declare input

	Serial.begin(115200);
}

void loop() {

	if (digitalRead(sensorPIR_Pin) == HIGH) { // input HIGH
		digitalWrite(ledPIRpin, HIGH); // LED ON
		if (pirState == LOW)
		{
			// we have just turned on
			Serial.println("OPS!!! Someone here!!! motion DETECTED!");

			// We only want to print on the output change, not state
			pirState = HIGH;
		}
	}
	else
	{
		digitalWrite(ledPIRpin, LOW); // turn LED OFF
		if (pirState == HIGH) {

			// we have just turned of
			Serial.println("Waiting for next moviment");

			// We only want to print on the output change, not state
			pirState = LOW;
		}
	}
}