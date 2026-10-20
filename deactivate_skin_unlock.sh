#!/system/bin/sh
# CODM Skin Unlock OFF
# Author: zynn
FLAG="/data/local/tmp/codm_skin_unlock.enabled"
PREF="/data/local/tmp/codm_skin_pref_id"
TARGET="/data/adb/deactivate_skin_unlock.sh"
[ -d /data/adb ] && cp "$0" "$TARGET" 2>/dev/null && chmod 755 "$TARGET" 2>/dev/null

echo "=== CODM Skin Unlock OFF — zynn ==="
id | grep -q "uid=0" || { echo "[!] not root"; exit 1; }
rm -f "$FLAG" "$PREF"
echo "[+] flags cleared"

PKG=""
for p in com.activision.callofduty.shooter com.garena.game.codm com.vng.codmvn com.tencent.tmgp.cod; do
    pm path "$p" >/dev/null 2>&1 && PKG="$p" && break
done
if [ -n "$PKG" ]; then
    am force-stop "$PKG" 2>/dev/null
    for pid in $(pidof "$PKG" 2>/dev/null); do kill -9 "$pid" 2>/dev/null; done
    echo "[+] game stopped"
fi
echo "=== DONE — next launch normal ==="
