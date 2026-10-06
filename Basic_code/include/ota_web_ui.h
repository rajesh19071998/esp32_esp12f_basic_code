#pragma once

// HTML templates for the OTA login and firmware upload pages.
#if defined(ESP8266)
constexpr char OTA_LOGIN_UI[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>Firmware Update Login</title></head>
<body>
<h1>Firmware Update Login</h1>
<form method="POST" action="/login">
  <p>Username:<br><input type="text" name="username" required></p>
  <p>Password:<br><input type="password" name="password" required></p>
  <p><button type="submit">Login</button></p>
  <p>{{ERROR}}</p>
</form>
</body>
</html>
)rawliteral";

constexpr char OTA_WEB_UI[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>Firmware Update</title></head>
<body>
<h1>Firmware Update</h1>
<form id="uploadForm">
  <p>Upload Type:
    <select id="uploadType">
      <option value="firmware">Firmware (.bin)</option>
      <option value="littlefs">LittleFS (.bin)</option>
    </select>
  </p>
  <p><input id="firmware" type="file" accept=".bin" required></p>
  <p><button type="submit">Flash Selected Image</button></p>
  <p id="statusText">Waiting for file selection.</p>
</form>
<script>
const form=document.getElementById('uploadForm');
const file=document.getElementById('firmware');
const type=document.getElementById('uploadType');
const status=document.getElementById('statusText');
form.addEventListener('submit',async(e)=>{
  e.preventDefault();
  if(!file.files.length){ status.textContent='Select a .bin file first.'; return; }
  await fetch('/upload-mode',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'type='+encodeURIComponent(type.value)});
  const data=new FormData();
  data.append('firmware', file.files[0]);
  data.append('uploadType', type.value);
  const xhr=new XMLHttpRequest();
  xhr.open('POST','/update');
  xhr.onload=function(){ status.textContent = xhr.responseText || 'Update complete.'; };
  xhr.onerror=function(){ status.textContent='Network error during OTA upload.'; };
  xhr.send(data);
});
</script>
</body>
</html>
)rawliteral";
#else
constexpr char OTA_LOGIN_UI[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Firmware Update Login</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #0f172a;
      --panel: rgba(15, 23, 42, 0.9);
      --panel-soft: rgba(15, 23, 42, 0.7);
      --ink: #e5eefb;
      --muted: #94a3b8;
      --accent: #38bdf8;
      --danger: #f87171;
      --line: rgba(148, 163, 184, 0.18);
      --shadow: 0 25px 60px rgba(15, 23, 42, 0.6);
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      font-family: "Segoe UI", Tahoma, sans-serif;
      background: linear-gradient(135deg, #0f172a, #111827);
      color: var(--ink);
      padding: 24px;
    }
    .shell {
      width: min(420px, 100%);
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 22px;
      box-shadow: var(--shadow);
      padding: 28px;
    }
    h1 { margin: 0 0 10px; font-size: 2rem; }
    p { margin: 0 0 20px; color: var(--muted); line-height: 1.5; }
    form { display: grid; gap: 16px; }
    label { display: grid; gap: 8px; font-weight: 700; color: var(--ink); }
    input {
      width: 100%;
      padding: 12px 14px;
      border-radius: 12px;
      border: 1px solid var(--line);
      background: var(--panel-soft);
      color: var(--ink);
      font-size: 1rem;
    }
    button {
      border: none;
      border-radius: 12px;
      background: linear-gradient(135deg, var(--accent), #3b82f6);
      color: white;
      font-size: 1rem;
      font-weight: 700;
      padding: 12px 16px;
      cursor: pointer;
    }
    .error { color: var(--danger); min-height: 1.2em; font-weight: 700; }
  </style>
</head>
<body>
  <main class="shell">
    <h1>Firmware Update Login</h1>
    <p>Enter the OTA credentials to open the firmware update page.</p>
    <form method="POST" action="/login">
      <label>
        Username
        <input type="text" name="username" required />
      </label>
      <label>
        Password
        <input type="password" name="password" required />
      </label>
      <button type="submit">Login</button>
      <div class="error">{{ERROR}}</div>
    </form>
  </main>
</body>
</html>
)rawliteral";

constexpr char OTA_WEB_UI[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 Firmware Update</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f4efe7;
      --panel: rgba(255, 251, 246, 0.88);
      --ink: #1e1d1a;
      --muted: #6d675f;
      --accent: #e76836;
      --accent-dark: #a94019;
      --line: rgba(30, 29, 26, 0.12);
      --ok: #1f7a4c;
      --err: #b02a1b;
      --shadow: 0 24px 60px rgba(62, 39, 20, 0.18);
    }
    * { box-sizing: border-box; }
    body {
      margin: 0; min-height: 100vh; font-family: "Segoe UI", "Trebuchet MS", sans-serif; color: var(--ink);
      background: radial-gradient(circle at top left, rgba(231, 104, 54, 0.25), transparent 34%),
                  radial-gradient(circle at bottom right, rgba(30, 125, 112, 0.15), transparent 28%),
                  linear-gradient(135deg, #f9f2ea 0%, #efe5d8 100%);
      display: grid; place-items: center; padding: 24px;
    }
    .shell { width: min(720px, 100%); background: var(--panel); border: 1px solid var(--line); border-radius: 28px; box-shadow: var(--shadow); overflow: hidden; }
    .hero { padding: 28px 28px 10px; }
    .eyebrow { margin: 0 0 12px; letter-spacing: 0.14em; text-transform: uppercase; font-size: 0.74rem; color: var(--accent-dark); font-weight: 700; }
    h1 { margin: 0; font-size: clamp(2rem, 5vw, 3.3rem); line-height: 0.95; }
    .subtitle { margin: 14px 0 0; font-size: 1rem; line-height: 1.6; color: var(--muted); max-width: 48ch; }
    .content { padding: 18px 28px 28px; display: grid; gap: 18px; }
    .card { border-radius: 22px; padding: 20px; border: 1px solid var(--line); background: rgba(255,255,255,0.6); }
    .picker { display: grid; gap: 14px; }
    input[type="file"] { width: 100%; padding: 14px; border-radius: 16px; border: 1px dashed rgba(30,29,26,0.26); background: rgba(255,255,255,0.88); color: var(--ink); }
    button { appearance: none; border: 0; border-radius: 999px; padding: 14px 20px; font-size: 1rem; font-weight: 700; color: white; background: linear-gradient(135deg, var(--accent) 0%, #f08e41 100%); cursor: pointer; box-shadow: 0 12px 30px rgba(231,104,54,0.28); }
    .progress-bar { width: 100%; height: 14px; overflow: hidden; border-radius: 999px; background: rgba(30,29,26,0.08); }
    .progress-fill { height: 100%; width: 0%; border-radius: inherit; background: linear-gradient(90deg, #d84f2d 0%, #f0a340 100%); transition: width 160ms linear; }
    .status { min-height: 1.5em; font-weight: 600; }
    .status.ok { color: var(--ok); }
    .status.err { color: var(--err); }
    .tips { margin: 0; padding-left: 18px; color: var(--muted); line-height: 1.6; }
    .tips li + li { margin-top: 6px; }
  </style>
</head>
<body>
  <main class="shell">
    <section class="hero">
      <p class="eyebrow">ESP32 OTA Console</p>
      <h1>Firmware Update</h1>
      <p class="subtitle">Upload a compiled <code>.bin</code> image and the board will write it to flash over Wi-Fi, verify the update, then reboot into the new firmware.</p>
    </section>
    <section class="content">
      <form class="card picker" id="uploadForm">
        <div>
          <label class="upload-label" for="uploadType">Upload Type</label>
          <select id="uploadType" style="width:100%; padding:12px 14px; border-radius:12px; border:1px solid rgba(30,29,26,0.12); background: rgba(255,255,255,0.88); color: var(--ink); font-size: 1rem;">
            <option value="firmware">Firmware (.bin)</option>
            <option value="littlefs">LittleFS (.bin)</option>
          </select>
        </div>
        <div>
          <label class="upload-label" for="firmware">Image File</label>
          <input id="firmware" name="firmware" type="file" accept=".bin" required />
        </div>
        <div class="progress-wrap">
          <div class="progress-bar"><div class="progress-fill" id="progressFill"></div></div>
          <div id="progressText">Waiting for file selection.</div>
        </div>
        <button id="uploadButton" type="submit">Flash Selected Image</button>
        <div class="status" id="statusMessage"></div>
      </form>
      <div class="card">
        <strong>Before You Upload</strong>
        <ul class="tips">
          <li>Build the image from PlatformIO and use the generated <code>firmware.bin</code> file.</li>
          <li>Keep the browser tab open until the board confirms the update result.</li>
          <li>After a successful upload the board reboots automatically within a few seconds.</li>
        </ul>
      </div>
      <div class="card"><button type="button" onclick="window.location.href='/dashboard'">Dashboard</button></div>
    </section>
  </main>
  <script>
    const form = document.getElementById('uploadForm');
    const firmwareInput = document.getElementById('firmware');
    const uploadType = document.getElementById('uploadType');
    const uploadButton = document.getElementById('uploadButton');
    const progressFill = document.getElementById('progressFill');
    const progressText = document.getElementById('progressText');
    const statusMessage = document.getElementById('statusMessage');
    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      if (!firmwareInput.files.length) {
        statusMessage.textContent = 'Select a .bin file first.';
        statusMessage.className = 'status err';
        return;
      }
      await fetch('/upload-mode', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: `type=${encodeURIComponent(uploadType.value)}` });
      const data = new FormData();
      data.append('firmware', firmwareInput.files[0]);
      data.append('uploadType', uploadType.value);
      const request = new XMLHttpRequest();
      request.open('POST', '/update');
      uploadButton.disabled = true;
      statusMessage.textContent = '';
      statusMessage.className = 'status';
      progressFill.style.width = '0%';
      progressText.textContent = 'Preparing upload...';
      request.upload.addEventListener('progress', (event) => {
        if (!event.lengthComputable) {
          progressText.textContent = 'Uploading firmware...';
          return;
        }
        const percent = Math.round((event.loaded / event.total) * 100);
        progressFill.style.width = `${percent}%`;
        progressText.textContent = `Uploading firmware... ${percent}%`;
      });
      request.addEventListener('load', () => {
        const success = request.status >= 200 && request.status < 300;
        statusMessage.textContent = request.responseText || (success ? 'Update complete.' : 'Update failed.');
        statusMessage.className = success ? 'status ok' : 'status err';
        progressText.textContent = success ? 'Firmware uploaded. Device is rebooting...' : 'Upload failed.';
        if (success) progressFill.style.width = '100%';
        uploadButton.disabled = false;
      });
      request.addEventListener('error', () => {
        statusMessage.textContent = 'Network error during OTA upload.';
        statusMessage.className = 'status err';
        progressText.textContent = 'Upload interrupted.';
        uploadButton.disabled = false;
      });
      request.send(data);
    });
  </script>
</body>
</html>
)rawliteral";
#endif