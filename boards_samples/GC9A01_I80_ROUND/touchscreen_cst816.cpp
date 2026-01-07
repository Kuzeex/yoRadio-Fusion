#include "options.h"
#if (TS_MODEL==TS_MODEL_CST816)

#include "touchscreen_cst816.h"
#include <Adafruit_CST8XX.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifndef TS_I2C_ADDR
#define TS_I2C_ADDR 0x15
#endif

//
// <<< FONTOS >>>
// Globális I2C1 busz – a main.cpp-ben inicializáljuk!
//
TwoWire TouchWire = TwoWire(1);

static Adafruit_CST8XX cst;
CST816_Adapter ts;

static portMUX_TYPE tsMux = portMUX_INITIALIZER_UNLOCKED;

// ======================================
// ROTATION MAP
// ======================================
static inline void rotateMap(uint16_t &x, uint16_t &y, uint16_t w, uint16_t h, uint8_t rot) {
  uint16_t nx=x, ny=y;
  switch (rot & 3) {
    case 0: nx = x;          ny = y;          break;
    case 1: nx = h - 1 - y;  ny = x;          break;
    case 2: nx = w - 1 - x;  ny = h - 1 - y;  break;
    case 3: nx = y;          ny = w - 1 - x;  break;
  }
  x = nx; y = ny;
}

// ======================================
// Optional TCA6408 support
// ======================================
#ifndef TS_TCA6408_PRESENT
#define TS_TCA6408_PRESENT 0
#endif

#if TS_TCA6408_PRESENT

#ifndef TCA6408_ADDR
  #define TCA6408_ADDR 0x20
#endif
#ifndef TCA6408_TS_BIT
  #define TCA6408_TS_BIT 0
#endif

static bool tca_read_u8(uint8_t reg, uint8_t &val) {
  TouchWire.beginTransmission(TCA6408_ADDR);
  TouchWire.write(reg);
  if (TouchWire.endTransmission(false) != 0) return false;
  if (TouchWire.requestFrom(TCA6408_ADDR, (uint8_t)1) != 1) return false;
  val = TouchWire.read();
  return true;
}

static bool tca_write_u8(uint8_t reg, uint8_t val) {
  TouchWire.beginTransmission(TCA6408_ADDR);
  TouchWire.write(reg);
  TouchWire.write(val);
  return TouchWire.endTransmission() == 0;
}

static inline bool tca_ts_irq_active() {
  uint8_t in = 0xFF;
  if (!tca_read_u8(0x00, in)) return false;
  return ((in & (1 << TCA6408_TS_BIT)) == 0);
}

static volatile bool g_touch_irq = false;
#if (TS_INT >= 0)
void IRAM_ATTR _tca_isr() { g_touch_irq = true; }
#endif

#else
// NO TCA
static inline bool tca_ts_irq_active() { return false; }
static volatile bool g_touch_irq = false;

#if (TS_INT >= 0)
void IRAM_ATTR _direct_ts_isr() { g_touch_irq = true; }
#endif

#endif

// ======================================
// BACKGROUND TASK
// ======================================
static void cst816_task(void *arg) {
  CST816_Adapter *self = static_cast<CST816_Adapter*>(arg);

  for (;;) {
    self->_pollOnce();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ======================================
// BEGIN()
// ======================================
bool CST816_Adapter::begin(uint16_t w, uint16_t h, bool flip) {
  _w = w; _h = h; _flip = flip;

  //
  // <<< FONTOS >>> 
  // Itt már NEM indítjuk az I2C-t!
  // A main.cpp-ben kell meghívni:
  //   TouchWire.begin(TS_SDA, TS_SCL, 100000);
  //

#if TS_TCA6408_PRESENT
  tca_write_u8(0x02, 0x00);
  tca_write_u8(0x03, 0xFF);
  uint8_t dummy; tca_read_u8(0x00, dummy);

  #if (TS_INT >= 0)
    pinMode(TS_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TS_INT), _tca_isr, FALLING);
  #endif

#else

  #if (TS_INT >= 0)
    pinMode(TS_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TS_INT), _direct_ts_isr, FALLING);
  #endif

#endif

#if (TS_RST >= 0)
  pinMode(TS_RST, OUTPUT);
  digitalWrite(TS_RST, HIGH);
#endif

  // <<< CST816 init I2C1-en >>>
  cst.begin(&TouchWire, TS_I2C_ADDR);

  setRotation(flip ? 2 : 0);
  _hadTouch = false;
  _last = {0,0};
  isTouched = false;

  if (_taskHandle == nullptr) {
    xTaskCreatePinnedToCore(
      cst816_task,
      "ts_cst816",
      4096,
      this,
      1,
      &_taskHandle,
      0
    );
  }

  return true;
}

// ======================================
void CST816_Adapter::setRotation(uint8_t r) { _rot = (r & 3); }

void CST816_Adapter::read() { /* háttér-task intézi */ }

bool CST816_Adapter::touched() const { return isTouched; }

// ======================================
// POLL
// ======================================
void CST816_Adapter::_pollOnce() {
  bool localTouched = false;
  TP_Point localPoint = {0,0};

  bool shouldProbe = false;

#if TS_TCA6408_PRESENT
  if (tca_ts_irq_active()) shouldProbe = true;
#endif

#if (TS_INT >= 0)
  if (g_touch_irq) shouldProbe = true;
#endif

  static uint32_t last_keep = 0;
  uint32_t now = millis();
  if (!shouldProbe && (now - last_keep) < 50) return;
  if (!shouldProbe) last_keep = now;

  const int N = 3;
  uint16_t xs[N], ys[N];
  uint8_t count = 0;

  for (int i=0; i<N; ++i) {
    if (cst.touched()) {
      auto p = cst.getPoint(0);
      if (!(p.x==0 && p.y==0)) {
        xs[count] = p.x;
        ys[count] = p.y;
        ++count;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }

  if (count == 0) {
    g_touch_irq = false;
    portENTER_CRITICAL(&tsMux);
    isTouched = false;
    portEXIT_CRITICAL(&tsMux);
    return;
  }

  auto sort3 = [](uint16_t* a, uint8_t n){
    for (uint8_t i=0; i<n; i++)
      for (uint8_t j=i+1; j<n; j++)
        if (a[j] < a[i]) { uint16_t t=a[i]; a[i]=a[j]; a[j]=t; }
  };

  sort3(xs, count);
  sort3(ys, count);

  uint16_t x = xs[count/2];
  uint16_t y = ys[count/2];

  rotateMap(x, y, _w, _h, _rot);

#if TS_SWAP_XY
  uint16_t t = x; x = y; y = t;
#endif
#if TS_INVERT_X
  x = (_w - 1) - x;
#endif
#if TS_INVERT_Y
  y = (_h - 1) - y;
#endif

  const uint16_t DZ = 4;
  if (_hadTouch &&
      abs((int)x - (int)_last.x) <= DZ &&
      abs((int)y - (int)_last.y) <= DZ) {
    x = _last.x;
    y = _last.y;
  }

  localPoint.x = x;
  localPoint.y = y;
  localTouched = true;
  _last = localPoint;
  _hadTouch = true;

  g_touch_irq = false;

  portENTER_CRITICAL(&tsMux);
  points[0] = localPoint;
  isTouched = localTouched;
  portEXIT_CRITICAL(&tsMux);
}

#endif  // TS_MODEL==TS_MODEL_CST816
