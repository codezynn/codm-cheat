#!/system/bin/sh
# CODM Skin Unlock ON (+ optional local visual skin id)
# Author: zynn
# MT Manager: put in /data/adb → Execute
#
# Usage:
#   sh activate_skin_unlock.sh              # ownership force only
#   sh activate_skin_unlock.sh 12345678     # also force this skin id on YOUR model in-match

FLAG="/data/local/tmp/codm_skin_unlock.enabled"
PREF="/data/local/tmp/codm_skin_pref_id"
LOG="/data/local/tmp/codm_skin_unlock.log"
MODID="com.night.codm.skin"
TARGET="/data/adb/activate_skin_unlock.sh"

[ -d /data/adb ] && cp "$0" "$TARGET" 2>/dev/null && chmod 755 "$TARGET" 2>/dev/null
: > "$LOG" 2>/dev/null
log() { echo "$*"; echo "[$(date '+%H:%M:%S')] $*" >> "$LOG" 2>/dev/null; }

log "=== CODM Skin Unlock ON — zynn ==="
id | grep -q "uid=0" || { log "[!] not root"; exit 1; }
log "[+] root ok"

PKG=""
for p in com.activision.callofduty.shooter com.garena.game.codm com.vng.codmvn com.tencent.tmgp.cod; do
    pm path "$p" >/dev/null 2>&1 && PKG="$p" && break
done
[ -z "$PKG" ] && { log "[!] CODM not installed"; exit 1; }
log "[+] package: $PKG"

[ -d "/data/adb/modules/$MODID" ] && log "[+] Zygisk module installed" || log "[!] module missing — flash zip + reboot"

echo 1 > "$FLAG"
chmod 644 "$FLAG"
log "[+] flag ON"

# optional preferred skin id for local 1P visual in-match
if [ -n "$1" ]; then
    echo "$1" > "$PREF"
    chmod 644 "$PREF"
    log "[+] local visual skin id: $1"
else
    rm -f "$PREF"
    log "[*] no pref id — ownership force only (loadout selection)"
fi

log "[*] force-stop $PKG"
am force-stop "$PKG" 2>/dev/null
sleep 1
for pid in $(pidof "$PKG" 2>/dev/null); do kill -9 "$pid" 2>/dev/null; done
sleep 1

log "[*] launching..."
monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1
sleep 2
pidof "$PKG" >/dev/null 2>&1 || am start -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -p "$PKG" >/dev/null 2>&1
sleep 2
PID=$(pidof "$PKG" 2>/dev/null | awk '{print $1}')
[ -n "$PID" ] && log "[+] game pid=$PID" || log "[!] open CODM manually"

log ""
log "=== DONE ==="
log "you: ownership + local visual (if id given)"
log "others/server: unchanged"
log "disable: sh /data/adb/deactivate_skin_unlock.sh"
