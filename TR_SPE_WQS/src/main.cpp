  #include <Arduino.h>
  #include <WiFi.h>
  #include <Wire.h>
  #include <Adafruit_SSD1306.h>
  #include <Firebase_ESP_Client.h>
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #include "Adafruit_ADS1015.h"
  #include <EEPROM.h>


  // Replace these with the actual libraries for your TDS sensor and DS18B20
  // Make sure to install the necessary libraries using the Arduino Library Manager.
  // For example, for DS18B20: https://github.com/milesburton/Arduino-Temperature-Control-Library
  #include <DFRobot_ESP_EC.h>
  #include "DallasTemperature.h"

  // Provide the token generation process info.
  #include "addons/TokenHelper.h"
  // Provide the RTDB payload printing info and other helper functions.
  #include "addons/RTDBHelper.h"

  // Insert your network credentials
  #define WIFI_SSID "Mtthoe"
  #define WIFI_PASSWORD "password"

  // Insert Firebase project API Key
  #define API_KEY "AIzaSyBI7gMtmCqj6owIiOOyap1bZOyLEbzuEAw"

  // Insert RTDB URLefine the RTDB URL
  #define DATABASE_URL "https://trspe-bec09-default-rtdb.asia-southeast1.firebasedatabase.app/"

  // Define Firebase Data object
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  unsigned long sendDataPrevMillis = 0;
  int count = 150;
  bool signupOK = false;
  bool uploadFlag = false;

  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  #define OLED_RESET -1
  #define ONE_WIRE_BUS 14
  #define TDS_SENSOR_PIN 33

  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  const char *ssid = WIFI_SSID;        // Enter your SSID here
  const char *password = WIFI_PASSWORD; // Enter your Password here

  #define REPORTING_PERIOD_MS 5000  
  
float voltage, ecValue, temperature = 25;

  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);

  DFRobot_ESP_EC ec;
  Adafruit_ADS1015 ads;

  void sendToFirebase() {
  if (Firebase.RTDB.setFloat(&fbdo, "TDS", ecValue)) {
    Serial.println("TDS data sent to Firebase");
  } else {
    Serial.print("Failed to send TDS data to Firebase. Reason: ");
    Serial.println(String(fbdo.errorReason()));
  }

  if (Firebase.RTDB.setFloat(&fbdo, "TEMP", temperature)) {
    Serial.println("TEMP data sent to Firebase");
  } else {
    Serial.print("Failed to send TEMP data to Firebase. Reason: ");
    Serial.println(String(fbdo.errorReason()));
  }

  uploadFlag = false;
}

  void sendFirebaseFlag()
  {
    uploadFlag = true;
  }

  void setup()
  {
    pinMode(5, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(5), sendFirebaseFlag, FALLING);
    Serial.begin(115200);
    pinMode(19, OUTPUT);
    delay(100);
    EEPROM.begin(32); // needed EEPROM.begin to store calibration k in eeprom

    sensors.begin();
    ec.begin();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(250);
    }

    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
      Serial.println(F("SSD1306 allocation failed!"));
      while (true)
        ;
    }

    Serial.print("Initializing device..");

  //   uint8_t address[8];
  //   if (!oneWire.search(address)) {
  //   Serial.println("No devices found on the OneWire bus!");
  //   while (1); // or use a suitable method to handle this situation
  // } else {
  //   Serial.println("OneWire device found!");
  //     // Modify this part based on the initialization of your TDS sensor
  //   }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("WQM Device"));
    display.display();

    delay(2000);

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
  }

 void loop() {
  voltage = analogRead(A0); // A0 is the GPIO 36
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);  
  ecValue = ec.readEC(voltage, temperature);

  if (uploadFlag) {
    sendToFirebase();
  }

  if (millis() - sendDataPrevMillis > REPORTING_PERIOD_MS) {
    Serial.print("TDS: ");
    Serial.println(ecValue);
    Serial.print("TEMP: ");
    Serial.println(temperature);
    Serial.println("***********************************");
    Serial.println();
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(F("TDS: "));
    display.println(ecValue);
    display.print(F("TEMP: "));
    display.println(temperature);
    display.display();

    sendDataPrevMillis = millis();
  }
}