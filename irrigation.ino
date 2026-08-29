#include <DHT.h>

#define SOIL_PIN 33
#define POT_PIN 35
#define DHT_PIN 4
#define MQ2_PIN 32
#define RELAY_PIN 26

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

const int SOIL_DRY_RAW = 4095;
const int SOIL_WET_RAW = 1500;
const int SOIL_TURN_OFF_RAW = 2000; // turn pump OFF when soil_raw < this (wet enough)

int SOIL_TURN_ON_RAW = 3000; // turn pump ON when soil_raw > this (too dry)

const int SOIL_TURN_ON_MIN = 2400; // lowest the user can dial the turn-on threshold to in potentiometer
const int SOIL_TURN_ON_MAX = 4000; // highest the user can dial the turn-on threshold to in potentiometer


// Field condition limits
const float TEMP_MIN = 15.0;
const float TEMP_MAX = 35.0;
const float HUM_MIN = 30.0;
const float HUM_MAX = 85.0;
const int SOIL_MIN_PERCENT = 20;
const int GAS_SAFE_LIMIT = 700;

// Timing values
const unsigned long STARTUP_DELAY_MS = 30000;
const unsigned long NORMAL_INTERVAL_MS = 10000; // 10 seconds when pump OFF
const unsigned long PUMP_INTERVAL_MS = 1000; // 1 second when pump ON (continuous)

// System state variables
bool pumpOn = false;
unsigned long lastCheck = 0;
int wateringCount = 0;

int soilRaw = 0;
int soilPercent = 0;
float temperature = 0;
float humidity = 0;
int gasLevel = 0;
int potRaw = 0; // 

// Convert soil reading to percentage
int soilToPercent(int raw) {
  int pct = map(raw, SOIL_DRY_RAW, SOIL_WET_RAW, 0, 100);
  return constrain(pct, 0, 100);
}

// Read soil moisture
void readSoil() {
  soilRaw = analogRead(SOIL_PIN);
  soilPercent = soilToPercent(soilRaw);
}

// Read all sensors (Stage 2 only — soil, potentiometer, temp, humidity, gas)
void readAllSensors() {
  readSoil();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  gasLevel = analogRead(MQ2_PIN);
  potRaw = analogRead(POT_PIN); 
  SOIL_TURN_ON_RAW = map(potRaw, 0, 4095, SOIL_TURN_ON_MIN, SOIL_TURN_ON_MAX);
  SOIL_TURN_ON_RAW = constrain(SOIL_TURN_ON_RAW, SOIL_TURN_ON_MIN, SOIL_TURN_ON_MAX);
}

// Check field conditions
bool checkSuitability(String &reason) {
  if (isnan(temperature) || isnan(humidity)) {
    reason = "DHT11 SENSOR ERROR";
    return false;
  }
  if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
    reason = "TEMPERATURE OUT OF RANGE";
    return false;
  }
  if (humidity < HUM_MIN || humidity > HUM_MAX) {
    reason = "HUMIDITY OUT OF RANGE";
    return false;
  }
  if (soilPercent < SOIL_MIN_PERCENT) {
    reason = "SOIL TOO DRY FOR SAFE CROPPING";
    return false;
  }
  if (gasLevel >= GAS_SAFE_LIMIT) {
    reason = "UNSAFE GAS LEVEL DETECTED";
    return false;
  }
  reason = "CONDITIONS OPTIMAL";
  return true;
}

// Control the pump
void pumpControl(bool state) {
  pumpOn = state;
  digitalWrite(RELAY_PIN, state ? LOW : HIGH);
}

// Display system status
void printStatus(bool apt, String reason) {
  Serial.println();
  Serial.print(F("FIELD STATUS: "));
  Serial.println(apt ? F("APT FOR CROPPING") : F("NOT APT FOR CROPPING"));
  Serial.print(F("Reason: "));
  Serial.println(reason);
  Serial.println(F("SENSOR READINGS:"));
  Serial.print(F(" Temperature: ")); Serial.print(temperature); 
  Serial.println(F(" C"));  
  Serial.print(F(" Humidity: ")); Serial.print(humidity);
  Serial.println(F(" %"));
  Serial.print(F(" Soil Moisture: Raw ")); Serial.print(soilRaw);
  Serial.print(F(" (")); Serial.print(soilPercent); 
  Serial.println(F("%)"));
  Serial.print(F(" Potentiometer: Raw ")); Serial.println(potRaw);  
  Serial.print(F(" Gas Level: ")); Serial.print(gasLevel);
  Serial.println(gasLevel < GAS_SAFE_LIMIT ? F(" SAFE") : F(" UNSAFE"));
  Serial.println(F("PUMP STATUS:"));
  Serial.print(F(" State: ")); Serial.println(pumpOn ? F("ON") : F("OFF"));
  Serial.print(F(" Turn ON threshold: raw > "));
  Serial.print(SOIL_TURN_ON_RAW);
  Serial.println(F(" (live, set by potentiometer)")); // >>> CHANGED: threshold is no longer fixed, now pot-driven
  Serial.print(F(" Turn OFF threshold: raw < "));
  Serial.println(SOIL_TURN_OFF_RAW);
  Serial.print(F(" Total waterings: ")); 
  Serial.println(wateringCount);
}

// Setup sensors and relay
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  dht.begin();

  Serial.println(F("Smart Irrigation System starting..."));
  Serial.println(F("Sensors warming up. Pump locked OFF for 30 seconds."));

  unsigned long startTime = millis();
  while (millis() - startTime < STARTUP_DELAY_MS) {
    int secondsLeft = (STARTUP_DELAY_MS - (millis() - startTime)) / 1000;
    Serial.print(F("Starting in: "));
    Serial.print(secondsLeft);
    Serial.println(F("s"));
    delay(1000);
  }

  Serial.println(F("SYSTEM READY"));
  lastCheck = millis();
}


// Main system loop
void loop() {
  unsigned long now = millis();

  if (!pumpOn) {
    // Check conditions every 10 seconds
    if (now - lastCheck >= NORMAL_INTERVAL_MS || lastCheck == 0) {
      readAllSensors();

      String reason;
      bool apt = checkSuitability(reason);
      printStatus(apt, reason);

      if (apt && soilRaw > SOIL_TURN_ON_RAW) {
        Serial.println(F(">>> PUMP ON - Soil too dry!"));
        pumpControl(true);
      }

      lastCheck = now;
    }
  } else {
    // Check soil every 1 second while pumping
    static unsigned long lastPumpCheck = 0;
    if (now - lastPumpCheck >= PUMP_INTERVAL_MS) {
      readSoil();
      Serial.print(F("Pumping... Soil: Raw "));
      Serial.print(soilRaw);
      Serial.print(F(" ("));
      Serial.print(soilPercent);
      Serial.println(F("%)"));

      if (soilRaw <= SOIL_TURN_OFF_RAW) {
        Serial.println(F("===================================================="));
        Serial.println(F(">>> PUMP OFF - Soil wet enough!"));
        Serial.print(F(" Soil raw: ")); Serial.print(soilRaw);
        Serial.print(F(" (threshold: ")); Serial.print(SOIL_TURN_OFF_RAW); Serial.println(F(")"));
        Serial.println(F("===================================================="));

        pumpControl(false);
        wateringCount++;
        lastCheck = now;
      }
      lastPumpCheck = now;
    }
  }
}
