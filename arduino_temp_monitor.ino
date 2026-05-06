#include <DHT.h>

const uint8_t dhtPin = 2;
const uint8_t dhtType = DHT22;
const unsigned long sampleDelayMs = 2000;

DHT dht(dhtPin, dhtType);
unsigned long lastSampleMs = 0;

void setup()
{
  Serial.begin(9600);
  dht.begin();
}

void loop()
{
  const unsigned long now = millis();
  if (now - lastSampleMs < sampleDelayMs) {
    return;
  }

  lastSampleMs = now;

  const float temperatureC = dht.readTemperature();
  if (isnan(temperatureC)) {
    Serial.println("ERR");
    return;
  }

  Serial.println(temperatureC, 1);
}