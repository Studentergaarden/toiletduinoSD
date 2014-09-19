/* -*- coding: utf-8 -*- */

/* FROM
   http://bildr.org/2011/03/ds1307-arduino/
   The RTC returns the date and time as a "Binary Coded Decimal".
   This byte value is then converted into the corresponding digit.

   * SD card attached to SPI bus as follows:
   ** MOSI - pin 11
   ** MISO - pin 12
   ** CLK/SCK - pin 13
   ** CS - pin 4

   * IC2
   ** SDA - pin A4
   ** SCL - pin A5
*/




#include <Wire.h>
#include <Time.h>
#include <SPI.h>
#include <SD.h>
#define DS1307_ADDRESS 0x68

#define DEBUG

typedef struct {
  time_t start, duration;
  bool busy, ledState;
  int sensorVal; // analog readings
  float val; // scaled vals
  byte pin;
  unsigned long previousMillis, currentMillis;
  // char filename[10];
  const char* filename;
} toilets_t;


toilets_t t1, t2;
void setupToilet(toilets_t *t, byte pin, const char *filename);
void checkToilet(toilets_t *t);
void blink(int times);
void digitalClockPrint(File *dataFile, toilets_t *t);
void printDigits(File *dataFile, int digits);


// for RTC work
byte RTCsecond, RTCminute, RTChour, RTCdayOfWeek, RTCdayOfMonth, RTCmonth, RTCyear;

void clkSync();
void getDate(byte *RTCsecond,
             byte *RTCminute,
             byte *RTChour,
             byte *RTCdayOfWeek,
             byte *RTCdayOfMonth,
             byte *RTCmonth,
             byte *RTCyear);
byte bcdToDec();

const byte chipSelect = 4;
const byte ledPin = 7;
const unsigned long blinkInterval = 1000; // ms


void setup(){

  Wire.begin();
  Serial.begin(9600);

  /* Init SD-card */
  pinMode(10, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    // return;
  }
  Serial.println("card initialized.");

  /* Set internal time from RTC */
  clkSync();

  setupToilet(&t1,2,"t1.txt"); // struct, pin, filename for SD
  setupToilet(&t2,3,"t2.txt");

  pinMode(ledPin, OUTPUT);   
}

void loop(){
  //  delay(1000);

  checkToilet(&t1);
  checkToilet(&t2);

}


void setupToilet(toilets_t *t, byte pin, const char *filename){

  t->pin = pin;
  t->val = 1;
  t->busy = false;
  t->previousMillis = 0;
  t->currentMillis = 0;
  t->ledState = LOW;
  //strcpy(t->filename, "t_output->txt"); // arrays in C are not assignable:
  t->filename = filename; // dynamic memory decleration instead->
  pinMode(t->pin, INPUT_PULLUP);
  digitalWrite(t->pin,HIGH);
}

void checkToilet(toilets_t *t){
  /* set a flag and start timer when door is locked. door is not declared 'locked' until after a set interval */

  const float scaleb = 0.999;
  const float scalea = 0.001;
  File dataFile;

  t->sensorVal = digitalRead(t->pin);
  t->val = scalea*t->val + scaleb*t->sensorVal;
  
  
  /* blink if occupied */
  t->currentMillis = millis();
  if((t->busy == true) && (t->currentMillis - t->previousMillis > blinkInterval)) {
    // save the last time you blinked the LED 
      t->previousMillis = t->currentMillis;   
      t->ledState = !t->ledState;
      digitalWrite(ledPin, t->ledState);
  }

  if (t->val >= 0.999 && t->busy == true) { 
    /* VACANT */
    t->busy = false;
    t->duration = now() - t->start;

    /* ignore visits under 15 sec */
    if(t->duration > 15){
      /* write to file */
      /* dataFile = SD.open("dag1.txt", FILE_WRITE); */
      dataFile = SD.open(t->filename, FILE_WRITE);
      // if the file is available, write to it:
      if (dataFile) {
        /* UNIX time */
        dataFile.print("date: \t"), dataFile.print(t->start), dataFile.print(",\t");
        /* HUMAN time */
        digitalClockPrint(&dataFile, t);
        dataFile.print("duration: \t"), dataFile.print(t->duration), dataFile.println();
        dataFile.close();
        // print to the serial port too:
        // Serial.println(t->duration);

        /* blink if succeded */
        blink(3);
      }  
      // if the file isn't open, pop up an error:
      else {
        Serial.print("error opening "), Serial.println(t->filename);
        /* double blink led if there's a SD-card error */
        blink(6);
      }
    }

    /* Make sure to diable led */
    digitalWrite(ledPin, LOW);

#ifdef DEBUG
    Serial.print("toilet frit igen - ");
    Serial.print("Varighed[s]: "), Serial.println(t->duration);
#endif

  } else if (t->val <= 0.0001 && t->busy == false) { 
    /* OCCUPIED */
    t->busy = true;
    t->start = now();
    
#ifdef DEBUG
    Serial.println("toilet optaget");
#endif
  }


/* #ifdef DEBUG */
/*   Serial.print("scaled: "), Serial.println(t->val); */
/*   Serial.print("sensor: "), Serial.println(t->sensorVal); */
/* #endif */

}


void blink(int times){
  /* blink led the *stupid* way */
  for(int i=0;i<times;i++){
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(100);
  }

}

void digitalClockPrint(File *dataFile, toilets_t *t){
  // digital clock display of the time
  dataFile->print(hour(t->start));
  printDigits(dataFile,minute(t->start));
  printDigits(dataFile,second(t->start));
  dataFile->print(" ");
  dataFile->print(day(t->start));
  dataFile->print(" ");
  dataFile->print(month(t->start));
  dataFile->print(" ");
  dataFile->print(year(t->start)); 
  dataFile->print(",\t"); 
}

void printDigits(File *dataFile, int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  dataFile->print(":");
  if(digits < 10)
    dataFile->print('0');
  dataFile->print(digits);
}


/*************/
/* RTC STUFF */
/*************/
void clkSync(){

  getDate(&RTCsecond, &RTCminute, &RTChour, &RTCdayOfWeek, &RTCdayOfMonth, &RTCmonth, &RTCyear);
  setTime(RTChour,RTCminute,RTCsecond,RTCdayOfMonth,RTCmonth,RTCyear);
 
}

byte bcdToDec(byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}


// Gets the date and time from the ds1307
void getDate(byte *RTCsecond,
             byte *RTCminute,
             byte *RTChour,
             byte *RTCdayOfWeek,
             byte *RTCdayOfMonth,
             byte *RTCmonth,
             byte *RTCyear){


  byte zero = 0x00;
  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *RTCsecond     = bcdToDec(Wire.read());
  *RTCminute     = bcdToDec(Wire.read());
  *RTChour       = bcdToDec(Wire.read() & 0b111111);  //24 hour time
  *RTCdayOfWeek  = bcdToDec(Wire.read());  //0-6 -> sunday - Saturday
  *RTCdayOfMonth = bcdToDec(Wire.read());
  *RTCmonth      = bcdToDec(Wire.read());
  *RTCyear       = bcdToDec(Wire.read());
}