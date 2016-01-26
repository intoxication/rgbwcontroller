// MySensors
#include <MySigningNone.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <MySensor.h>
#include <SPI.h>
#include <DHT.h>
#include <Button.h>

MyTransportNRF24 radio(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_LEVEL_GW);
MyHwATMega328 hw;
MySensor gw(radio, hw);
DHT dht;

#define RED_PIN 3
#define GREEN_PIN 4
#define BLUE_PIN 5

#define SAVE_LIGHT_STATE 1
#define SAVE_DIMMER_LEVEL 2
#define SAVE_LIGHT_COLOR 3

#define RELAY_ON 1
#define RELAY_OFF 0

// Settings for the push button
#define BTN_PIN 7
#define DEBOUNCE_MS 20
#define INVERT false
#define PULLUP false

long RGB_values[3] = {0,0,0};
String hexstring;

int RValue;
int GValue;
int BValue;
int CurrentLevel;

#define DHT_PIN 8
float lastTemp;
float lastHum;
boolean metric = true;
MyMessage msgHum(1, V_HUM);
MyMessage msgTemp(2, V_TEMP);
unsigned long reportInterval = 5000;
unsigned long lastReport = 0;


Button myBtn(BTN_PIN, PULLUP, INVERT, DEBOUNCE_MS);

void setup() {
  // MySensors
  gw.begin(incomingMessage, AUTO, true);
  gw.sendSketchInfo("RGB LED Dimmer", "1.0");
  gw.present(0, S_RGB_LIGHT);
  gw.present(1, V_HUM);
  gw.present(2, V_TEMP);
  
  // Set pin output mode
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  //pinMode(BTN_PIN, INPUT);
  dht.setup(DHT_PIN);
}

void loop() {
  gw.process();
  myBtn.read();
  if (myBtn.wasReleased()) {       //If the button was released, change the LED state
    Serial.println("Button pressed.");
    Serial.println(gw.loadState(SAVE_LIGHT_STATE));
    if(gw.loadState(SAVE_LIGHT_STATE) == RELAY_OFF) {
      setColor("FFFFFF");
      setDimLevel(100);
      gw.saveState(SAVE_LIGHT_STATE, RELAY_ON);
    }
    else if(gw.loadState(SAVE_LIGHT_STATE) == RELAY_ON) {
      Serial.println("Turn off");
      analogWrite(RED_PIN, 0);
      analogWrite(GREEN_PIN, 0);
      analogWrite(BLUE_PIN, 0);
      gw.saveState(SAVE_LIGHT_STATE, RELAY_OFF);
    }
  }
    
  if ((unsigned long)(millis() - lastReport) >= reportInterval) {
    reportDht();
    lastReport = millis();
  }
}

void reportDht() {
  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  if(isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  }
  else if (temperature != lastTemp) {
    lastTemp = temperature;
    if(!metric) {
      temperature = dht.toFahrenheit(temperature);
    }

    gw.send(msgTemp.set(temperature, 1));
    Serial.print("Temp: ");
    Serial.print(temperature);
  }

  if(isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  }
  else if (humidity != lastHum) {
    lastHum = humidity;
    gw.send(msgHum.set(humidity, 1));
    Serial.print("Humidity: ");
    Serial.println(humidity);
  }
}

void incomingMessage(const MyMessage &message) {
  
  if (message.type==V_RGB) {
    hexstring = message.getString();
    Serial.print("RGB command: ");
    Serial.println(hexstring);
    setColor(hexstring);
  }

  else if (message.type==V_DIMMER) {
    int dimLevel = message.getInt(); //0-100%
    Serial.print("Dim command: ");
    Serial.println(dimLevel);
    setDimLevel(dimLevel);
    gw.saveState(SAVE_DIMMER_LEVEL, dimLevel);
  }
  
  else if (message.type==V_LIGHT) {
   if(message.getBool() == RELAY_ON) {
    setColor("FFFFFF");
    gw.saveState(SAVE_LIGHT_STATE, RELAY_ON);
   }
   if(message.getBool() == RELAY_OFF) {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    gw.saveState(SAVE_LIGHT_STATE, RELAY_OFF);
   }
  }
  
}

void setDimLevel(int level) {
  level = level > 100 ? 100 : level;
  level = level < 0 ? 0: level;
  
  int delta = ( level - CurrentLevel) < 0 ? -1 : 1;
  
  RValue = (int)(level / 100. * RValue);
  BValue = (int)(level / 100. * BValue);
  GValue = (int)(level / 100. * GValue);
  
  analogWrite(RED_PIN, RValue);
  analogWrite(GREEN_PIN, GValue);
  analogWrite(BLUE_PIN, BValue);
  
  CurrentLevel = level;
}

void setColor(String hexstring) {
  long number = (long) strtol( &hexstring[0], NULL, 16);
  Serial.print("Color long: ");
  Serial.println(number);
  RValue = number >> 16;
  GValue = number >> 8 & 0xFF;
  BValue = number & 0xFF;

  Serial.print("Color: ");
  Serial.println(hexstring);
  Serial.print("Red: ");
  Serial.println(RValue);
  Serial.print("Green: ");
  Serial.println(GValue);
  Serial.print("Blue: ");
  Serial.println(BValue);
  
  analogWrite(RED_PIN, RValue);
  analogWrite(GREEN_PIN, GValue);
  analogWrite(BLUE_PIN, BValue);
}
