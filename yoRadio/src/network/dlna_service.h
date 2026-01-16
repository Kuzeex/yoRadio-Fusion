#pragma once
#include "../core/options.h"
#include <Arduino.h>

#ifdef USE_DLNA

bool dlnaInit(const String& rootObjectId, String& err);
bool dlnaList(const String& objectId, String& outJson, String& err);
bool dlnaBuild(const String& objectId, bool activate, int& count, String& err);

extern String g_dlnaControlUrl;

#endif // USE_DLNA
