#include "PMW3389/src/PMW3389.h"
// This is a hack to workaround the seeming inability to add this file
// to the build.
#include "PMW3389/src/PMW3389.cpp"

#define SS 28

PMW3389 sensor;

void setup() {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  delay(100);

  Serial.begin(9600);
  while (!Serial) {}

  if (sensor.begin(SS, 16000)) // 10 is the pin connected to SS of the module.
    Serial.println("Sensor initialization successed");
  else
    Serial.println("Sensor initialization failed");
}

void loop() {
  PMW3389_DATA data = sensor.readBurst();

  if (data.isOnSurface && data.isMotion) {
    Serial.print(data.dx);
    Serial.print("\t");
    Serial.print(data.dy);
    Serial.println();
  }

  delay(10);
}
