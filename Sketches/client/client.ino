#include "Arduino.h"
#include <stdio.h>

// includes for the UDP connections
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// keypad
#include <Keypad.h>

// debugging
#define DEBUG 1 // 1 to see the debug messages in the serial console, or 0 to disable

#define BUFFERSIZE 512 // UDP is limited and must be very short. 512 bytes is more than enough

#define SKETCH_UDP_PORT 2000 // this port is used to receive message events from Node.js

#define WEBSERVER_UDP_PORT 2010 // this port is used to send message events to Node.js

#define SENSOR_READ_INTERVAL 10 // number of seconds to read sensors and report to website

// for the UDP server
struct sockaddr_in my_addr, cli_addr;
int sockfd, i;
socklen_t slen=sizeof(cli_addr);
char msg_buffer[BUFFERSIZE];
fd_set readfds;
struct timeval tv;
int rv,n;

// pin connections
const byte sensorAnalogPin = 0;

// Keypad
enum SYSTEM_STATUS{
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
{'1','2','3'},
{'4','5','6'},
{'7','8','9'},
{'*','0','#'}
};

byte rowPins[ROWS] = {5, 4, 3, 2}; // pins on Intel Galielo I/O
byte colPins[COLS] = {8, 7, 6}; // pins on Intel Galielo I/O

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// PIR sensor
const byte sensorPIR_Pin = 9; // input pin
byte pirState = LOW; //
const byte ledPIRpin = 13;

// Relays
const byte relay1 = 10; // relay 1 command
const byte relay2 = 11; // relay 2 command

// time control
unsigned long time0 = 0;

// this function is only called when some error happens
void printError(char *str)
{
    Serial.print("ERROR: ");
    Serial.println(str);
}

// this function is reponsible for sending UDP datagrams
void sendUDPMessage(String protocol)
{
    struct sockaddr_in serv_addr;
    int sockfd, i, slen=sizeof(serv_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        printError("socket");
        return;
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(WEBSERVER_UDP_PORT);

    // considering the sketch and the web server run into Galileo
    // let's use the loopback address
    if (inet_aton("0.0.0.0", &serv_addr.sin_addr)==0)
    {
        printError("inet_aton() failed\n");
        close(sockfd);
        return;
    }
    char send_msg[BUFFERSIZE]; // more than enough
    memset((void *)send_msg, sizeof(send_msg), 0);
    protocol.toCharArray(send_msg, sizeof(send_msg), 0);

    if (sendto(sockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)
    &serv_addr, sizeof(serv_addr))==-1) 
    {
        printError("sendto()");
    }
    close(sockfd);
}

// this function is responsible to init the UDP datagram server
int populateUDPServer(void)
{
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        printError("socket");
    else if (DEBUG) Serial.println("Server : Socket() successful\n");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SKETCH_UDP_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
        printError("bind");
    else if (DEBUG) Serial.println("Server : bind() successful\n");
   
    memset(msg_buffer, 0, sizeof(msg_buffer));
}

// reading the temperature sensor in Celsius
float readTemperatureSensor()
{
    // getting the voltage reading from the temperature sensor
    int reading = analogRead(sensorAnalogPin);

    float VOUT = (reading * 5.0)/1024.0;
    if (DEBUG) {
        Serial.print(" volts");
        Serial.println(VOUT);
    }

    // converting to Celsius according to the datasheet
    float tempCelsius = (VOUT - 0.5) * 100 ;

    if (DEBUG) {
        Serial.print(" degrees Celsius:");
        Serial.println(tempCelsius);
    }
    return tempCelsius;
}

// convert celsius to fahrenheit
float convertTempToF(int celsius) {

    // converting to Fahrenheit
    float tempF = (celsius * 9.0 / 5.0) + 32.0;

    if (DEBUG) {
        Serial.print("degrees Fahrenheit:");
        Serial.println(tempF);
    }

    return tempF;
}

// update the LED status when the system is armed or disarmed
void updateLEDStatus() {
    if (currentStatus == LOCKED)
    {
        currentStatus = UNLOCKED;
        if (DEBUG)
        {
            Serial.println("SYSTEM UNLOCKED");
        }

        //turn OFF the LED
        digitalWrite(ledPin, LOW);
    }
    else
    {
        currentStatus = LOCKED;

        if (DEBUG)
        {
            Serial.println("SYSTEM LOCKED");
        }

        // turn ON the LED
        digitalWrite(ledPin, HIGH);
    }
}

// this is the key handler for the PRESS, RELEASE, and HOLD event
void handleKey(KeypadEvent key){
switch (keypad.getState())
{
    case PRESSED: // this is our ENTER
        digitalWrite(ledPin, !digitalRead(ledPin));
        delay(500);
        digitalWrite(ledPin, !digitalRead(ledPin));
        if (key == '#') {
            if (DEBUG) Serial.println(input);
            if (input == password)
            {
                updateLEDStatus();
            }
            input = "";
        }
        break;
    case RELEASED: // this is our CLEAR
        if (key == '*') {
            input = "";
        }
        break;
    }
}

void setup() {
    Serial.begin(115200);

    delay(3000);

    // init variables for UDP server
    populateUDPServer();

    // keypad
    pinMode(ledPin, OUTPUT);
    pinMode(ledPIRpin, OUTPUT);

    digitalWrite(ledPin, HIGH); // The default is system locked.. so, the LED must be HIGH
    digitalWrite(ledPIRpin, LOW); // Let's let the PIR sensor change the LED state
    
    keypad.addEventListener(handleKey); // this is the listener to handle the keys
    
    // relays
    pinMode(relay1, OUTPUT); // declare output
    pinMode(relay2, OUTPUT); // declare output
    
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
}
    
void loop() {

    if (time0 == 0) time0 = millis();

    // checking the keypad
    char key = keypad.getKey();

    if (key) {

        if ((key != '#') && (key != '*'))
        {
            input += key;
        }
        if (DEBUG)
        {
            Serial.print("key:");
            Serial.println(key);
        }
    }

    // PIR sensor
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
        if (pirState == HIGH){

            // we have just turned off
            if (DEBUG) Serial.println("Waiting for next movement");

            // We only want to print on the output change, not state
            pirState = LOW;
        }
    }

    // clear the set ahead of time
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    // wait until either socket has data ready to be recv()d (timeout 1000 usecs)
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if(rv==-1)
    {
        if (DEBUG)
        {
            Serial.println("Error in Select!!!");
        }
    }

    if(rv==0)
    {
        // TIMEOUT!!!!

        if ((millis()-time0) >= 1000)
            {
            // reached 1 seconds let's read the sensor and send a message!!!
            time0 = millis();

            String protocol = "";

            if (pirState == HIGH)
            {
                protocol += "*INTRUDER!!!*";
            }
            else
            {
                protocol += "*NO DETECTION*";
            }

            // reading the temperature sensor
            int tempC = readTemperatureSensor();
            int tempF = convertTempToF(tempC);

            char msg[20];
            memset(msg, 0, sizeof(msg));
            sprintf(msg, "%dC - %dF", tempC, tempF);

            protocol += "*";
            protocol += msg;

            // checking the system status
            if (currentStatus == LOCKED)
            {
                protocol += "*ARMED*";
            }
            else
            {
                protocol += "*DISARMED*";
            }
            sendUDPMessage(protocol);
        }
    }

    // checking if the UDP server received some message from the web page
    if (FD_ISSET(sockfd, &readfds))
    {
        if (recvfrom(sockfd, msg_buffer, BUFFERSIZE, 0,
        (struct sockaddr*)&cli_addr, &slen)==-1)
        {
            printError("recvfrom()");
            return; // let's abort the loop
        }
        if (DEBUG)
        {
            Serial.println("Received packet from %s:%d\nData:");
            Serial.println(inet_ntoa(cli_addr.sin_addr));
            Serial.println(msg_buffer);
        }

        String checkResp = msg_buffer;
        if (checkResp.lastIndexOf("L1ON", 0) < 0)
        {
            // There is no L1ON in the string.. let's switch off the relay
            digitalWrite(relay1, HIGH);

            if (DEBUG) Serial.println("The lamp 1 is OFF");
        }
        else
        {
            // Oops.. let’s switch relay 1 to ON
            digitalWrite(relay1, LOW);
            if (DEBUG) Serial.println("The lamp 1 is ON");
        }

        if (checkResp.lastIndexOf("L2ON", 6) < 0)
        {
            // There is no L2ON in the string.. let's switch off the relay
            digitalWrite(relay2, HIGH);

            if (DEBUG) Serial.println("The lamp 2 is OFF");
        }

        else
        {
            // Oops.. let switch relay 2 to ON
            digitalWrite(relay2, LOW);

            if (DEBUG)Serial.println("The lamp 2 is ON");
        }
        
        // let's clear the message buffer
        memset(msg_buffer, 0, sizeof(msg_buffer));
    }
}
