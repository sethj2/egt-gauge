// Pull-style OTA from GitHub Releases:
//   https://github.com/<repo>/releases/latest/download/version.txt   (plain text, e.g. "0.2.0")
//   https://github.com/<repo>/releases/latest/download/firmware.bin  (app-only image)
// NOTE: TLS certificates are not validated (setInsecure), so this protects against casual
// interference only, not a hostile network. Fine for a bench prototype.
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include "ota.h"
#include "version.h"
// release.ps1 creates src/release_build.h (gitignored) for the duration of a release build.
#if __has_include("release_build.h")
#define RELEASE_BUILD 1
#endif
// Release builds must NOT contain the Wi-Fi password, because
// firmware.bin is published on a public GitHub release. Instead, a USB/dev build copies the
// credentials from secrets.h into NVS, and every build reads them back from there. NVS survives OTA.
#ifndef RELEASE_BUILD
#include "secrets.h"
#endif

#define RELEASE_URL "https://github.com/" GITHUB_REPO "/releases/latest/download/"

static OtaStatusFn say;

static void report(const char *fmt, ...) {
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.println(buf);
  if (say) say(buf);
}

static bool connect_wifi() {
  Preferences prefs;
  prefs.begin("wifi", false);
#ifndef RELEASE_BUILD
  prefs.putString("ssid", WIFI_SSID);
  prefs.putString("pass", WIFI_PASS);
#endif
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();
  if (ssid.isEmpty()) {
    report("No Wi-Fi saved. Flash by USB");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {  // ~20 s
    if (i % 4 == 0) report("Connecting to Wi-Fi...");
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

static void on_progress(int cur, int total) {
  static int lastPct = -1;
  int pct = total > 0 ? (int)((int64_t)cur * 100 / total) : 0;
  if (pct != lastPct) {
    lastPct = pct;
    report("Downloading %d%%", pct);
  }
}

void ota_run(OtaStatusFn status) {
  say = status;

  if (!connect_wifi()) {
    report("Wi-Fi failed. Hotspot on?");
    WiFi.mode(WIFI_OFF);
    return;
  }

  report("Checking for update...");
  String latest;
  {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(10000);
    if (!http.begin(client, RELEASE_URL "version.txt")) {
      report("Bad update URL");
      WiFi.mode(WIFI_OFF);
      return;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
      report("No release found (HTTP %d)", code);
      http.end();
      WiFi.mode(WIFI_OFF);
      return;
    }
    latest = http.getString();
    http.end();
  }
  latest.trim();

  if (latest == FW_VERSION) {
    report("Up to date (v%s)", FW_VERSION);
    WiFi.mode(WIFI_OFF);
    return;
  }

  report("Updating v%s -> v%s", FW_VERSION, latest.c_str());
  WiFiClientSecure client;
  client.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);  // does not return on success
  httpUpdate.onProgress(on_progress);
  t_httpUpdate_return r = httpUpdate.update(client, RELEASE_URL "firmware.bin");
  if (r == HTTP_UPDATE_FAILED) report("Update failed: %s", httpUpdate.getLastErrorString().c_str());
  else                         report("No update applied");
  WiFi.mode(WIFI_OFF);
}
