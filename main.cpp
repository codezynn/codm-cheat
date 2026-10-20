/*
 * CODM Skin Unlock + Local Visual Force (rank-safe)
 * Author: zynn
 * Target: CODM 1.6.57 libil2cpp
 *
 * 1) Ownership: Find → auto AddFromItemId (client list)
 * 2) Visual: DoChange / Process — when is1P (local), keep forced skin
 * Other players + server unchanged.
 */

#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <link.h>
#include <stdint.h>
#include <fcntl.h>
#include "zygisk.hpp"

#define LOG_TAG "CODMSkin"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ===== 1.6.57 offsets =====
constexpr uintptr_t OFF_ADD_FROM_ITEM_ID = 0x845D78C; // PlayerOwnItemData.AddFromItemId
constexpr uintptr_t OFF_FIND             = 0x845DD90; // PlayerOwnItemData.Find
constexpr uintptr_t OFF_DO_CHANGE        = 0x9B82570; // WeaponSkinManager.DoChange
constexpr uintptr_t OFF_PROCESS          = 0x9B8B0CC; // WeaponSkinManager.Process

static const char* FLAG_PATH = "/data/local/tmp/codm_skin_unlock.enabled";
static const char* PREF_PATH = "/data/local/tmp/codm_skin_pref_id"; // optional forced skin id

static uintptr_t g_il2cpp = 0;
static bool g_enabled = false;
static uint32_t g_pref_skin = 0; // 0 = don't override id, only keep ownership path

static uint8_t tramp_find[64]  __attribute__((aligned(16)));
static uint8_t tramp_do[64]    __attribute__((aligned(16)));
static uint8_t tramp_proc[64]  __attribute__((aligned(16)));

using Find_fn     = void* (*)(void*, uint32_t);
using Add_fn      = void  (*)(void*, uint32_t);
// DoChange(uint skinID, ulong objID, ManagedAsset*, bool is1P, int changeType, bool IsUIMesh, bool isCleanCamo, bool isCamoMaterial, List*, List*)
using DoChange_fn = void  (*)(void*, uint32_t, uint64_t, void*, bool, int, bool, bool, bool, void*, void*);
// Process(uint skinID, ulong objID, ManagedAsset*, bool is1P, int changeType, bool IsUIMesh, bool isCleanCamo, List*, List*)
using Process_fn  = void  (*)(void*, uint32_t, uint64_t, void*, bool, int, bool, bool, void*, void*);

static Find_fn     orig_Find     = nullptr;
static Add_fn      orig_Add      = nullptr;
static DoChange_fn orig_DoChange = nullptr;
static Process_fn  orig_Process  = nullptr;

static inline uintptr_t page_start(uintptr_t a) {
    return a & ~(uintptr_t)(getpagesize() - 1);
}
static bool set_prot(uintptr_t addr, size_t len, int prot) {
    uintptr_t start = page_start(addr);
    size_t sz = (addr - start) + len;
    sz = (sz + getpagesize() - 1) & ~(getpagesize() - 1);
    return mprotect((void*)start, sz, prot) == 0;
}

static bool flag_on() {
    int fd = open(FLAG_PATH, O_RDONLY);
    if (fd < 0) return false;
    char c = 0;
    read(fd, &c, 1);
    close(fd);
    return c == '1';
}

static uint32_t load_pref() {
    int fd = open(PREF_PATH, O_RDONLY);
    if (fd < 0) return 0;
    char buf[32] = {};
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return 0;
    return (uint32_t)strtoul(buf, nullptr, 0);
}

static void* hooked_Find(void* thiz, uint32_t itemId) {
    if (!thiz) return nullptr;
    void* ret = orig_Find ? orig_Find(thiz, itemId) : nullptr;
    if (ret || !g_enabled) return ret;
    if (orig_Add) {
        orig_Add(thiz, itemId);
        ret = orig_Find ? orig_Find(thiz, itemId) : nullptr;
    }
    return ret;
}

// Local visual force: if is1P and we have a preferred id, swap skinID
static void hooked_DoChange(void* thiz, uint32_t skinID, uint64_t objID, void* root,
                            bool is1P, int changeType, bool IsUIMesh, bool isCleanCamo,
                            bool isCamoMaterial, void* rejectList, void* ultList) {
    if (g_enabled && is1P && g_pref_skin != 0)
        skinID = g_pref_skin;
    if (orig_DoChange)
        orig_DoChange(thiz, skinID, objID, root, is1P, changeType, IsUIMesh,
                      isCleanCamo, isCamoMaterial, rejectList, ultList);
}

static void hooked_Process(void* thiz, uint32_t skinID, uint64_t objID, void* root,
                           bool is1P, int changeType, bool IsUIMesh, bool isCleanCamo,
                           void* rejectList, void* ultList) {
    if (g_enabled && is1P && g_pref_skin != 0)
        skinID = g_pref_skin;
    if (orig_Process)
        orig_Process(thiz, skinID, objID, root, is1P, changeType, IsUIMesh,
                     isCleanCamo, rejectList, ultList);
}

static bool hook_arm64(uintptr_t target, void* replace, void** original, uint8_t* tramp) {
    if (!target || !replace) return false;
    if (!set_prot(target, 16, PROT_READ | PROT_WRITE | PROT_EXEC)) return false;
    memcpy(tramp, (void*)target, 16);
    uint32_t* p = (uint32_t*)target;
    p[0] = 0x58000050u; // LDR X16, #8
    p[1] = 0xD61F0200u; // BR X16
    *(uint64_t*)(p + 2) = (uint64_t)replace;
    set_prot(target, 16, PROT_READ | PROT_EXEC);
    uint32_t* t = (uint32_t*)(tramp + 16);
    t[0] = 0x58000050u;
    t[1] = 0xD61F0200u;
    *(uint64_t*)(t + 2) = (uint64_t)(target + 16);
    set_prot((uintptr_t)tramp, 64, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (original) *original = (void*)tramp;
    return true;
}

static int phdr_cb(struct dl_phdr_info* info, size_t, void* data) {
    if (info->dlpi_name && strstr(info->dlpi_name, "libil2cpp.so")) {
        *(uintptr_t*)data = (uintptr_t)info->dlpi_addr;
        return 1;
    }
    return 0;
}
static uintptr_t find_il2cpp() {
    uintptr_t base = 0;
    dl_iterate_phdr(phdr_cb, &base);
    return base;
}

using zygisk::Api;
using zygisk::AppSpecializeArgs;

class CodmSkinModule : public zygisk::ModuleBase {
public:
    void onLoad(Api* api, JNIEnv* env) override {
        api_ = api; env_ = env;
    }
    void preAppSpecialize(AppSpecializeArgs* args) override {
        const char* name = env_->GetStringUTFChars(args->nice_name, nullptr);
        bool target = false;
        if (name) {
            target = strstr(name, "callofduty") || strstr(name, "codm") ||
                     strstr(name, "activision") || strstr(name, "garena") || strstr(name, "vng");
            env_->ReleaseStringUTFChars(args->nice_name, name);
        }
        if (!target) api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        else LOGI("CODM process detected");
    }
    void postAppSpecialize(const AppSpecializeArgs*) override {
        g_enabled = flag_on();
        g_pref_skin = load_pref();
        LOGI("enabled=%d pref_skin=%u", g_enabled ? 1 : 0, g_pref_skin);

        for (int i = 0; i < 40 && !g_il2cpp; ++i) {
            g_il2cpp = find_il2cpp();
            if (!g_il2cpp) usleep(150000);
        }
        if (!g_il2cpp) { LOGE("libil2cpp not found"); return; }
        LOGI("libil2cpp @ %p", (void*)g_il2cpp);

        orig_Add = (Add_fn)(g_il2cpp + OFF_ADD_FROM_ITEM_ID);

        if (hook_arm64(g_il2cpp + OFF_FIND, (void*)hooked_Find, (void**)&orig_Find, tramp_find))
            LOGI("Find hooked");
        if (hook_arm64(g_il2cpp + OFF_DO_CHANGE, (void*)hooked_DoChange, (void**)&orig_DoChange, tramp_do))
            LOGI("DoChange hooked (local visual)");
        if (hook_arm64(g_il2cpp + OFF_PROCESS, (void*)hooked_Process, (void**)&orig_Process, tramp_proc))
            LOGI("Process hooked (local visual)");
    }
private:
    Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
};
REGISTER_ZYGISK_MODULE(CodmSkinModule)
