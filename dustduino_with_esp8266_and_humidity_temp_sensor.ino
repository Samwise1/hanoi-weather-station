//took all resets out/


//DustDuino with ESP8266 to emoncms.org
//based on Matthew Schroyer, MentalMunition.com
//ESP8266+emoncms.org and SHARP sensor added by Tom Tobback Oct 2015
/* Interface to Sharp GP2Y1010AU0F Particle Sensor
 based on Christopher Nafis  
 Sharp pin 1 (V-LED)   => 5V + 150 ohm resistor + 220uF capacitor
 Sharp pin 2 (LED-GND) => Arduino GND pin
 Sharp pin 3 (LED)     => Arduino pin 7
 Sharp pin 4 (S-GND)   => Arduino GND pin
 Sharp pin 5 (Vo)      => Arduino A0 pin
 Sharp pin 6 (Vcc)     => 5V
 */

// DESCRIPTION
// Outputs particle concentration per cubic foot and
// mass concentration (microgram per cubic meter) to
// serial, from a Shinyei particuate matter sensor and SHARP sensor

// CONNECTING THE PDD42
// P1 channel on sensor is connected to digital 3
// P2 channel on sensor is connected to digital 2

// THEORY OF OPERATION - PDD sensor
// Sketch measures the width of pulses through
// boolean triggers, on each channel.
// Pulse width is converted into a percent integer
// of time on, and equation uses this to determine
// particle concentration.
// Shape, size, and density are assumed for PM10
// and PM2.5 particles. Mass concentration is
// estimated based on these assumptions, along with
// the particle concentration.
// SHARP sensor: switch on LED, measure analog output, switch off LED

// add code from the library of DHT 


//DHT variables
#include "DHT.h"

#define DHTPIN 4     // what digital pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);





// WIFI VARIABLES
#include <SoftwareSerial.h>
#define SSID "xxxx"
#define PASS "xxxx"
#define DST_IP "80.243.190.58"    // emoncms.org
SoftwareSerial espSerial(10, 11); // RX, TX
// reset on pin9 (LOW to reset)

// PDD SENSOR VARIABLES dustduino code
unsigned long starttime;
unsigned long triggerOnP1;
unsigned long triggerOffP1;
unsigned long pulseLengthP1;
unsigned long durationP1;
boolean valP1 = HIGH;
boolean triggerP1 = false;

unsigned long triggerOnP2;
unsigned long triggerOffP2;
unsigned long pulseLengthP2;
unsigned long durationP2;
boolean valP2 = HIGH;
boolean triggerP2 = false;

float ratioP1 = 0;
float ratioP2 = 0;
unsigned long sampletime_ms = 60000;   // length of measurement
float countP1;
float countP2;
/*
// SHARP SENSOR VARIABLES
int dust_analog;
float dust_voltage;  // measured
int concSHARP;       // calculated
int samples = 100;    // with delay of 100ms in between
*/


void setup(){
  pinMode(3, INPUT); // PDD sensor channel P1
  pinMode(2, INPUT); // PDD sensor channel P2
//  pinMode(7, OUTPUT); // SHARP LED switch
  pinMode(10, INPUT);    // softserial RX for ESP
  pinMode(11, OUTPUT);   // softserial TX for ESP
  pinMode(9, OUTPUT);    // ESP reset

  espSerial.begin(9600); // for ESP, can't be faster than 19200 for softserial
  espSerial.setTimeout(2000);

  Serial.begin(9600);
  Serial.print("PDD dust sensor measuring period (sec): ");
  Serial.println(sampletime_ms/1000);
 // Serial.print("SHARP dust sensor reading with interval 0.1sec and sample number: ");
//  Serial.println(samples);
  Serial.println();
  Serial.println("ratioP1 \tratioP2 \tcountP1 \tcountP2 \tPM10count \tPM2.5count \tPM10conc \tPM2.5conc \tTemp \tHumidity \tFeels like ");
}

void loop() {
  
  // READ PDD SENSOR
  durationP1 = 0;
  durationP2 = 0;
  starttime = millis();
  
  while (millis() - starttime < sampletime_ms) {
  valP1 = digitalRead(3);
  valP2 = digitalRead(2);
  
  if(valP1 == LOW && triggerP1 == false){
    triggerP1 = true;
    triggerOnP1 = micros();
  }
  
  if (valP1 == HIGH && triggerP1 == true){
      triggerOffP1 = micros();
      pulseLengthP1 = triggerOffP1 - triggerOnP1;
      durationP1 = durationP1 + pulseLengthP1;
      triggerP1 = false;
  }
  
    if(valP2 == LOW && triggerP2 == false){
    triggerP2 = true;
    triggerOnP2 = micros();
  }
  
    if (valP2 == HIGH && triggerP2 == true){
      triggerOffP2 = micros();
      pulseLengthP2 = triggerOffP2 - triggerOnP2;
      durationP2 = durationP2 + pulseLengthP2;
      triggerP2 = false;
  }
  }
  
  // CALCULATE PDD RESULTS             
  ratioP1 = durationP1/(sampletime_ms*10.0);  //  percentage 0=>100
  ratioP2 = durationP2/(sampletime_ms*10.0);
  countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
  countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
  float PM10count = countP2;                  // as in original dustduino, to count only the larger particles
  float PM25count = countP1 - countP2;
      
  // first, PM10 count to mass concentration conversion
  double r10 = 2.6*pow(10,-6);
  double pi = 3.14159;
  double vol10 = (4/3)*pi*pow(r10,3);
  double density = 1.65*pow(10,12);
  double mass10 = density*vol10;
  double K = 3531.5;
  float concLarge = (PM10count)*K*mass10;
  if (concLarge > 500) concLarge = 500;        // to clip bad readings
      
   // next, PM2.5 count to mass concentration conversion
   double r25 = 0.44*pow(10,-6);
   double vol25 = (4/3)*pi*pow(r25,3);
   double mass25 = density*vol25;
   float concSmall = (PM25count)*K*mass25;
   if (concSmall > 500) concSmall = 500;        // to clip bad readings
      
   // PRINT PDD DATA TO SERIAL 
   Serial.print(ratioP1);
   Serial.print("\t\t");
   Serial.print(ratioP2);
   Serial.print("\t\t");
   Serial.print(countP1);
   Serial.print("\t\t");
   Serial.print(countP2);
   Serial.print("\t\t");
   Serial.print(PM10count);
   Serial.print("\t\t");
   Serial.print(PM25count);
   Serial.print("\t\t");
   Serial.print(concLarge);
   Serial.print("\t\t");
   Serial.print(concSmall);
   Serial.print("\t\t");

// get, calculate and print temperature and humity to serial

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(false);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, true);

  //Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  //Serial.print("Temperature: ");
  Serial.print(t);
  //Serial.print(" *C ");
  //Serial.print(f);
  //Serial.print(" *F\t");
  //Serial.print("Heat index: ");
  Serial.print(hic);
  //Serial.println(" *C ");//added println
   
 /*  // READ SHARP SENSOR 
   dust_voltage = 0;
   for (int i=0;i<samples;i++) {
    
    digitalWrite(7,LOW); // power on the LED
    delayMicroseconds(280);
    dust_analog = analogRead(A0); // read the dust value
    delayMicroseconds(40);
    digitalWrite(7,HIGH); // turn the LED off
    delayMicroseconds(9680);
  
    dust_voltage += dust_analog * 5.0 / 1023.0 ;  // voltage 0-5V
    delay(100);
  }
  dust_voltage = dust_voltage / samples;
  
  // CALCULATE SHARP RESULTS
  concSHARP = 172*dust_voltage-100;        // density microgram/m3
  if (concSHARP > 500) concSHARP = 500;
  
  // PRINT SHARP DATA TO SERIAL
  Serial.print(dust_voltage);
  Serial.print("\t\t");
  Serial.println(concSHARP);
    
*/    

  // SEND DATA OVER WIFI
  boolean sent=false;
  for(int i=0;i<5;i++)
           {
           if(sendData(concLarge,concSmall, t, h, hic))    //perhaps? 
             {
             sent = true;
             break;
             }
           }
       if (!sent){
           Serial.println(F("failed 5 times to send"));
           }
     
    }    
  

boolean sendData(int PM10, int PM25, int TEMP, int HUMIDITY, int FEEL)  
{
 Serial.print(F("reset ESP8266.."));
 //test if the module is ready

 digitalWrite(9, LOW);     // reset the ESP module
 delay(100);
 digitalWrite(9, HIGH);
  
 espSerial.println("AT+RST");
 delay(500); 

 //if(espSerial.find("ready"))
//// {
 //Serial.print(F("module is ready.."));
// }
// else
 //{
// Serial.println(F("module has no response."));
 //return false;
 //}

 //connect to the wifi
 boolean connected=false;

 for(int i=0;i<5;i++)
 {
 if(connectWiFi())
 {
 connected = true;
 break;
 }
 }
 if (!connected){
 Serial.println(F("tried 5 times to connect, please reset"));
 return false;
 }
 delay(500);
  
 //set the single connection mode
 espSerial.println("AT+CIPMUX=0");
 delay(500);

 espSerial.flush();

 String cmd; 
 cmd = "AT+CIPSTART=\"TCP\",\"";
 cmd += DST_IP;
 cmd += "\",80";
 espSerial.println(cmd);
// Serial.println(cmd);

 if(espSerial.find("Error")) return false;


String action;
 action = "GET /emoncms/input/post.json?node=22&apikey=xxxxxx&csv=";
 action += PM10;
 action += ",";
 action += PM25;
 action += ",";
 action += TEMP;
 action += ",";
 action += HUMIDITY;
  action += ",";
 action += FEEL;
 action += " HTTP/1.0\r\n\r\n";

 espSerial.print("AT+CIPSEND=");
 espSerial.println(action.length());

 if(espSerial.find(">"))
   {
   Serial.print(">");
   } else 
     {
     espSerial.println("AT+CIPCLOSE");
     Serial.println(F("connect timeout"));
     return false;
     }

 espSerial.println(action);
// Serial.println(action);
 delay(1000);

 if (espSerial.find("SEND OK"))
   {
   Serial.print(F("send OK.."));
   }
 if (espSerial.find("200 OK"))
   {
     Serial.println(F(" receive OK!"));
     return true;
   }
   else {
     return false;
   }
 }


boolean connectWiFi() {
 espSerial.println("AT+CWMODE=1");
 delay(2000);
 String cmd="AT+CWJAP=\"";
 cmd+=SSID;
 cmd+="\",\"";
 cmd+=PASS;
 cmd+="\"";
// Serial.println(cmd);
 espSerial.println(cmd);
 delay(5000);

 if(espSerial.find("OK")) {
   Serial.print(F("connected to wifi.."));
   return true;
   } else 
     {
     Serial.println(F("cannot connect to wifi"));
     return false;
     }
}

