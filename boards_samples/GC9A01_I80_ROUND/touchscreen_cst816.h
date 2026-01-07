#pragma once
#include <Arduino.h>
#include <Wire.h>      // szükséges a TwoWire típus miatt

// A touchscreen_cst816.cpp fájlban definiálva:
// TwoWire TouchWire(1);
extern TwoWire TouchWire;

struct TP_Point {
  uint16_t x = 0;
  uint16_t y = 0;
};

class CST816_Adapter {
public:
  bool begin(uint16_t w, uint16_t h, bool flip);
  void setRotation(uint8_t r);
  void read();                 // kompatibilitás – nem olvas közvetlenül I2C-t
  bool touched() const;

  // A háttér-task hívja folyamatosan
  void _pollOnce();

public:
  bool     isTouched = false;
  TP_Point points[1];

private:
  uint16_t _w = 0, _h = 0;
  uint8_t  _rot = 0;
  bool     _flip = false;

  TP_Point _last = {0,0};
  bool     _hadTouch = false;

  TaskHandle_t _taskHandle = nullptr;
};

extern CST816_Adapter ts;

