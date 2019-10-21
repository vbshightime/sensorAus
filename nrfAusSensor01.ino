#include <SPI.h>
#include <nRF24L01.h>
#include <DS3232RTC.h> 
#include <ArduinoJson.h>
#include <RF24.h>
#include <printf.h>
#include <EEPROM.h>
#include <Wire.h>
#include<avr/power.h>
#include<avr/sleep.h>
#define CHANNEL 102
#define MAX_MESSAGE_LEN 31
#include <OneWire.h> 
#include <DallasTemperature.h>


#define ONE_WIRE_BUS 9 // port of NodeMCU=D4
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

unsigned long sensorTime = 0;
int sensorInterval = 30000;

RF24 radio(8, 10); // CE, CSN


volatile uint8_t wakeupPin= 2;   // RTC provides a 1Hz interrupt signal on this pin
//uint8_t configpin = 7;
volatile uint8_t configpin = 5;
int ledPin = 4;
//int ledPin = 7;//for node 5 
unsigned long configTimer;

const byte txAddr[6] = "00002";
const byte rxAddr[6] = "00001";

int sensorConfig = 0;

int retries = 0;
const char MESSAGE_BODY[] = "{\"d\":\"%s\",\"s\":%d}";
char messagePayload[MAX_MESSAGE_LEN];
char timestamp[32];
char dataAck[] = "";
//char ackRecp[]= "Success";
char configPayload[MAX_MESSAGE_LEN]="";
int sensorValue = 0;  // variable to store the value coming from the sensor
int sensorPin = A0;  // variable to store the value coming from the sensor
int alarmSet = 0;
int TIME_INTERVAl= 1;

String DEVICE = "006"; //change

void pin2Interrupt()
{  
     alarmSet = 1;
    detachInterrupt(digitalPinToInterrupt(wakeupPin));
  
}
 
void enterSleep(){ 
    retries = 0;
    attachInterrupt(digitalPinToInterrupt(wakeupPin), pin2Interrupt, FALLING);
    delay(100);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_bod_disable();
    sleep_mode();
    
    sleep_disable(); 
  }

  
void setup() {
    Serial.begin(115200);
    printf_begin();
    Serial.println("Started");
    pinMode(wakeupPin, INPUT_PULLUP);
    pinMode(configpin,INPUT);
    pinMode(ledPin, OUTPUT);
    //digitalWrite(ledPin,HIGH);//for node 5 change
    pinMode(ONE_WIRE_BUS, INPUT);
    sensors.begin(); 
    RTC.squareWave(SQWAVE_NONE); // Disable the default square wave of the SQW pin
    //RTC.setAlarm(alarmType, seconds, minutes, hours, dayOrDate);
    RTC.setAlarm(ALM1_MATCH_SECONDS, 50, 0, 0, 0 );
    RTC.alarm(ALARM_1);
    RTC.alarmInterrupt(ALARM_1, true); // Enable alarm 1 interrupt A1IE
    radio.begin();
    //radio.setAutoAck(1);                   
  //radio.enableAckPayload(); 
    radio.setPALevel(RF24_PA_MIN);
    radio.setChannel(CHANNEL);
    //radio.setDataRate(RF24_1MBPS);
    radio.setDataRate(RF24_250KBPS);
    radio.setAutoAck(1);                  
    radio.enableAckPayload(); 
    radio.setRetries(5,15);               
    radio.openWritingPipe(txAddr);
    radio.openReadingPipe(1,rxAddr);
    radio.stopListening();
    Serial.println(radio.isChipConnected()?"connected":"not connected");
    delay(5000);
  //radio.printDetails();
}

void loop() {
    //if(millis()-sensorTime>=sensorInterval){
  sensors.requestTemperatures(); 
  //int t = sensors.getTempCByIndex(0);
  int t = 23;
  snprintf(messagePayload,MAX_MESSAGE_LEN,MESSAGE_BODY,DEVICE.c_str(),t);
   if(radio.write(&messagePayload, sizeof(messagePayload))){
        Serial.println("Data Written");
        digitalWrite(ledPin,HIGH);
        //digitalWrite(ledPin,LOW);//for node5 change
        delay(1000);
        //digitalWrite(ledPin,HIGH);//for node 5 change
        digitalWrite(ledPin,LOW);
        Serial.println(messagePayload);
        Serial.println(sizeof(messagePayload));
        Serial.print("Going to sleep");
        enterSleep();
   }
//else if(retries==5){
//          Serial.println("Retries completed Going to sleep");
//          enterSleep();  
//      }else{
//           Serial.println("retry");
//           retries++;
//        }
   delay(10);
   //sensorTime = millis();
    //}
   
    
   if(alarmSet==1 & RTC.alarm(ALARM_1)){
      time_t timeNow;
      timeNow = RTC.get();
      formatTime(timestamp,timeNow);
      Serial.println(timestamp);
      int timeSleep = minute(timeNow)+TIME_INTERVAl;
      if(timeSleep>=59){
          timeSleep=timeSleep-59;
        }
       Serial.println(timeSleep);
       RTC.setAlarm(ALM1_MATCH_MINUTES, 0, timeSleep, 0, 0); 
       alarmSet=0;
    }
}

void formatTime(char *buf, time_t t)
{
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t));
}
