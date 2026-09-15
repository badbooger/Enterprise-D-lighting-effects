// Bridge web control page HTML/CSS/JS, served from PROGMEM.
// Verbatim copy of Web_Control_Prototype/control.html — do not hand-edit here;
// change the source file and re-copy if the page needs updating.
#ifndef CONTROL_HTML_H
#define CONTROL_HTML_H

const char CONTROL_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Enterprise Control</title>
<style>
  :root {
    --amber:  #FF9C00;
    --peach:  #FF9966;
    --peach2: #FFBBAA;
    --purple: #CC99FF;
    --purple2:#CC99CC;
    --blue:   #8899FF;
    --paleyel:#FFFFAA;
    --red:    #CC0000;
    --bg:     #000000;
    --text:   #FF9966;
    --dim:    #8899AA;
    --offbtn: #99CCFF;

    --sidebar-w: 90px;
    --block-gap: 4px;
    --hdr-wide-w: 150px;
    --hdr-row1-h: 40px;
    --hdr-row2-h: 54px;
    --hdr-notch-r: 26px;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: "Arial Narrow", Arial, sans-serif;
    max-width: 560px;
    margin-inline: auto;
    padding: 8px;
  }

  /* Whole frame is the ui_Screen1.c sidebar+header, traced from its exact
     block coordinates/colors (see CLAUDE.md / Data_Pad/ui_Screen1.c). The
     real frame's purple block is WIDER (132px) than the normal sidebar
     column (~90px) for the stretch beside the status text, then narrows
     back to sidebar width right where the header bar begins beside it —
     the notch curve rounds that narrowing step, not a random corner. A
     plain-width sidebar has nothing to narrow from, so the curve has to
     live in a wider-then-narrower construction like this one. */
  .header-region {
    position: relative;
    height: calc(var(--hdr-row1-h) + var(--hdr-row2-h));
    margin-bottom: var(--block-gap);
  }
  .hdr-wide {
    position: absolute; left: 0; top: 0;
    width: var(--hdr-wide-w); height: var(--hdr-row1-h);
    background: var(--purple);
  }
  .hdr-narrow {
    position: absolute; left: 0;
    top: var(--hdr-row1-h);
    width: var(--sidebar-w); height: var(--hdr-row2-h);
    background: var(--purple);
  }
  /* Rounds the concave step where hdr-wide (150px) meets hdr-narrow (90px):
     a purple quarter-circle filling the notch, same corner-smoothing idea
     as site/css/lcars.css's .lcars-elbow — plain border-radius can't round
     a step between two differently-sized boxes directly. Both pieces must
     touch with zero gap at the row1/row2 seam or the curve has nothing to
     bridge — hence no block-gap anywhere in this region. */
  .hdr-notch {
    position: absolute;
    left: var(--sidebar-w);
    top: var(--hdr-row1-h);
    width: calc(var(--hdr-notch-r) * 2);
    height: calc(var(--hdr-notch-r) * 2);
    background: var(--purple);
    border-radius: 50%;
  }
  #statusTitle {
    position: absolute; right: 6px; top: 0;
    height: var(--hdr-row1-h);
    display: flex; align-items: center;
    font-size: 20px;
    font-weight: 700;
    letter-spacing: 1px;
    text-transform: uppercase;
    color: #fff;
  }

  .header-bar {
    position: absolute;
    left: var(--sidebar-w);
    top: var(--hdr-row1-h);
    right: 0;
    height: var(--hdr-row2-h);
    display: flex; align-items: stretch;
  }
  /* Fixed percentage widths, not flex-grow — flex-grow's default content-based
     flex-basis skewed hb-title (it has text/padding, the others don't) way
     wider than its intended ~43% share. */
  .hb-seg { display: flex; align-items: center; flex-shrink: 0; }
  .hb-title { background: var(--purple); padding: 0 10px; }
  .hb-title span { color: #000; font-size: 11px; font-weight: 700; letter-spacing: 0.5px; white-space: nowrap; overflow: hidden; }
  .hb-d1 { background: var(--peach2); }
  .hb-d2 { background: var(--peach2); }
  .hb-d3 { background: var(--peach2); }
  .hb-b1 { background: var(--blue); }
  .hb-b2 { background: var(--blue); }

  .page { display: grid; grid-template-columns: var(--sidebar-w) 1fr; column-gap: var(--block-gap); row-gap: var(--block-gap); }
  .lower-sidebar { display: flex; flex-direction: column; gap: var(--block-gap); }
  .lb-block { flex: 1; background: var(--peach); min-height: 40px; }

  .content { padding: 4px 6px 16px; }

  .panel { margin-bottom: 22px; }
  .panel-hdr {
    display: inline-flex;
    align-items: center;
    background: var(--purple);
    color: #000;
    border-radius: 999px 4px 4px 999px;
    padding: 6px 16px 6px 20px;
    font-weight: bold;
    text-transform: uppercase;
    letter-spacing: 1px;
    font-size: 13px;
    margin-bottom: 10px;
  }
  .panel[data-color="amber"]  .panel-hdr { background: var(--amber); }
  .panel[data-color="blue"]   .panel-hdr { background: var(--blue); }
  .panel[data-color="peach"]  .panel-hdr { background: var(--peach); }
  .panel[data-color="purple2"] .panel-hdr { background: var(--purple2); }

  .row { display: flex; flex-wrap: wrap; gap: 10px; align-items: stretch; }
  .row + .row { margin-top: 10px; }

  button {
    flex: 1 1 120px;
    background: var(--amber);
    color: #000;
    border: none;
    border-radius: 999px;
    padding: 12px 20px;
    min-height: 44px;
    font-size: 13px;
    font-weight: bold;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    cursor: pointer;
  }
  button.off      { background: var(--offbtn); color: #000; }
  button.warn     { background: var(--red); color: #fff; }
  button.alt      { background: var(--purple); color: #000; }
  button.blue     { background: var(--blue); color: #000; }
  button.peach    { background: var(--peach); color: #000; }
  button.pwron    { background: var(--paleyel); color: #000; }
  /* Decorative LCARS filler block — not a real control, never clickable. */
  button.filler   { background: var(--offbtn); color: #000; opacity: 0.6; cursor: default; }

  /* Solid color swap for "active" — on data-toggle rows it persists (last
     clicked = current commanded state, optimistic, same honesty as the
     SYSTEM ONLINE/OFFLINE text); on one-shot buttons it's removed after a
     short delay for a brief flash instead. */
  button.is-active { background: #fff; color: #000; }

  #log {
    background: #050505;
    border: 1px solid #222;
    border-radius: 8px;
    font-family: "Consolas", monospace;
    font-size: 11px;
    color: var(--dim);
    height: 140px;
    overflow-y: auto;
    padding: 8px;
    white-space: pre-wrap;
  }

  #macList {
    background: #050505;
    border: 1px solid #222;
    border-radius: 8px;
    font-family: "Consolas", monospace;
    font-size: 13px;
    color: var(--dim);
    padding: 8px;
    white-space: pre-wrap;
  }
  #macList:empty { display: none; }

  .lcars-footer { grid-column: 1 / -1; display: flex; height: 26px; margin-top: 2px; }
  .ftr-bar {
    background: var(--purple2);
    border-radius: 13px;
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: flex-end;
    padding-right: 18px;
  }
  .ftr-bar span { color: #000; font-size: 10px; text-transform: uppercase; letter-spacing: 1px; font-weight: bold; }

  #statusBanner {
    background: var(--amber);
    color: #000;
    font-size: 11px;
    font-weight: bold;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    text-align: center;
    padding: 6px;
    border-radius: 4px;
    margin-bottom: var(--block-gap);
  }
  #statusBanner.hidden { display: none; }
</style>
</head>
<body>

<div class="header-region">
  <div class="hdr-wide"></div>
  <div class="hdr-notch"></div>
  <div class="hdr-narrow"></div>
  <span id="statusTitle">SYSTEM ONLINE</span>
  <div class="header-bar">
    <div class="hb-seg hb-title" style="width:42.72%"><span>Bridge Control</span></div>
    <div class="hb-seg hb-d1" style="width:2.13%"></div>
    <div class="hb-seg hb-d2" style="width:12.60%"></div>
    <div class="hb-seg hb-d3" style="width:2.13%"></div>
    <div class="hb-seg hb-b1" style="width:4.42%"></div>
    <div class="hb-seg hb-b2" style="width:36.00%"></div>
  </div>
</div>

<div id="statusBanner" class="hidden"></div>

<div class="page">

  <div class="lower-sidebar">
    <div class="lb-block"></div>
    <div class="lb-block"></div>
    <div class="lb-block"></div>
    <div class="lb-block"></div>
  </div>

  <div class="content">

    <div class="panel" data-color="amber">
      <div class="panel-hdr">Power</div>
      <div class="row" data-toggle="1">
        <button class="pwron" onclick="press(this); sendCmd(0, CMD.LED_STARTUP, 1)">Power On</button>
        <button class="off" onclick="press(this); sendCmd(0, CMD.LED_SHUTDOWN, 0)">Power Off</button>
        <button class="warn" onclick="press(this); sendCmd(0, CMD.LED_ALL_OFF, 0)">All Off</button>
      </div>
    </div>

    <div class="panel">
      <div class="panel-hdr">Navigation Lights</div>
      <div class="row" data-toggle="1">
        <button onclick="press(this); sendCmd(GROUP.NAV, CMD.LED_ON, 0)">On</button>
        <button class="off" onclick="press(this); sendCmd(GROUP.NAV, CMD.LED_OFF, 0)">Off</button>
        <button class="alt" onclick="press(this); sendCmd(GROUP.NAV, CMD.LED_BLINK, 0)">Blink</button>
      </div>
    </div>

    <div class="panel" data-color="blue">
      <div class="panel-hdr">Engine Controls</div>
      <div class="row">
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'blue', 'Impulse',
          () => { sendCmd(GROUP.IM_LEFT, CMD.LED_ON, 0); sendCmd(GROUP.IM_RIGHT, CMD.LED_ON, 0) },
          () => { sendCmd(GROUP.IM_LEFT, CMD.LED_OFF, 0); sendCmd(GROUP.IM_RIGHT, CMD.LED_OFF, 0) })">Impulse: Off</button>
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'blue', 'Deflector',
          () => sendCmd(GROUP.DEFLECTOR, CMD.LED_ON, 0),
          () => sendCmd(GROUP.DEFLECTOR, CMD.LED_OFF, 0))">Deflector: Off</button>
      </div>
      <div class="row">
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'blue', 'Nacelles',
          () => sendCmd(GROUP.NAC_BOTH, CMD.LED_ON, 0),
          () => sendCmd(GROUP.NAC_BOTH, CMD.LED_OFF, 0))">Nacelles: Off</button>
        <!-- Decorative only — LCARS panels are dense with labeled filler
             blocks like this, not every pill maps to a real function.
             Non-interactive on purpose so it never looks like it does
             something it doesn't. -->
        <button class="filler" disabled>Plasma Relay</button>
      </div>
    </div>

    <div class="panel" data-color="peach">
      <div class="panel-hdr">Warp Core</div>
      <div class="row">
        <button class="peach" onclick="press(this); sendCmd(0, CMD.LED_WARP, 1)">Engage Warp</button>
      </div>
    </div>

    <div class="panel">
      <div class="panel-hdr">Weapons</div>
      <div class="row">
        <button class="alt" onclick="press(this); sendCmd(GROUP.PHOTON, CMD.LED_ON, 0)">Fire Torpedoes</button>
        <button class="alt" onclick="press(this); sendCmd(GROUP.PHOTON_AFT, CMD.LED_ON, 0)">Fire Aft Torpedo</button>
      </div>
    </div>

    <div class="panel" data-color="blue">
      <div class="panel-hdr">Sound</div>
      <div class="row">
        <button class="blue" onclick="press(this); sendCmd(0, CMD.SND_CMD_PLAY, SND.RED_ALERT)">Red Alert</button>
        <button class="blue" onclick="press(this); sendCmd(GROUP.PHOTON, CMD.LED_ON, 0); sendCmd(GROUP.PHOTON_AFT, CMD.LED_ON, 0); sendCmd(0, CMD.SND_CMD_PLAY, SND.FIRE_WEAPONS)">Fire All Weapons</button>
        <button class="alt" onclick="press(this); sendCmd(0, CMD.SND_CMD_PLAY, randomSoundFile())">Random Sound</button>
        <button class="off" onclick="press(this); sendCmd(0, CMD.SND_CMD_STOP, 0)">Stop</button>
      </div>
      <div class="row">
        <button class="off" onclick="press(this); adjustVolume(-5)">Volume −</button>
        <button class="blue" onclick="press(this); adjustVolume(5)">Volume +</button>
      </div>
    </div>

    <div class="panel" data-color="purple2">
      <div class="panel-hdr">Effects</div>
      <div class="row">
        <button class="warn" onclick="press(this); sendCmd(0, CMD.LED_ELEC_SHORT, 0)">Trigger Damage</button>
      </div>
      <div class="row">
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'alt', 'Assembly Mode',
          () => sendCmd(0, CMD.LED_ASSEMBLY_MODE, 1),
          () => sendCmd(0, CMD.LED_ASSEMBLY_MODE, 0))">Assembly Mode: Off</button>
      </div>
    </div>

    <div class="panel" data-color="purple2">
      <div class="panel-hdr">Demo Mode</div>
      <!-- LED_DEMO_MANUAL is Bridge-local only — start/stop the autonomous
           effect loop from this page. Never sent over ESP-NOW, so it's not
           part of the shared cross-sketch constant table in CLAUDE.md. -->
      <div class="row">
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'alt', 'Demo Mode',
          () => sendCmd(0, CMD.LED_DEMO_MANUAL, 1),
          () => sendCmd(0, CMD.LED_DEMO_MANUAL, 0))">Demo Mode: Off</button>
      </div>
      <!-- Demo mode defaults muted (volume 0) so a unit left running at a
           store/convention/home display doesn't disturb anyone — sound is
           opt-in here, not a "forgot to turn it off" reminder. -->
      <div class="row">
        <button class="off" data-state="off" onclick="toggleOnOff(this, 'blue', 'Demo Sound',
          () => sendCmd(0, CMD.SND_CMD_VOL, 15),
          () => sendCmd(0, CMD.SND_CMD_VOL, 0))">Demo Sound: Off</button>
      </div>
    </div>

    <div class="panel" data-color="amber">
      <div class="panel-hdr">Settings</div>
      <!-- No WiFi-join UI here on purpose — the kit connects via its own
           self-hosted AP for control and OTA, so there's no home network to
           join or forget. See project_prop_controller_kit notes. -->
      <!-- Mirrors DataPad's existing nav_timing preference (1=1.0s, 2=1.5s,
           3=always on) so both front-ends offer the same choices. -->
      <div class="row" data-toggle="1">
        <button onclick="press(this); sendCmd(0, CMD.LED_NAV_MODE, 0); sendCmd(0, CMD.LED_SET_BLINK_MS, 1000)">Nav Blink: 1.0s</button>
        <button onclick="press(this); sendCmd(0, CMD.LED_NAV_MODE, 0); sendCmd(0, CMD.LED_SET_BLINK_MS, 1500)">Nav Blink: 1.5s</button>
        <button class="alt" onclick="press(this); sendCmd(0, CMD.LED_NAV_MODE, 1)">Nav: Always On</button>
      </div>
      <div class="row">
        <button class="warn" onclick="if(confirm('Reset Bridge and EngRoom to factory default settings?')){press(this); sendCmd(0, CMD.LED_FACTORY_RESET, 0)}">Reset to Default Settings</button>
      </div>
      <!-- Manual trigger for DataPad auto-adopt pairing — for a customer who
           added a DataPad after buying the base kit. Bridge/EngRoom open a
           temporary listen-and-adopt window on this command; not implemented
           in firmware yet, see DEVNOTES Fix Queue #9. -->
      <div class="row">
        <button onclick="press(this); sendCmd(0, CMD.LED_PAIR_DATAPAD, 0)">Pair New DataPad</button>
      </div>
    </div>

    <div class="panel">
      <div class="panel-hdr">MAC Addresses</div>
      <!-- Recovers Bridge/EngRoom/DataPad MACs without USB/serial access —
           the boards may already be installed in the model by the time
           someone wants to check or re-pair one. -->
      <div class="row">
        <button onclick="press(this); showMacs()">Show MAC Addresses</button>
      </div>
      <div id="macList"></div>
    </div>

    <div class="panel">
      <div class="panel-hdr">Log</div>
      <div id="log"></div>
    </div>

  </div>

  <div class="lcars-footer">
    <div class="ftr-bar"><span>Bridge Control</span></div>
  </div>

</div>

<script>
  // Mirrors the shared ledCmd constants and Bridge_ESP.ino pinArray indices — see CLAUDE.md.
  const CMD = {
    LED_ON: 1, LED_OFF: 2, LED_DIM: 3, LED_BLINK: 4, LED_ENGINE: 5,
    LED_ELEC_SHORT: 6, LED_STARTUP: 7, LED_SHUTDOWN: 8,
    LED_SET_BLINK_MS: 20,
    LED_ASSEMBLY_MODE: 27, LED_WARP: 28,
    LED_FACTORY_RESET: 32, // shared — Bridge broadcasts to EngRoom too
    LED_NAV_MODE: 33, // shared — Bridge broadcasts to EngRoom too; not yet in firmware, see DEVNOTES Fix Queue #8
    LED_PAIR_DATAPAD: 34, // shared — Bridge broadcasts to EngRoom too; not yet in firmware, see DEVNOTES Fix Queue #9
    LED_ALL_OFF: 99,
    SND_CMD_PLAY: 50, SND_CMD_STOP: 51, SND_CMD_VOL: 52,
    // Bridge-local only (see Demo Mode panel below) — never sent over ESP-NOW,
    // so it's kept well clear of the shared ledCmd number range (1-33, 50+).
    LED_DEMO_MANUAL: 100
  };
  // GROUP.PHOTON/PHOTON_AFT/DEFLECTOR/NAC_BOTH are EngRoom's IDX_PHOTON/
  // IDX_PHOTON_AFT/IDX_DEFLECTOR/GRP_BOTH_NAC (Engine_Room_ESP.ino) — Bridge's
  // /cmd handler relays these (and LED_WARP) to EngRoom over ESP-NOW rather
  // than applying them locally. PHOTON_AFT is the Photon 2x variant's aft LED.
  const GROUP = { NAV: 15, IM_LEFT: 16, IM_RIGHT: 17, PHOTON: 10, PHOTON_AFT: 11, DEFLECTOR: 6, NAC_BOTH: 20 };
  // File numbers from SOUND_MAP.md — Bridge DY-SV17F, files 01-25 only.
  const SND = { RED_ALERT: 6, FIRE_WEAPONS: 19 };
  function randomSoundFile() {
    return Math.floor(Math.random() * 25) + 1;
  }

  const logEl = document.getElementById('log');
  const statusBannerEl = document.getElementById('statusBanner');
  const statusTitleEl = document.getElementById('statusTitle');

  // Starts at 0 to match the demo-mode muted default (see Demo Mode panel).
  let bridgeVolume = 0;
  function adjustVolume(step) {
    bridgeVolume = Math.min(30, Math.max(0, bridgeVolume + step));
    sendCmd(0, CMD.SND_CMD_VOL, bridgeVolume);
  }

  // Single-button on/off toggle — swaps label/color instead of the old
  // separate On/Off button pair. colorClass is the button's "on" color
  // (e.g. 'blue', 'alt'); dimmed via .off when in the off state.
  function toggleOnOff(btn, colorClass, label, onAction, offAction) {
    const turningOn = btn.dataset.state !== 'on';
    btn.dataset.state = turningOn ? 'on' : 'off';
    btn.textContent = `${label}: ${turningOn ? 'On' : 'Off'}`;
    btn.classList.toggle('off', !turningOn);
    btn.classList.toggle(colorClass, turningOn);
    if (turningOn) onAction(); else offAction();
  }

  function press(btn) {
    const row = btn.closest('.row');
    if (row && row.dataset.toggle) {
      row.querySelectorAll('button').forEach(b => b.classList.remove('is-active'));
      btn.classList.add('is-active');
    } else {
      btn.classList.add('is-active');
      setTimeout(() => btn.classList.remove('is-active'), 350);
    }
  }

  function log(msg) {
    const t = new Date().toLocaleTimeString();
    logEl.textContent += `[${t}] ${msg}\n`;
    logEl.scrollTop = logEl.scrollHeight;
  }

  async function showMacs() {
    const el = document.getElementById('macList');
    el.textContent = 'Loading...';
    try {
      const res = await fetch('/macs');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const d = await res.json();
      el.textContent = `Bridge:  ${d.bridge}\nEngRoom: ${d.engroom}\nDataPad: ${d.datapad}`;
    } catch (err) {
      el.textContent = `Error: ${err.message}`;
    }
  }

  async function sendCmd(group, cmd, value) {
    const url = `/cmd?group=${group}&cmd=${cmd}&value=${value}`;
    log(`-> ${url}`);
    try {
      const res = await fetch(url);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      log(`<- ok`);
    } catch (err) {
      log(`<- Bridge unreachable (${err.message})`);
    }
    pollStatus();   // refresh the header immediately rather than waiting for the next tick
  }

  // SYSTEM ONLINE/OFFLINE reflects real EngRoom connectivity (Bridge's /status
  // endpoint — espNowReceived/connectionLost), NOT whether this fetch itself
  // succeeded. A fetch to /cmd or /status always succeeds as long as Bridge's
  // own web server answers, which tells you nothing about EngRoom — that was
  // the bug in the old sendCmd()-sets-ONLINE-on-any-success logic (see
  // DEVNOTES Fix History): unplugging EngRoom entirely still showed ONLINE
  // because Bridge itself was still reachable.
  async function pollStatus() {
    try {
      const res = await fetch('/status');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const d = await res.json();
      if (d.online) {
        statusBannerEl.classList.add('hidden');
        statusTitleEl.textContent = 'SYSTEM ONLINE';
      } else {
        statusBannerEl.textContent = 'EngRoom not responding — check it is powered on. Nav, Impulse, and Sound may still work from Bridge alone.';
        statusBannerEl.classList.remove('hidden');
        statusTitleEl.textContent = 'SYSTEM OFFLINE';
      }
    } catch (err) {
      // Bridge itself unreachable — a different failure than "EngRoom hasn't
      // been heard from," worth distinguishing in the header.
      statusBannerEl.textContent = 'Cannot reach Bridge — confirm your phone is still joined to its WiFi (Enterprise D), then reload.';
      statusBannerEl.classList.remove('hidden');
      statusTitleEl.textContent = 'BRIDGE UNREACHABLE';
    }
  }
  pollStatus();
  setInterval(pollStatus, 5000);
</script>

</body>
</html>
)HTMLPAGE";

#endif
