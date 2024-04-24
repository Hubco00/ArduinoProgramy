#include <ESP8266HTTPClient.h>

#include <ESP8266WiFi.h>

#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>

// Nastavenia WiFi
const char* ssid = "Hubcoo";
const char* password = "jozkomrkvicka";

// Nastavenie pinov pre I2C
#define SCL_PIN D1
#define SDA_PIN D2

// Nastavenie pinu pre YL-83 dažďový senzor
#define RAIN_PIN A0

// Inicializácia senzorov

Adafruit_BMP280 bmp;
WiFiClient client;

void setup() {
    Serial.begin(115200);

   
     WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
    if(!bmp.begin(0x76))
    {
      Serial.println("Failed to initialize sensors!");
        while (1) delay(10);
    }
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop() {


    float pressure = bmp.readPressure() / 100.0; // Pressure in hPa
    float temperatureBMP = bmp.readTemperature();
    float rainValue = analogRead(RAIN_PIN);
    Serial.print("Pressure (hPa): ");
    Serial.println(pressure);

    Serial.print("Temperature (C): ");
    Serial.println(temperatureBMP);

    Serial.print("Rain Value: ");
    Serial.println(rainValue);
    StaticJsonDocument<300> doc;
    JsonArray sensors = doc.createNestedArray("sensors");

    if(rainValue <= 600)
    {

      JsonObject sensorRain = sensors.createNestedObject();
      sensorRain["type"] = 7;
      sensorRain["data"] = rainValue;
    }
    HTTPClient http;
    http.begin(client, "http://192.168.54.88:8081/api/data/addData");
    http.addHeader("Content-Type", "application/json");

  

    JsonObject sensorBMP = sensors.createNestedObject();
    sensorBMP["type"] = 3;
    sensorBMP["data"] = pressure;

   

    String postData;
    serializeJson(doc, postData);
    int httpResponseCode = http.POST(postData);
    String response = http.getString();
    Serial.print("HTTP Response Body: ");
    Serial.println(response);
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    http.end();

    delay(5000); // Data sending interval
}
