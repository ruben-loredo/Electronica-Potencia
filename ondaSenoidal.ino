/*
 * ESP32 DevKit V1 – PWM senoidal en GPIO25 (compatible con core 3.x y 2.x)
 * - Portadora PWM: 20 kHz (ajustable)
 * - Resolución PWM: 10 bits (0..1023)
 * - Señal objetivo: 60 Hz (ajustable)
 * - duty = 50% + 50% * sin(θ)  =>  0% (mín) … 50% (cero) … 100% (máx)
 */

#include <Arduino.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ----------------------------- Parámetros PWM
constexpr int   PWM_PIN      = 25;      // GPIO25
constexpr int   PWM_CHANNEL  = 0;       // Solo usado en core 2.x
constexpr int   PWM_FREQ_HZ  = 20000;   // Portadora PWM (20 kHz)
constexpr int   PWM_RES_BITS = 10;      // Resolución (10 bits => 0..1023)
constexpr int   PWM_MAX_DUTY = (1 << PWM_RES_BITS) - 1;

// ----------------------------- Señal seno
constexpr uint16_t SINE_POINTS   = 256; // puntos por ciclo
constexpr double   SINE_FREQ_HZ  = 60.0;

hw_timer_t* s_timer   = nullptr;
volatile uint16_t s_index = 0;
uint16_t s_sineTable[SINE_POINTS];

// ISR: actualiza el duty en cada muestra
void IRAM_ATTR onTimer() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x: escribir por pin
  ledcWrite(PWM_PIN, s_sineTable[s_index]);
#else
  // Core 2.x: escribir por canal
  ledcWrite(PWM_CHANNEL, s_sineTable[s_index]);
#endif

  s_index++;
  if (s_index >= SINE_POINTS) s_index = 0;
}

void buildSineTable() {
  for (uint16_t n = 0; n < SINE_POINTS; ++n) {
    const double theta  = (2.0 * M_PI * n) / static_cast<double>(SINE_POINTS);
    const double unit   = (sin(theta) + 1.0) * 0.5; // [0..1], 0.5 en cero
    const uint16_t duty = static_cast<uint16_t>(round(unit * PWM_MAX_DUTY));
    s_sineTable[n] = duty;
  }
}

void setupPWM() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x: API por pin
  ledcAttach(PWM_PIN, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcWrite(PWM_PIN, PWM_MAX_DUTY / 2);   // arranca a 50%
#else
  // Core 2.x: API por canal
  ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, PWM_MAX_DUTY / 2);
#endif
}

void setupTimer() {
  // Frecuencia de muestreo = puntos * f_sine (p.ej., 256*60 = 15360 Hz)
  const double sampleRateHz = static_cast<double>(SINE_POINTS) * SINE_FREQ_HZ;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x
  s_timer = timerBegin((uint32_t)round(sampleRateHz)); // base frequency en Hz
  timerAttachInterrupt(s_timer, &onTimer);
  // Firma 3.x: timerAlarm(timer, alarm_value, autoreload, reload_count)
  // Ponemos alarm_value=1 tick a la base 'sampleRateHz' y autoreload.
  timerAlarm(s_timer, 1, true, 0);
  timerStart(s_timer);
#else
  // Core 2.x
  const int   TIMER_ID      = 0;
  const int   TIMER_DIVIDER = 80; // 80 MHz / 80 = 1 MHz
  const uint32_t alarm_us   = (uint32_t)round(1e6 / sampleRateHz);

  s_timer = timerBegin(TIMER_ID, TIMER_DIVIDER, true);   // count up a 1 MHz
  timerAttachInterrupt(s_timer, &onTimer, true);         // edge
  timerAlarmWrite(s_timer, alarm_us, true);              // autoreload
  timerAlarmEnable(s_timer);
#endif
}

void setup() {
  buildSineTable();
  setupPWM();
  setupTimer();

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("== PWM senoidal en GPIO25 (ESP32) =="));
  Serial.printf("PWM: %d Hz, %d bits (max duty = %d)\n", PWM_FREQ_HZ, PWM_RES_BITS, PWM_MAX_DUTY);
  Serial.printf("Sine: %.2f Hz, %u puntos\n", SINE_FREQ_HZ, SINE_POINTS);
  const double sampleRateHz = static_cast<double>(SINE_POINTS) * SINE_FREQ_HZ;
  Serial.printf("Sample rate: %.2f Hz\n", sampleRateHz);
  Serial.println(F("Mapa: 0% = min, 50% = cero, 100% = max de la onda."));
}

void loop() {
  // Todo ocurre en la ISR.
}
