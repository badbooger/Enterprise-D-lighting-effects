#!/usr/bin/env bash
# Sets up a dedicated, isolated bench WiFi AP on a Raspberry Pi (onboard wlan0,
# Raspberry Pi OS Lite / NetworkManager) for batch-provisioning ESP32 boards
# running Batch_Bootstrap/Batch_Bootstrap.ino.
#
# Deliberately isolated: no gateway/DNS handed out, no IP forwarding, no NAT.
# A laptop joined to this AP keeps using its own Ethernet connection for
# internet access — this network only exists to let boards join and get an IP.
#
# Usage:
#   sudo ./setup-bootstrap-ap.sh            # set up the AP
#   sudo ./setup-bootstrap-ap.sh --status   # report current state, no changes
#   sudo ./setup-bootstrap-ap.sh --rollback # undo everything this script applied

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/bootstrap-ap.env"
DNSMASQ_TEMPLATE="$SCRIPT_DIR/dnsmasq-bootstrap-ap.conf.template"
DNSMASQ_DROPIN="/etc/dnsmasq.d/bootstrap-ap.conf"
CON_NAME="bootstrap-ap"
IFACE="wlan0"

if [ ! -f "$ENV_FILE" ]; then
  echo "ERROR: $ENV_FILE not found." >&2
  exit 1
fi
# shellcheck source=bootstrap-ap.env
source "$ENV_FILE"

for var in BOOTSTRAP_AP_SSID BOOTSTRAP_AP_PASS BOOTSTRAP_AP_CHANNEL BOOTSTRAP_AP_GATEWAY \
           BOOTSTRAP_AP_DHCP_START BOOTSTRAP_AP_DHCP_END BOOTSTRAP_AP_LEASE_TIME; do
  if [ -z "${!var:-}" ]; then
    echo "ERROR: $var is not set in $ENV_FILE" >&2
    exit 1
  fi
done

if [[ "$BOOTSTRAP_AP_SSID" == "EDIT_BEFORE_FLASHING_SSID" || "$BOOTSTRAP_AP_PASS" == "EDIT_BEFORE_FLASHING_PASS" ]]; then
  echo "ERROR: bootstrap-ap.env still has placeholder SSID/password." >&2
  echo "Edit $ENV_FILE first, matching Batch_Bootstrap.ino's BOOTSTRAP_WIFI_SSID/PASS exactly." >&2
  exit 1
fi

status() {
  echo "=== NetworkManager connection ==="
  nmcli connection show --active | grep -F "$CON_NAME" || echo "(not active)"
  echo
  echo "=== $IFACE ==="
  ip addr show "$IFACE" 2>/dev/null || echo "(no such interface)"
  echo
  echo "=== dnsmasq ==="
  systemctl is-active dnsmasq 2>/dev/null || true
  echo
  echo "=== isolation check ==="
  echo "ip_forward: $(cat /proc/sys/net/ipv4/ip_forward 2>/dev/null || echo unknown)"
  echo "(expect 0 — this network must not route anywhere)"
}

rollback() {
  echo "Rolling back bootstrap AP setup..."
  nmcli connection down "$CON_NAME" 2>/dev/null || true
  nmcli connection delete "$CON_NAME" 2>/dev/null || true
  rm -f "$DNSMASQ_DROPIN"
  systemctl restart dnsmasq 2>/dev/null || systemctl stop dnsmasq 2>/dev/null || true
  echo "Done. $IFACE reverts to its prior NetworkManager-managed state."
}

if [[ "${1:-}" == "--status" ]]; then
  status
  exit 0
fi

if [[ "${1:-}" == "--rollback" ]]; then
  rollback
  exit 0
fi

if [ "$(id -u)" -ne 0 ]; then
  echo "Run as root: sudo $0" >&2
  exit 1
fi

# --- Pre-flight: warn if this very SSH session is routed over wlan0 ---
if [ -n "${SSH_CONNECTION:-}" ]; then
  CLIENT_IP="${SSH_CONNECTION%% *}"
  if ip route get "$CLIENT_IP" 2>/dev/null | grep -q "$IFACE"; then
    echo "WARNING: this SSH session appears to be routed over $IFACE."
    echo "Switching $IFACE to AP mode will likely disconnect you immediately."
    echo "Confirm you have another way back onto this Pi (Ethernet + known IP, or"
    echo "physical console) before continuing."
    read -rp "Continue anyway? [y/N] " ans
    [[ "$ans" == "y" || "$ans" == "Y" ]] || exit 1
  fi
fi

echo "Installing dnsmasq..."
apt-get update -qq
apt-get install -y dnsmasq
systemctl stop dnsmasq   # configure the drop-in before (re)starting

echo "Rendering dnsmasq config -> $DNSMASQ_DROPIN"
sed \
  -e "s/__DHCP_START__/$BOOTSTRAP_AP_DHCP_START/" \
  -e "s/__DHCP_END__/$BOOTSTRAP_AP_DHCP_END/" \
  -e "s/__LEASE_TIME__/$BOOTSTRAP_AP_LEASE_TIME/" \
  "$DNSMASQ_TEMPLATE" > "$DNSMASQ_DROPIN"

echo "Configuring $CON_NAME via NetworkManager..."
nmcli radio wifi on

# Drop any stale profile from a previous run so this stays idempotent.
nmcli connection delete "$CON_NAME" 2>/dev/null || true

# Priority above any client WiFi profile set up in Imager (default 0), so wlan0
# comes back as the AP after a reboot instead of rejoining the home network.
nmcli connection add type wifi ifname "$IFACE" con-name "$CON_NAME" autoconnect yes \
  connection.autoconnect-priority 10 \
  ssid "$BOOTSTRAP_AP_SSID"

# Deliberately ipv4.method manual, NOT "shared" — "shared" bundles DHCP together
# with NAT/IP-forwarding through the Pi's default route interface, which would
# give WiFi clients real internet access through the Pi's Ethernet uplink. That
# is the opposite of what this bench-only network is for.
# proto/pairwise/group pinned to WPA2-CCMP: left unset, NM advertises mixed
# WPA1+WPA2 with a TKIP group cipher, and ESP32s time out joining it.
nmcli connection modify "$CON_NAME" \
  802-11-wireless.mode ap \
  802-11-wireless.band bg \
  802-11-wireless.channel "$BOOTSTRAP_AP_CHANNEL" \
  wifi-sec.key-mgmt wpa-psk \
  wifi-sec.psk "$BOOTSTRAP_AP_PASS" \
  wifi-sec.proto rsn \
  wifi-sec.pairwise ccmp \
  wifi-sec.group ccmp \
  ipv4.method manual \
  ipv4.addresses "${BOOTSTRAP_AP_GATEWAY}/24" \
  ipv4.gateway "" \
  ipv6.method disabled

nmcli connection up "$CON_NAME"

echo "Enabling dnsmasq..."
systemctl enable --now dnsmasq

echo
echo "Bootstrap AP is up. Run '$0 --status' to check it, or '$0 --rollback' to undo."
