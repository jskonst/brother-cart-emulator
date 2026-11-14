#include "pattern_editor.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

#include <cstring>

#include "pattern_page.h"

PatternEditor patternEditor;

static AsyncWebServer server(80);
static const char* AP_SSID = "CK35-Editor";
static const char* AP_PASS = "ck35-editor";

static uint8_t bodyBuf[48 * 1024];
static size_t bodyLen = 0;
static bool bodyOverflow = false;

void todoEncodeCk35(const uint8_t* json, size_t len) {
  // TODO(CK-35 encoder): json is the editor document (version 1), not a
  // Brother Pattern Memory image. A real encoder has to wait for a read-only
  // dump of an original CK-35 cartridge. Do not write a 32 KiB placeholder,
  // do not guess the directory, the block table, or the bitmap packing.
  (void)json;
  (void)len;
}

static bool validId(const char* id) {
  size_t n = strlen(id);
  if (n < 1 || n > 24) return false;
  for (size_t i = 0; i < n; i++) {
    char c = id[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!ok) return false;
  }
  return true;
}

static bool extractId(const uint8_t* data, size_t len, char* out, size_t outCap) {
  static const char key[] = "\"id\"";
  for (size_t i = 0; i + 4 < len; i++) {
    if (memcmp(data + i, key, 4) != 0) continue;
    size_t j = i + 4;
    while (j < len && (data[j] == ' ' || data[j] == '\t' || data[j] == '\n' ||
                       data[j] == '\r' || data[j] == ':')) {
      j++;
    }
    if (j >= len || data[j] != '"') continue;
    j++;
    size_t k = 0;
    while (j < len && data[j] != '"' && k + 1 < outCap) {
      out[k++] = (char)data[j++];
    }
    if (j >= len || data[j] != '"') return false;
    out[k] = 0;
    return true;
  }
  return false;
}

static String patternPath(const char* id) {
  return String("/patterns/") + id + ".json";
}

static void onBody(AsyncWebServerRequest* request, uint8_t* data, size_t len,
                   size_t index, size_t total) {
  (void)request;
  if (index == 0) {
    bodyLen = 0;
    bodyOverflow = false;
  }
  if (total > sizeof(bodyBuf) || bodyLen + len > sizeof(bodyBuf)) {
    bodyOverflow = true;
    return;
  }
  memcpy(bodyBuf + bodyLen, data, len);
  bodyLen += len;
}

static void handleSave(AsyncWebServerRequest* request) {
  if (bodyOverflow || bodyLen == 0 || bodyBuf[0] != '{') {
    request->send(400, "text/plain", "need a JSON object under 48 KiB");
    return;
  }
  char id[25];
  if (!extractId(bodyBuf, bodyLen, id, sizeof(id)) || !validId(id)) {
    request->send(400, "text/plain", "id must be 1-24 of [A-Za-z0-9_-]");
    return;
  }
  File file = LittleFS.open(patternPath(id), "w");
  if (!file) {
    request->send(500, "text/plain", "LittleFS open failed");
    return;
  }
  file.write(bodyBuf, bodyLen);
  file.close();
  todoEncodeCk35(bodyBuf, bodyLen);
  request->send(200, "application/json", "{\"ok\":true}");
}

static void handleLoad(AsyncWebServerRequest* request) {
  if (!request->hasParam("id")) {
    request->send(400, "text/plain", "missing id");
    return;
  }
  String id = request->getParam("id")->value();
  if (!validId(id.c_str())) {
    request->send(400, "text/plain", "bad id");
    return;
  }
  String path = patternPath(id.c_str());
  if (!LittleFS.exists(path)) {
    request->send(404, "text/plain", "missing");
    return;
  }
  request->send(LittleFS, path, "application/json");
}

static void handleDelete(AsyncWebServerRequest* request) {
  if (!request->hasParam("id")) {
    request->send(400, "text/plain", "missing id");
    return;
  }
  String id = request->getParam("id")->value();
  if (!validId(id.c_str())) {
    request->send(400, "text/plain", "bad id");
    return;
  }
  String path = patternPath(id.c_str());
  if (!LittleFS.exists(path) || !LittleFS.remove(path)) {
    request->send(404, "text/plain", "missing");
    return;
  }
  request->send(200, "application/json", "{\"ok\":true}");
}

static void handleList(AsyncWebServerRequest* request) {
  String out = "[";
  File dir = LittleFS.open("/patterns");
  bool first = true;
  if (dir) {
    File file = dir.openNextFile();
    while (file) {
      String name = file.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (name.endsWith(".json")) {
        name.remove(name.length() - 5);
        if (validId(name.c_str())) {
          if (!first) out += ',';
          first = false;
          out += '"';
          out += name;
          out += '"';
        }
      }
      file = dir.openNextFile();
    }
  }
  out += ']';
  request->send(200, "application/json", out);
}

void PatternEditor::begin() {
  String staSsid;
  String staPass;
  bool useSta = false;
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS failed, editor cannot store patterns");
  } else {
    if (!LittleFS.exists("/patterns")) LittleFS.mkdir("/patterns");
    File sta = LittleFS.open("/sta.txt", "r");
    if (sta) {
      staSsid = sta.readStringUntil('\n');
      staPass = sta.readStringUntil('\n');
      sta.close();
      staSsid.trim();
      staPass.trim();
      useSta = staSsid.length() > 0;
    }
  }

  WiFi.mode(useSta ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  if (useSta) {
    WiFi.begin(staSsid.c_str(), staPass.c_str());
    Serial.println("STA requested from /sta.txt");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", pattern_index_html);
  });
  server.on("/api/patterns", HTTP_GET, handleList);
  server.on("/api/pattern", HTTP_GET, handleLoad);
  server.on("/api/pattern", HTTP_DELETE, handleDelete);
  server.on("/api/pattern", HTTP_POST, handleSave, nullptr, onBody);
  server.begin();

  Serial.print("CK35 editor at http://192.168.4.1/  AP ");
  Serial.print(AP_SSID);
  Serial.print(" password ");
  Serial.println(AP_PASS);
}

void PatternEditor::loop() {}
