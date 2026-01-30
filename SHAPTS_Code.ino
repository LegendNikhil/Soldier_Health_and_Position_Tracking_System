/*
-------------------------------------------------
 Soldier Health Monitoring and Position Tracking
 Arduino UNO Implementation
-------------------------------------------------
 Features:
 - GPS location tracking
 - GSM SMS alerts
 - Body temperature (LM35)
 - Heartbeat monitoring (LDR-based)
 - Emergency push button
 - 16x2 I2C LCD display
-------------------------------------------------
*/

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* -------- Pin Definitions -------- */
#define GSM_RX 2
#define GSM_TX 3
#define GPS_RX 4
#define GPS_TX 5

#define LM35_PIN A0
#define HEARTBEAT_PIN A1
#define EMERGENCY_BTN 8

/* -------- Objects -------- */
SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

LiquidCrystal_I2C lcd(0x27, 16, 2);

/* -------- Variables -------- */
float temperatureC = 0.0;
int heartbeatValue = 0;
bool emergencyTriggered = false;

/* -------- Setup -------- */
void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
  gpsSerial.begin(9600);

  pinMode(EMERGENCY_BTN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Soldier Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);

  initializeGSM();
}

/* -------- Main Loop -------- */
void loop() {

  readGPS();
  readSensors();
  displayLCD();

  if (digitalRead(EMERGENCY_BTN) == LOW && !emergencyTriggered) {
    emergencyTriggered = true;
    sendEmergencySMS();
  }
}

/* -------- GSM Initialization -------- */
void initializeGSM() {
  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CMGF=1");  // SMS text mode
  delay(1000);
}

/* -------- Read Sensors -------- */
void readSensors() {

  // LM35 Temperature
  int tempRaw = analogRead(LM35_PIN);
  temperatureC = (tempRaw * 5.0 * 100.0) / 1024.0;

  // Heartbeat (LDR-based)
  heartbeatValue = analogRead(HEARTBEAT_PIN);
}

/* -------- Read GPS -------- */
void readGPS() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
}

/* -------- LCD Display -------- */
void displayLCD() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(temperatureC);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Heart:");
  lcd.print(heartbeatValue);

  delay(1000);
}

/* -------- Send Emergency SMS -------- */
void sendEmergencySMS() {

  if (!gps.location.isValid()) {
    Serial.println("GPS not locked");
    return;
  }

  double lat = gps.location.lat();
  double lng = gps.location.lng();

  gsmSerial.println("AT+CMGS=\"+91XXXXXXXXXX\""); // Replace number
  delay(1000);

  gsmSerial.print("EMERGENCY ALERT!\n");
  gsmSerial.print("Temp: ");
  gsmSerial.print(temperatureC);
  gsmSerial.print(" C\n");

  gsmSerial.print("Heart: ");
  gsmSerial.print(heartbeatValue);
  gsmSerial.print("\n");

  gsmSerial.print("Location:\n");
  gsmSerial.print("https://maps.google.com/?q=");
  gsmSerial.print(lat, 6);
  gsmSerial.print(",");
  gsmSerial.print(lng, 6);

  delay(1000);
  gsmSerial.write(26); // CTRL+Z

  Serial.println("Emergency SMS Sent");
}
