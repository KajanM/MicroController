void setup() {
Serial.begin(9600);
//It’ll not continue until you send an ‘a’ through the serial monitor
while(Serial.read()!='a'){
Serial.println("hello");
delay(1000);
}

//Display ifconfig result to serial monitor
system("ifconfig > /dev/ttyGS0");
}
void loop() { }
