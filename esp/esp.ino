
#include "esp_camera.h"
#include "ESPAsyncWebServer.h"
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Nastavenia WiFi
const char* ssid = "Hubcoo";
const char* password = "jozkomrkvicka";


#define DHTPIN 14
#define DHTTYPE DHT11
#define MQ9_PIN 15

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(8082);
WiFiClient client;
bool cameraInit() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = 5;
    config.pin_d1 = 18;
    config.pin_d2 = 19;
    config.pin_d3 = 21;
    config.pin_d4 = 36;
    config.pin_d5 = 39;
    config.pin_d6 = 34;
    config.pin_d7 = 35;
    config.pin_xclk = 0;
    config.pin_pclk = 22;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_sscb_sda = 26;
    config.pin_sscb_scl = 27;
    config.pin_pwdn = 32;
    config.pin_reset = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    return esp_camera_init(&config) == ESP_OK;
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(MQ9_PIN, INPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());
    Serial.println("WiFi connected");

     if (!cameraInit()) {
        Serial.println("Camera init failed!");
        return;
    }

    server.on("/api/data/savePicture", HTTP_POST, [](AsyncWebServerRequest *request) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            request->send(500, "text/plain", "Failed to capture image");
            return;
        }
        
     
        request->send_P(200, "image/jpeg", fb->buf, fb->len);
        esp_camera_fb_return(fb);
    });

    server.begin();
    Serial.println("Server started");
}

void loop() {
    static unsigned long lastDHTReadTime = 0;
    if (millis() - lastDHTReadTime > 30000) { // Measure every hour
    lastDHTReadTime = millis();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    Serial.print("Vlhkost = ");
    Serial.print(humidity);
    Serial.print(" Teplota = ");
    Serial.println(temperature);
    if (!isnan(humidity) && !isnan(temperature)) {

      StaticJsonDocument<200> doc;
        JsonArray sensors = doc.createNestedArray("sensors");
        JsonObject sensor1 = sensors.createNestedObject();
        sensor1["type"] = 1;
        sensor1["data"] = temperature;
        JsonObject sensor2 = sensors.createNestedObject();
        sensor2["type"] = 2;
        sensor2["data"] = humidity;

        String postData;
        serializeJson(doc, postData);

        Serial.print("POST Data: ");
        Serial.println(postData); // Debug line to print the POST data
        
        HTTPClient http;
        http.begin(client, "http://192.168.54.88:8081/api/data/addData"); 

        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(postData);
        String response = http.getString();
        Serial.print("HTTP Response Body: ");
        Serial.println(response);
        Serial.print("HTTP Response Code: ");
        Serial.println(httpResponseCode); // Debug line to print the HTTP response code
        http.end();
    } else {
        Serial.println("Error: Invalid humidity or temperature reading");
    }
    }

    int mq9Value = digitalRead(MQ9_PIN);
    if (mq9Value == HIGH) { // Detect smoke
        Serial.print("Smoke = ");
        Serial.println(mq9Value);
        StaticJsonDocument<200> doc;
        doc["type"] = 4;
        doc["value"] = "Smoke detected";
        doc["sensorName"] = "Smoke Sensor";

        String postData;
        serializeJson(doc, postData);

     
        HTTPClient http;
        http.begin(client, "http://192.168.54.88:8081/api/data/smokeDetected"); 
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(postData);
        http.end();
    }
}
