#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include "wifi_secrets.h"

// ─── Configuration ───────────────────────────────────────────────────────────
static const int    SERVO_PIN    = 27;          // GPIO27 – PWM-capable
static const int    DEFAULT_OPEN = 106;         // default open  angle (degrees)
static const int    DEFAULT_CLOSE = 74;         // default close angle (degrees)
// ──────────────────────────────────────────────────────────────────────────────

static Servo       servo;
static WebServer   server(80);
static Preferences prefs;

static int  openAngle   = DEFAULT_OPEN;
static int  closeAngle  = DEFAULT_CLOSE;
static bool isOpen      = false;               // tracks current shutter state
static unsigned long snapshotCloseTime = 0;     // 0 = no pending auto-close

// ─── NVS helpers ─────────────────────────────────────────────────────────────
static const char* NVS_NAMESPACE = "shutter";
static const char* KEY_OPEN      = "openAngle";
static const char* KEY_CLOSE     = "closeAngle";

static void loadSettings() {
    prefs.begin(NVS_NAMESPACE, true);          // read-only
    openAngle  = prefs.getInt(KEY_OPEN,  DEFAULT_OPEN);
    closeAngle = prefs.getInt(KEY_CLOSE, DEFAULT_CLOSE);
    prefs.end();
    Serial.printf("Loaded settings — open: %d°, close: %d°\n", openAngle, closeAngle);
}

static void saveSettings() {
    prefs.begin(NVS_NAMESPACE, false);         // read-write
    prefs.putInt(KEY_OPEN,  openAngle);
    prefs.putInt(KEY_CLOSE, closeAngle);
    prefs.end();
    Serial.printf("Saved settings — open: %d°, close: %d°\n", openAngle, closeAngle);
}

// ─── HTML page (embedded raw string) ─────────────────────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Camera Shutter Controller</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: #1a1a2e; color: #e0e0e0;
    display: flex; justify-content: center; align-items: center;
    min-height: 100vh; padding: 20px;
  }
  .card {
    background: #16213e; border-radius: 16px; padding: 32px;
    max-width: 400px; width: 100%;
    box-shadow: 0 8px 32px rgba(0,0,0,0.4);
  }
  h1 { text-align: center; font-size: 1.4rem; margin-bottom: 20px; color: #e94560; }
  .status {
    text-align: center; font-size: 1.1rem; margin-bottom: 24px;
    padding: 12px; border-radius: 8px; background: #0f3460;
  }
  .status .pos { font-weight: bold; color: #e94560; }
  .field { margin-bottom: 14px; }
  .field label {
    display: block; font-size: 0.85rem; color: #aaa;
    margin-bottom: 4px;
  }
  .field input {
    width: 100%; padding: 10px 12px; border: 1px solid #0f3460;
    border-radius: 8px; background: #1a1a2e; color: #e0e0e0;
    font-size: 1rem; outline: none;
  }
  .field input:focus { border-color: #e94560; }
  .btn-row { display: flex; gap: 12px; margin-top: 20px; }
  .btn {
    flex: 1; padding: 14px; border: none; border-radius: 10px;
    font-size: 1rem; font-weight: bold; cursor: pointer;
    transition: opacity 0.2s;
  }
  .btn:hover { opacity: 0.85; }
  .btn:active { transform: scale(0.97); }
  .btn-open  { background: #e94560; color: #fff; }
  .btn-close { background: #0f3460; color: #fff; }
  .btn-save  { background: #533483; color: #fff; margin-top: 12px; width: 100%; }
  .btn-snap  { background: #1a936f; color: #fff; margin-top: 12px; width: 100%; }
  .snap-row  { display: flex; gap: 12px; margin-top: 12px; align-items: stretch; }
  .snap-row .field { margin-bottom: 0; flex: 1; }
  .snap-row .btn { flex: 1; margin: 0; }
  .ip { text-align: center; margin-top: 16px; font-size: 0.8rem; color: #555; }
  .toast {
    display: none; position: fixed; bottom: 30px; left: 50%;
    transform: translateX(-50%); background: #533483; color: #fff;
    padding: 12px 24px; border-radius: 8px; font-size: 0.9rem;
    box-shadow: 0 4px 16px rgba(0,0,0,0.4);
  }
  .toast.show { display: block; animation: fade 2s forwards; }
  @keyframes fade { 0%{opacity:1} 70%{opacity:1} 100%{opacity:0} }
</style>
</head>
<body>
<div class="card">
  <h1>📷 Camera Shutter</h1>
  <div class="status" id="status">Shutter: <span class="pos" id="posText">---</span></div>

  <div class="field">
    <label for="openAngle">Open angle (degrees)</label>
    <input type="number" id="openAngle" min="0" max="180" value="%d">
  </div>
  <div class="field">
    <label for="closeAngle">Close angle (degrees)</label>
    <input type="number" id="closeAngle" min="0" max="180" value="%d">
  </div>

  <button class="btn btn-save" onclick="saveSettings()">Save Settings</button>

  <div class="btn-row">
    <button class="btn btn-open"  onclick="moveServo('open')">OPEN</button>
    <button class="btn btn-close" onclick="moveServo('close')">CLOSE</button>
  </div>

  <div class="snap-row">
    <div class="field">
      <label for="snapDuration">Duration (ms)</label>
      <input type="number" id="snapDuration" min="1" max="60000" value="500">
    </div>
    <button class="btn btn-snap" onclick="takeSnapshot()">SNAPSHOT</button>
  </div>

  <div class="ip">IP: %s</div>
</div>
<div class="toast" id="toast"></div>

<script>
function showToast(msg) {
  var t = document.getElementById('toast');
  t.textContent = msg; t.className = 'toast show';
  setTimeout(function(){ t.className = 'toast'; }, 2000);
}

function moveServo(dir) {
  fetch('/' + dir, {method:'POST'})
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('posText').textContent = d.position + ' (' + d.angle + '\u00B0)';
      showToast(d.position === 'OPEN' ? 'Shutter opened' : 'Shutter closed');
    })
    .catch(function(){ showToast('Error communicating with ESP32'); });
}

function saveSettings() {
  var o = parseInt(document.getElementById('openAngle').value);
  var c = parseInt(document.getElementById('closeAngle').value);
  if (isNaN(o) || isNaN(c) || o < 0 || o > 180 || c < 0 || c > 180) {
    showToast('Angles must be 0\u2013180'); return;
  }
  fetch('/save?open=' + o + '&close=' + c, {method:'POST'})
    .then(function(r){ return r.json(); })
    .then(function(d){ showToast(d.ok ? 'Settings saved' : 'Save failed'); })
    .catch(function(){ showToast('Error saving settings'); });
}

function takeSnapshot() {
  var dur = parseInt(document.getElementById('snapDuration').value);
  if (isNaN(dur) || dur < 1) { showToast('Enter a valid duration (ms)'); return; }
  document.getElementById('posText').textContent = 'OPENING...';
  fetch('/snapshot?duration=' + dur, {method:'POST'})
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('posText').textContent = d.position + ' (' + d.angle + '\u00B0)';
      showToast('Snapshot: open for ' + dur + ' ms');
    })
    .catch(function(){ showToast('Error during snapshot'); });
}
</script>
</body>
</html>
)rawliteral";

// ─── Route handlers ──────────────────────────────────────────────────────────
static void handleRoot() {
    char buf[sizeof(HTML_PAGE) + 64];
    snprintf(buf, sizeof(buf), HTML_PAGE, openAngle, closeAngle, WiFi.localIP().toString().c_str());
    server.send(200, "text/html", buf);
}

static void handleOpen() {
    servo.write(openAngle);
    isOpen = true;
    Serial.printf("-> OPEN  (%d°)\n", openAngle);
    String json = "{\"position\":\"OPEN\",\"angle\":" + String(openAngle) + "}";
    server.send(200, "application/json", json);
}

static void handleClose() {
    servo.write(closeAngle);
    isOpen = false;
    Serial.printf("-> CLOSE (%d°)\n", closeAngle);
    String json = "{\"position\":\"CLOSED\",\"angle\":" + String(closeAngle) + "}";
    server.send(200, "application/json", json);
}

static void handleSave() {
    if (server.hasArg("open") && server.hasArg("close")) {
        int o = server.arg("open").toInt();
        int c = server.arg("close").toInt();
        if (o >= 0 && o <= 180 && c >= 0 && c <= 180) {
            openAngle  = o;
            closeAngle = c;
            saveSettings();
            server.send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    server.send(400, "application/json", "{\"ok\":false}");
}

static void handleSnapshot() {
    if (!server.hasArg("duration")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing duration\"}");
        return;
    }
    long dur = server.arg("duration").toInt();
    if (dur < 1) dur = 500;                     // sensible default

    servo.write(openAngle);
    isOpen = true;
    snapshotCloseTime = millis() + (unsigned long)dur;
    Serial.printf("-> SNAPSHOT open (%d°), auto-close in %ld ms\n", openAngle, dur);

    String json = "{\"position\":\"OPEN\",\"angle\":" + String(openAngle) + ",\"duration\":" + String(dur) + "}";
    server.send(200, "application/json", json);
}

// ─── setup & loop ────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);

    // Servo
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, 500, 2400);

    // Load persisted angles
    loadSettings();

    // Connect to WiFi
    Serial.printf("Connecting to WiFi SSID: %s ", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.printf("Connected! IP address: %s\n", WiFi.localIP().toString().c_str());

    // HTTP routes
    server.on("/",     HTTP_GET,  handleRoot);
    server.on("/open", HTTP_POST, handleOpen);
    server.on("/close",HTTP_POST, handleClose);
    server.on("/save",     HTTP_POST, handleSave);
    server.on("/snapshot", HTTP_POST, handleSnapshot);
    server.begin();
    Serial.println("Web server started");

    // Move to closed position on boot
    servo.write(closeAngle);
    isOpen = false;
    Serial.printf("Initial position: CLOSED (%d°)\n", closeAngle);
}

void loop() {
    server.handleClient();

    // Non-blocking snapshot auto-close
    if (snapshotCloseTime > 0 && millis() >= snapshotCloseTime) {
        servo.write(closeAngle);
        isOpen = false;
        snapshotCloseTime = 0;
        Serial.printf("-> SNAPSHOT close (%d°, auto)\n", closeAngle);
    }
}
