#pragma once
#include <Arduino.h>

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
  <h1>&#x1F4F7; Camera Shutter</h1>
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
