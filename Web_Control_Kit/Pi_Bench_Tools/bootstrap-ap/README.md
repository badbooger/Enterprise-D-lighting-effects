# bootstrap-ap

Hosts the dedicated, isolated bench WiFi network that
`../../Batch_Bootstrap/Batch_Bootstrap.ino` joins during batch
provisioning. Runs on a Raspberry Pi (tested on a **Pi 4B, onboard WiFi**).

**Target OS: Raspberry Pi OS Lite, current release.** This relies on
NetworkManager (the current default networking stack) — it will not work on an
older Bullseye-or-earlier image using dhcpcd/hostapd. Flash a fresh SD card with
the current Raspberry Pi OS Lite image if starting from scratch.

**This network is deliberately isolated — no internet access over it, on
purpose.** It hands out an IP address and nothing else: no gateway, no DNS, no
NAT/IP-forwarding anywhere in this setup. A laptop joined to this AP for testing
keeps using its own Ethernet connection for internet access — the two connections don't compete, since this AP never offers a
competing default route.

---

## Before running

Edit `bootstrap-ap.env`:

```bash
BOOTSTRAP_AP_SSID="..."
BOOTSTRAP_AP_PASS="..."
```

**These two values must exactly match `../../Batch_Bootstrap/Batch_Bootstrap.ino`'s
`BOOTSTRAP_WIFI_SSID`/`BOOTSTRAP_WIFI_PASS`.** A mismatch is the #1 cause of a
board stuck printing `WiFi join attempt timed out — retrying...` forever.

The rest of `bootstrap-ap.env` (channel, subnet, DHCP range) has sane defaults —
no need to change these unless you have a specific reason to.

## Running it

```bash
sudo ./setup-bootstrap-ap.sh
```

Safe to re-run — it drops and recreates its own NetworkManager connection
profile each time, and doesn't touch any other network profile on the Pi.

If you're SSH'd into the Pi over WiFi (rather than Ethernet or a direct
console), the script will warn you before proceeding — switching `wlan0` to AP
mode will very likely disconnect that session immediately.

## Checking it

```bash
sudo ./setup-bootstrap-ap.sh --status
```

Or manually:
- `nmcli connection show --active` — should list `bootstrap-ap`
- `ip addr show wlan0` — should show `192.168.50.1/24`
- `systemctl is-active dnsmasq` — should be `active`
- `cat /proc/sys/net/ipv4/ip_forward` — should read `0` (confirms isolation)

From a phone or laptop: the SSID from `bootstrap-ap.env` should be joinable,
and a joined device should get an address in `192.168.50.50`–`192.168.50.150`.
**With your laptop joined to this AP and also connected via Ethernet, confirm
your default internet route still points at Ethernet** (`ip route` / `route -n`
on Linux, `route print` on Windows, `netstat -nr` on macOS) — that's the direct
proof this network isn't fighting your real internet connection.

## Undoing it

```bash
sudo ./setup-bootstrap-ap.sh --rollback
```

Removes the `bootstrap-ap` NetworkManager connection and the dnsmasq drop-in.
`wlan0` and NetworkManager itself are otherwise untouched.

## End-to-end test (once you have a flashed board)

Flash `../../Batch_Bootstrap/Batch_Bootstrap.ino` with matching
`BOOTSTRAP_WIFI_SSID`/`PASS`, watch Serial at 115200 baud:

- **Success:** `WiFi joined.` then `IP address: 192.168.50.x`
- **Failure:** repeating `WiFi join attempt timed out — retrying...` — stop and
  diagnose rather than waiting longer (it retries forever by design). Check, in
  order: (1) SSID/password match between this `.env` and the `.ino` exactly,
  (2) whether a phone can join (isolates Pi-side vs. firmware-side problems),
  (3) `journalctl -u NetworkManager -f` on the Pi while the board retries, for
  an association attempt from the ESP32's MAC.
