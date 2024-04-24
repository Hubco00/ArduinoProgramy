#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Definícia pinov pre rotačný enkoder
#define CLK D6
#define DT D5
#define SW D3

// Definícia pinu pre Hall senzor
#define HALL_SENSOR_PIN D1

volatile int lastEncoded = 0;
volatile int encoderValue = 0;
String directions[] = {"North", "North-East", "East", "South-East", "South", "South-West", "West", "North-West"};

int directionIndex = 0;
float windSpeed = 0;

const char* ssid = "Hubcoo"; 
const char* password = "jozkomrkvicka"; 
const char* directionServer = "http://192.168.54.88:8081/api/data/direction";
const char* windSpeedServer = "http://192.168.54.88:8081/api/data/windSpeedData";


void rotate();
void buttonPressed();
void measureWindSpeed();
void sendPostRequest();
float calculateWindSpeed(unsigned long timeBetweenPulses);

WiFiClient client;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLK), rotate, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW), buttonPressed, FALLING);

  pinMode(HALL_SENSOR_PIN, INPUT);
}

void loop() {
  measureWindSpeed();
  sendPostRequest();
  delay(1000);
}

void sendPostRequest() {
  if (WiFi.status() == WL_CONNECTED) {
     
  

    // Second POST: Send Wind Speed
    StaticJsonDocument<200> docWindSpeed;
    docWindSpeed["type"] = 9;
    docWindSpeed["value"] = windSpeed;
    docWindSpeed["sensorName"] = "Wind Speed Sensor";
    String jsonDataWind;
    serializeJson(docWindSpeed, jsonDataWind);

 
   
    sendDirection(directionIndex);
    
    http.begin(client, windSpeedServer);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonDataWind);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

void IRAM_ATTR rotate() {
  static int lastStateCLK = digitalRead(CLK);
  int currentStateCLK = digitalRead(CLK);
  int currentStateDT = digitalRead(DT);

  if (currentStateCLK != lastStateCLK) {
      encoderValue += (currentStateDT != currentStateCLK) ? 1 : -1;
      encoderValue = (encoderValue + 40) % 40; 
  }

  lastStateCLK = currentStateCLK; 


  int currentAngle = (encoderValue * 9) % 360; 


  if (currentAngle >= 337.5 || currentAngle < 22.5) {
      directionIndex = 0; // North
  } else if (currentAngle >= 22.5 && currentAngle < 67.5) {
      directionIndex = 1; // North-East
  } else if (currentAngle >= 67.5 && currentAngle < 112.5) {
      directionIndex = 2; // East
  } else if (currentAngle >= 112.5 && currentAngle < 157.5) {
      directionIndex = 3; // South-East
  } else if (currentAngle >= 157.5 && currentAngle < 202.5) {
      directionIndex = 4; // South
  } else if (currentAngle >= 202.5 && currentAngle < 247.5) {
      directionIndex = 5; // South-West
  } else if (currentAngle >= 247.5 and currentAngle < 292.5) {
      directionIndex = 6; // West
  } else if (currentAngle >= 292.5 && currentAngle < 337.5) {
      directionIndex = 7; // North-West
  }

}

void sendDirection(int direction)
{
    Serial.print("Direction: ");
  Serial.println(directions[direction]);
  StaticJsonDocument<200> docDirection;
  docDirection["type"] = 8;
  docDirection["value"] = direction;
  docDirection["sensorName"] = "Direction Sensor";

  String jsonDataDirection;
  serializeJson(docDirection, jsonDataDirection);

  http.begin(client, directionServer);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonDataDirection);


  http.end();
}

void IRAM_ATTR buttonPressed() {
  encoderValue = 0;
  directionIndex = 0; 
  Serial.println("Reset to Sever");
}

void measureWindSpeed() {
  static int lastState = LOW;
  int currentState = digitalRead(HALL_SENSOR_PIN);
  Serial.println(currentState);
  static unsigned long lastHallPulseTime = 0;
  
  if (currentState != lastState) {
    lastState = currentState;
    if (currentState == HIGH) {
      unsigned long currentTime = millis();
      unsigned long timeBetweenPulses = currentTime - lastHallPulseTime;
      lastHallPulseTime = currentTime;
      if (timeBetweenPulses > 0) {
        float windSpeed = calculateWindSpeed(timeBetweenPulses);
        Serial.print("Rýchlosť vetra: ");
        Serial.print(windSpeed);
        Serial.println(" m/s");
      }
    }
  }
}

float calculateWindSpeed(unsigned long timeBetweenPulses) {

  float circumference = 0.2; 
  return (circumference / timeBetweenPulses) * 1000;
}
