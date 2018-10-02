//Libraries
#include "SD.h"
#include <Wire.h>
#include "RTClib.h"

const int ledPin = 12; // led for motion and sound sensor
const int ldrPin = A5; // output for ldr
int soundSensor=5; // output for sound sensor
int LED= 4; // led for sound sensor
boolean LEDStatus=false;




#define LOG_INTERVAL  1000 // mills between entries. 
// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()


/*
  determines whether to send the stuff thats being written to the card also out to the Serial monitor.
  This makes the logger a little more sluggish and you may want the serial monitor for other stuff.
  On the other hand, its hella useful. We'll set this to 1 to keep it on. Setting it to 0 will turn it off.

*/
#define ECHO_TO_SERIAL   1 // echo data to serial port. 


//Variables
char activeMotion [] = "Active";
char inactiveMotion [] = "Inactive";

RTC_DS1307 rtc;
// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void setup()
{
  Serial.begin(9600);
  pinMode(7, INPUT);//for motion sensor
  pinMode(ldrPin, INPUT); // for ldr
  pinMode(ledPin, OUTPUT);// led for for motion and ldr
  pinMode(soundSensor,INPUT);//sound sensor
  pinMode(LED,OUTPUT);// led for sound sensor

  // initialize the SD card
  initSDcard();

  // create a new file
  createFile();


  /**
   * connect to RTC
     Now we kick off the RTC by initializing the Wire library and poking the RTC to see if its alive.
  */
  initRTC();


  /**
     Now we print the header. The header is the first line of the file and helps your spreadsheet or math program identify whats coming up next.
     The data is in CSV (comma separated value) format so the header is too: "millis,stamp, datetime,hum,temp" the first item millis is milliseconds since the Arduino started,
     stamp is the timestamp, datetime is the time and date from the RTC in human readable format, hum is the humidity and temp is the temperature read.
  */
   logfile.println("datetime,motion,ldr-status,sound");
#if ECHO_TO_SERIAL
  Serial.println("datetime,motion,ldr-status,sound");
#endif //ECHO_TO_SERIAL

}

void loop()
{
  Serial.begin(9600);

  DateTime now;
 

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));

  now = rtc.now();
  
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
#if ECHO_TO_SERIAL
  
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
#endif //ECHO_TO_SERIAL


  int ldrStatus = analogRead(ldrPin);
  char lightStatus;
  char dark []= "dark";
  char bright [] = "bright";
  char motion;
  
  if (digitalRead(7) == HIGH) 
  {
    motion = activeMotion;
  }
  else 
  {
     motion = inactiveMotion;
  }
  
  if (ldrStatus <=200)
  {
    lightStatus = dark;
  }
  else
  {
    lightStatus = bright;
  }
  
   char sound;
   char yes[]="yes";
   char no[]= "no";
  if (soundSensor=HIGH)
  {
    sound = yes;
  }
  else
  {
    sound = no;
  }
  Serial.print("motion: ");
  Serial.print(motion);
  Serial.print(", ");
  
  Serial.print("light Status: ");
  Serial.print(lightStatus);
  Serial.print(", ");

  Serial.print("sound: ");
  Serial.print(sound);
  Serial.print(" ");

  
  

  
  if (digitalRead(7) == HIGH && ldrStatus <=200)
  {
    digitalWrite(ledPin, HIGH);   
    delay(5000); 
  }
  else if (digitalRead(7) == HIGH && ldrStatus >= 200)
  {
     digitalWrite(ledPin, LOW);  
  }
  
  else if(digitalRead(7) == LOW && ldrStatus >= 200)
  {
    digitalWrite(ledPin, LOW);  
  }
  else if(digitalRead(7) == LOW && ldrStatus <= 200)
  {
    digitalWrite(ledPin, LOW);  
  }

 int SensorData=digitalRead(soundSensor); 
  if(SensorData == 1)
    {
      if(LEDStatus==false)
      {
        LEDStatus=true;
        digitalWrite(LED,HIGH);
      }
   else
    {
        LEDStatus=false;
        digitalWrite(LED,LOW);
    }
   }
   delay(1000);


  logfile.print(", ");
  logfile.print(motion);
  logfile.print(", ");
  logfile.print(lightStatus);
  logfile.print(", ");
  logfile.print(sound);
  logfile.println(" ");
  
  logfile.flush();
}


/**
   The error() function, is just a shortcut for us, we use it when something Really Bad happened.
   For example, if we couldn't write to the SD card or open it.
   It prints out the error to the Serial Monitor, and then sits in a while(1); loop forever, also known as a halt
*/
void error(char const *str)
{
  Serial.print("error: ");
  Serial.println(str);

  while (1);
}

void initSDcard()
{
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

}

void createFile()
{
  //file name must be in 8.3 format (name length at most 8 characters, follwed by a '.' and then a three character extension.
  char filename[] = "MLOG00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[4] = i / 10 + '0';
    filename[5] = i % 10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }

  if (! logfile) {
    error("couldnt create file");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);
}

void initRTC()
{
  Wire.begin();
  if (!rtc.begin())
  {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
}
  
