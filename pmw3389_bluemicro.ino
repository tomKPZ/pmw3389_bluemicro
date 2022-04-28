#include <bluefruit.h>

#include "PMW3389/src/PMW3389.h"
// This is a hack to workaround the seeming inability to add this file
// to the build.
#include "PMW3389/src/PMW3389.cpp"

BLEDis bledis;
BLEHidAdafruit blehid;

PMW3389 sensor1;
PMW3389 sensor2;

void setup() {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  pinMode(28, OUTPUT);
  digitalWrite(28, HIGH);

  Serial.begin(115200);

  init_bluetooth();

  sensor1.begin(3, 16000);
  sensor2.begin(28, 16000);
}

void init_bluetooth() {
  Bluefruit.begin();
  // HID Device can have a min connection interval of 9*1.25 = 11.25 ms
  Bluefruit.Periph.setConnInterval(
      9, 16);              // min = 9*1.25=11.25 ms, max = 16*1.25=20ms
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  // BLE HID
  blehid.begin();

  // Set up and start advertising
  startAdv();
}

void startAdv() {
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);

  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for 'Name' in the advertising packet
  Bluefruit.Advertising.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
  Bluefruit.Advertising.start(0); // 0 = Don't stop advertising after n seconds
}

void loop() {
  PMW3389_DATA data1 = sensor1.readBurst();
  PMW3389_DATA data2 = sensor2.readBurst();

  if (data1.isOnSurface && data2.isOnSurface &&
      (data1.isMotion || data2.isMotion)) {
    Serial.print(data1.dx);
    Serial.print("\t");
    Serial.print(data2.dy);
    Serial.print("\t");
    Serial.print(data1.dx);
    Serial.print("\t");
    Serial.print(data2.dy);
    Serial.println();

    blehid.mouseMove(data1.dx + data2.dx, data1.dy + data2.dy);
  }

  delay(10);
}
