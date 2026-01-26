#include "../core/options.h"
#ifdef USE_DLNA

#include "dlna_service.h"
#include "dlna_ssdp.h"
#include "dlna_desc.h"
#include "dlna_index.h"
#include "dlna_worker.h"
#include "../core/network.h"
#include "../core/player.h"

String g_dlnaControlUrl;

static bool s_serviceStarted = false;
static uint32_t s_reqId = 1;

bool dlnaInit(const String& rootObjectId, String& err) {

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

void dlna_service_begin() {
  if (s_serviceStarted) return;
  s_serviceStarted = true;

  dlna_worker_start();
}

uint32_t dlna_next_reqId() {
  uint32_t v = s_reqId++;
  if (v == 0) v = s_reqId++; // 0 ne legyen
  return v;
}

bool dlna_isBusy() {
  return g_dlnaStatus.busy;
}

#endif   // USE_DLNA
