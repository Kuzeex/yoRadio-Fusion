#include "../core/options.h"
#ifdef USE_DLNA

#include "dlna_service.h"
#include "dlna_ssdp.h"
#include "dlna_desc.h"
#include "dlna_index.h"
#include "../core/network.h"
#include "../core/player.h"

String g_dlnaControlUrl;

bool dlnaInit(const String& rootObjectId, String& err) {

  if (network.status != CONNECTED) {
    err = "WiFi not connected";
    return false;
  }

  player.sendCommand({PR_STOP, 0});

  DlnaSSDP ssdp;
  DlnaDescription desc;
  DlnaIndex idx;

  String descUrl;
  String controlUrl;

  if (!ssdp.resolve(dlnaHost, descUrl)) {
    err = "SSDP discover failed";
    return false;
  }

  bool cdOk = false;
  for (int i = 0; i < 3; i++) {
    if (desc.resolveControlURL(descUrl, controlUrl)) {
      cdOk = true;
      break;
    }
    delay(500);
  }

  if (!cdOk) {
    err = "ContentDirectory not found";
    return false;
  }

  g_dlnaControlUrl = controlUrl;

  if (!idx.buildContainerIndex(controlUrl, rootObjectId)) {
    err = "Root container browse failed";
    return false;
  }

  return true;
}

#endif   // USE_DLNA
