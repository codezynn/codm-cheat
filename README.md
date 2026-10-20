# CODM Skin Unlock — zynn (1.6.57)
Client ownership + **local 1P visual force** (your model only).

## Rank
- You can see the skin on **your** character in-match if you pass a skin id.
- Other players and server still see your real loadout.

## MT Manager
1. Flash module (build .so first) → Zygisk ON → reboot
2. Copy scripts to `/data/adb/`
3. ON:  `sh /data/adb/activate_skin_unlock.sh`  
   or with forced visual id: `sh /data/adb/activate_skin_unlock.sh <skinId>`
4. OFF: `sh /data/adb/deactivate_skin_unlock.sh`

## Offsets
Find 0x845DD90 | AddFromItemId 0x845DA8C | DoChange 0x9B82570 | Process 0x9B8B0CC
