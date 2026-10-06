#pragma once

#include <Arduino.h>

#include "device_config.h"

// HTML templates for the dashboard and Wi-Fi settings pages served by the device.
constexpr char WEB_DASHBOARD_UI_TEMPLATE[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 Control Dashboard</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f6efe6;
      --panel: rgba(255, 251, 246, 0.9);
      --panel-soft: rgba(255, 255, 255, 0.68);
      --text: #1f2937;
      --muted: #6b7280;
      --line: rgba(148, 163, 184, 0.28);
      --accent: #d97706;
      --accent-strong: #b45309;
      --success: #22c55e;
      --danger: #ef4444;
      --warning: #f59e0b;
      --shadow: 0 24px 60px rgba(120, 53, 15, 0.16);
    }

    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      min-height: 100vh;
      padding: 24px;
      font-family: "Segoe UI", Tahoma, sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at top left, rgba(217, 119, 6, 0.18), transparent 28%),
        radial-gradient(circle at bottom right, rgba(14, 165, 233, 0.14), transparent 24%),
        linear-gradient(135deg, #f8f1e7 0%, #efe4d1 100%);
    }

    .wrap {
      width: min(1120px, 100%);
      margin: 0 auto;
      display: grid;
      gap: 20px;
    }

    .topbar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 16px;
      padding: 24px 28px;
      border: 1px solid var(--line);
      border-radius: 28px;
      background: var(--panel);
      box-shadow: var(--shadow);
    }

    .title {
      margin: 0;
      font-size: clamp(2rem, 5vw, 3rem);
      line-height: 0.95;
    }

    .nav {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      align-items: center;
    }

    .nav a,
    .nav-btn {
      appearance: none;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.72);
      color: var(--text);
      text-decoration: none;
      padding: 10px 16px;
      font-size: 0.95rem;
      font-weight: 700;
      cursor: pointer;
      transition: background 140ms ease, color 140ms ease, border-color 140ms ease;
    }

    .nav-btn.active,
    .nav a:hover,
    .nav-btn:hover {
      color: white;
      background: linear-gradient(135deg, var(--accent), var(--accent-strong));
      border-color: transparent;
    }

    .tab-panel {
      display: none;
    }

    .tab-panel.active {
      display: block;
    }

    .grid {
      display: grid;
      grid-template-columns: minmax(0, 1.35fr) minmax(300px, 0.95fr);
      gap: 20px;
    }

    .panel {
      border: 1px solid var(--line);
      border-radius: 28px;
      background: var(--panel);
      box-shadow: var(--shadow);
      padding: 24px;
    }

    .panel h2 {
      margin: 0 0 18px;
      font-size: 1.4rem;
    }

    .led-grid,
    .switch-list {
      display: grid;
      gap: 14px;
    }

    .led-card,
    .switch-row {
      border: 1px solid var(--line);
      border-radius: 18px;
      background: var(--panel-soft);
      padding: 16px;
    }

    .led-header,
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
    }

    .led-name,
    .switch-name {
      font-size: 1rem;
      font-weight: 700;
    }

    .status-badge,
    .switch-state {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      border: 1px solid transparent;
      border-radius: 999px;
      padding: 7px 12px;
      font-size: 0.9rem;
      font-weight: 700;
      white-space: nowrap;
    }

    .status-badge .light {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: currentColor;
      box-shadow: 0 0 0 4px rgba(255, 255, 255, 0.22);
    }

    .status-badge.on {
      color: var(--success);
      background: rgba(34, 197, 94, 0.1);
      border-color: rgba(34, 197, 94, 0.35);
    }

    .status-badge.off {
      color: var(--danger);
      background: rgba(239, 68, 68, 0.08);
      border-color: rgba(239, 68, 68, 0.26);
    }

    .controls {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      margin-top: 14px;
    }

    .toggle {
      appearance: none;
      border: 0;
      border-radius: 12px;
      padding: 11px 16px;
      font-size: 0.95rem;
      font-weight: 700;
      color: white;
      cursor: pointer;
    }

    .toggle.on {
      background: linear-gradient(135deg, #16a34a, #22c55e);
    }

    .toggle.off {
      background: linear-gradient(135deg, #dc2626, #f97316);
    }

    .toggle:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }

    .switch-state.open {
      color: #22c55e;
      background: rgba(34, 197, 94, 0.1);
      border-color: rgba(34, 197, 94, 0.45);
    }

    .switch-state.closed {
      color: #fbbf24;
      background: rgba(245, 158, 11, 0.08);
      border-color: rgba(245, 158, 11, 0.4);
    }

    .meta-card {
      display: grid;
      gap: 10px;
    }

    .meta-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 10px;
      background: rgba(15, 23, 42, 0.7);
      border: 1px solid var(--line);
      border-radius: 12px;
      padding: 12px 14px;
      color: var(--muted);
    }

    .persist-row {
      background: linear-gradient(135deg, rgba(217, 119, 6, 0.18), rgba(251, 191, 36, 0.16));
      border-color: rgba(217, 119, 6, 0.28);
    }

    .persist-row span {
      color: #92400e;
      font-weight: 700;
    }

    .info-row {
      background: linear-gradient(135deg, rgba(14, 165, 233, 0.12), rgba(59, 130, 246, 0.08));
      border-color: rgba(14, 165, 233, 0.22);
      color: #4b5563;
    }

    .info-row span {
      color: #475569;
      font-weight: 700;
    }

    .info-row strong {
      color: #0f766e;
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(14, 165, 233, 0.16);
      border-radius: 999px;
      padding: 6px 10px;
    }

    .meta-row strong {
      color: var(--text);
    }

    .tab-actions {
      margin-top: 18px;
      display: flex;
      justify-content: flex-start;
    }

    @media (max-width: 860px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <header class="topbar">
      <h1 class="title">ESP32 Control Dashboard</h1>
      <nav class="nav">
        <button class="nav-btn active" id="dashboardTabBtn" type="button">Dashboard</button>
        <button class="nav-btn" id="infoTabBtn" type="button">Info</button>
        <a href="/wifi-settings">Wi‑Fi Settings</a>
        <a href="/update">Firmware Update</a>
      </nav>
    </header>

    <section id="dashboardTab" class="tab-panel active">
      <div class="grid">
        <section class="panel">
          <h2>LED Controls</h2>
          <div class="meta-row persist-row" style="margin-bottom: 16px;">
            <span>LED State Memory</span>
            <strong id="persistState">Enabled</strong>
          </div>
          <div class="controls" style="margin-bottom: 16px;">
            <button class="toggle on" id="enablePersistBtn" type="button">Enable Save</button>
            <button class="toggle off" id="disablePersistBtn" type="button">Disable Save</button>
          </div>
          <div id="ledGrid" class="led-grid"></div>
        </section>

        <aside class="panel">
          <h2>Switch Status</h2>
          <div id="switchList" class="switch-list"></div>
        </aside>
      </div>

    </section>

    <section id="infoTab" class="tab-panel">
      <div class="panel">
        <h2>Device Info</h2>
        <div class="meta-card">
          <div class="meta-row info-row"><span>Board</span><strong id="boardName">ESP32-OTA</strong></div>
          <div class="meta-row info-row"><span>Hostname</span><strong id="deviceHost">--</strong></div>
          <div class="meta-row info-row"><span>IP</span><strong id="deviceIp">--</strong></div>
          <div class="meta-row info-row"><span>SW Version</span><strong id="swVersion">--</strong></div>
          <div class="meta-row info-row"><span>Build Date</span><strong id="buildDate">--</strong></div>
          <div class="meta-row info-row"><span>Build Time</span><strong id="buildTime">--</strong></div>
          <div class="meta-row info-row"><span>RAM Total</span><strong id="ramTotal">--</strong></div>
          <div class="meta-row info-row"><span>RAM Used</span><strong id="ramUsed">--</strong></div>
          <div class="meta-row info-row"><span>RAM Used %</span><strong id="ramUsedPercent">--</strong></div>
          <div class="meta-row info-row"><span>Flash Total</span><strong id="flashTotal">--</strong></div>
          <div class="meta-row info-row"><span>Flash Used</span><strong id="flashUsed">--</strong></div>
          <div class="meta-row info-row"><span>Flash Used %</span><strong id="flashUsedPercent">--</strong></div>
          <div class="meta-row info-row"><span>Wi‑Fi Mode</span><strong id="wifiMode">--</strong></div>
          <div class="meta-row info-row"><span>Uptime</span><strong id="deviceUptime">--</strong></div>
        </div>
        <div class="tab-actions">
          <button class="toggle on" id="dashboardFromInfoBtn" type="button">Open Dashboard</button>
        </div>
      </div>
    </section>
  </div>

  <script>
    const ledCount = {{LED_COUNT}};
    const switchCount = {{SWITCH_COUNT}};
    let ledLabels = Array(ledCount).fill('');
    let switchLabels = Array(switchCount).fill('');
    let ledStates = Array(ledCount).fill(false);
    let switchStates = Array(switchCount).fill(false);

    const ledGrid = document.getElementById('ledGrid');
    const switchList = document.getElementById('switchList');
    const lastUpdate = document.getElementById('lastUpdate');
    const boardName = document.getElementById('boardName');
    const deviceHost = document.getElementById('deviceHost');
    const deviceIp = document.getElementById('deviceIp');
    const swVersion = document.getElementById('swVersion');
    const buildDate = document.getElementById('buildDate');
    const buildTime = document.getElementById('buildTime');
    const ramTotal = document.getElementById('ramTotal');
    const ramUsed = document.getElementById('ramUsed');
    const ramUsedPercent = document.getElementById('ramUsedPercent');
    const flashTotal = document.getElementById('flashTotal');
    const flashUsed = document.getElementById('flashUsed');
    const flashUsedPercent = document.getElementById('flashUsedPercent');
    const wifiMode = document.getElementById('wifiMode');
    const deviceUptime = document.getElementById('deviceUptime');
    const persistState = document.getElementById('persistState');
    const enablePersistBtn = document.getElementById('enablePersistBtn');
    const disablePersistBtn = document.getElementById('disablePersistBtn');
    const dashboardTabBtn = document.getElementById('dashboardTabBtn');
    const infoTabBtn = document.getElementById('infoTabBtn');
    const dashboardTab = document.getElementById('dashboardTab');
    const infoTab = document.getElementById('infoTab');
    const dashboardFromInfoBtn = document.getElementById('dashboardFromInfoBtn');

    function showTab(tabName) {
      const isDashboard = tabName === 'dashboard';
      dashboardTab.classList.toggle('active', isDashboard);
      infoTab.classList.toggle('active', !isDashboard);
      dashboardTabBtn.classList.toggle('active', isDashboard);
      infoTabBtn.classList.toggle('active', !isDashboard);
    }

    dashboardTabBtn.addEventListener('click', () => showTab('dashboard'));
    infoTabBtn.addEventListener('click', () => showTab('info'));
    dashboardFromInfoBtn.addEventListener('click', () => showTab('dashboard'));

    function resolveLabel(labels, index, prefix) {
      const label = labels[index];
      if (typeof label === 'string' && label.length > 0) {
        return label;
      }
      return `${prefix} ${index + 1}`;
    }

    function formatMemoryBytes(value) {
      if (value === undefined || value === null || value === 0) return '0 KB';
      return `${Math.round(value / 1024)} KB`;
    }

    function formatPercentage(value) {
      if (value === undefined || value === null || Number.isNaN(value)) return '--';
      return `${value.toFixed(1)}%`;
    }

    async function refreshDeviceInfo() {
      try {
        const response = await fetch('/device');
        const data = await response.json();
        boardName.textContent = data.hostname || 'ESP32-OTA';
        deviceHost.textContent = data.hostname || '--';
        deviceIp.textContent = data.ip || '--';
        swVersion.textContent = data.swVersion || '--';
        buildDate.textContent = data.buildDate || '--';
        buildTime.textContent = data.buildTime || '--';
        const ramTotalValue = Number(data.ramTotal) || 0;
        const ramUsedValue = Number(data.ramUsed) || 0;
        const flashTotalValue = Number(data.flashTotal) || 0;
        const flashUsedValue = Number(data.flashUsed) || 0;

        ramTotal.textContent = formatMemoryBytes(ramTotalValue);
        ramUsed.textContent = formatMemoryBytes(ramUsedValue);
        ramUsedPercent.textContent = formatPercentage(ramTotalValue > 0 ? (ramUsedValue / ramTotalValue) * 100 : 0);
        flashTotal.textContent = formatMemoryBytes(flashTotalValue);
        flashUsed.textContent = formatMemoryBytes(flashUsedValue);
        flashUsedPercent.textContent = formatPercentage(flashTotalValue > 0 ? (flashUsedValue / flashTotalValue) * 100 : 0);
        wifiMode.textContent = data.wifiMode || 'Unknown';
        deviceUptime.textContent = data.uptime || '--';
      } catch (error) {
        boardName.textContent = 'ESP32-OTA';
        deviceHost.textContent = '--';
        deviceIp.textContent = '--';
        swVersion.textContent = '--';
        buildDate.textContent = '--';
        buildTime.textContent = '--';
        ramTotal.textContent = '--';
        ramUsed.textContent = '--';
        ramUsedPercent.textContent = '--';
        flashTotal.textContent = '--';
        flashUsed.textContent = '--';
        flashUsedPercent.textContent = '--';
        wifiMode.textContent = 'Unknown';
        deviceUptime.textContent = '--';
      }
    }

    function renderLedGrid() {
      ledGrid.innerHTML = '';
      ledStates.forEach((state, index) => {
        const card = document.createElement('div');
        card.className = 'led-card';
        card.innerHTML = `
          <div class="led-header">
            <div class="led-name">${resolveLabel(ledLabels, index, 'LED')}</div>
            <span class="status-badge ${state ? 'on' : 'off'}">
              <span class="light"></span>
              ${state ? 'On' : 'Off'}
            </span>
          </div>
          <div class="controls">
            <button class="toggle ${state ? 'on' : 'off'}" data-led="${index}" data-state="1">Turn On</button>
            <button class="toggle ${state ? 'off' : 'on'}" data-led="${index}" data-state="0">Turn Off</button>
          </div>
        `;
        ledGrid.appendChild(card);
      });

      ledGrid.querySelectorAll('[data-led]').forEach((button) => {
        button.addEventListener('click', async () => {
          const led = Number(button.dataset.led);
          const value = Number(button.dataset.state);
          await fetch('/led', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `led=${led}&state=${value}`
          });
          await refreshStatus();
        });
      });
    }

    function renderSwitchList() {
      switchList.innerHTML = '';
      switchStates.forEach((state, index) => {
        const row = document.createElement('div');
        row.className = 'switch-row';
        row.innerHTML = `
          <div class="switch-name">${resolveLabel(switchLabels, index, 'Switch')}</div>
          <span class="switch-state ${state ? 'open' : 'closed'}">${state ? 'Pressed' : 'Released'}</span>
        `;
        switchList.appendChild(row);
      });
    }

    async function refreshPersistenceState() {
      try {
        const response = await fetch('/persist');
        const data = await response.json();
        const enabled = !!data.persistenceEnabled;
        persistState.textContent = enabled ? 'Enabled' : 'Disabled';
        enablePersistBtn.disabled = enabled;
        disablePersistBtn.disabled = !enabled;
      } catch (error) {
        persistState.textContent = 'Unknown';
      }
    }

    async function setPersistence(enabled) {
      await fetch(`/persist?enabled=${enabled ? 1 : 0}`, { method: 'POST' });
      await refreshPersistenceState();
    }

    enablePersistBtn.addEventListener('click', () => setPersistence(true));
    disablePersistBtn.addEventListener('click', () => setPersistence(false));

    async function refreshStatus() {
      await refreshDeviceInfo();
      await refreshPersistenceState();

      const ledRes = await fetch('/leds');
      const ledData = await ledRes.json();
      ledStates = Array.isArray(ledData.leds) ? ledData.leds : [];
      ledLabels = Array.isArray(ledData.ledLabels)
        ? ledData.ledLabels
        : Array.isArray(ledData.channelLabels)
          ? ledData.channelLabels
          : ledLabels;

      const switchRes = await fetch('/switches');
      const switchData = await switchRes.json();
      switchStates = Array.isArray(switchData.switches) ? switchData.switches : [];
      switchLabels = Array.isArray(switchData.switchLabels)
        ? switchData.switchLabels
        : Array.isArray(switchData.channelLabels)
          ? switchData.channelLabels
          : switchLabels;

      renderLedGrid();
      renderSwitchList();
    }

    refreshStatus();
    setInterval(refreshStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

// Injects channel counts into the dashboard template before sending it.
inline String webDashboardUi() {
  String html = WEB_DASHBOARD_UI_TEMPLATE;
  html.replace("{{LED_COUNT}}", String(device_config::kLedCount));
  html.replace("{{SWITCH_COUNT}}", String(device_config::kSwitchCount));
  return html;
}

constexpr char WIFI_SETTINGS_UI[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Wi‑Fi Settings</title>
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
      width: min(620px, 100%);
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 22px;
      box-shadow: var(--shadow);
      padding: 28px;
    }
    h1 {
      margin: 0 0 10px;
      font-size: 2rem;
    }
    p {
      margin: 0 0 20px;
      color: var(--muted);
      line-height: 1.5;
    }
    form {
      display: grid;
      gap: 18px;
    }
    .field-grid {
      display: grid;
      gap: 16px;
    }
    label {
      display: grid;
      gap: 8px;
      font-weight: 700;
      color: var(--ink);
    }
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
    .status {
      min-height: 1.2em;
      color: var(--muted);
      font-weight: 700;
    }
    .status.success {
      color: #34d399;
    }
    .status.error {
      color: var(--danger);
    }
    .actions {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
    }
  </style>
</head>
<body>
  <main class="shell">
    <h1>Wi‑Fi Settings</h1>
    <p>Update the Wi‑Fi station credentials and the device hotspot credentials, then save them to EEPROM.</p>

    <form id="wifiSettingsForm">
      <div class="field-grid">
        <label>
          Wi‑Fi Name
          <input id="wifiSsidInput" name="wifiSsid" type="text" maxlength="31" placeholder="MyWiFi" required />
        </label>
        <label>
          Wi‑Fi Password
          <input id="wifiPasswordInput" name="wifiPassword" type="text" maxlength="31" placeholder="MyPassword" required />
        </label>
        <label>
          Hotspot Name
          <input id="apNameInput" name="apName" type="text" maxlength="31" placeholder="ESP32-OTA" required />
        </label>
        <label>
          Hotspot Password
          <input id="apPasswordInput" name="apPassword" type="text" maxlength="31" placeholder="firmware123" required />
        </label>
      </div>

      <div class="actions">
        <button type="submit">Save Settings</button>
        <button type="button" onclick="window.location.href='/dashboard'">Dashboard</button>
      </div>

      <div id="wifiSettingsStatus" class="status"></div>
    </form>
  </main>

  <script>
    const wifiSettingsForm = document.getElementById('wifiSettingsForm');
    const wifiSsidInput = document.getElementById('wifiSsidInput');
    const wifiPasswordInput = document.getElementById('wifiPasswordInput');
    const apNameInput = document.getElementById('apNameInput');
    const apPasswordInput = document.getElementById('apPasswordInput');
    const wifiSettingsStatus = document.getElementById('wifiSettingsStatus');

    async function loadSettings() {
      try {
        const response = await fetch('/wifi-settings?json=1');
        const data = await response.json();
        wifiSsidInput.value = data.wifiSsid || '';
        wifiPasswordInput.value = data.wifiPassword || '';
        apNameInput.value = data.apName || '';
        apPasswordInput.value = data.apPassword || '';
      } catch (error) {
        wifiSettingsStatus.textContent = 'Unable to load Wi‑Fi settings.';
        wifiSettingsStatus.className = 'status error';
      }
    }

    wifiSettingsForm.addEventListener('submit', async (event) => {
      event.preventDefault();
      const formData = new URLSearchParams({
        wifiSsid: wifiSsidInput.value,
        wifiPassword: wifiPasswordInput.value,
        apName: apNameInput.value,
        apPassword: apPasswordInput.value
      });

      try {
        const response = await fetch('/wifi-settings?json=1', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: formData.toString()
        });
        const result = await response.json();
        if (result.status === 'saved') {
          wifiSettingsStatus.textContent = 'Wi‑Fi settings saved successfully.';
          wifiSettingsStatus.className = 'status success';
        } else {
          wifiSettingsStatus.textContent = 'Failed to save Wi‑Fi settings.';
          wifiSettingsStatus.className = 'status error';
        }
      } catch (error) {
        wifiSettingsStatus.textContent = 'Unable to save Wi‑Fi settings.';
        wifiSettingsStatus.className = 'status error';
      }
    });

    loadSettings();
  </script>
</body>
</html>
)rawliteral";
