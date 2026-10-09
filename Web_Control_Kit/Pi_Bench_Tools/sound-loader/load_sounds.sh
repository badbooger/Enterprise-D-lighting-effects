#!/bin/bash
# Loads the 25 Bridge sound files onto a plugged-in DY-SV17F module.
# Started by sound-loader@<dev>.service, which 99-sound-loader.rules triggers.
# Progress goes to sound_status.json for bin_server.py's /status dashboard.
set -u

DEV="/dev/$1"
SRC=/home/pi/bench/Sound_Loader/sounds
OUT=/home/pi/bench/OTA_Server
STATUS="$OUT/sound_status.json"
LOG="$OUT/sound_log.csv"
MNT=/run/sound-loader
# The module is 4MB. Anything bigger is some other USB drive — never touch it.
MAX_BYTES=$((8 * 1024 * 1024))

ts() { date +%Y-%m-%dT%H:%M:%S; }

status() {   # state message copied total
  printf '{"state":"%s","message":"%s","copied":%d,"total":%d,"time":"%s","device":"%s"}\n' \
    "$1" "$2" "$3" "$4" "$(ts)" "$DEV" > "$STATUS.tmp" && mv "$STATUS.tmp" "$STATUS"
}

log() {   # result detail
  [ -f "$LOG" ] || echo "timestamp,device,result,detail" > "$LOG"
  echo "$(ts),$DEV,$1,$2" >> "$LOG"
}

fail() {
  status failed "FAILED: $1 — unplug and plug back in to retry" 0 0
  log FAILED "$1"
  mountpoint -q "$MNT" && umount "$MNT"
  exit 1
}

size=$(blockdev --getsize64 "$DEV" 2>/dev/null) || fail "can't read $DEV"
[ "$size" -le "$MAX_BYTES" ] || fail "$DEV is $size bytes — too big to be a sound module, left alone"

files=("$SRC"/*.mp3)
total=${#files[@]}
[ "$total" -eq 25 ] || fail "expected 25 .mp3 files in $SRC, found $total"

status checking "Mounting module" 0 "$total"
mkdir -p "$MNT"
mountpoint -q "$MNT" && umount "$MNT"
mount -t vfat -o flush "$DEV" "$MNT" || fail "mount failed"

# Clear first: 25 files are 3.9MB on a 4MB module, and the player plays in the
# order files were written, so everything goes in fresh and in order.
status checking "Clearing old files" 0 "$total"
find "$MNT" -maxdepth 1 -type f -delete || fail "couldn't clear old files"
sync

need=$(du -cb "${files[@]}" | tail -1 | cut -f1)
avail=$(df --output=avail -B1 "$MNT" | tail -1 | tr -d ' ')
[ "$need" -le "$avail" ] || fail "not enough space: need $need bytes, module has $avail"

i=0
for f in "${files[@]}"; do
  cp "$f" "$MNT/" || fail "copy failed on $(basename "$f")"
  sync
  i=$((i + 1))
  status copying "Copying sounds" "$i" "$total"
done

status checking "Verifying" "$total" "$total"
echo 3 > /proc/sys/vm/drop_caches   # make cmp read back from the module, not the page cache
for f in "${files[@]}"; do
  cmp -s "$f" "$MNT/$(basename "$f")" || fail "verify failed on $(basename "$f")"
done

umount "$MNT" || fail "unmount failed — don't unplug yet"
status done "DONE — safe to remove" "$total" "$total"
log DONE "$total files"
