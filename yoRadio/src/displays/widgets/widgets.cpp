#include "../../core/options.h"
#undef HIDE_DATE
#define HIDE_DATE
#if DSP_MODEL!=DSP_DUMMY
#ifdef NAMEDAYS_FILE
#include "../../core/namedays.h"
#endif
#include "../dspcore.h"
#include "../fonts/clockfont_api.h"
#include "../fonts/iconsweather.h"
#include "../fonts/iconsweather_mono.h"
#include "Arduino.h"
#include "widgets.h"
#include "../../core/player.h"    //  for VU widget
#include "../../core/network.h"   //  for Clock widget
#include "../../core/config.h"
#include "../tools/l10n.h"
#include "../tools/psframebuffer.h"
#include <math.h> 

namespace {

enum WeatherIconId {
  WI_01D, WI_01N, WI_02D, WI_02N, WI_03, WI_04D, WI_04N,
  WI_09D, WI_09N, WI_10D, WI_10N, WI_11, WI_13D, WI_13N, WI_50D, WI_50N,
  WI_UNKNOWN
};

static WeatherIconId iconIdFromCode(const char* code) {
  if (!code || !*code) return WI_UNKNOWN;

  if      (strstr(code, "01d")) return WI_01D;
  else if (strstr(code, "01n")) return WI_01N;
  else if (strstr(code, "02d")) return WI_02D;
  else if (strstr(code, "02n")) return WI_02N;
  else if (strstr(code, "03d") || strstr(code, "03n")) return WI_03;
  else if (strstr(code, "04d")) return WI_04D;
  else if (strstr(code, "04n")) return WI_04N;
  else if (strstr(code, "09d")) return WI_09D;
  else if (strstr(code, "09n")) return WI_09N;
  else if (strstr(code, "10d")) return WI_10D;
  else if (strstr(code, "10n")) return WI_10N;
  else if (strstr(code, "11d") || strstr(code, "11n")) return WI_11;
  else if (strstr(code, "13d")) return WI_13D;
  else if (strstr(code, "13n")) return WI_13N;
  else if (strstr(code, "50d")) return WI_50D;
  else if (strstr(code, "50n")) return WI_50N;

  return WI_UNKNOWN;
}

struct IconSet {
  const uint16_t* icons[WI_UNKNOWN];  // WI_UNKNOWN-ig (tehát 16 db)
  const uint16_t* fallback;
  uint16_t w;
  uint16_t h;
};

static const IconSet ICONSET_CLASSIC = {
  { img_01d, img_01n, img_02d, img_02n, img_03, img_04d, img_04n,
    img_09d, img_09n, img_10d, img_10n, img_11, img_13d, img_13n, img_50d, img_50n },
  eradio,
  64, 64
};

static const IconSet ICONSET_MONO = {
  { mono__01d, mono__01n, mono__02d, mono__02n, mono__03, mono__04d, mono__04n,
    mono__09d, mono__09n, mono__10d, mono__10n, mono__11, mono__13d, mono__13n, mono__50d, mono__50n },
  mono_eradio,
  64, 64
};

static const IconSet& currentIconSet() {
  return (config.store.weatherIconSet == 1) ? ICONSET_MONO : ICONSET_CLASSIC;
}

} // namespace

static inline void _pad2(char* out, int v) {
  out[0] = '0' + (v / 10);
  out[1] = '0' + (v % 10);
  out[2] = '\0';
}

static void formatDateCustom(char* out, size_t outlen, const tm& t, uint8_t fmt) {
  const char* month = LANG::mnths[t.tm_mon];
  const char* dow   = LANG::dowf[t.tm_wday];

  char d2[3], m2[3];
  _pad2(d2, t.tm_mday);
  _pad2(m2, t.tm_mon + 1);
  int y = t.tm_year + 1900;

#if L10N_LANGUAGE == HU
  // Hungarian
  switch (fmt) {
    case 1: // 2025. szeptember 16.  
      snprintf(out, outlen, "%d. %s %d.", y, month, t.tm_mday);
      break;

    case 2: // 2025. 09. 16. - kedd 
      snprintf(out, outlen, "%d. %s. %s. - %s", y, m2, d2, dow);
      break;

    case 3: // 2025 szeptember 16. kedd  
      snprintf(out, outlen, "%d %s %d. %s", y, month, t.tm_mday, dow);
      break;

    case 4: // 09. 16. - kedd 
      snprintf(out, outlen, "%s. %s. - %s", m2, d2, dow);
      break;

    default: // 2025. 09. 16. 
      snprintf(out, outlen, "%d. %s. %s.", y, m2, d2);
      break;
  }

#else 

  #ifdef USDATE
    // US format MM/DD/YYYY)
    switch (fmt) {
      case 1: // September 16, 2025
        snprintf(out, outlen, "%s %d, %d", month, t.tm_mday, y);
        break;

      case 2: // Tue - 09.16.2025
        snprintf(out, outlen, "%s - %s.%s.%d", dow, m2, d2, y);
        break;

      case 3: // Tue, September 16, 2025
        snprintf(out, outlen, "%s, %s %d, %d", dow, month, t.tm_mday, y);
        break;

      case 4: // Tue - 09.16.
        snprintf(out, outlen, "%s - %s.%s.", dow, m2, d2);
        break;

      default: // 09.16.2025
        snprintf(out, outlen, "%s.%s.%d", m2, d2, y);
        break;
    }

  #else
    // National format (DD/MM/YYYY)
    switch (fmt) {
      case 1: // 16 September 2025   
        snprintf(out, outlen, "%d. %s %d", t.tm_mday, month, y);
        break;

      case 2: // Tue - 16.09.2025 
        snprintf(out, outlen, "%s - %s. %s. %d", dow, d2, m2, y);
        break;

      case 3: // Tue, 16 September 2025  
        snprintf(out, outlen, "%s, %d. %s %d", dow, t.tm_mday, month, y);
        break;

      case 4: // Tue - 16.09. 
        snprintf(out, outlen, "%s - %s. %s.", dow, d2, m2);
        break;

      default: // 16.09.2025 
        snprintf(out, outlen, "%s. %s. %d", d2, m2, y);
        break;
    }
  #endif

#endif

}

static inline size_t utf8len(const char* s) {
  size_t len = 0;
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    if ( (*p & 0xC0) != 0x80 ) ++len; 
  }
  return len;
}

static uint32_t vuLastDecay = 0;

static inline float _applyVuCurve(float x, uint8_t ly) {
    // 0..1 normalizált bemenet
    float floor = REF_BY_LAYOUT(config.store, vuFloor, ly) / 100.0f;
    float ceil  = REF_BY_LAYOUT(config.store, vuCeil,  ly) / 100.0f;
    float expo  = REF_BY_LAYOUT(config.store, vuExpo,  ly) / 100.0f;  if (expo < 0.01f) expo = 0.01f;
    float gain  = REF_BY_LAYOUT(config.store, vuGain,  ly) / 100.0f;
    float knee  = REF_BY_LAYOUT(config.store, vuKnee,  ly) / 100.0f;

    // normalizálás floor..ceil tartományra
    float denom = (ceil - floor);
    if (denom < 0.001f) denom = 0.001f;
    x = (x - floor) / denom;
    // puha küszöb (smoothstep a 0..knee sávban)
    if (knee > 0.0f) {
      float t = fminf(fmaxf(x / knee, 0.0f), 1.0f);
      float smooth = t * t * (3.0f - 2.0f * t);   // smoothstep(0..1)
      x = (x < knee) ? (smooth * knee) : x;
    }
    x = fminf(fmaxf(x, 0.0f), 1.0f);
    // exponenciális alakítás + gain
    x = powf(x, expo) * gain;
    return fminf(fmaxf(x, 0.0f), 1.0f);
  }

/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor){
  Widget::init(conf.widget, bgcolor, bgcolor);
  _width = conf.width;
  _height = conf.height;
  
}

void FillWidget::_draw(){
  if(!_active) return;
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::setHeight(uint16_t newHeight){
  _height = newHeight;
  //_draw();
}
/************************
      TEXT WIDGET
 ************************/
TextWidget::~TextWidget() {
  free(_text);
  free(_oldtext);
}

void TextWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
#ifndef DSP_LCD
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
#else
  width = 1;
  height = 1;
#endif
}

void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _charSize(_config.textsize, _charWidth, _textheight);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
}

void TextWidget::setText(const char* txt) {
  strlcpy(_text, utf8To(txt, _uppercase), _buffsize);
  _textwidth = utf8len(_text) * _charWidth;
  if (strcmp(_oldtext, _text) == 0) return;
  if (_active) dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top, max(_oldtextwidth, _textwidth), _textheight, _bgcolor);
  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void TextWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void TextWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

/*uint16_t TextWidget::_realLeft(bool w_fb) {
  uint16_t realwidth = (_width>0 && w_fb)?_width:dsp.width();
  switch (_config.align) {
    case WA_CENTER: return (realwidth - _textwidth) / 2; break;
    case WA_RIGHT: return (realwidth - _textwidth - (!w_fb?_config.left:0)); break;
    default: return !w_fb?_config.left:0; break;
  }
}*/
uint16_t TextWidget::_realLeft(bool w_fb) {
  uint16_t realwidth = (_width>0 && w_fb) ? _width : dsp.width();
  int32_t calc = 0;
  switch (_config.align) {
    case WA_CENTER:
      calc = (int32_t)realwidth - (int32_t)_textwidth;
      calc /= 2;
      break;
    case WA_RIGHT:
      calc = (int32_t)realwidth - (int32_t)_textwidth - (!w_fb ? _config.left : 0);
      break;
    case WA_LEFT:
    default:
      calc = !w_fb ? _config.left : 0;
      break;
  }
  if (calc < 0) calc = 0;               // ← fontos!
  return (uint16_t)calc;
}

void TextWidget::_draw() {
  if(!_active) return;
  dsp.setTextColor(_fgcolor, _bgcolor);
  dsp.setCursor(_realLeft(), _config.top);
  dsp.setFont();
  dsp.setTextSize(_config.textsize);
  dsp.print(_text);
  strlcpy(_oldtext, _text, _buffsize);
}

/************************
      SCROLL WIDGET
 ************************/
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  init(separator, conf, fgcolor, bgcolor);
}

ScrollWidget::~ScrollWidget() {
  free(_fb);
  free(_sep);
  free(_window);
}

uint16_t ScrollWidget::_winLeft(bool fb) const {
  if (fb) return 0;

  switch (_config.align) {
    case WA_CENTER:
      return (uint16_t)((dsp.width() - _width) / 2);
    case WA_RIGHT:
      // WA_RIGHT 
      return (uint16_t)(dsp.width() - _width - _config.left);
    case WA_LEFT:
    default:
      return _config.left;
  }
}

void ScrollWidget::init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, fgcolor, bgcolor);
  _sep = (char *) malloc(sizeof(char) * 4);
  memset(_sep, 0, 4);
  snprintf(_sep, 4, " %.*s ", 1, separator);
  _startscrolldelay = conf.startscrolldelay;
  _scrolldelta      = conf.scrolldelta;
  _scrolltime       = conf.scrolltime;
  _charSize(_config.textsize, _charWidth, _textheight);
  _sepwidth = strlen(_sep) * _charWidth;
  _width = conf.width;
  _backMove.width = _width;
  _window = (char *) malloc(sizeof(char) * (MAX_WIDTH / _charWidth + 1));
  memset(_window, 0, (MAX_WIDTH / _charWidth + 1));
  _doscroll = false;
  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb && _fb->ready() ? 0 : wl;
  _x = fbl;

#ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  _fb->begin(&dsp, wl, _config.top, _width, _textheight, _bgcolor);
#endif
}


void ScrollWidget::_setTextParams() {
  if (_config.textsize == 0) return;
  if(_fb->ready()){
  #ifdef PSFBUFFER
    _fb->setTextSize(_config.textsize);
    _fb->setTextColor(_fgcolor, _bgcolor);
  #endif
  }else{
    dsp.setTextSize(_config.textsize);
    dsp.setTextColor(_fgcolor, _bgcolor);
  }
}

bool ScrollWidget::_checkIsScrollNeeded() {
  return _textwidth > _width;
}

void ScrollWidget::setText(const char* txt) {
  strlcpy(_text, utf8To(txt, _uppercase), _buffsize - 1);
  if (strcmp(_oldtext, _text) == 0) return;

  _textwidth = strlen(_text) * _charWidth;

  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb->ready() ? 0 : wl;
  _x = fbl;

  _doscroll = _checkIsScrollNeeded();
  if (dsp.getScrollId() == this) dsp.setScrollId(NULL);
  _scrolldelay = millis();

  if (_active) {
    _setTextParams();

    if (_doscroll) {
#ifdef PSFBUFFER
      if (_fb->ready()) {
        _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
        _fb->setCursor(0, 0);
        snprintf(_window, _width / _charWidth + 1, "%s", _text); // első ablak
        _fb->print(_window);
        _fb->display();
      } else
#endif
      {
        dsp.fillRect(wl, _config.top, _width, _textheight, _bgcolor);
        // első ablakot kiírjuk, clippinggel
        dsp.setClipping({wl, _config.top, _width, _textheight});
        dsp.setCursor(wl, _config.top);  // első pozíció: window bal széle
        snprintf(_window, _width / _charWidth + 1, "%s", _text);
        dsp.print(_window);
        dsp.clearClipping();
      }
    } else {
#ifdef PSFBUFFER
      if (_fb->ready()) {
        _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
        _fb->setCursor(_realLeft(true), 0);
        _fb->print(_text);
        _fb->display();
      } else
#endif
      {
        dsp.fillRect(wl, _config.top, _width, _textheight, _bgcolor);
        dsp.setCursor(_realLeft(), _config.top);  // TextWidget kezeli az igazítást
        dsp.setClipping({wl, _config.top, _width, _textheight});
        dsp.print(_text);
        dsp.clearClipping();
      }
    }

    strlcpy(_oldtext, _text, _buffsize);
  }
}

void ScrollWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

void ScrollWidget::loop() {
  if (_locked) return;
  if (!_doscroll || _config.textsize == 0 || (dsp.getScrollId() != NULL && dsp.getScrollId() != this)) return;

  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb->ready() ? 0 : wl;

  if (_checkDelay(_x == fbl ? _startscrolldelay : _scrolltime, _scrolldelay)) {
    _calcX();
    if (_active) _draw();
  }
}


void ScrollWidget::_clear(){
  if(_fb->ready()){
    #ifdef PSFBUFFER
    _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
    _fb->display();
    #endif
  } else {
    const uint16_t wl = _winLeft(false);
    //dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
    dsp.fillRect(wl, _config.top, _width, _textheight, _bgcolor);
  }
}

void ScrollWidget::_draw() {
  if (!_active || _locked) return;
  _setTextParams();

  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb->ready() ? 0 : wl;

  if (_doscroll) {
    uint16_t _newx = fbl - _x;
    const char* _cursor = _text + _newx / _charWidth;
    uint16_t hiddenChars = _cursor - _text;
    uint8_t addChars = _fb->ready() ? 2 : 1;

    if (hiddenChars < strlen(_text)) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation="
      snprintf(_window, _width / _charWidth + addChars, "%s%s%s", _cursor, _sep, _text);
#pragma GCC diagnostic pop
    } else {
      const char* _scursor = _sep + (_cursor - (_text + strlen(_text)));
      snprintf(_window, _width / _charWidth + addChars, "%s%s", _scursor, _text);
    }
    if (_fb->ready()) {
#ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      _fb->setCursor(_x + hiddenChars * _charWidth, 0);
      _fb->print(_window);
      _fb->display();
#endif
    } else {
      dsp.setClipping({wl, _config.top, _width, _textheight});
      // kurzor: window bal (wl) + lokális eltolás (_x - fbl) + rejtett karakterek szélessége
      dsp.setCursor(wl + (_x - fbl) + hiddenChars * _charWidth, _config.top);
      dsp.print(_window);
#ifndef DSP_LCD
      dsp.print(" ");
#endif
      dsp.clearClipping();
    }
  } else {
    if (_fb->ready()) {
#ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      _fb->setCursor(_realLeft(true), 0);
      _fb->print(_text);
      _fb->display();
#endif
    } else {
      dsp.fillRect(wl, _config.top, _width, _textheight, _bgcolor);
      dsp.setCursor(_realLeft(), _config.top);
      dsp.setClipping({wl, _config.top, _width, _textheight});
      dsp.print(_text);
      dsp.clearClipping();
    }
  }
}

void ScrollWidget::_calcX() {
  if (!_doscroll || _config.textsize == 0) return;

  _x -= _scrolldelta;

  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb->ready() ? 0 : wl;

  if (-_x > _textwidth + _sepwidth - fbl) {
    _x = fbl;
    dsp.setScrollId(NULL);
  } else {
    dsp.setScrollId(this);
  }
}


bool ScrollWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ScrollWidget::setWindowWidth(uint16_t w) {
  if (w == 0 || w == _width) return;

  _width = w;
  _backMove.width = _width;

#ifdef PSFBUFFER
  if (_fb) {
    _fb->freeBuffer();
    const uint16_t wl = _winLeft(false);
    _fb->begin(&dsp, wl, _config.top, _width, _textheight, _bgcolor);
  }
#endif

  const uint16_t fbl = _fb->ready() ? 0 : _winLeft(false);
  _x = fbl;

  _clear();
  _doscroll = _checkIsScrollNeeded();
}

void ScrollWidget::_reset(){
  dsp.setScrollId(NULL);

  const uint16_t wl  = _winLeft(false);
  const uint16_t fbl = _fb->ready() ? 0 : wl;
  _x = fbl;

  _scrolldelay = millis();
  _doscroll = _checkIsScrollNeeded();

#ifdef PSFBUFFER
  _fb->freeBuffer();
  _fb->begin(&dsp, wl, _config.top, _width, _textheight, _bgcolor);
#endif
}


/************************
      SLIDER WIDGET
 ************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
  Widget::init(conf.widget, fgcolor, bgcolor);
  _width = conf.width; _height = conf.height; _outlined = conf.outlined; _oucolor = oucolor, _max = maxval;
  _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
  _value = val;
  if (_active && !_locked) _drawslider();

}

void SliderWidget::_drawslider() {
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  if (_oldvalwidth == valwidth) return;
  dsp.fillRect(_config.left + _outlined + min(valwidth, _oldvalwidth), _config.top + _outlined, abs(_oldvalwidth - valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
  _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
  if(_locked) return;
  _clear();
  if(!_active) return;
  if (_outlined) dsp.drawRect(_config.left, _config.top, _width, _height, _oucolor);
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  dsp.fillRect(_config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
//  _oldvalwidth = 0;
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
  _oldvalwidth = 0;
}

/************************
      VU WIDGET
 ************************/
#if !defined(DSP_LCD) && !defined(DSP_OLED)

VuWidget::~VuWidget() {
  if (_canvas) {
    delete _canvas;
    _canvas = nullptr;
  }
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands,
                    uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
  _vumaxcolor = vumaxcolor;
  _vumincolor = vumincolor;
  _bands      = bands;

  _maxDimension = _config.align ? _bands.width : _bands.height;

  _peakL = 0;
  _peakR = 0;
  _peakFallDelayCounter = 0;

  // Canvas méret: NEM változtatjuk!
  switch (config.store.vuLayout) {
    case 3: // Studio
      _canvas = new Canvas(_maxDimension, _bands.height * 2 + _bands.space);
      break;
    case 2: // Boombox
    case 1: // Streamline
      _canvas = new Canvas(_maxDimension * 2 + _bands.space, _bands.height);
      break;
    case 0:
    default: // Default
      _canvas = new Canvas(_bands.width * 2 + _bands.space, _maxDimension);
      break;
  }
}

static inline void _labelTextStyle(Canvas* c, uint16_t textColor) {
  c->setFont();
  c->setTextSize(1);
  c->setTextColor(textColor);
  c->setTextWrap(false);
}

static inline void _drawCenteredCharInBox(Canvas* c,
                                         int boxX, int boxY, int boxW, int boxH,
                                         char ch)
{
  char s[2] = { ch, 0 };

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  c->getTextBounds(s, 0, 0, &tbx, &tby, &tbw, &tbh);

  int cx = boxX + (boxW - (int)tbw) / 2 - tbx;
  int cy = boxY + (boxH - (int)tbh) / 2 - tby;

  c->setCursor(cx, cy);
  c->print(s);
}

// Studio: bal oldali oszlop, két sor
void VuWidget::_drawLabelsVertical(int x, int y, int h, int labelSize) {
  uint16_t bg  = config.store.vuLabelBgColor;
  uint16_t txt = config.store.vuLabelTextColor;
  _labelTextStyle(_canvas, txt);

  const int pad = 1;
  int boxW = labelSize - 2 * pad;
  if (boxW < 1) return;

  int boxX = x + pad;

  // L
  int boxY = y;
  _canvas->fillRect(boxX, boxY, boxW, h, bg);
  #if DSP_MODEL != DSP_ST7735
  _drawCenteredCharInBox(_canvas, boxX, boxY, boxW, h, 'L');
  #endif

  // R
  boxY = y + h + _bands.space;
  _canvas->fillRect(boxX, boxY, boxW, h, bg);
  #if DSP_MODEL != DSP_ST7735
  _drawCenteredCharInBox(_canvas, boxX, boxY, boxW, h, 'R');
  #endif
}

// Default: alul egy csík, L/R cellával
void VuWidget::_drawLabelsHorizontal(int x, int y, int w, int labelSize) {
  uint16_t bg  = config.store.vuLabelBgColor;
  uint16_t txt = config.store.vuLabelTextColor;
  _labelTextStyle(_canvas, txt);

  const int pad = 1;
  int boxH = labelSize - 2 * pad;
  if (boxH < 1) return;

  _canvas->fillRect(x, y + pad, w, boxH, bg);

  int cellW = w / 2;
  int boxY  = y + pad;

  _drawCenteredCharInBox(_canvas, x,         boxY, cellW,     boxH, 'L');
  _drawCenteredCharInBox(_canvas, x + cellW, boxY, w-cellW,   boxH, 'R');
}

// Egy darab label doboz (szélessége = labelSize)
void VuWidget::_drawSingleLabel(int x, int y, int h, char ch, int labelSize)
{
  uint16_t bg  = config.store.vuLabelBgColor;
  uint16_t txt = config.store.vuLabelTextColor;
  _labelTextStyle(_canvas, txt);

  const int pad = 1;
  int boxW = labelSize - 2 * pad;
  if (boxW < 1) return;

  int boxX = x + pad;

  _canvas->fillRect(boxX, y, boxW, h, bg);
  _drawCenteredCharInBox(_canvas, boxX, y, boxW, h, ch);
}

void VuWidget::_draw() {
  if (!_active || _locked) return;

  // ------------------------------------------------------------
  // GEO: egyetlen közös geometria (a label MINDIG a VU-ból vesz el!)
  // ------------------------------------------------------------
  struct Geo {
    uint8_t ly;

    int canvasW, canvasH;

    // label méret + flag
    int labelSize;
    bool showLabels;

    // alap gap (a két csatorna közti fix rés)
    int gap;

    // Default (függőleges) VU terület (két oszlop)
    int def_vuX, def_vuY, def_vuW, def_vuH; // def_vuW == canvasW, def_vuH == canvasH-labelSize

    // Studio (két sor) VU terület
    int std_vuX, std_vuY, std_vuW, std_vuH; // std_vuW == canvasW-labelSize, std_vuH == _bands.height

    // Streamline: két csatorna egymás mellett, mindkettőből levágunk labelSize-t balról
    int str_LX, str_LW; // teljes csatorna sáv (névleges: _maxDimension)
    int str_RX, str_RW;
    int str_vuLX, str_vuLW; // tényleges VU rész (label levonva)
    int str_vuRX, str_vuRW;

    // Boombox: középről kifelé, a label a közép környékén a csatornákból vesz el
    int bbx_center;
    int bbx_leftEdge;   // bal csatorna "belső" széle (közép felőli)
    int bbx_rightEdge;  // jobb csatorna "belső" széle (közép felőli)
    int bbx_lblLX, bbx_lblRX; // label dobozok X kezdete (L a gap bal oldalán, R a gap jobb oldalán)
    int bbx_vuLW;       // bal csatorna max hossza (a label levonása után)
    int bbx_vuRW;       // jobb csatorna max hossza
    int bbx_vuY, bbx_vuH;

    int scaleDim; // 0..scaleDim (a VU “hossza”)
  } g;

  g.ly = config.store.vuLayout;

  // --- layoutfüggő label méret ---
  switch (g.ly) {
    case 3: g.labelSize = config.store.vuLabelHeightStd; break; // Studio
    case 2: g.labelSize = config.store.vuLabelHeightBbx; break; // Boombox
    case 1: g.labelSize = config.store.vuLabelHeightStr; break; // Streamline
    case 0:
    default: g.labelSize = config.store.vuLabelHeightDef; break; // Default
  }
  if (g.labelSize < 0) g.labelSize = 0;
  g.showLabels = (g.labelSize > 2);

  // --- canvas méretek (a meglévő init logikával egyezően) ---
  g.canvasW = (g.ly == 3) ? (int)_maxDimension
            : (g.ly == 0) ? (int)(_bands.width * 2 + _bands.space)
                          : (int)(_maxDimension * 2 + _bands.space);

  g.canvasH = (g.ly == 3) ? (int)(_bands.height * 2 + _bands.space)
            : (g.ly == 0) ? (int)_maxDimension
                          : (int)_bands.height;

  g.gap = (int)_bands.space;
  if (g.gap < 0) g.gap = 0;

  // --- Default geometry (függőleges) ---
  g.def_vuX = 0;
  g.def_vuY = 0;
  g.def_vuW = g.canvasW;
  g.def_vuH = g.canvasH - g.labelSize;
  if (g.def_vuH < 1) g.def_vuH = 1;

  // --- Studio geometry (két sor, label bal oldalt) ---
  g.std_vuX = g.labelSize;
  g.std_vuY = 0;
  g.std_vuW = g.canvasW - g.labelSize;
  if (g.std_vuW < 1) g.std_vuW = 1;
  g.std_vuH = (int)_bands.height;

  // --- Streamline geometry ---
  // két csatorna: [0.._maxDimension] és [(_maxDimension+gap)..]
  g.str_LX = 0;
  g.str_LW = (int)_maxDimension;
  g.str_RX = (int)_maxDimension + g.gap;
  g.str_RW = (int)_maxDimension;

  // label mindkét csatornából levág
  int cut = g.showLabels ? g.labelSize : 0;
  if (cut > g.str_LW - 1) cut = g.str_LW - 1;
  if (cut < 0) cut = 0;

  g.str_vuLX = g.str_LX + cut;
  g.str_vuLW = g.str_LW - cut;
  g.str_vuRX = g.str_RX + cut;
  g.str_vuRW = g.str_RW - cut;
  if (g.str_vuLW < 1) g.str_vuLW = 1;
  if (g.str_vuRW < 1) g.str_vuRW = 1;

  // --- Boombox geometry (középről kifelé) ---
  g.bbx_center    = g.canvasW / 2;
  g.bbx_leftEdge  = g.bbx_center - (g.gap / 2); // bal csatorna belső széle
  g.bbx_rightEdge = g.bbx_center + (g.gap / 2); // jobb csatorna belső széle

  // label a gap két oldalán, és a csatornákból vesz el
  int bbxLabel = g.showLabels ? g.labelSize : 0;
  // ha túl nagy, legalább 1px maradjon a csatornának
  int maxLeft  = g.bbx_leftEdge;
  int maxRight = g.canvasW - g.bbx_rightEdge;
  if (bbxLabel > maxLeft - 1)  bbxLabel = maxLeft - 1;
  if (bbxLabel > maxRight - 1) bbxLabel = maxRight - 1;
  if (bbxLabel < 0) bbxLabel = 0;

  g.bbx_lblLX = g.bbx_leftEdge - bbxLabel;
  g.bbx_lblRX = g.bbx_rightEdge;

  g.bbx_vuLW = g.bbx_leftEdge - bbxLabel;          // bal csatorna max hossza (0..bbx_vuLW)
  g.bbx_vuRW = g.canvasW - (g.bbx_rightEdge + bbxLabel); // jobb csatorna max hossza
  if (g.bbx_vuLW < 1) g.bbx_vuLW = 1;
  if (g.bbx_vuRW < 1) g.bbx_vuRW = 1;

  g.bbx_vuY = 0;
  g.bbx_vuH = g.canvasH;

  // --- scaleDim (VU hossza) layout szerint ---
  switch (g.ly) {
    case 0: g.scaleDim = g.def_vuH; break;                 // függőleges
    case 3: g.scaleDim = g.std_vuW; break;                 // Studio: vízszintes hossza
    case 1: g.scaleDim = min(g.str_vuLW, g.str_vuRW); break; // Streamline: csatorna hossza
    case 2: g.scaleDim = min(g.bbx_vuLW, g.bbx_vuRW); break; // Boombox: csatorna hossza
    default: g.scaleDim = g.def_vuH; break;
  }
  if (g.scaleDim < 1) return;

  // ------------------------------------------------------------
  // INPUT + SCALING
  // ------------------------------------------------------------
  uint16_t vulevel = player.getVUlevel();

  uint16_t L_raw = (vulevel >> 8) & 0xFF;
  uint16_t R_raw = (vulevel) & 0xFF;


  // AUTOGAIN
  uint16_t currentMax = max(L_raw, R_raw);

  uint32_t now = millis();

  if (currentMax > config.vuRefLevel)
      config.vuRefLevel = currentMax;

  static uint32_t vuLastDecay = 0;

  if (now - vuLastDecay > 120) //original value 200
  {
      vuLastDecay = now;

      if (config.vuRefLevel > 1)
          config.vuRefLevel--;
  }

  if (config.vuRefLevel < 50)
      config.vuRefLevel = 50;


  // SCALE
  uint16_t vuLpx = map(L_raw, 0, config.vuRefLevel, 0, g.scaleDim);
  uint16_t vuRpx = map(R_raw, 0, config.vuRefLevel, 0, g.scaleDim);


  // CURVE
  float vuNorm_L = (float)vuLpx / g.scaleDim;
  float vuNorm_R = (float)vuRpx / g.scaleDim;

  float vuAdj_L = _applyVuCurve(vuNorm_L, g.ly);
  float vuAdj_R = _applyVuCurve(vuNorm_R, g.ly);

  uint16_t L_scaled = (uint16_t)roundf(vuAdj_L * g.scaleDim);
  uint16_t R_scaled = (uint16_t)roundf(vuAdj_R * g.scaleDim);

  // ------------------------------------------------------------
  // BANDS + COLORS + SMOOTH
  // ------------------------------------------------------------
  const uint16_t vumidcolor = config.store.vuMidColor;
  const uint8_t  bandsCount = _bands.perheight ? _bands.perheight : 1;
  const uint16_t h = (uint16_t)max(1, g.scaleDim / (int)bandsCount);

  const float alpha_up   = REF_BY_LAYOUT(config.store, vuAlphaUp, g.ly) / 100.0f;
  const float alpha_down = REF_BY_LAYOUT(config.store, vuAlphaDn, g.ly) / 100.0f;

  uint8_t midPct  = REF_BY_LAYOUT(config.store, vuMidPct,  g.ly);
  uint8_t highPct = REF_BY_LAYOUT(config.store, vuHighPct, g.ly);

  const uint16_t peakColor = config.store.vuPeakColor;

  int cfgBars = (int)REF_BY_LAYOUT(config.store, vuBarCount, g.ly);
  int effectiveBars = (int)bandsCount;
  if (effectiveBars <= 0) effectiveBars = cfgBars;
  if (effectiveBars <= 0) effectiveBars = 1;

  int midBandIndex  = (effectiveBars * midPct  + 99) / 100;
  int highBandIndex = (effectiveBars * highPct + 99) / 100;
  if (midBandIndex  < 0) midBandIndex = 0;
  if (highBandIndex < 0) highBandIndex = 0;
  if (midBandIndex  > effectiveBars) midBandIndex = effectiveBars;
  if (highBandIndex > effectiveBars) highBandIndex = effectiveBars;
  if (highBandIndex < midBandIndex) highBandIndex = midBandIndex;

  static float smoothL = 0, smoothR = 0;
  static float peakSmoothL = 0, peakSmoothR = 0;
  static uint16_t measL = 0, measR = 0;

  smoothL = (L_scaled > smoothL) ? (alpha_up   * L_scaled + (1.0f - alpha_up)   * smoothL)
                                 : (alpha_down * L_scaled + (1.0f - alpha_down) * smoothL);
  smoothR = (R_scaled > smoothR) ? (alpha_up   * R_scaled + (1.0f - alpha_up)   * smoothR)
                                 : (alpha_down * R_scaled + (1.0f - alpha_down) * smoothR);

  bool played = player.isRunning();
  if (played) {
    measL = (L_raw >= measL) ? (measL + _bands.fadespeed) : L_raw;
    measR = (R_raw >= measR) ? (measR + _bands.fadespeed) : R_raw;
  } else {
    if (measL < (uint16_t)g.scaleDim) measL += _bands.fadespeed;
    if (measR < (uint16_t)g.scaleDim) measR += _bands.fadespeed;
    peakSmoothL = 0; peakSmoothR = 0;
  }
  if (measL > (uint16_t)g.scaleDim) measL = (uint16_t)g.scaleDim;
  if (measR > (uint16_t)g.scaleDim) measR = (uint16_t)g.scaleDim;

#if DSP_MODEL == DSP_ST7735
  const int peakWidth = 2;
#else
  const int peakWidth = 4;
#endif

  // ------------------------------------------------------------
  // CLEAR CANVAS
  // ------------------------------------------------------------
  _canvas->fillRect(0, 0, g.canvasW, g.canvasH, _bgcolor);

  // ------------------------------------------------------------
  // DRAW LABELS (mindenhol a VU-ból levonva számolt geo szerint!)
  // ------------------------------------------------------------
  switch (g.ly) {
    case 0: // Default: alul
      if (g.showLabels) _drawLabelsHorizontal(0, g.def_vuH, g.canvasW, g.labelSize);
      break;

    case 3: // Studio: bal oszlop, két sor
      if (g.showLabels) _drawLabelsVertical(0, 0, _bands.height, g.labelSize);
      break;

    case 1: // Streamline: label mindkét csatornában baloldalt
      if (g.showLabels) {
        _drawSingleLabel(g.str_LX, 0, _bands.height, 'L', g.labelSize);
        _drawSingleLabel(g.str_RX, 0, _bands.height, 'R', g.labelSize);
      }
      break;

    case 2: // Boombox: label a gap két oldalán (L bal oldal, R jobb oldal), a csatornákból levonva
      if (g.showLabels) {
        _drawSingleLabel(g.bbx_lblLX, 0, _bands.height, 'L', g.labelSize);
        _drawSingleLabel(g.bbx_lblRX, 0, _bands.height, 'R', g.labelSize);
      }
      break;
  }

  // ------------------------------------------------------------
  // PEAK SMOOTH + SNAP
  // ------------------------------------------------------------
  uint16_t drawL = (uint16_t)smoothL;
  uint16_t drawR = (uint16_t)smoothR;

  const float p_up   = REF_BY_LAYOUT(config.store, vuPeakUp, g.ly) / 100.0f;
  const float p_down = REF_BY_LAYOUT(config.store, vuPeakDn, g.ly) / 100.0f;

  peakSmoothL = (drawL > peakSmoothL) ? (p_up * drawL + (1 - p_up) * peakSmoothL)
                                      : (p_down * drawL + (1 - p_down) * peakSmoothL);
  peakSmoothR = (drawR > peakSmoothR) ? (p_up * drawR + (1 - p_up) * peakSmoothR)
                                      : (p_down * drawR + (1 - p_down) * peakSmoothR);

  uint16_t snappedL = ((drawL + h - 1) / h) * h;
  uint16_t snappedR = ((drawR + h - 1) / h) * h;
  if (snappedL > (uint16_t)g.scaleDim) snappedL = (uint16_t)g.scaleDim;
  if (snappedR > (uint16_t)g.scaleDim) snappedR = (uint16_t)g.scaleDim;

  // ------------------------------------------------------------
  // DRAW BARS (switch-case, átlátható)
  // ------------------------------------------------------------
  switch (g.ly) {

    case 3: { // ===================== Studio =====================
      const int peakOfs = 10;
      int peakX_L = constrain((int)max((int)snappedL, (int)peakSmoothL + peakOfs), 0, g.scaleDim - peakWidth);
      int peakX_R = constrain((int)max((int)snappedR, (int)peakSmoothR + peakOfs), 0, g.scaleDim - peakWidth);

      for (int band = 0; band < bandsCount; band++) {
        int i = band * h;
        int bandW = h - _bands.space;
        if (band == bandsCount - 1) bandW = h;

        uint16_t col = (band < midBandIndex) ? _vumincolor : (band < highBandIndex ? vumidcolor : _vumaxcolor);

        if (i + bandW <= snappedL)
          _canvas->fillRect(g.std_vuX + i, 0, bandW, _bands.height, col);

        if (i + bandW <= snappedR)
          _canvas->fillRect(g.std_vuX + i, _bands.height + _bands.space, bandW, _bands.height, col);
      }

      _canvas->fillRect(g.std_vuX + peakX_L, 0, peakWidth, _bands.height, peakColor);
      _canvas->fillRect(g.std_vuX + peakX_R, _bands.height + _bands.space, peakWidth, _bands.height, peakColor);
    } break;

    case 1: { // ===================== Streamline =====================
      const int peakOfs = 4;

      // bal csatorna VU: g.str_vuLX.. + g.scaleDim
      int peakX_L = constrain((int)max((int)snappedL, (int)peakSmoothL + peakOfs), 0, g.scaleDim - peakWidth);
      int peakX_R = constrain((int)max((int)snappedR, (int)peakSmoothR + peakOfs), 0, g.scaleDim - peakWidth);

      for (int band = 0; band < bandsCount; band++) {
        int i = band * h;
        int bandW = h - _bands.vspace;
        if (band == bandsCount - 1) bandW = h;

        uint16_t col = (band < midBandIndex) ? _vumincolor : (band < highBandIndex ? vumidcolor : _vumaxcolor);

        if (i + bandW <= snappedL)
          _canvas->fillRect(g.str_vuLX + i, 0, bandW, _bands.height, col);

        if (i + bandW <= snappedR)
          _canvas->fillRect(g.str_vuRX + i, 0, bandW, _bands.height, col);
      }

      _canvas->fillRect(g.str_vuLX + peakX_L, 0, peakWidth, _bands.height, peakColor);
      _canvas->fillRect(g.str_vuRX + peakX_R, 0, peakWidth, _bands.height, peakColor);
    } break;

    case 2: { // ===================== Boombox =====================
      const int peakOfs = 10;

      // bal: a belső széltől (bbx_lblLX) BALRA rajzolunk, max hossza g.scaleDim
      // jobb: a belső széltől (bbx_lblRX + labelSize) JOBBRA
      int leftInner  = g.bbx_lblLX;                 // L label bal széle (itt kezdődik a "közép előtti" blokk)
      int rightInner = g.bbx_lblRX + g.labelSize;   // R label utáni rész a jobb csatornának

      // csatorna végek
      int leftMinX   = 0;
      int leftMaxX   = leftInner;                   // ideig rajzolhatunk (bal csatorna jobboldali széle)
      int rightMinX  = rightInner;
      int rightMaxX  = g.canvasW;

      // peak hely
      int peakLenL = (int)max((int)snappedL, (int)peakSmoothL);
      int peakLenR = (int)max((int)snappedR, (int)peakSmoothR);

      int peakX_L = leftMaxX - peakLenL - peakOfs - peakWidth;
      int peakX_R = rightMinX + peakLenR + peakOfs;

      if (peakX_L < leftMinX) peakX_L = leftMinX;
      if (peakX_L > leftMaxX - peakWidth) peakX_L = leftMaxX - peakWidth;

      if (peakX_R < rightMinX) peakX_R = rightMinX;
      if (peakX_R > rightMaxX - peakWidth) peakX_R = rightMaxX - peakWidth;

      for (int band = 0; band < bandsCount; band++) {
        int i = band * h;
        int bandW = h - _bands.vspace;
        if (band == bandsCount - 1) bandW = h;

        uint16_t col = (band < midBandIndex) ? _vumincolor : (band < highBandIndex ? vumidcolor : _vumaxcolor);

        // bal: belső széltől BALRA
        if (i + bandW <= snappedL) {
          int x = leftMaxX - i - bandW;
          if (x < leftMinX) x = leftMinX;
          _canvas->fillRect(x, 0, bandW, _bands.height, col);
        }

        // jobb: belső széltől JOBBRA
        if (i + bandW <= snappedR) {
          int x = rightMinX + i;
          if (x + bandW > rightMaxX) bandW = max(1, rightMaxX - x);
          _canvas->fillRect(x, 0, bandW, _bands.height, col);
        }
      }

      _canvas->fillRect(peakX_L, 0, peakWidth, _bands.height, peakColor);
      _canvas->fillRect(peakX_R, 0, peakWidth, _bands.height, peakColor);
    } break;

    case 0:
    default: { // ===================== Default (vertical) =====================
      const int peakOfs = 4;

      int peakY_L = constrain((int)peakSmoothL + peakOfs, 0, g.scaleDim - peakWidth);
      int peakY_R = constrain((int)peakSmoothR + peakOfs, 0, g.scaleDim - peakWidth);

      int baseY = g.def_vuY + g.def_vuH;

      for (int band = 0; band < bandsCount; band++) {
        int i = band * h;
        int bandH = h - _bands.vspace;
        if (band == bandsCount - 1) bandH = h;

        uint16_t col = (band < midBandIndex) ? _vumincolor : (band < highBandIndex ? vumidcolor : _vumaxcolor);

        if (i + bandH <= snappedL)
          _canvas->fillRect(0, baseY - i - bandH, _bands.width, bandH, col);

        if (i + bandH <= snappedR)
          _canvas->fillRect(_bands.width + _bands.space, baseY - i - bandH, _bands.width, bandH, col);
      }

      _canvas->fillRect(0, baseY - peakY_L - peakWidth, _bands.width, peakWidth, peakColor);
      _canvas->fillRect(_bands.width + _bands.space, baseY - peakY_R - peakWidth, _bands.width, peakWidth, peakColor);
    } break;
  }

  // ------------------------------------------------------------
  // BLIT TO DISPLAY
  // ------------------------------------------------------------
  dsp.drawRGBBitmap(_config.left, _config.top, _canvas->getBuffer(), g.canvasW, g.canvasH);
}

void VuWidget::loop() {
  if (_active && !_locked) _draw();
}

void VuWidget::_clear() {
  switch (config.store.vuLayout) {
    case 3:
      dsp.fillRect(_config.left, _config.top, _maxDimension, _bands.height * 2 + _bands.space, _bgcolor);
      break;
    case 2:
    case 1:
      dsp.fillRect(_config.left, _config.top, _maxDimension * 2 + _bands.space, _bands.height, _bgcolor);
      break;
    case 0:
    default:
      dsp.fillRect(_config.left, _config.top, _bands.width * 2 + _bands.space, _maxDimension, _bgcolor);
      break;
  }
}

#else // DSP_LCD

VuWidget::~VuWidget() {}
void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
}
void VuWidget::_draw() {}
void VuWidget::loop() {}
void VuWidget::_clear() {}

#endif

/************************
      NUM & CLOCK
 ************************/
#if !defined(DSP_LCD)
  #if TIME_SIZE<19 //19->NOKIA
    const GFXfont* Clock_GFXfontPtr = nullptr;
    #define CLOCKFONT5x7
  #else
    const GFXfont* Clock_GFXfontPtr = nullptr;
  #endif
#endif //!defined(DSP_LCD)

static inline void applyClockFontFromConfig() {
  #if !defined(DSP_LCD)
    const uint8_t id = clockfont_clamp_id(config.store.clockFontId); // 0..4
    Clock_GFXfontPtr = clockfont_get(id);
  #endif
}

#if !defined(CLOCKFONT5x7) && !defined(DSP_LCD)
// --- GFXfont metrika-alapú számolás (nincs ':' kivétel, nincs API-advance) ---
inline const GFXglyph* glyphPtr(const GFXfont* f, unsigned char c){
  if (!f) return nullptr;
  if (c < f->first || c > f->last) return nullptr;
  return f->glyph + (c - f->first);
}

uint8_t _charWidth(unsigned char c){
  if (!Clock_GFXfontPtr) return TIME_SIZE;        // 5x7 fallback biztosíték
  const GFXglyph* g = glyphPtr(Clock_GFXfontPtr, c);
  return g ? pgm_read_byte(&g->xAdvance) : 0;
}

uint16_t _textHeight(){
  if (!Clock_GFXfontPtr) return TIME_SIZE;        // 5x7 fallback biztosíték
  const GFXglyph* g = glyphPtr(Clock_GFXfontPtr, '8'); // reprezentatív magas szám
  return g ? pgm_read_byte(&g->height) : TIME_SIZE;
}

#else // CLOCKFONT5x7 || DSP_LCD
// --- 5x7 / LCD fallback: marad a régi skálázás ---
uint8_t _charWidth(unsigned char){
#ifndef DSP_LCD
  return CHARWIDTH * TIME_SIZE;
#else
  return 1;
#endif
}
uint16_t _textHeight(){
  return CHARHEIGHT * TIME_SIZE;
}
#endif

uint16_t _textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  //#if DSP_MODEL==DSP_ILI9225
  //return w+l;
  //#else
  return w;
  //#endif
}

/************************
      NUM WIDGET
 ************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
  _textheight = TIME_SIZE/*wconf.textsize*/;
}

void NumWidget::setText(const char* txt) {
  strlcpy(_text, txt, _buffsize);
  _getBounds();
  if (strcmp(_oldtext, _text) == 0) return;
  uint16_t realth = _textheight;
#if defined(DSP_OLED) && DSP_MODEL!=DSP_SSD1322
  if(Clock_GFXfontPtr==nullptr) realth = _textheight * 8; //CHARHEIGHT
#endif
  if (_active)
  #ifndef CLOCKFONT5x7
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top-_textheight+1, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #else
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #endif

  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void NumWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void NumWidget::_getBounds() {
  _textwidth= _textWidth(_text);
}

void NumWidget::_draw() {
#ifndef DSP_LCD
  if(!_active || TIME_SIZE<2) return;
  applyClockFontFromConfig();
  dsp.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  dsp.setFont(Clock_GFXfontPtr);
  dsp.setTextColor(_fgcolor, _bgcolor);
#endif
  if(!_active) return;
  dsp.setCursor(_realLeft(), _config.top);
  dsp.print(_text);
  strlcpy(_oldtext, _text, _buffsize);
  dsp.setFont();
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
  char buf[_width + 1];
  snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
  _pg++; if (_pg >= _width + _barwidth) _pg = 0;
  setText(buf);
}

bool ProgressWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ProgressWidget::loop() {
  if (_checkDelay(_speed, _scrolldelay)) {
    _progress();
  }
}

/**************************
      CLOCK WIDGET
 **************************/
void ClockWidget::init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(wconf, fgcolor, bgcolor);
  _timeheight = _textHeight();
  _fullclock = TIME_SIZE>35 || DSP_MODEL==DSP_ILI9225 || DSP_MODEL==DSP_ST7789_170 || DSP_MODEL==DSP_ST7735 || DSP_MODEL == DSP_ST7789 || DSP_MODEL==DSP_ILI9341;
/*#if DSP_MODEL == DSP_ST7789 || DSP_MODEL==DSP_ILI9341
    if (config.store.vuLayout != 0) {
        _fullclock = false;
    }
#endif*/
  if(_fullclock) _superfont = TIME_SIZE / 17; //magick
  else if(TIME_SIZE==19 || TIME_SIZE==2) _superfont=1;
  else _superfont=0;
  _space = (5*_superfont)/2; //magick
  #ifndef HIDE_DATE
  if(_fullclock){
    _dateheight = _superfont<4?1:2;
    _clockheight = _timeheight + _space + CHARHEIGHT * _dateheight;
  } else {
    _clockheight = _timeheight;
    _dateheight = 0;
  }
  #else
    _clockheight = _timeheight;
  #endif
  applyClockFontFromConfig();

  auto& gfx = getRealDsp();
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  
  _getTimeBounds();
#ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  _begin();
#endif
}

void ClockWidget::_begin(){
#ifdef PSFBUFFER
  _fb->begin(&dsp, _clockleft, _config.top-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
#endif
}

bool ClockWidget::_getTime(){
  if (config.store.hours12) {
  strftime(_timebuffer, sizeof(_timebuffer), "%I:%M", &network.timeinfo);
  } else {
  strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
  }
  bool ret = network.timeinfo.tm_sec==0 || _forceflag!=network.timeinfo.tm_year;
  _forceflag = network.timeinfo.tm_year;
  return ret;
}


uint16_t ClockWidget::_left(){
  if(_fb->ready()) return 0; else return _clockleft;
}
uint16_t ClockWidget::_top(){
  if(_fb->ready()) return _timeheight; else return _config.top;
}

void ClockWidget::_getTimeBounds() {
  _timewidth = _textWidth(_timebuffer);
  uint8_t fs = _superfont>0?_superfont:TIME_SIZE;
  uint16_t rightside = CHARWIDTH * fs * 2; // seconds
  if(_fullclock){
    rightside += _space*2+1; //2space+vline
    _clockwidth = _timewidth+rightside;
  } else {
    if(_superfont==0)
      _clockwidth = _timewidth;
    else
      _clockwidth = _timewidth + rightside;
  }
  switch(_config.align){
    case WA_LEFT: _clockleft = _config.left; break;
    case WA_RIGHT: _clockleft = dsp.width()-_clockwidth-_config.left; break;
    default:
      _clockleft = (dsp.width()/2 - _clockwidth/2)+_config.left;
      break;
  }
  char buf[4];
  strftime(buf, 4, "%H", &network.timeinfo);
  _dotsleft=_textWidth(buf);
}

#ifndef DSP_LCD

  Adafruit_GFX& ClockWidget::getRealDsp(){
  #ifdef PSFBUFFER
    if (_fb && _fb->ready()) return *_fb;
  #endif
    return dsp;
  }

void ClockWidget::_printClock(bool force){
  time_t now = time(nullptr);
  struct tm ti;
  localtime_r(&now, &ti);
  
  auto& gfx = getRealDsp();
  applyClockFontFromConfig();
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  const int16_t timeLeft        = _left();
  const int16_t timeTop         = _top() - _timeheight;
  const int16_t timeWidthLive   = _textWidth(_timebuffer);
  const int16_t vlineX          = timeLeft + timeWidthLive + _space;
  const int16_t colTop    = _top() - _timeheight;
  const int16_t colBottom = _top();
  const int16_t colCenter = (colTop + colBottom) / 2;
  const int16_t secCellH  = CHARHEIGHT * _superfont;
  const int16_t secBaseY = colCenter - (secCellH / 2);
  const int16_t secLeftFull    = timeLeft + timeWidthLive + _space*2;
  
  int16_t divY = -1;
  
  static const GFXfont* s_lastPrintFont = nullptr;
  if (Clock_GFXfontPtr != s_lastPrintFont) {
    _getTimeBounds();
    s_lastPrintFont = Clock_GFXfontPtr;
  }

  bool clockInTitle=!config.isScreensaver && _config.top<_timeheight; //DSP_SSD1306x32
  if(force){
    _clearClock();
    _getTimeBounds();
    #ifndef DSP_OLED
    if (config.store.clockFontMono) {
      gfx.setTextColor(config.theme.clockbg, config.theme.background);
      gfx.setCursor(_left(), _top());
      gfx.print("88:88");
    }
    #endif
    if (clockInTitle) {
      gfx.setTextColor(config.theme.meta, config.theme.metabg);
    } else {
      gfx.setTextColor(config.theme.clock, config.theme.background);
    }

    gfx.setCursor(_left(), _top());

  if (config.store.hours12) {
    if (_timebuffer[0] == '0') {
      int16_t leadX = _left();
      int16_t leadY = _top() - _timeheight + 1; 
      uint16_t leadW = _charWidth('0'); 
      uint16_t leadH = _timeheight + 2;

     gfx.fillRect(leadX, leadY, leadW, leadH, config.theme.background);

      if (clockInTitle) {
        gfx.setTextColor(config.theme.meta, config.theme.metabg);
      } else {
        gfx.setTextColor(config.theme.clock, config.theme.background);
      }

      gfx.setCursor(leadX + leadW, _top());
      gfx.print(_timebuffer + 1);
    } else {
      gfx.print(_timebuffer);
    }
  } else {
    gfx.print(_timebuffer);
  }

 /*   if(_fullclock){
      // lines, date & dow
      bool fullClockOnScreensaver = (!config.isScreensaver || (_fb->ready() && FULL_SCR_CLOCK));
      _linesleft = _left()+_timewidth+_space;
      if(fullClockOnScreensaver){
int16_t hlineBaseY = secBaseY + secCellH;
if (hlineBaseY >= _top()) hlineBaseY = _top() - 1;

if (config.store.hours12) {

  // függőleges elválasztó (marad, ahogy volt)
  gfx.drawFastVLine(_linesleft, secBaseY - 5, _top() - secBaseY + 10, config.theme.div);
  gfx.setFont();

  // ---- VÍZSZINTES VONAL Y KISZÁMÍTÁSA (EGY HELYEN!) ----
  int16_t yLine;
  int16_t ampmTextY;
  uint8_t ampmTextSize;

  if (TIME_SIZE == 19) {
    yLine        = hlineBaseY + _space/2 - 8;
    ampmTextY    = hlineBaseY + CHARHEIGHT - 8;
    ampmTextSize = 1;
  } else {
    yLine        = hlineBaseY + _space/2 - 8;
    ampmTextY    = hlineBaseY +
                   (TIME_SIZE == 70 ? CHARHEIGHT - 2 :
                    TIME_SIZE == 52 ? CHARHEIGHT - 8 :
                                      CHARHEIGHT - 12);
    ampmTextSize = 2;
  }

  // ---- VONAL RAJZOLÁS + ELMENTÉS ----
  gfx.drawFastHLine(_linesleft, yLine, CHARWIDTH * _superfont * 2 + _space + 15, config.theme.div);
  divY = yLine; 

  // ---- AM / PM FELIRAT ----
  gfx.setCursor(_linesleft + _space + 1, ampmTextY);
  gfx.setTextSize(ampmTextSize);
  gfx.setTextColor(config.theme.dow, config.theme.background);

  char buf[3];
  strftime(buf, sizeof(buf), "%p", &network.timeinfo);
  gfx.print(buf);

} else { // ----- 24 ÓRÁS NÉZET -----

  gfx.drawFastVLine(
    _linesleft,
    secBaseY,
    _top() - secBaseY,
    config.theme.div
  );

  int16_t yLine = hlineBaseY + _space/2;

  gfx.drawFastHLine(
    _linesleft,
    yLine,
    CHARWIDTH * _superfont * 2 + _space,
    config.theme.div
  );
  divY = yLine;   // itt is eltároljuk, egységesen

}

        formatDateCustom(_tmp, sizeof(_tmp), ti, config.store.dateFormat);
        #ifndef HIDE_DATE
        strlcpy(_datebuf, utf8To(_tmp, true), sizeof(_datebuf));
        uint16_t _datewidth = strlen(_datebuf) * CHARWIDTH*_dateheight;
        gfx.setTextSize(_dateheight);
        #if DSP_MODEL==DSP_GC9A01A || DSP_MODEL==DSP_GC9A01 || DSP_MODEL==DSP_GC9A01_I80
        gfx.setCursor((dsp.width()-_datewidth)/2, _top() + _space);
        #else
        gfx.setCursor(_left()+_clockwidth-_datewidth, _top() + _space);
        #endif
        gfx.setTextColor(config.theme.date, config.theme.background);
        gfx.print(_datebuf);
        #endif
      }
    }*/
  }

// ------------------------------------------------------------
// FULLCLOCK: vonalak + AM/PM (vagy 24h) + dátum
// ------------------------------------------------------------
if (_fullclock) {
  bool fullClockOnScreensaver = (!config.isScreensaver || (_fb->ready() && FULL_SCR_CLOCK));
  _linesleft = _left() + _timewidth + _space;

  if (fullClockOnScreensaver) {

    int16_t hlineBaseY = secBaseY + secCellH;
    if (hlineBaseY >= _top()) hlineBaseY = _top() - 1;

if (config.store.hours12) {
    // függőleges elválasztó (marad)
    gfx.drawFastVLine(_linesleft, secBaseY - 5, _top() - secBaseY + 10, config.theme.div);

    gfx.setFont();

    // ---- VÍZSZINTES VONAL + AM/PM szöveg pozíció (EGY HELYEN) ----
    int16_t yLine = 0;
    int16_t ampmTextY = 0;
    uint8_t ampmTextSize = 2;

    if (TIME_SIZE == 19) {
      yLine        = hlineBaseY + _space/2 - 4;
      ampmTextY    = hlineBaseY + CHARHEIGHT - 8;
      ampmTextSize = 1;
    } else if (TIME_SIZE == 70) {
      yLine        = hlineBaseY + _space/2 - 8;
      ampmTextY    = hlineBaseY + CHARHEIGHT - 2;
      ampmTextSize = 2;
    } else if (TIME_SIZE == 52) {
      yLine        = hlineBaseY + _space/2 - 8;
      ampmTextY    = hlineBaseY + CHARHEIGHT - 8;
      ampmTextSize = 2;
    } else { // pl. 35, stb.
      yLine        = hlineBaseY + _space/2 - 8;
      ampmTextY    = hlineBaseY + CHARHEIGHT - 12;
      ampmTextSize = 2;
    }

    // vonal rajzolás + divY mentése
    gfx.drawFastHLine(_linesleft, yLine, CHARWIDTH * _superfont * 2 + _space + 15, config.theme.div);
    divY = yLine;

    // AM/PM felirat
    gfx.setCursor(_linesleft + _space + 1, ampmTextY);
    gfx.setTextSize(ampmTextSize);
    gfx.setTextColor(config.theme.dow, config.theme.background);

    char buf[3];
    strftime(buf, sizeof(buf), "%p", &network.timeinfo);
    gfx.print(buf);

} else {
    // 24 órás
    gfx.drawFastVLine(_linesleft, secBaseY, _top() - secBaseY, config.theme.div);

    const int16_t yLine = hlineBaseY + _space/2;
    gfx.drawFastHLine(_linesleft, yLine, CHARWIDTH * _superfont * 2 + _space, config.theme.div);
    divY = yLine;
}

    // dátum (ha nincs HIDE_DATE)
    formatDateCustom(_tmp, sizeof(_tmp), ti, config.store.dateFormat);
#ifndef HIDE_DATE
    strlcpy(_datebuf, utf8To(_tmp, true), sizeof(_datebuf));
    uint16_t _datewidth = strlen(_datebuf) * CHARWIDTH * _dateheight;
    gfx.setTextSize(_dateheight);
#if DSP_MODEL==DSP_GC9A01A || DSP_MODEL==DSP_GC9A01 || DSP_MODEL==DSP_GC9A01_I80
    gfx.setCursor((dsp.width() - _datewidth) / 2, _top() + _space);
#else
    gfx.setCursor(_left() + _clockwidth - _datewidth, _top() + _space);
#endif
    gfx.setTextColor(config.theme.date, config.theme.background);
    gfx.print(_datebuf);
#endif
  }
}

// ------------------------------------------------------------
// SECONDS: GFX small font + törlés a divY-ig clampelve
// ------------------------------------------------------------
if (_fullclock || _superfont > 0) {

  // seconds érték
  int sec = ti.tm_sec;
  if (_lastSec >= 0) {
    int diff = sec - _lastSec;
    if (diff < 0 && diff > -3) sec = _lastSec;
  }
  _lastSec = sec;
  sprintf(_tmp, "%02d", sec);

  const uint8_t fid = clockfont_clamp_id(config.store.clockFontId);
  const GFXfont* secFont = clockfont_get_small(fid);

  // 🔑 TIME_SIZE==19: mindig fallback (túl kicsi a glyph-özéshez)
  const bool forceFallback19 = (TIME_SIZE == 19);
  const bool useGfxSecFont   = (!forceFallback19 && secFont != nullptr);

  int16_t x = secLeftFull;
  int16_t y = secBaseY;

  // cella méret (fallback alap)
  int16_t secCellW = CHARWIDTH * _superfont * 2;

  // --- Pozíció: GFXfont esetén (CSAK ha nem TIME_SIZE==19) ---
  if (useGfxSecFont) {
    int8_t bl = clockfont_baseline_small(fid);

    // Y pozíció (baseline)
    y = secBaseY + secCellH + bl - 2;
if (config.store.hours12) {
    y -= 7;
}

    // "88" bounding box (szélesség + tbx kompenzáció)
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    gfx.setFont(secFont);
    gfx.setTextSize(1);
    gfx.getTextBounds("88", 0, 0, &tbx, &tby, &tbw, &tbh);

    // secCellW maradhat a fallback szélesség (fix cella), csak x-et számoljuk abból
    // (ha te nálad jobb, hogy tbw legyen a cella, itt átírhatod: secCellW = tbw;)
    x = secLeftFull + (secCellW - (int16_t)tbw) / 2 - tbx;
  } else {
    // --- Fallback (ide tartozik a TIME_SIZE==19 trükk) ---
    // Itt tedd meg azt az X ofszetet, ami "tökéletes lett" nálad:
    // (példa: picit balra húzom)
    if (forceFallback19) {
      x = secLeftFull + 1;   // <-- EZT az értéket hagyd azon, ami nálad bevált (pl. +1 / 0 / -1)
    }
  }

  // --- TÖRLÉS: clamp a divY-ig ---
  const int16_t clearAbove = secCellH / 2;
  const int16_t clearBelow = secCellH;

  int16_t clearTop    = secBaseY - clearAbove;
  int16_t clearBottom = secBaseY + clearBelow;

  if (divY >= 0 && clearBottom >= divY) {
    clearBottom = divY - 1;
  }
  const int16_t clearH = clearBottom - clearTop;

  if (clearH > 0) {

    // 🔑 TIME_SIZE==19 fallback: ne terjeszd balra (ne törölj bele az órába / függőleges vonalba)
    if (forceFallback19) {
      const int16_t PAD_R = 6;   // csak jobbra bővítünk
      gfx.fillRect(
        secLeftFull,
        clearTop,
        secCellW + PAD_R,
        clearH,
        config.theme.background
      );
    } else {
      // Normál (glyph vagy nem-19 fallback): eredeti, "szűk" törlés
      gfx.fillRect(
        secLeftFull-2,
        clearTop,
        secCellW+4,
        clearH,
        config.theme.background
      );
    }
  }

  // --- MONO maszk "88" (ha kell) ---
  if (config.store.clockFontMono) {
    gfx.setTextColor(config.theme.clockbg, config.theme.background);

    if (useGfxSecFont) {
      gfx.setFont(secFont);
      gfx.setTextSize(1);
      gfx.setCursor(x, y);
      gfx.print("88");
    } else {
      gfx.setFont();
      gfx.setTextSize(_superfont);
      gfx.setCursor(x, secBaseY);
      gfx.print("88");
    }
  }

  // --- valódi seconds ---
  gfx.setTextColor(
    config.theme.seconds,
    config.store.clockFontMono ? config.theme.clockbg : config.theme.background
  );

  if (useGfxSecFont) {
    gfx.setFont(secFont);
    gfx.setTextSize(1);
    gfx.setCursor(x, y);
  } else {
    gfx.setFont();
    gfx.setTextSize(_superfont);
    gfx.setCursor(x, secBaseY);
  }

  gfx.print(_tmp);
}

  applyClockFontFromConfig();
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  bool even = (ti.tm_sec % 2) == 0;
#ifndef DSP_OLED
  uint16_t bg = config.store.clockFontMono ? config.theme.clockbg : config.theme.background;
  uint16_t fg = even ? config.theme.clock : bg;
  gfx.setTextColor(fg, bg);
#else
  if(clockInTitle)
    gfx.setTextColor(even ? config.theme.meta : config.theme.metabg, config.theme.metabg);
  else
    gfx.setTextColor(even ? config.theme.clock : config.theme.background, config.theme.background);
#endif

  gfx.setCursor(_left()+_dotsleft, _top());
  gfx.print(":");
  gfx.setFont();
  if(_fb->ready()) _fb->display();
}

void ClockWidget::_clearClock(){
#ifdef PSFBUFFER
  if(_fb->ready()) _fb->clear();
  else
#endif
#ifndef CLOCKFONT5x7
  dsp.fillRect(_left(), _top()-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
#else
  dsp.fillRect(_left(), _top(), _clockwidth+1, _clockheight+1, config.theme.background);
#endif
}

void ClockWidget::draw(bool force){
  if(!_active) return;

  applyClockFontFromConfig();

  static const GFXfont* s_lastFontPtr = nullptr;
  if (Clock_GFXfontPtr != s_lastFontPtr) {
    _getTimeBounds(); 
    s_lastFontPtr = Clock_GFXfontPtr;
    _printClock(true);
  } else {
    _printClock(_getTime());
  }
}

void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}

uint16_t ClockWidget::clockWidth() const {
  return _clockwidth;
}

uint16_t ClockWidget::clockHeight() const {
  return _clockheight;
}

uint16_t ClockWidget::dateSize() const {
#ifdef HIDE_DATE
  return 0; 
#else
  return _fullclock ? (CHARHEIGHT * _dateheight) : 0;
#endif
}

void ClockWidget::_reset(){
#ifdef PSFBUFFER
  if(_fb->ready()) {
    _fb->freeBuffer();
    _getTimeBounds();
    _begin();
  }
#endif
}

void ClockWidget::_clear(){
  _clearClock();
}
#else //#ifndef DSP_LCD

void ClockWidget::_printClock(bool force){
  strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &ti);
  if(force){
    dsp.setCursor(dsp.width()-5, 0);
    dsp.print(_timebuffer);
  }
  dsp.setCursor(dsp.width()-5+2, 0);
  dsp.print((network.timeinfo.tm_sec % 2 == 0)?":":" ");
}

void ClockWidget::_clearClock(){}

void ClockWidget::draw(bool force){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_reset(){}
void ClockWidget::_clear(){}
#endif //#ifndef DSP_LCD

/**************************
      BITRATE WIDGET
 **************************/
void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor) {
    Widget::init(bconf.widget, fgcolor, bgcolor);
    _dimension = bconf.dimension;      // ez lesz a magasság
    _bitrate   = 0;
    _format    = BF_UNKNOWN;

    _charSize(bconf.widget.textsize, _charWidth, _textheight);
    memset(_buf, 0, sizeof(_buf));
}

void BitrateWidget::setBitrate(uint16_t bitrate) {
    _bitrate = bitrate;

    // Ha nagyobb mint 20000 → kbps konverzió
    if (_bitrate > 20000)
        _bitrate /= 1000;

    _draw();
}

void BitrateWidget::setFormat(BitrateFormat format) {
    _format = format;
    _draw();
}

void BitrateWidget::_charSize(uint8_t textsize, uint8_t &width, uint16_t &height) {
#ifndef DSP_LCD
    width  = textsize * CHARWIDTH;
    height = textsize * CHARHEIGHT;
#else
    width  = 1;
    height = 1;
#endif
}

void BitrateWidget::_draw() {
    _clear();

    if (!_active || _format == BF_UNKNOWN || _bitrate == 0)
        return;

    uint16_t boxW = _dimension * 2;      // teljes szélesség
    uint16_t boxH = _dimension / 2;      // magasság

    // --- Keret és kitöltés ---
    dsp.drawRect(_config.left, _config.top, boxW, boxH, _fgcolor);
    dsp.fillRect(_config.left + _dimension, _config.top, _dimension, boxH, _fgcolor);

    dsp.setFont();
    dsp.setTextSize(_config.textsize);
    dsp.setTextColor(_fgcolor, _bgcolor);

    // --- Bitrate string ---
    if (_bitrate < 1000)
        snprintf(_buf, sizeof(_buf), "%d", _bitrate);
    else
        snprintf(_buf, sizeof(_buf), "%.1f", _bitrate / 1000.0);

    // Bitrate középre → BAL blokk
    uint16_t leftX = _config.left;
    uint16_t centerX = leftX + (_dimension / 2);
    uint16_t textW = strlen(_buf) * _charWidth;
    uint16_t textH = _textheight;

    dsp.setCursor(centerX - textW/2,
                  _config.top + boxH/2 - textH/2);
    dsp.print(_buf);

    // --- Formátum → JOBB blokk ---
    dsp.setTextColor(_bgcolor, _fgcolor);

    char fmt[4] = "";
    switch(_format){
        case BF_MP3: strcpy(fmt,"MP3"); break;
        case BF_AAC: strcpy(fmt,"AAC"); break;
        case BF_FLAC:strcpy(fmt,"FLC"); break;
        case BF_OGG: strcpy(fmt,"OGG"); break;
        case BF_WAV: strcpy(fmt,"WAV"); break;
        case BF_VOR: strcpy(fmt,"VOR"); break;
        case BF_OPU: strcpy(fmt,"OPU"); break;
        default: break;
    }

    uint16_t rightCenterX = _config.left + _dimension + (_dimension/2);
    uint16_t fmtW = strlen(fmt) * _charWidth;

    dsp.setCursor(rightCenterX - fmtW/2,
                  _config.top + boxH/2 - textH/2);
    dsp.print(fmt);
}

void BitrateWidget::_clear() {
    dsp.fillRect(_config.left, _config.top,
                 _dimension * 2, _dimension / 2,
                 _bgcolor);
}

/**********************************************************************************
                                          PLAYLIST WIDGET
    **********************************************************************************/
void PlayListWidget::init(ScrollWidget* current) {
  Widget::init({0, 0, 0, WA_LEFT}, 0, 0);
  _current = current;

#ifndef DSP_LCD
  _plItemHeight = playlistConf.widget.textsize * (CHARHEIGHT - 1)
                + playlistConf.widget.textsize * 4;

  if (_plItemHeight < 10) _plItemHeight = 10;

  _plTtemsCount = (dsp.height() - 2) / _plItemHeight;
  if (_plTtemsCount < 1) _plTtemsCount = 1;
  if (_plTtemsCount > MAX_PL_PAGE_ITEMS)
      _plTtemsCount = MAX_PL_PAGE_ITEMS;

  // center alignment
  uint16_t contentHeight = _plTtemsCount * _plItemHeight;
  _plYStart = (dsp.height() - contentHeight) / 2;

  // center-scroll módhoz legyen páratlan elemszám
  if ((_plTtemsCount % 2) == 0 && _plTtemsCount > 1)
      _plTtemsCount--;

  _plCurrentPos = _plTtemsCount / 2;

  // moving cache reset
  for (int i = 0; i < MAX_PL_PAGE_ITEMS; i++)
      _plCache[i] = "";

  _plLoadedPage = -1;
  _plLastGlobalPos = -1;
  _plLastDrawTime = 0;

#else
  _plTtemsCount = PLMITEMS;
  _plCurrentPos = 1;
#endif
}

void PlayListWidget::_loadPlaylistPage(int pageIndex, int itemsPerPage) {

  for (int i = 0; i < MAX_PL_PAGE_ITEMS; i++)
      _plCache[i] = "";

  if (config.playlistLength() == 0) return;

  File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
  File index    = config.SDPLFS()->open(REAL_INDEX, "r");
  if (!playlist || !index) return;

  int startIdx = pageIndex * itemsPerPage;

  for (int i = 0; i < itemsPerPage; i++) {

    int globalIdx = startIdx + i;
    if (globalIdx >= config.playlistLength()) break;

    index.seek(globalIdx * 4, SeekSet);

    uint32_t posAddr;
    if (index.readBytes((char*)&posAddr, 4) != 4) break;

    playlist.seek(posAddr, SeekSet);

    String line = playlist.readStringUntil('\n');
    int tabIdx = line.indexOf('\t');
    if (tabIdx > 0) line = line.substring(0, tabIdx);
    line.trim();

    if (config.store.numplaylist && line.length() > 0)
      _plCache[i] = String(globalIdx + 1) + " " + line;
    else
      _plCache[i] = line;
  }

  playlist.close();
  index.close();
}

void PlayListWidget::drawPlaylist(uint16_t currentItem) {

#ifndef DSP_LCD
  if (config.store.playlistMode == 1) {
    _drawMovingCursor(currentItem);
  } else {
    _drawScrollCenter(currentItem);
  }
#else
  dsp.clear();
  _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
  dsp.setCursor(0, 1);
  dsp.write(uint8_t(126));
#endif
}

void PlayListWidget::_drawMovingCursor(uint16_t currentItem) {

  bool isLongPause = (millis() - _plLastDrawTime > 2000);
  _plLastDrawTime = millis();

  int activeIdx   = (currentItem > 0) ? (currentItem - 1) : 0;
  int itemsPerPage = _plTtemsCount;

  int newPage     = activeIdx / itemsPerPage;
  int newLocalPos = activeIdx % itemsPerPage;

  _plCurrentPos = newLocalPos;

  bool pageChanged = (newPage != _plLoadedPage);

  if (pageChanged || isLongPause || _plCache[0].length() == 0) {

    if (pageChanged || _plCache[0].length() == 0) {
      _loadPlaylistPage(newPage, itemsPerPage);
      _plLoadedPage = newPage;
    }

    dsp.fillRect(0, _plYStart,
                 dsp.width(),
                 itemsPerPage * _plItemHeight,
                 config.theme.background);

    for (int i = 0; i < itemsPerPage; i++)
      _printMoving(i, _plCache[i].c_str());

  } else {

    int oldLocalPos = -1;
    if (_plLastGlobalPos > 0) {
       int oldIdx = _plLastGlobalPos - 1;
       oldLocalPos = oldIdx % itemsPerPage;
    }

    if (oldLocalPos >= 0 &&
        oldLocalPos < itemsPerPage &&
        oldLocalPos != newLocalPos) {

        _printMoving(oldLocalPos, _plCache[oldLocalPos].c_str());
    }

    _printMoving(newLocalPos, _plCache[newLocalPos].c_str());
  }

  _plLastGlobalPos = currentItem;
}

void PlayListWidget::_printMoving(uint8_t pos, const char* item) {

  if (pos >= _plTtemsCount) return;

  int16_t yPos = _plYStart + pos * _plItemHeight;
  bool isSelected = (pos == _plCurrentPos);

  uint16_t fgColor = isSelected
        ? config.theme.plcurrent
        : config.theme.playlist[0];

  uint16_t bgColor = config.theme.background;

  dsp.fillRect(0, yPos, dsp.width(),
               _plItemHeight - 1,
               bgColor);

  if (item && item[0] != '\0') {
    dsp.setTextColor(fgColor, bgColor);
    dsp.setTextSize(playlistConf.widget.textsize);
    dsp.setCursor(TFT_FRAMEWDT, yPos + 4);
    dsp.print(utf8To(item, true));
  }
}

void PlayListWidget::_drawScrollCenter(uint16_t currentItem) {

  uint8_t lastPos =
    _fillPlMenu((int)currentItem - _plCurrentPos,
                _plTtemsCount);

  if (lastPos < _plTtemsCount) {
    dsp.fillRect(0,
                 lastPos * _plItemHeight + _plYStart,
                 dsp.width(),
                 dsp.height() / 2,
                 config.theme.background);
  }
}

void PlayListWidget::_printScroll(uint8_t pos,
                                  const char* item) {

  dsp.setTextSize(playlistConf.widget.textsize);

  if (pos == _plCurrentPos) {
    _current->setText(item);
  } else {

    uint8_t dist =
      abs((int)pos - (int)_plCurrentPos);

    uint8_t plColor =
      (dist > 5) ? 4 : (dist - 1);

    dsp.setTextColor(
      config.theme.playlist[plColor],
      config.theme.background);

    dsp.setCursor(TFT_FRAMEWDT,
                  _plYStart + pos * _plItemHeight);

    dsp.fillRect(0,
                 _plYStart + pos * _plItemHeight - 1,
                 dsp.width(),
                 _plItemHeight - 2,
                 config.theme.background);

    dsp.print(utf8To(item, true));
  }
}

uint8_t PlayListWidget::_fillPlMenu(int from, uint8_t count) {
    int     ls = from;
    uint8_t c = 0;
    bool    finded = false;
    if (config.playlistLength() == 0) { return 0; }
    File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
    File index = config.SDPLFS()->open(REAL_INDEX, "r");
    while (true) {
        if (ls < 1) {
            ls++;
            _printScroll(c, "");
            c++;
            continue;
        }
        if (!finded) {
            index.seek((ls - 1) * 4, SeekSet);
            uint32_t pos;
            index.readBytes((char*)&pos, 4);
            finded = true;
            index.close();
            playlist.seek(pos, SeekSet);
        }
        bool pla = true;
        while (pla) {
            pla = playlist.available();
            String stationName = playlist.readStringUntil('\n');
            stationName = stationName.substring(0, stationName.indexOf('\t'));
            if (config.store.numplaylist && stationName.length() > 0) { stationName = String(from + c) + " " + stationName; }
            _printScroll(c, stationName.c_str());
            c++;
            if (c >= count) { break; }
        }
        break;
    }
    playlist.close();
    return c;
}
/**************************
      DATE WIDGET
 **************************/
void DateWidget::update() {
  if (!_active) return;

  time_t now = time(nullptr);
  struct tm ti;
  localtime_r(&now, &ti);

  char dateBuf[64];
  uint8_t fmt = config.store.dateFormat;
  #if (DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ILI9341)
    if (fmt > 1) {
      fmt = 0; 
    }
  #endif

  formatDateCustom(dateBuf, sizeof(dateBuf), ti, fmt);

  uint8_t charWidth =
  #ifndef DSP_LCD
      (_config.textsize * CHARWIDTH);
  #else
      1;
  #endif

  auto utf8len = [](const char* s) -> size_t {
    size_t len = 0;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
      if ( (*p & 0xC0) != 0x80 ) ++len;
    return len;
  };

  uint16_t desired = (uint16_t)(utf8len(dateBuf) * charWidth);
  desired += charWidth * 2; 

  uint16_t minW = charWidth * 6;
  uint16_t maxW = dsp.width() > 10 ? dsp.width() - 10 : dsp.width();
  if (desired < minW) desired = minW;
  if (desired > maxW) desired = maxW;

#if DSP_MODEL == DSP_ST7789_76
  setWindowWidth(dsp.width() / 2 + 23);
#else
  setWindowWidth(desired);
#endif

  char line[192];
  strlcpy(line, dateBuf, sizeof(line));

#ifdef NAMEDAYS_FILE
  if (config.store.showNameday) {
    char nd[160] = {0};
    if (namedays_get_str((uint8_t)ti.tm_mon + 1, (uint8_t)ti.tm_mday, nd, sizeof(nd)) && nd[0]) {
      strlcat(line, " \035 ", sizeof(line));
      strlcat(line, nd,    sizeof(line));
    }
  }
#endif

  setText(utf8To(line, false));
}

/************************
   WEATHER ICON WIDGET
 ************************/

void WeatherIconWidget::init(WidgetConfig wconf, uint16_t bgcolor){
  // fgcolor itt nem kell, az ikon egy bitmap
  Widget::init(wconf, 0, bgcolor);
  _img = nullptr;
  _iw  = 0;
  _ih  = 0;
}

void WeatherIconWidget::_clear(){
  if (!_active) return;
  uint16_t w = _width  ? _width  : (_iw ? _iw : 64);
  uint16_t h = _ih ? _ih : 64;
  dsp.fillRect(_config.left, _config.top, w, h, _bgcolor);
}

void WeatherIconWidget::_draw(){
  if(!_active) return;

  uint16_t w = _iw ? _iw : 64;
  uint16_t h = _ih ? _ih : 64;
  uint16_t areaW = _width ? _width : w;
  
  int16_t iconX = _config.left;
  int16_t iconY = _config.top;

  // ikon kirajzolás
  if (_img) dsp.drawRGBBitmap(iconX, iconY, _img, w, h);

  // ha nincs hőmérséklet → nincs további rajz
  if (_temp[0] == '\0') return;

  dsp.setFont();
  dsp.setTextSize(_config.textsize);
  dsp.setTextColor(config.theme.dow, _bgcolor);

  uint16_t txtW = strlen(_temp) * CHARWIDTH * _config.textsize;
  uint16_t txtH = CHARHEIGHT * _config.textsize;

  // 🔻 **Layout logika**
  bool vertical = (config.store.vuLayout == 0);  
  // default layout = vertical (egymás alatt)  
  // többi layout = side-by-side

  if (vertical) {
      // ===========================
      // ikon fölé szöveg (Default)
      // ===========================
      int16_t tx = _config.left + (areaW - txtW) / 2;
      int16_t ty = _config.top - (txtH-2);
      dsp.setCursor(tx, ty);
      dsp.print(_temp);
  } else {
      // ===========================
      // ikon mellé szöveg (StreamLine, BoomBox, Studio)
      // ===========================
      int16_t tx = iconX + w + 6;
      int16_t ty = iconY + (h - txtH) / 2;

      dsp.setCursor(tx, ty);
      dsp.print(_temp);
  }
}

void WeatherIconWidget::setIcon(const char* code) {
  const IconSet& set = currentIconSet();

  WeatherIconId id = iconIdFromCode(code);

  if (id == WI_UNKNOWN) {
    _img = set.fallback;
  } else {
    _img = set.icons[(int)id];
  }

  _iw = set.w;
  _ih = set.h;

  if (_active && !_locked) {
    _clear();
    _draw();
  }
}

void WeatherIconWidget::setTemp(float tempC) {
#ifdef IMPERIALUNIT
  snprintf(_temp, sizeof(_temp), "%.0f\011F", tempC);
#else
  snprintf(_temp, sizeof(_temp), "%.0f\011C", tempC);
#endif

  _hasTemp = true;

  if (_active && !_locked) {
    _clear();
    _draw();
  }
}

#endif // #if DSP_MODEL!=DSP_DUMMY
