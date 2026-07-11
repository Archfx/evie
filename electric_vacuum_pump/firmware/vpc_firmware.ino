// ATtiny412 (megaTinyCore)
// PA7 = analog input (vacuum sensor)
// PA6 = LED indicator
// PA3 = relay control (HIGH = pump ON)

const uint8_t LED_PIN    = PIN_PA6;
const uint8_t RELAY_PIN  = PIN_PA3;
const uint8_t SENSOR_PIN = PIN_PA7;

// Hysteresis thresholds
const int START_ADC = (int)(3.1f / 5.0f * 1023.0f + 0.5f);
const int STOP_ADC  = (int)(3.5f / 5.0f * 1023.0f + 0.5f);

// Timing protections
const uint32_t MIN_ON_TIME_MS  = 3000;
const uint32_t MIN_OFF_TIME_MS = 1000;

// Sensor fault limits
const int SENSOR_MIN = 10;
const int SENSOR_MAX = 1013;

enum PumpState {
  PUMP_OFF,
  PUMP_ON,
  FAULT
};

PumpState state = PUMP_OFF;

uint32_t stateChangeTime = 0;

int readFilteredSensor() {

  const int samples = 8;
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
  }

  return sum / samples;
}

void setPump(bool on) {

  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  digitalWrite(LED_PIN, on ? HIGH : LOW);

  stateChangeTime = millis();
}

void startupBlink() {

  digitalWrite(LED_PIN, HIGH);
  delay(80);
  digitalWrite(LED_PIN, LOW);
  delay(80);

  digitalWrite(LED_PIN, HIGH);
  delay(180);
  digitalWrite(LED_PIN, LOW);
}

void setup() {

  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  startupBlink();

  setPump(false);

  delay(2000); // allow sensor to stabilize
}

void loop() {

  int sensor = readFilteredSensor();
  uint32_t now = millis();

  // Detect sensor fault
  if (sensor < SENSOR_MIN || sensor > SENSOR_MAX) {
    state = FAULT;
  }

  switch (state) {

    case PUMP_OFF:

      if (sensor < START_ADC) {

        setPump(true);
        state = PUMP_ON;
      }

      break;

    case PUMP_ON:

      // If vacuum drops again, reset timer
      if (sensor < START_ADC) {
        stateChangeTime = now;
      }

      // Only stop if condition is continuously satisfied
      if (sensor > STOP_ADC &&
          (now - stateChangeTime) > MIN_ON_TIME_MS) {

        setPump(false);
        state = PUMP_OFF;
      }

      break;

    case FAULT:

      // Fail-safe: run pump continuously
      setPump(true);

      break;
  }

  delay(10);
}