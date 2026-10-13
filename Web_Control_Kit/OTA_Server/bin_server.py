#!/usr/bin/env python3
# Serves compiled .bin files for Batch_Bootstrap's HTTPUpdate pull, a
# bench dashboard (/status) showing each board's pairing/flash progress and the
# sound loader's state, and a kit packing checklist (/pack). No dependencies.
# Bench-only — never shipped.

import http.server
import functools
import os
import csv
import json
import datetime
import threading
import urllib.parse

PORT = 8000
BINS_DIR = "bins"
MAC_LOG = "mac_log.csv"
EVENT_LOG = "events.csv"
SOUND_STATUS = "sound_status.json"
SOUND_LOG = "sound_log.csv"
EVENT_FIELDS = ["timestamp", "mac", "role", "stage", "peer", "detail", "client_ip"]
PACK_LIST = "packing_list.json"
PACK_STATE = "packing_state.json"
PACK_LOG = "packing_log.csv"

lock = threading.Lock()
boards = {}   # mac -> {role, peer, ip, stages: {stage: timestamp}, detail, last}


def now():
    return datetime.datetime.now().isoformat(timespec="seconds")


def apply_event(ev):
    b = boards.setdefault(ev["mac"], {"role": "", "peer": "", "ip": "", "stages": {}, "detail": "", "last": ""})
    b["role"] = ev["role"] or b["role"]
    if ev["peer"]:
        b["peer"] = ev["peer"]
    b["ip"] = ev["client_ip"]
    b["last"] = ev["timestamp"]
    if ev["stage"] == "joined":
        # A fresh boot of bootstrap firmware — earlier flash results no longer apply.
        b["stages"] = {}
    b["stages"][ev["stage"]] = ev["timestamp"]
    if ev["stage"] in ("flash_retry", "flash_failed", "flashed"):
        b["detail"] = ev["detail"]


def load_events():
    if not os.path.exists(EVENT_LOG):
        return
    with open(EVENT_LOG, newline="") as f:
        for ev in csv.DictReader(f):
            apply_event(ev)


def record_event(ev):
    new_file = not os.path.exists(EVENT_LOG)
    with open(EVENT_LOG, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=EVENT_FIELDS)
        if new_file:
            w.writeheader()
        w.writerow(ev)
    apply_event(ev)


def board_state(b):
    s = b["stages"]
    if "flashed" in s:
        return "flashed"
    if "flash_failed" in s:
        return "failed"
    if "paired" in s:
        return "flashing" if "flash_retry" not in s else "retrying"
    if "joined" in s:
        return "pairing"
    return "unknown"


def build_state():
    rows, used = [], set()
    for mac, b in boards.items():
        if mac in used:
            continue
        peer = boards.get(b["peer"])
        if peer and peer["peer"] == mac:
            used.update({mac, b["peer"]})
            bridge, eng = (mac, b["peer"]) if b["role"] == "Bridge" else (b["peer"], mac)
            members = [bridge, eng]
        else:
            used.add(mac)
            members = [mac]
        rows.append(members)

    def view(mac):
        b = boards[mac]
        return {"mac": mac, "role": b["role"], "peer": b["peer"], "ip": b["ip"],
                "stages": b["stages"], "detail": b["detail"], "last": b["last"],
                "state": board_state(b)}

    pairs = []
    for members in rows:
        views = [view(m) for m in members]
        ok = len(views) == 2 and all(v["state"] == "flashed" for v in views)
        failed = any(v["state"] == "failed" for v in views)
        pairs.append({"boards": views, "result": "PASS" if ok else "FAIL" if failed else "IN PROGRESS",
                      "last": max(v["last"] for v in views)})
    pairs.sort(key=lambda p: p["last"], reverse=True)

    sound = {"state": "idle", "message": "No module plugged in yet", "copied": 0, "total": 0, "time": ""}
    if os.path.exists(SOUND_STATUS):
        try:
            with open(SOUND_STATUS) as f:
                sound = json.load(f)
        except (OSError, json.JSONDecodeError):
            pass   # loader is mid-write; next poll picks it up
    loaded = 0
    if os.path.exists(SOUND_LOG):
        with open(SOUND_LOG) as f:
            loaded = sum(1 for line in f if ",DONE," in line)

    return {"pairs": pairs, "sound": sound, "sound_loaded": loaded, "now": now()}


def archive_events():
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    for name in (EVENT_LOG, SOUND_LOG):
        if os.path.exists(name):
            os.rename(name, name.replace(".csv", f"_{stamp}.csv"))
    boards.clear()


def item_id(section, item):
    return f"{section['name']}/{item['label']}"


def load_pack_list():
    with open(PACK_LIST, encoding="utf-8") as f:
        return json.load(f)


def load_pack_checked():
    # Kept on disk so a tablet refresh or a Pi reboot mid-kit doesn't lose ticks.
    if not os.path.exists(PACK_STATE):
        return set()
    with open(PACK_STATE) as f:
        return set(json.load(f))


def save_pack_checked(checked):
    with open(PACK_STATE, "w") as f:
        json.dump(sorted(checked), f)


def packed_count():
    if not os.path.exists(PACK_LOG):
        return 0
    with open(PACK_LOG, newline="") as f:
        return sum(1 for _ in csv.DictReader(f))


def pack_state():
    return {"list": load_pack_list(), "checked": sorted(load_pack_checked()), "kit": packed_count() + 1}


def finish_kit():
    lst, checked = load_pack_list(), load_pack_checked()
    missing = [item_id(sec, it) for sec in lst["sections"] if not sec.get("optional")
               for it in sec["items"] if item_id(sec, it) not in checked]
    if missing:
        return False, "Not everything is checked: " + ", ".join(missing)
    extras = [it["label"] for sec in lst["sections"] if sec.get("optional")
              for it in sec["items"] if item_id(sec, it) in checked]
    new_file = not os.path.exists(PACK_LOG)
    with open(PACK_LOG, "a", newline="") as f:
        w = csv.writer(f)
        if new_file:
            w.writerow(["kit", "timestamp", "options"])
        w.writerow([packed_count() + 1, now(), "; ".join(extras)])
    save_pack_checked(set())
    return True, "ok"


class Handler(http.server.SimpleHTTPRequestHandler):
    def send_body(self, code, ctype, body):
        data = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        q = {k: v[0] for k, v in urllib.parse.parse_qs(url.query).items()}

        if url.path == "/report":
            ev = {"timestamp": now(), "mac": q.get("mac", "").upper(), "role": q.get("role", ""),
                  "stage": q.get("stage", ""), "peer": q.get("peer", "").upper(),
                  "detail": q.get("detail", ""), "client_ip": self.client_address[0]}
            if not ev["mac"] or not ev["stage"]:
                return self.send_body(400, "text/plain", "mac and stage required\n")
            print(f"REPORT {ev['role']} {ev['mac']} {ev['stage']} {ev['detail']}", flush=True)
            with lock:
                record_event(ev)
            return self.send_body(200, "text/plain", "ok\n")

        if url.path in ("/status", "/status/"):
            return self.send_body(200, "text/html; charset=utf-8", DASHBOARD_HTML)

        if url.path in ("/pack", "/pack/"):
            return self.send_body(200, "text/html; charset=utf-8", PACK_HTML)

        if url.path == "/api/pack":
            with lock:
                return self.send_body(200, "application/json", json.dumps(pack_state()))

        if url.path == "/api/state":
            with lock:
                return self.send_body(200, "application/json", json.dumps(build_state()))

        if url.path.endswith(".bin"):
            # HTTPUpdate.cpp sends this automatically on every request — free MAC capture,
            # no firmware-side code needed.
            mac = self.headers.get("x-ESP32-STA-MAC", "unknown")
            print(f"GET {self.path}  from MAC={mac}", flush=True)
            new_file = not os.path.exists(MAC_LOG)
            with open(MAC_LOG, "a") as f:
                if new_file:
                    f.write("timestamp,file,mac,client_ip\n")
                f.write(f"{now()},{self.path.lstrip('/')},{mac},{self.client_address[0]}\n")
        return super().do_GET()

    def do_POST(self):
        url = urllib.parse.urlparse(self.path)
        q = {k: v[0] for k, v in urllib.parse.parse_qs(url.query).items()}
        if url.path == "/api/new-batch":
            with lock:
                archive_events()
            return self.send_body(200, "text/plain", "ok\n")
        if url.path == "/api/pack/toggle":
            with lock:
                checked = load_pack_checked()
                checked ^= {q.get("id", "")}
                save_pack_checked(checked)
                return self.send_body(200, "application/json", json.dumps(pack_state()))
        if url.path == "/api/pack/clear":
            with lock:
                save_pack_checked(set())
                return self.send_body(200, "application/json", json.dumps(pack_state()))
        if url.path == "/api/pack/done":
            with lock:
                ok, msg = finish_kit()
                return self.send_body(200 if ok else 409, "text/plain", msg + "\n")
        self.send_body(404, "text/plain", "not found\n")

    def log_message(self, fmt, *args):
        if not self.path.startswith("/api/"):   # dashboard polls every 2s — keep the journal readable
            super().log_message(fmt, *args)


DASHBOARD_HTML = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bench Provisioning</title>
<style>
  :root { --bg:#111; --panel:#1c1c1c; --text:#eee; --dim:#888; --ok:#3c3; --bad:#e44; --run:#fc3; --line:#333; }
  body { background:var(--bg); color:var(--text); font:15px system-ui, sans-serif; margin:0; padding:16px; }
  h1 { font-size:20px; margin:0 0 4px; } h2 { font-size:16px; margin:24px 0 8px; color:var(--dim); text-transform:uppercase; letter-spacing:.05em; }
  .sub { color:var(--dim); font-size:13px; }
  .card { background:var(--panel); border:1px solid var(--line); border-radius:8px; padding:12px 14px; margin-bottom:10px; }
  .head { display:flex; justify-content:space-between; align-items:center; gap:8px; margin-bottom:8px; }
  .badge { font-weight:700; padding:3px 10px; border-radius:4px; color:#000; }
  .PASS { background:var(--ok); } .FAIL { background:var(--bad); } .INPROGRESS { background:var(--run); }
  .board { display:grid; grid-template-columns:90px 1fr; gap:2px 10px; padding:6px 0; border-top:1px solid var(--line); }
  .mono { font-family:ui-monospace, monospace; }
  .steps span { display:inline-block; margin-right:12px; } .done { color:var(--ok); } .wait { color:var(--dim); } .err { color:var(--bad); }
  .sound-big { font-size:22px; font-weight:700; }
  .sound-done { color:var(--ok); } .sound-failed { color:var(--bad); } .sound-copying, .sound-checking { color:var(--run); }
  button { background:#333; color:var(--text); border:1px solid #555; border-radius:4px; padding:6px 12px; cursor:pointer; }
  .empty { color:var(--dim); }
</style></head><body>
<h1>Bench Provisioning</h1>
<div class="sub">Updates every 2s · <span id="now"></span> · <span id="conn"></span> · <a href="/pack" style="color:var(--run)">Packing checklist</a></div>

<h2>Sound module</h2>
<div class="card" id="sound"></div>

<h2>Board pairs</h2>
<div id="pairs"></div>
<p><button onclick="newBatch()">Start new batch (archives the list)</button></p>

<script>
const STEPS = [["joined","Joined"],["paired","Paired"],["flashed","Flashed"]];
function esc(s){ return String(s ?? "").replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c])); }
function t(ts){ return ts ? ts.slice(11) : ""; }

function boardHtml(b){
  const steps = STEPS.map(([k,label]) => b.stages[k]
      ? `<span class="done">✔ ${label} ${t(b.stages[k])}</span>`
      : `<span class="wait">○ ${label}</span>`).join("");
  let extra = "";
  if (b.stages.flash_failed) extra = `<span class="err">✘ Flash failed: ${esc(b.detail)}</span>`;
  else if (b.stages.flash_retry && !b.stages.flashed) extra = `<span class="err">retrying: ${esc(b.detail)}</span>`;
  return `<div class="board"><b>${esc(b.role || "?")}</b><span class="mono">${esc(b.mac)}${b.peer ? " → " + esc(b.peer) : ""}</span>
          <span></span><span class="steps">${steps}${extra}</span></div>`;
}

function render(d){
  document.getElementById("now").textContent = d.now.replace("T"," ");
  const s = d.sound;
  const prog = s.total ? ` (${s.copied}/${s.total})` : "";
  document.getElementById("sound").innerHTML =
    `<div class="sound-big sound-${esc(s.state)}">${esc(s.message)}${prog}</div>
     <div class="sub">${s.time ? "at " + esc(t(s.time)) + " · " : ""}${d.sound_loaded} module(s) loaded this batch</div>`;
  const el = document.getElementById("pairs");
  if (!d.pairs.length) { el.innerHTML = `<div class="card empty">No boards have reported yet. Power on a Bridge + EngRoom pair running Batch_Bootstrap.</div>`; return; }
  el.innerHTML = d.pairs.map(p => `<div class="card"><div class="head">
      <span>${p.boards.length === 2 ? "Bridge + EngRoom" : "Waiting for partner"}</span>
      <span class="badge ${p.result.replace(" ","")}">${p.result}</span></div>
      ${p.boards.map(boardHtml).join("")}</div>`).join("");
}

async function poll(){
  try {
    const r = await fetch("/api/state", {cache:"no-store"});
    render(await r.json());
    document.getElementById("conn").textContent = "connected";
  } catch(e) { document.getElementById("conn").textContent = "Pi not responding"; }
}
async function newBatch(){
  if (!confirm("Archive the current list and start a new batch?")) return;
  await fetch("/api/new-batch", {method:"POST"}); poll();
}
poll(); setInterval(poll, 2000);
</script></body></html>
"""

PACK_HTML = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Kit Packing</title>
<style>
  :root { --bg:#111; --panel:#1c1c1c; --text:#eee; --dim:#888; --ok:#3c3; --bad:#e44; --run:#fc3; --line:#333; }
  body { background:var(--bg); color:var(--text); font:18px system-ui, sans-serif; margin:0; padding:16px; max-width:900px; }
  a { color:var(--run); }
  .top { display:flex; justify-content:space-between; align-items:baseline; gap:12px; flex-wrap:wrap; }
  h1 { font-size:26px; margin:0; } .sub { color:var(--dim); font-size:15px; }
  h2 { font-size:15px; margin:22px 0 6px; color:var(--dim); text-transform:uppercase; letter-spacing:.05em; }
  .item { display:flex; align-items:center; gap:14px; background:var(--panel); border:1px solid var(--line); border-radius:8px;
          padding:14px; margin-bottom:6px; cursor:pointer; user-select:none; -webkit-tap-highlight-color:transparent; }
  .box { width:30px; height:30px; flex:none; border:2px solid #666; border-radius:6px; display:grid; place-items:center; font-size:22px; font-weight:700; }
  .item.on { border-color:#264d26; } .item.on .box { background:var(--ok); border-color:var(--ok); color:#000; }
  .item.on .label { color:var(--dim); text-decoration:line-through; }
  .qty { margin-left:auto; font-weight:700; font-size:20px; white-space:nowrap; }
  .note { color:var(--dim); font-size:14px; }
  .bar { position:sticky; bottom:0; background:var(--bg); padding:12px 0; border-top:1px solid var(--line); margin-top:18px;
         display:flex; gap:10px; align-items:center; flex-wrap:wrap; }
  button { background:#333; color:var(--text); border:1px solid #555; border-radius:6px; padding:14px 18px; font-size:18px; cursor:pointer; }
  #done:enabled { background:var(--ok); color:#000; border-color:var(--ok); font-weight:700; }
  #done:disabled { opacity:.5; }
  #msg { font-size:16px; } .err { color:var(--bad); } .good { color:var(--ok); }
</style></head><body>
<div class="top"><h1>Packing kit #<span id="kit">–</span></h1>
  <span class="sub"><span id="count"></span> · <a href="/status">Bench dashboard</a></span></div>
<div id="list"></div>
<div class="bar">
  <button id="done" onclick="done()">Kit packed</button>
  <button onclick="clearAll()">Clear checks</button>
  <span id="msg"></span>
</div>
<script>
let state = null;
function esc(s){ return String(s ?? "").replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c])); }
function id(sec, it){ return sec.name + "/" + it.label; }

function render(){
  const checked = new Set(state.checked);
  let need = 0, have = 0;
  document.getElementById("kit").textContent = state.kit;
  document.getElementById("list").innerHTML = state.list.sections.map(sec => {
    const rows = sec.items.map(it => {
      const k = id(sec, it), on = checked.has(k);
      if (!sec.optional) { need++; if (on) have++; }
      return `<div class="item ${on ? "on" : ""}" data-id="${esc(k)}">
        <div class="box">${on ? "✓" : ""}</div>
        <div><div class="label">${esc(it.label)}</div>${it.note ? `<div class="note">${esc(it.note)}</div>` : ""}</div>
        <div class="qty">× ${esc(it.qty)}</div></div>`;
    }).join("");
    return `<h2>${esc(sec.name)}</h2>${rows}`;
  }).join("");
  document.getElementById("count").textContent = `${have} of ${need} checked`;
  document.getElementById("done").disabled = have < need;
}

async function call(path, method){
  try {
    const r = await fetch(path, {method: method || "GET", cache:"no-store"});
    if (!r.ok) throw new Error(await r.text());
    return r;
  } catch(e) {
    document.getElementById("msg").innerHTML = `<span class="err">Didn't reach the Pi — ${esc(e.message)}</span>`;
    return null;
  }
}
async function load(){ const r = await call("/api/pack"); if (r) { state = await r.json(); render(); } }

document.getElementById("list").addEventListener("click", async e => {
  const row = e.target.closest(".item"); if (!row) return;
  document.getElementById("msg").textContent = "";
  const r = await call("/api/pack/toggle?id=" + encodeURIComponent(row.dataset.id), "POST");
  if (r) { state = await r.json(); render(); }
});
async function done(){
  const kit = state.kit;
  if (!confirm(`Kit #${kit} packed and box closed?`)) return;
  if (await call("/api/pack/done", "POST")) {
    await load();
    document.getElementById("msg").innerHTML = `<span class="good">Kit #${kit} logged. Ready for the next one.</span>`;
    window.scrollTo(0, 0);
  }
}
async function clearAll(){
  if (!confirm("Untick everything for this kit?")) return;
  const r = await call("/api/pack/clear", "POST");
  if (r) { state = await r.json(); render(); }
}
document.addEventListener("visibilitychange", () => { if (!document.hidden) load(); });
load();
</script></body></html>
"""

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    load_events()
    handler = functools.partial(Handler, directory=BINS_DIR)
    # Threaded so dashboard polls never stall a board's .bin download (or vice versa).
    with http.server.ThreadingHTTPServer(("", PORT), handler) as httpd:
        print(f"Serving {BINS_DIR}/ at http://0.0.0.0:{PORT}  (dashboard: /status)")
        httpd.serve_forever()
