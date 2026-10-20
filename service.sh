#!/system/bin/sh
FLAG=/data/local/tmp/codm_skin_unlock.enabled
[ -f "$FLAG" ] || echo 1 > "$FLAG"
