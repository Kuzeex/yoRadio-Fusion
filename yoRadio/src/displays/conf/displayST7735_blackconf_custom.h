/*************************************************************************************
    ST7735 160x128 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7735conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/
#ifndef displayST7735conf_h
#define displayST7735conf_h

#include "../../core/config.h"

#define DSP_WIDTH       160
#define TFT_FRAMEWDT    4
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#if BITRATE_FULL
  #define TITLE_FIX 24
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     34

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 26, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 36, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 56, 1, WA_LEFT }, 140, true, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 128-TFT_FRAMEWDT-8, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 3, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 42, 1, WA_LEFT }, 140, true, MAX_WIDTH, 0, 2, 50 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig   metaBGConfLine PROGMEM = {{ 1, 21, 0, WA_CENTER }, DSP_WIDTH - 2, 1, true };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 22, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 118, 0, WA_LEFT }, MAX_WIDTH, 5, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 52, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 127, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 110, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 26, 1, WA_RIGHT };
//const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 99, 1, WA_LEFT };
const WidgetConfig voltxtConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_LEFT };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_CENTER };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 86, 0, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 0, 40, 1, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { 0, 54, 1, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { 0, 74, 1, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { 0, 88, 1, WA_CENTER };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 90, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };

//const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-19, 23, 1, WA_LEFT}, 22 };
inline BitrateConfig getfullbitrateConf() {
  switch (config.store.vuLayout) {
    case 1: return {{TFT_FRAMEWDT, 65, 1, WA_RIGHT}, 26 };  // Streamline
    case 2: return {{TFT_FRAMEWDT, 65, 1, WA_RIGHT}, 26 };  // Boombox
    case 3: return {{TFT_FRAMEWDT, 65, 1, WA_RIGHT}, 26 };  // Studio
    default: return {{DSP_WIDTH-TFT_FRAMEWDT-52, 58, 1, WA_LEFT}, 26 }; // Default
  }
}

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
//const VUBandsConfig bandsConf     PROGMEM = { 12, 50, 2, 1, 10, 1 };
inline VUBandsConfig getbandsConf() {
  switch (config.store.vuLayout) {
    case 1: return { 73, config.store.vuBarHeightStr, 6, config.store.vuBarGapStr,  config.store.vuBarCountStr,  config.store.vuFadeSpeedStr };  // Streamline
    case 2: return { 73, config.store.vuBarHeightBbx, 6, config.store.vuBarGapBbx,  config.store.vuBarCountBbx,  config.store.vuFadeSpeedBbx };  // Boombox
    case 3: return { 149, config.store.vuBarHeightStd, config.store.vuBarGapStd, 3, config.store.vuBarCountStd,  config.store.vuFadeSpeedStd };  // Studio
    default: return { config.store.vuBarHeightDef, 47, 2, config.store.vuBarGapDef,  config.store.vuBarCountDef,  config.store.vuFadeSpeedDef }; // Default
  }
}

//const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 54, 1, WA_LEFT };
inline WidgetConfig getvuConf() {
  switch (config.store.vuLayout) {
    case 1: return { TFT_FRAMEWDT, 95, 1, WA_CENTER };  // Streamline
    case 2: return { TFT_FRAMEWDT, 95, 1, WA_CENTER };  // Boombox
    case 3: return { TFT_FRAMEWDT, 95, 1, WA_CENTER };  // Studio
    default: return { TFT_FRAMEWDT, 58, 1, WA_LEFT }; // Default
  }
}

//const WidgetConfig  clockConf     PROGMEM = { 0, 94, 0, WA_CENTER };
inline WidgetConfig getclockConf() {
  switch (config.store.vuLayout) {
    case 1: return { 8, 80, 0, WA_RIGHT };  // Streamline
    case 2: return { 8, 80, 0, WA_RIGHT };  // Boombox
    case 3: return { 8, 80, 0, WA_RIGHT };  // Studio
    default: return { 4, 93, 0, WA_RIGHT }; // Default
  }
}

//const ScrollConfig dateConf       PROGMEM = {{ DSP_WIDTH-TFT_FRAMEWDT, 185, 2, WA_RIGHT }, 128, false, 220, 5000, 1, 50};
static constexpr ScrollConfig kDateBase = {
  { TFT_FRAMEWDT, 95, 1, WA_RIGHT }, 128, false, 220, 5000, 1, 50 };

static inline uint16_t dateTopByLayout(uint8_t ly) {
  switch (ly) {
    case 1:  return 85;  // StreamLine
    case 2:  return 85;  // BoomBox
    case 3:  return 85;  // Studio
    default: return 97;  // Default
  }
}

static inline uint16_t dateLeftByLayout(uint8_t ly) {
  switch (ly) {
    case 1:  return  8;              // StreamLine
    case 2:  return  8;              // BoomBox
    case 3:  return  8;              // Studio
    default: return  TFT_FRAMEWDT;              // Default
  }
}

inline ScrollConfig getDateConf(uint8_t ly) {
  ScrollConfig c = kDateBase;
  c.widget.top   = dateTopByLayout(ly);
  c.widget.left  = dateLeftByLayout(ly);
  return c;
}

inline ScrollConfig getDateConf() {
  return getDateConf(config.store.vuLayout);
}

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "%d";
const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char        bitrateFmt[]    PROGMEM = "%d";

/* MOVES  */                             /* { left, top, width } */
//const MoveConfig    clockMove     PROGMEM = { 14, 94, 0};
inline MoveConfig getclockMove() {
  switch (config.store.vuLayout) {
    case 1: return { 8, 80, -1 };  // Streamline
    case 2: return { 8, 80, -1 };  // BoomBox
    case 3: return { 8, 80, -1 };  // Studio
    default: return { 4, 93, -1 }; // Default
  }
}

inline MoveConfig getdateMove() {
  const ScrollConfig& dc = getDateConf();
  const auto cc = getclockConf(); 
  const auto cm = getclockMove();
  const int16_t dy = (int16_t)cm.y - (int16_t)cc.top;
  const int16_t dx = (int16_t)cm.x - (int16_t)cc.left;

  MoveConfig m;
  m.x     = (uint16_t)((int16_t)dc.widget.left + dx);
  m.y     = (uint16_t)((int16_t)dc.widget.top + dy);
  m.width = dc.width;
  return m;
}

/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock } */
const MoveConfig   weatherMove    PROGMEM = {TFT_FRAMEWDT, 48, MAX_WIDTH};
const MoveConfig   weatherMoveVU  PROGMEM = {TFT_FRAMEWDT, 48, MAX_WIDTH};

#endif
