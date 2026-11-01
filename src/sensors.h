#pragma once
#include <Arduino.h>
#include <math.h>

// Units: degrees F, psi (gauge; negative = vacuum).
struct Readings {
  float egtF;
  float coolantF;
  float boostPsi;
  bool  egtValid;  // false on sensor fault / open thermocouple
};

class Sensors {
 public:
  virtual ~Sensors() {}
  virtual Readings read() = 0;
};

// Fake data so the UI can be developed without any hardware attached.
// EGT and boost follow a repeating "pull": idle -> full boost -> lift off.
// Coolant warms up slowly then holds near 195F.
class SimulatedSensors : public Sensors {
 public:
  Readings read() override {
    float t = millis() / 1000.0f;
    float phase = fmodf(t, 20.0f) / 20.0f;  // 20 s cycle

    float load = 0;  // 0..1
    if (phase < 0.2f)      load = 0;
    else if (phase < 0.5f) load = (phase - 0.2f) / 0.3f;
    else if (phase < 0.6f) load = 1;
    else if (phase < 0.8f) load = 1 - (phase - 0.6f) / 0.2f;

    Readings r;
    r.boostPsi = -8.0f + load * 36.0f;        // -8 .. 28 psi
    r.egtF     = 400.0f + load * 900.0f;      // 400 .. 1300 F (full gauge scale)
    r.coolantF = 115.0f + 120.0f * sinf(t * 0.15f);  // slow sweep, about -5 .. 235 F
    r.egtValid = true;
    return r;
  }
};
