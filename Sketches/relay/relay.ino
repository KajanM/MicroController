const byte relay = 10; 

void setup() {
  pinMode(relay, OUTPUT); // declare output
  Serial.begin(115200);
}

void loop(){
  digitalWrite(relay, LOW); // turn ON
  delay(5000);
  
  digitalWrite(relay, HIGH); // turn OFF
  delay(5000);
}
