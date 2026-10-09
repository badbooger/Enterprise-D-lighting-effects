#!/bin/bash
# One-time install on the bench Pi: sudo ./install.sh
set -e
cd "$(dirname "$0")"
chmod +x load_sounds.sh
install -m 644 99-sound-loader.rules /etc/udev/rules.d/99-sound-loader.rules
install -m 644 sound-loader@.service /etc/systemd/system/sound-loader@.service
systemctl daemon-reload
udevadm control --reload-rules
echo "Sound loader installed. Plug in a DY-SV17F module to load it."
