#include <TinyGPS++.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>

#include <WiFi.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "fir-max30100-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "6hppp9iHvu6aQP3zoZeBNVgyKwzO02IxbhAg5IJz"

#define WIFI_SSID "hoan"
#define WIFI_PASSWORD "12345678"

FirebaseData fbdo;
FirebaseJsonData json;
FirebaseConfig config;
FirebaseAuth auth;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
PulseOximeter pox;
TinyGPSPlus gps;

#define ENABLE_MAX30100 1
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  //64 // OLED display height, in pixels

#define OLED_RESET -1        // 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define REPORTING_PERIOD_MS 3000
#define REPORTING_PERIOD_TE 3000


uint32_t tsLastReport = 0;
uint32_t teLastReport = 0;

int xPos = 0;
// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
  Serial.println("Beat!");
  heart_beat(&xPos);
  delay(20);
  heart_beat(&xPos);
  delay(20);
  // heart_beat_1(&xPos);
}
void setup() {
  Serial.begin(115200);
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  // }
  Serial.println("Wifi connected !!!!!!");
  Serial.println("SSD1306 128x64 OLED TEST");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 18);
  // Display static text
  display.print("Pulse OxiMeter");
  int temp1 = 0;
  int temp2 = 40;
  int temp3 = 80;
  heart_beat(&temp1);
  heart_beat(&temp2);
  heart_beat(&temp3);
  xPos = 0;
  display.display();
  delay(2000);  // Pause for 2 seconds
  display.cp437(true);
  display.clearDisplay();
  Serial.print("Initializing pulse oximeter..");
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1)
      ;
  };

  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }
#if ENABLE_MAX30100


  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
  display_data(0, 0);
#endif
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}
void loop() {


  drawLine(&xPos);
  
  // Make sure to call update as fast as possible
  pox.update();

  // Khai báo biến để lưu giữ giá trị trước đó
  int lastBPM = -1;
  int lastSpo2 = -1;
  int lastTempO = -1;
  int lastTempA = -1;



  if (millis() - tsLastReport >= REPORTING_PERIOD_MS) {
    pox.shutdown();
    int tempO = mlx.readObjectTempC();
    int tempA = mlx.readAmbientTempC();
    int bpm = pox.getHeartRate();
    int spo2 = pox.getSpO2();
    display_data(bpm, tempO);

    // Kiểm tra và gửi dữ liệu khi có sự thay đổi
    if (bpm != lastBPM) {
      
      if (!Firebase.setIntAsync(fbdo, "/HeartBeat", bpm)) {
        Serial.println(fbdo.errorReason());
      }
      lastBPM = bpm;
    }

    if (spo2 != lastSpo2) {
      
      if (!Firebase.setIntAsync(fbdo, "/SPO2", spo2)) {
        Serial.println(fbdo.errorReason());
      }
      lastSpo2 = spo2;
    }

    if (tempO != lastTempO) {
      if (!Firebase.setIntAsync(fbdo, "/ObjectTempC", tempO)) {
        Serial.println(fbdo.errorReason());
      }
      lastTempO = tempO;
    }

    if (tempA != lastTempA) {
      if (!Firebase.setIntAsync(fbdo, "/Temperature", tempA)) {
        Serial.println(fbdo.errorReason());
      }
      lastTempA = tempA;
    }

    pox.resume();
    tsLastReport = millis();
  }
}
void display_data(int bpm, int tempO) {
  display.fillRect(0, 18, 127, 15, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 18);
  // Display static text
  display.print("BPM ");
  display.print(bpm);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(64, 18);
  // Display static text
  display.print("  Temp ");
  display.println(tempO);
  display.display();
}
void drawLine(int *x_pos) {
  // Draw a single pixel in white
  display.drawPixel(*x_pos, 8, SSD1306_WHITE);
  display.drawPixel((*x_pos)++, 8, SSD1306_WHITE);
  display.drawPixel((*x_pos)++, 8, SSD1306_WHITE);
  // display.drawPixel((*x_pos)++, 8, SSD1306_WHITE);
  // display.drawPixel((*x_pos), 8, BLACK); // -----
  //Serial.println(*x_pos);
  display.fillRect(*x_pos, 0, 31, 16, SSD1306_BLACK);
  display.display();
  delay(1);
  if (*x_pos >= SCREEN_WIDTH - 20) {
    *x_pos = 0;
  }
  delay(10);
}
void heart_beat(int *x_pos) {
  /************************************************/
  //display.clearDisplay();
  display.fillRect(*x_pos, 0, 30, 15, SSD1306_BLACK);
  // Draw a single pixel in white
  display.drawPixel(*x_pos + 0, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 1, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 2, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 3, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 4, 8, BLACK);  // -----
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 5, 7, SSD1306_WHITE);
  display.drawPixel(*x_pos + 6, 6, SSD1306_WHITE);
  display.drawPixel(*x_pos + 7, 7, SSD1306_WHITE);  // .~.
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 8, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 9, 8, SSD1306_WHITE);  // --
  //display.display();
  //delay(1);
  /******************************************/
  display.drawPixel(*x_pos + 10, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 10, 9, SSD1306_WHITE);
  display.drawPixel(*x_pos + 11, 10, SSD1306_WHITE);
  display.drawPixel(*x_pos + 11, 11, SSD1306_WHITE);
  //display.display();
  //delay(1);
  /******************************************/
  display.drawPixel(*x_pos + 12, 10, SSD1306_WHITE);
  display.drawPixel(*x_pos + 12, 9, SSD1306_WHITE);
  display.drawPixel(*x_pos + 12, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 12, 7, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 13, 6, SSD1306_WHITE);
  display.drawPixel(*x_pos + 13, 5, SSD1306_WHITE);
  display.drawPixel(*x_pos + 13, 4, SSD1306_WHITE);
  display.drawPixel(*x_pos + 13, 3, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 14, 2, SSD1306_WHITE);
  display.drawPixel(*x_pos + 14, 1, SSD1306_WHITE);
  display.drawPixel(*x_pos + 14, 0, SSD1306_WHITE);
  display.drawPixel(*x_pos + 14, 0, SSD1306_WHITE);
  //display.display();
  //delay(1);
  /******************************************/
  display.drawPixel(*x_pos + 15, 0, SSD1306_WHITE);
  display.drawPixel(*x_pos + 15, 1, SSD1306_WHITE);
  display.drawPixel(*x_pos + 15, 2, SSD1306_WHITE);
  display.drawPixel(*x_pos + 15, 3, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 15, 4, SSD1306_WHITE);
  display.drawPixel(*x_pos + 15, 5, SSD1306_WHITE);
  display.drawPixel(*x_pos + 16, 6, SSD1306_WHITE);
  display.drawPixel(*x_pos + 16, 7, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 16, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 16, 9, SSD1306_WHITE);
  display.drawPixel(*x_pos + 16, 10, SSD1306_WHITE);
  display.drawPixel(*x_pos + 16, 11, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 17, 12, SSD1306_WHITE);
  display.drawPixel(*x_pos + 17, 13, SSD1306_WHITE);
  display.drawPixel(*x_pos + 17, 14, SSD1306_WHITE);
  display.drawPixel(*x_pos + 17, 15, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 18, 15, SSD1306_WHITE);
  display.drawPixel(*x_pos + 18, 14, SSD1306_WHITE);
  display.drawPixel(*x_pos + 18, 13, SSD1306_WHITE);
  display.drawPixel(*x_pos + 18, 12, SSD1306_WHITE);
  //display.display();
  //delay(1);
  display.drawPixel(*x_pos + 19, 11, SSD1306_WHITE);
  display.drawPixel(*x_pos + 19, 10, SSD1306_WHITE);
  display.drawPixel(*x_pos + 19, 9, SSD1306_WHITE);
  display.drawPixel(*x_pos + 19, 8, SSD1306_WHITE);
  //display.display();
  //delay(1);
  /****************************************************/
  display.drawPixel(*x_pos + 20, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 21, 8, SSD1306_WHITE);
  //display.display();
  //delay(1);
  /****************************************************/
  display.drawPixel(*x_pos + 22, 7, SSD1306_WHITE);
  display.drawPixel(*x_pos + 23, 6, SSD1306_WHITE);
  display.drawPixel(*x_pos + 24, 6, SSD1306_WHITE);
  display.drawPixel(*x_pos + 25, 7, SSD1306_WHITE);
  //display.display();
  //delay(1);
  /************************************************/
  display.drawPixel(*x_pos + 26, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 27, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 28, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 29, 8, SSD1306_WHITE);
  display.drawPixel(*x_pos + 30, 8, SSD1306_WHITE);  // -----
  *x_pos = *x_pos + 20;
  display.display();
  delay(1);
}
