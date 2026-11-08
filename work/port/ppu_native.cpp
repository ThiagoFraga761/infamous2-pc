/* inFAMOUS 2 (BCUS98125): native stand-ins for lifted PPU functions.
 *
 * The lift calls PPU_NATIVE_HOOK at the entry of each function listed in
 * tools/ppu_native_entries.py (a post-pass over the lifter's output). The
 * hook asks ppu_native_slot, once, for the function registered at its
 * address; a registered function that returns nonzero has done the guest
 * function's work and left the context as the guest's `blr` would, by the
 * PPC64 ELF ABI: r1, r2, r13-r31, f14-f31, v20-v31 and cr2-cr4 as on entry,
 * the result in r3 (f1, v2). Volatile registers are not reproduced: no caller
 * may read them after a call, and the lifted callers do not (their reloads
 * after each call are the registers the localizer found live there). One that
 * returns 0 declines -- a range the inline memory paths would not take, an
 * SPU reservation on a line it would write -- and the lifted body runs.
 *
 * Every function here is a leaf the lifted code runs as a scalar loop over
 * guest memory, with a range test per access. Guest memory is big-endian at
 * vm_base + EA. A native function only takes a call when the whole range it
 * touches is one the inline accessors in ppu_recomp.h would take: the same
 * [0x10000, end of mapping) window, not the gcm control word, and for writes
 * no line an SPU holds a reservation on. Writes are checked again after they
 * land and told through vm_store_coh_notify, as VM_ST_INL does.
 *
 * Three entries are wraps rather than stand-ins: CView::SetFov runs its lifted
 * body with other inputs, for the adjustable field of view, and the scripts'
 * CView::POS_SCREEN_FROM_WORLD / POS_WORLD_FROM_SCREEN run theirs with the
 * screen x moved into the ultrawide HUD's band (see below).
 *
 * The same file switches the lift's native blocks (hot loops inside functions
 * too big to replace, patched in by the same post-pass): g_ppu_native_blocks.
 *
 * PS3RECOMP_NATIVE_PPU: 1 (default) on; 0 off (the hooks find nothing, the
 * blocks fall through to the lifted loops); 2 runs the lifted function as well
 * on every call and compares the ABI-visible registers and the bytes the
 * function writes, logging any difference and keeping the lifted result, with
 * a stats line every 10 s (the blocks run as in 1). */
#define PPU_INLINE_STORES 1
#include "ppu_recomp.h"
#include "ppu_native.h"
#include "rsx_res_axis.h"   /* the toolkit's per-axis target scale, for the ultrawide markers */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

/* ---- guest memory ranges ------------------------------------------------ */

/* Would every access in [ea, ea+n) take the inline paths? */
static inline int range_ok(uint64_t ea, uint64_t n)
{
    if (__builtin_expect(g_vm_inl_lim == 0, 0)) return 0;
    if (ea - 0x10000u >= g_vm_inl_lim || ea + n > 0x10000ull + g_vm_inl_lim) return 0;
    const uint64_t ctl = ppu_hle_inject_base + 0x2008u;
    return ctl + 4 <= ea || ctl >= ea + n;
}
/* ... and is no line of a write to it reserved by an SPU? */
static inline int range_wr_ok(uint64_t ea, uint64_t n)
{
    if (!range_ok(ea, n)) return 0;
    if (!VM_COH_ARMED() || !n) return 1;
    for (uint64_t l = ea >> 7; l <= (ea + n - 1) >> 7; l++)
        if ((VM_COH_BYTE(l >> 3) >> (l & 7)) & 1u) return 0;
    return 1;
}
/* After the write: a reservation taken while it ran is told, line by line. */
static inline void range_wr_done(uint64_t ea, uint64_t n)
{
    if (!VM_COH_ARMED() || !n) return;
    for (uint64_t l = ea >> 7; l <= (ea + n - 1) >> 7; l++) {
        if (!((VM_COH_BYTE(l >> 3) >> (l & 7)) & 1u)) continue;
        const uint64_t a = l << 7 > ea ? l << 7 : ea, e = (l + 1) << 7 < ea + n ? (l + 1) << 7 : ea + n;
        vm_store_coh_notify((uint32_t)a, (uint32_t)(e - a));
    }
}

/* ---- the functions ------------------------------------------------------ */

/* func_005DEF90: memcpy(r3 dst, r4 src, r5 n), returning dst. It copies eight
 * bytes at a time where it can, backward when src's offset in its 4 KB page is
 * below dst's: for ranges that overlap, what lands depends on that choice, so
 * those calls stay with the lifted code. It zero-extends r3 and r4 and
 * compares r5 as 64 bits. */
static int nat_memcpy(ppu_context* c)
{
    const uint64_t d = (uint32_t)c->gpr[3], s = (uint32_t)c->gpr[4], n = c->gpr[5];
    if (n >> 31 || (d < s + n && s < d + n) || !range_ok(s, n) || !range_wr_ok(d, n)) return 0;
    memcpy(vm_base + d, vm_base + s, n);
    range_wr_done(d, n);
    c->gpr[3] = d;
    return 1;
}
static int span_memcpy(const ppu_context* c, ppu_native_span* sp)
{
    sp[0].ea = (uint32_t)c->gpr[3];
    sp[0].n = c->gpr[5];
    return 1;
}

extern void func_005DEF90(ppu_context*);   /* the lifted original, for the compare mode */

static const ppu_native_desc s_funcs[] = {
    { 0x005DEF90u, "memcpy", nat_memcpy, func_005DEF90, span_memcpy, PPU_NATIVE_RET_R3 },
};

/* ---- registry ----------------------------------------------------------- */

#define PPU_NATIVE_MAX 32
static ppu_native_desc s_tab[PPU_NATIVE_MAX];
static int s_n;
static int s_mode = -1;
/* The lift's native blocks (tools/ppu_native_entries.py NATIVE_BLOCKS) run
 * while this is set. */
extern "C" { int g_ppu_native_blocks = 1; }

extern "C" int ppu_native_register_desc(const ppu_native_desc* d)
{
    if (s_n >= PPU_NATIVE_MAX || !d->fn) return -1;
    s_tab[s_n++] = *d;
    return 0;
}
extern "C" int ppu_native_register(uint32_t addr, int (*fn)(ppu_context*))
{
    ppu_native_desc d = {};
    d.addr = addr; d.name = "?"; d.fn = fn;
    return ppu_native_register_desc(&d);
}
extern "C" const ppu_native_desc* ppu_native_table(int* n)
{
    *n = s_n;
    return s_tab;
}
/* ---- call-count probes -------------------------------------------------- */

/* Functions listed in tools/ppu_native_entries.py PROBES get a hook whose
 * native side runs the lifted body and times it: calls a second, time a
 * second inside and the longest call, every 10 s on stderr ([ppu-probe]).
 * For finding which game loops run how often and how long (rpi/decomp-scene). */
static const uint32_t s_probe_addr[] = {
    0x001768B8u,    /* CHeroGunSkill::vf8 */
    0x0016C38Cu, 0x0021D5B4u, 0x0021B994u, 0x0022C6D4u, 0x0024C7ECu,
    0x0030B810u,    /* CSo::vf33 */
    0x00313E84u,    /* CSo::vf25 */
    0x0046B568u,    /* CXo::get_pos */
    0x002E66A8u,    /* vt_008D63E0::vf3 */
    0x0046B728u,    /* CXo::get_VELOCITY */
    0x00243138u, 0x00237AB0u,
};
extern void func_001768B8(ppu_context*); extern void func_0016C38C(ppu_context*);
extern void func_0021D5B4(ppu_context*); extern void func_0021B994(ppu_context*);
extern void func_0022C6D4(ppu_context*); extern void func_0024C7EC(ppu_context*);
extern void func_0030B810(ppu_context*); extern void func_00313E84(ppu_context*);
extern void func_0046B568(ppu_context*); extern void func_002E66A8(ppu_context*);
extern void func_0046B728(ppu_context*); extern void func_00243138(ppu_context*);
extern void func_00237AB0(ppu_context*);
/* The lifted bodies, in s_probe_addr's order: a probe runs the body itself to
 * time it (its own hook declines while it does, so a recursive call is timed
 * as part of the outermost one and not counted). */
static void (*const s_probe_body[])(ppu_context*) = {
    func_001768B8, func_0016C38C, func_0021D5B4, func_0021B994, func_0022C6D4, func_0024C7EC,
    func_0030B810, func_00313E84, func_0046B568, func_002E66A8, func_0046B728, func_00243138,
    func_00237AB0,
};
#define N_PROBES (sizeof s_probe_addr / sizeof s_probe_addr[0])
static_assert(sizeof s_probe_body / sizeof s_probe_body[0] == N_PROBES, "s_probe_body out of step");
static std::atomic<uint64_t> s_probe_n[N_PROBES], s_probe_ns[N_PROBES], s_probe_max[N_PROBES];
static __thread uint8_t t_probe_in[N_PROBES];
static uint64_t probe_now_ns(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
/* CSo::vf33's results in 00314BC8's solve passes: 0, 1, 2 (solved), other */
static std::atomic<uint64_t> s_vf33_ret[4];
static std::atomic<uint64_t> s_vf33_same;   /* a retry on the object retried just before */
/* What the SPUs run while vf33 says "not yet": every 256th retry samples each
 * SPURS SPU (spurs_tl_sample): idle, or the resident job overlay. */
extern "C" void spurs_tl_sample(void (*)(const char*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
static std::atomic<uint64_t> s_wait_ovl[256], s_wait_idle, s_wait_n;
static void wait_emit(const char*, uint32_t, uint32_t ovl, uint32_t, uint32_t idle, uint32_t, uint32_t)
{
    if (idle & 1u) s_wait_idle.fetch_add(1, std::memory_order_relaxed);
    else s_wait_ovl[ovl & 0xFFu].fetch_add(1, std::memory_order_relaxed);
}
static void probe_tick(void)
{
    static std::atomic<uint64_t> next;
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    const uint64_t now = (uint64_t)ts.tv_sec;
    uint64_t t = next.load(std::memory_order_relaxed);
    if (!t) { next.compare_exchange_strong(t, now + 10); return; }
    if (now < t || !next.compare_exchange_strong(t, now + 10)) return;
    /* per function: calls/s, ms/s spent inside, the longest call in us */
    fprintf(stderr, "[ppu-probe] calls/s ms/s max-us:");
    for (size_t i = 0; i < N_PROBES; i++)
        fprintf(stderr, " %08X=%llu/%.1f/%llu", s_probe_addr[i],
                (unsigned long long)(s_probe_n[i].exchange(0, std::memory_order_relaxed) / 10),
                (double)s_probe_ns[i].exchange(0, std::memory_order_relaxed) / 1e7,
                (unsigned long long)(s_probe_max[i].exchange(0, std::memory_order_relaxed) / 1000));
    fprintf(stderr, "\n[ppu-probe] vf33 ret0=%llu/s ret1=%llu/s ret2=%llu/s other=%llu/s same-obj=%llu/s\n",
            (unsigned long long)(s_vf33_ret[0].exchange(0, std::memory_order_relaxed) / 10),
            (unsigned long long)(s_vf33_ret[1].exchange(0, std::memory_order_relaxed) / 10),
            (unsigned long long)(s_vf33_ret[2].exchange(0, std::memory_order_relaxed) / 10),
            (unsigned long long)(s_vf33_ret[3].exchange(0, std::memory_order_relaxed) / 10),
            (unsigned long long)(s_vf33_same.exchange(0, std::memory_order_relaxed) / 10));
    const uint64_t ns = s_wait_n.exchange(0, std::memory_order_relaxed);
    if (ns) {
        fprintf(stderr, "[ppu-probe] while vf33 waits (%llu samples x SPUs): idle %.2f", (unsigned long long)ns,
                (double)s_wait_idle.exchange(0, std::memory_order_relaxed) / (double)ns);
        for (int o = 0; o < 256; o++) {
            const uint64_t k = s_wait_ovl[o].exchange(0, std::memory_order_relaxed);
            if (k * 50 >= ns) fprintf(stderr, " ovl%d %.2f", o, (double)k / (double)ns);
        }
        fputc('\n', stderr);
    }
}
template <size_t I> static int probe_fn(ppu_context* c)
{
    if (t_probe_in[I]) return 0;   /* the inner run, or a recursive call */
    if (I == 0) probe_tick();
    const uint32_t obj = (uint32_t)c->gpr[3];
    t_probe_in[I] = 1;
    const uint64_t t0 = probe_now_ns();
    s_probe_body[I](c);
    const uint64_t dt = probe_now_ns() - t0;
    t_probe_in[I] = 0;
    s_probe_n[I].fetch_add(1, std::memory_order_relaxed);
    s_probe_ns[I].fetch_add(dt, std::memory_order_relaxed);
    uint64_t m = s_probe_max[I].load(std::memory_order_relaxed);
    while (dt > m && !s_probe_max[I].compare_exchange_weak(m, dt, std::memory_order_relaxed)) { }
    if (s_probe_addr[I] == 0x0030B810u) {
        const int32_t r = (int32_t)c->gpr[3];
        s_vf33_ret[r >= 0 && r <= 2 ? r : 3].fetch_add(1, std::memory_order_relaxed);
        if (r != 2) {
            static __thread uint32_t last_retry;
            if (obj == last_retry) s_vf33_same.fetch_add(1, std::memory_order_relaxed);
            last_retry = obj;
            static __thread uint32_t k;
            if ((++k & 255u) == 0) { s_wait_n.fetch_add(1, std::memory_order_relaxed); spurs_tl_sample(wait_emit); }
        }
    }
    return 1;
}
template <size_t... I> static void probe_register(std::index_sequence<I...>)
{
    (ppu_native_register(s_probe_addr[I], probe_fn<I>), ...);
}

/* ---- CView::SetFov (func_0043E370): the field of view ------------------- */

/* The camera is a CView (vtable 0x008F0FD0; the water's reflection view is a
 * CViewReflection, 0x008F1468, with the same SetFov and projection). SetFov
 * takes the vertical FOV in radians in f1. When it equals +0x29C it returns at
 * once; otherwise it stores it there, writes tan_y = tan(fov/2) to +0x2AC and
 * tan_x = tan_y * aspect(+0x2A4) to +0x2A8, and runs virtual slots #27 (the
 * projection at +0xF0, from +0x2A8/+0x2AC and the clip planes) and #28. Those
 * two tangents are what the projection, the frustum the culling jobs get and
 * the screen<->world helpers (aiming included) read, so changing them here
 * widens all of it together.
 *
 * The wrap runs the lifted body with f1 = 2*atan(k*tan(fov/2)) -- and, for
 * PS3RECOMP_ASPECT, with +0x2A4 forced for the call -- then puts the game's
 * own FOV back in +0x29C and its aspect in +0x2A4. The game reads +0x29C back:
 * CView::vf24 interpolates FOV_TO from it, 0x43E43C derives the LOD factor
 * from it, the scripts' FIELD_OF_VIEW returns it. A scaled value left there
 * would be scaled again on the next change and drift. Only the tangents and
 * what vf27/vf28 built from them differ from the game's; the one other field
 * vf27 writes, +0x298 (sqrt(1 + aspect^2) * tan_y * near + 1, the near
 * plane's corner), follows the tangents, as it would for a game-set FOV.
 *
 * The game calls SetFov every frame with an unchanged FOV and the body then
 * returns early, so a view keeps the tangents its last real change computed.
 * Each view remembers which settings those were (a generation, 0 for the
 * game's own), and a view whose generation is not the one wanted -- the
 * scale changed in the menu, or a filtered camera set it -- is recomputed once
 * on its next call, the early return defeated by putting a different value
 * in +0x29C just for the call.
 *
 * PS3RECOMP_FOV_SCALE=k (default 1, the lifted code untouched; 0.5..2): the
 *   tangent of each half-angle times k, so 1.15 shows 15% more each way. The
 *   in-game menu changes it live (ppu_native_set_fov_scale).
 * PS3RECOMP_ASPECT=a (3440/1440, 3440x1440 or 2.389; default off): the aspect
 *   tan_x is taken with, for views at the game's 16:9. With PS3RECOMP_RT_ASPECT
 *   at the same shape the targets are drawn that wide and the picture is Hor+
 *   (the Ultrawide option sets both to "auto", which main turns into the
 *   monitor's WxH before this is read); alone, the 1280x720 picture comes
 *   out squeezed.
 * PS3RECOMP_FOV_CUTSCENE=0, PS3RECOMP_FOV_AIM=0: calls from the scripted
 *   camera (CCmAnimated::vf44) or the aim camera (CCmSnipe::vf44) keep the
 *   game's FOV. Best effort: a FOV_TO blend (vf24) is not told apart.
 * PS3RECOMP_FOV_LOG=1: a [fov] line a second for each view SetFov sees, with
 *   the FOV the game asked for, the one drawn and tan_x/tan_y.
 * PS3RECOMP_NATIVE_PPU_OFF=0043E370 takes the wrap out altogether. */
extern void func_0043E370(ppu_context*);

static const uint32_t k_vt_view = 0x008F0FD0u, k_vt_reflection = 0x008F1468u;
static const uint32_t k_cm_animated = 0x0008ED14u, k_cm_animated_n = 0x530u;   /* CCmAnimated::vf44 */
static const uint32_t k_cm_snipe = 0x00093D7Cu, k_cm_snipe_n = 0x11A8u;        /* CCmSnipe::vf44 */

struct fov_view { uint32_t ea, gen, log_s; };
static std::mutex s_fov_mu;
/* -1 before the environment is read (the ini is exported after the static
 * constructors run), 0 nothing to do, 1 on. */
static std::atomic<int> s_fov_state{-1};
static float s_fov_k = 1.0f, s_fov_aspect = 0.0f;
static int s_fov_cutscene = 1, s_fov_aim = 1, s_fov_log = 0;
static uint32_t s_fov_gen = 1;
static fov_view s_fov_views[32];
static unsigned s_fov_next;
static __thread uint32_t t_fov_view;   /* the view whose lifted SetFov this thread is running */

static float fov_rdf(uint32_t ea)
{
    const uint32_t b = vm_read32(ea);
    float f;
    memcpy(&f, &b, 4);
    return f;
}
static void fov_wrf(uint32_t ea, float f)
{
    uint32_t b;
    memcpy(&b, &f, 4);
    vm_write32(ea, b);
}
static float fov_clamp_k(float k)
{
    if (!(k > 0.0f)) return 1.0f;
    return k < 0.5f ? 0.5f : k > 2.0f ? 2.0f : k;
}
/* "2.389", "3440/1440", "3440x1440" or "21:9"; 0 when it is none of those. */
static float fov_parse_aspect(const char* v)
{
    char* e;
    double a = strtod(v, &e);
    if (e != v && (*e == '/' || *e == 'x' || *e == 'X' || *e == ':')) {
        const double b = strtod(e + 1, nullptr);
        a = b > 0.0 ? a / b : 0.0;
    }
    return a > 0.5 && a < 8.0 ? (float)a : 0.0f;
}
static void fov_read_env_locked(void)
{
    const char* v;
    if ((v = getenv("PS3RECOMP_FOV_SCALE"))) s_fov_k = fov_clamp_k(strtof(v, nullptr));
    if ((v = getenv("PS3RECOMP_ASPECT"))) s_fov_aspect = fov_parse_aspect(v);
    s_fov_cutscene = !(v = getenv("PS3RECOMP_FOV_CUTSCENE")) || atoi(v) != 0;
    s_fov_aim = !(v = getenv("PS3RECOMP_FOV_AIM")) || atoi(v) != 0;
    s_fov_log = (v = getenv("PS3RECOMP_FOV_LOG")) && atoi(v) != 0;
}
static void fov_announce_locked(void)
{
    fprintf(stderr, "[fov] scale %.3f, aspect %s%.4f, cutscenes %s, aim %s\n", s_fov_k,
            s_fov_aspect > 0.0f ? "forced " : "the game's, ", s_fov_aspect > 0.0f ? s_fov_aspect : 16.0f / 9.0f,
            s_fov_cutscene ? "too" : "kept", s_fov_aim ? "too" : "kept");
}
static int fov_init(void)
{
    std::lock_guard<std::mutex> g(s_fov_mu);
    int st = s_fov_state.load(std::memory_order_relaxed);
    if (st >= 0) return st;
    fov_read_env_locked();
    st = s_fov_k != 1.0f || s_fov_aspect > 0.0f || s_fov_log;
    if (st) fov_announce_locked();
    s_fov_state.store(st, std::memory_order_release);
    return st;
}
static fov_view* fov_find_locked(uint32_t ea, int make)
{
    for (fov_view& v : s_fov_views)
        if (v.ea == ea) return &v;
    if (!make) return nullptr;
    /* A view evicted here reads as generation 0 later: at worst one extra
     * recompute on its next call. */
    fov_view* v = &s_fov_views[s_fov_next++ % (sizeof s_fov_views / sizeof s_fov_views[0])];
    *v = fov_view{ea, 0, 0};
    return v;
}
static void fov_log_locked(uint32_t v, uint32_t vt, uint32_t lr)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fov_view* fv = fov_find_locked(v, 1);
    if (fv->log_s == (uint32_t)ts.tv_sec) return;
    fv->log_s = (uint32_t)ts.tv_sec;
    const float game = fov_rdf(v + 0x29C), ty = fov_rdf(v + 0x2AC), tx = fov_rdf(v + 0x2A8), a = fov_rdf(v + 0x2A4);
    const double deg = 180.0 / 3.14159265358979323846;
    fprintf(stderr, "[fov] view %08X %s lr %08X game %.2f deg drawn %.2f x %.2f deg tan_y %.5f tan_x %.5f "
            "x/y %.4f aspect %.4f gen %u\n", v, vt == k_vt_view ? "CView" : "CViewReflection", lr, game * deg,
            2.0 * atan(ty) * deg, 2.0 * atan(tx) * deg, ty, tx, ty != 0.0f ? tx / ty : 0.0f, a, fv->gen);
}

static int nat_setfov(ppu_context* c)
{
    int st = s_fov_state.load(std::memory_order_acquire);
    if (__builtin_expect(st < 0, 0)) st = fov_init();
    const uint32_t v = (uint32_t)c->gpr[3];
    if (!st || v == t_fov_view) return 0;   /* off, or the lifted body we are running */
    const uint32_t vt = vm_read32(v);
    if (vt != k_vt_view && vt != k_vt_reflection) return 0;
    const uint32_t lr = (uint32_t)c->lr;
    const double f = c->fpr[1];
    const float stored = fov_rdf(v + 0x29C);

    std::unique_lock<std::mutex> g(s_fov_mu);
    const bool keep = (!s_fov_cutscene && lr - k_cm_animated < k_cm_animated_n) ||
                      (!s_fov_aim && lr - k_cm_snipe < k_cm_snipe_n);
    const float k = keep ? 1.0f : s_fov_k, aspect = keep ? 0.0f : s_fov_aspect;
    const uint32_t want = k != 1.0f || aspect > 0.0f ? s_fov_gen : 0;
    const fov_view* fv = fov_find_locked(v, 0);
    const uint32_t have = fv ? fv->gen : 0;
    /* What the lifted code would do on its own is right: the game's tangents
     * are wanted and the view has them, or the FOV is unchanged and the view
     * already has the wanted ones (the body's early return). */
    if ((want == 0 && have == 0) || ((double)stored == f && have == want)) {
        if (s_fov_log) fov_log_locked(v, vt, lr);
        return 0;
    }
    const int log = s_fov_log;
    g.unlock();

    double fe = f;
    if (k != 1.0f && f > 0.0 && f < 3.14159265358979323846) {
        fe = (double)(float)(2.0 * atan(k * tan(f * 0.5)));
        if (fe > 3.0) fe = 3.0;   /* ~172 deg: past that the projection degenerates */
    }
    const uint32_t a_bits = vm_read32(v + 0x2A4);
    float a_game;
    memcpy(&a_game, &a_bits, 4);
    const bool force_a = aspect > 0.0f && fabsf(a_game - 16.0f / 9.0f) < 1e-3f;
    if (force_a) fov_wrf(v + 0x2A4, aspect);
    /* An unchanged FOV with stale tangents: the body must not return early. */
    if ((double)stored == fe) fov_wrf(v + 0x29C, nextafterf(stored, INFINITY));
    c->fpr[1] = fe;
    const uint32_t outer = t_fov_view;
    t_fov_view = v;
    func_0043E370(c);
    t_fov_view = outer;
    fov_wrf(v + 0x29C, (float)f);
    if (force_a) vm_write32(v + 0x2A4, a_bits);

    g.lock();
    if (fov_view* w = fov_find_locked(v, want != 0)) w->gen = want;
    if (log) fov_log_locked(v, vt, lr);
    return 1;
}

/* The in-game menu: a new scale for every view, applied on each one's next
 * SetFov. Once called, the wrap stays on, so going back to 1 also puts the
 * game's tangents back. */
extern "C" void ppu_native_set_fov_scale(float k)
{
    std::lock_guard<std::mutex> g(s_fov_mu);
    if (s_fov_state.load(std::memory_order_relaxed) < 0) fov_read_env_locked();
    k = fov_clamp_k(k);
    if (k == s_fov_k && s_fov_state.load(std::memory_order_relaxed) == 1) return;
    s_fov_k = k;
    if (++s_fov_gen == 0) s_fov_gen = 1;
    fov_announce_locked();
    s_fov_state.store(1, std::memory_order_release);
}

/* ---- the scripts' screen<->world on the ultrawide HUD -------------------- */

/* With the Ultrawide option the picture is drawn wider than 16:9 (the targets
 * by PS3RECOMP_RT_ASPECT, the camera's tan_x by PS3RECOMP_ASPECT above) and
 * the toolkit squeezes the flat draws into the display buffer -- the HUD --
 * into the middle r of the width, r = (16/9) / aspect (PS3RECOMP_UW_HUD,
 * rsx_draw_engine.c). A marker the game places over a world position is
 * computed in the wide picture's NDC and then drawn squeezed, so it lands at
 * r times its object's x: nearer the centre than the object.
 *
 * The projection is func_004415E0(view r3, world point v2) -> v2 = {x, y,
 * depth, depth}: the point in the view's frame (rows at +0x70..+0xA0), then
 * x = -side / (depth * tan_x(+0x2A8)) and y = up / (depth * tan_y(+0x2AC)),
 * NDC in [-1, 1] across the picture. Within 1e-4 of the eye plane it gives
 * {-1e6, -1e6, 0, 0} instead. func_0043E840 is its inverse (same tangents).
 * Callers of the projection:
 *   - CView::POS_SCREEN_FROM_WORLD (0x4E2FC0) and POS_WORLD_FROM_SCREEN
 *     (0x4E2E84), the script methods (no C++ caller; the scripts reach them
 *     through the member table). r4 is the argument block: the point in at
 *     +0x10, the result out at +0, each one aligned quadword.
 *   - C++, all gameplay: the lightning skill's pick of the NPC nearest the
 *     screen centre (0xEFC44), CSteerableMissile (0x10DB50), the grapple
 *     (0x138B40), CPoleSkill (0x21EE5C), CScreenAimSkill::FIRE_AMMO
 *     (0x2D43E0, which also takes the inverse), and the buddies' attack
 *     tactics, Kuo/Nix picking on-screen targets (0x6AE84, 0x6B1D0, 0x6BE68,
 *     0x6E000, 0x70D94). None of them draws: they want the wide picture,
 *     where the player sees the target, and stay as they are.
 * The game's UI is script-built (CWidget, CTextBox... are script objects),
 * so the world markers are expected to come from the script method.
 *
 * In the HUD's band the marker belongs at x_hud = x / r, which is the
 * projection with tan_x * r -- the 16:9 tangent for the camera's tan_y. The
 * wrap runs the lifted method and divides the x it wrote by r (the eye-plane
 * case, depth 0, is left alone); the inverse gets its x times r for the call
 * and the argument back afterwards. Only for views of the camera's class
 * (CView, vtable 0x008F0FD0); a reflection or an orthographic view is left
 * alone. A world point outside the 16:9 band so gets |x| > 1, as it would on
 * a 16:9 screen: the scripts treat it as off screen (or pin it to the edge),
 * and the HUD's scissor would cut a marker drawn out there anyway. Scripts
 * that use the method for something else than drawing (is it on screen?)
 * see the 16:9 screen too, as on the PS3.
 *
 * r comes from the same configuration the targets do (rsx_res_axis.h: the
 * 1280x720 target's image size each way), so it is the r of the HUD squeeze.
 * Nothing changes when the picture is not wider than 16:9 or the HUD is not
 * squeezed (PS3RECOMP_UW_HUD=0).
 *
 * PS3RECOMP_UW_MARKERS=1 (default off): on. A value between 0 and 1 is taken
 *   as r itself (for a test against a squeeze the toolkit does not report).
 * PS3RECOMP_UW_MARKERS_LOG=1: a line every 2 s with the calls a second, the
 *   ones on views left alone, and the last x before and after; with the
 *   markers off it only counts, so a run can tell whether the scripts call
 *   the method at all.
 * PS3RECOMP_NATIVE_PPU_OFF=004E2E84 leaves the inverse alone (004E2FC0 the
 *   forward one). */
extern void func_004E2FC0(ppu_context*);
extern void func_004E2E84(ppu_context*);

static std::mutex s_uwm_mu;
static std::atomic<int> s_uwm_state{-1};   /* -1 unread, 0 nothing to do, 1 on (adjust or count) */
static int s_uwm_adjust, s_uwm_log;
static float s_uwm_r = 1.0f;
static std::atomic<uint64_t> s_uwm_fwd, s_uwm_inv, s_uwm_other;
static std::atomic<uint32_t> s_uwm_last_in, s_uwm_last_out;
static __thread int t_uwm_in;   /* inside a lifted body this wrap is running */

/* The HUD squeeze's r: 1 when the HUD is not squeezed. */
static float uwm_hud_ratio(void)
{
    const char* e = getenv("PS3RECOMP_UW_HUD");
    if (e && (e[0] == '0' || !strcmp(e, "stretch") || !strcmp(e, "off"))) return 1.0f;
    const rsx_res_axis* a = rsx_res_axis_get();
    if (!a->on) return 1.0f;
    const float fx = (float)rsx_res_axis_up(1280u, a->xn, a->xd) / 1280.0f;
    const float fy = (float)rsx_res_axis_up(720u, a->yn, a->yd) / 720.0f;
    return fx > fy * 1.0001f ? fy / fx : 1.0f;
}
static int uwm_init(void)
{
    std::lock_guard<std::mutex> g(s_uwm_mu);
    int st = s_uwm_state.load(std::memory_order_relaxed);
    if (st >= 0) return st;
    const char* e = getenv("PS3RECOMP_UW_MARKERS");
    const double want = e ? atof(e) : 0.0;
    s_uwm_log = (e = getenv("PS3RECOMP_UW_MARKERS_LOG")) && atoi(e) != 0;
    s_uwm_r = want > 0.0 && want < 1.0 ? (float)want : want != 0.0 ? uwm_hud_ratio() : 1.0f;
    s_uwm_adjust = s_uwm_r < 0.9999f;
    if (s_uwm_adjust)
        fprintf(stderr, "[uw-markers] on: the scripts' screen x goes into the HUD's middle %.1f%% (r %.4f)\n",
                s_uwm_r * 100.0f, s_uwm_r);
    else if (want != 0.0)
        fprintf(stderr, "[uw-markers] nothing to do: the HUD is not squeezed (16:9 picture or PS3RECOMP_UW_HUD=0)\n");
    if (s_uwm_log && !s_uwm_adjust) fprintf(stderr, "[uw-markers] counting only\n");
    st = s_uwm_adjust || s_uwm_log;
    s_uwm_state.store(st, std::memory_order_release);
    return st;
}
static void uwm_tick(void)
{
    static std::atomic<uint64_t> next;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const uint64_t now = (uint64_t)ts.tv_sec;
    uint64_t t = next.load(std::memory_order_relaxed);
    if (!t) { next.compare_exchange_strong(t, now + 2); return; }
    if (now < t || !next.compare_exchange_strong(t, now + 2)) return;
    const uint32_t bi = s_uwm_last_in.load(std::memory_order_relaxed), bo = s_uwm_last_out.load(std::memory_order_relaxed);
    float xi, xo;
    memcpy(&xi, &bi, 4);
    memcpy(&xo, &bo, 4);
    fprintf(stderr, "[uw-markers] screen-from-world %.1f/s, world-from-screen %.1f/s, other views %.1f/s; "
            "last x %.4f -> %.4f\n", (double)s_uwm_fwd.exchange(0, std::memory_order_relaxed) / 2.0,
            (double)s_uwm_inv.exchange(0, std::memory_order_relaxed) / 2.0,
            (double)s_uwm_other.exchange(0, std::memory_order_relaxed) / 2.0, xi, xo);
}
/* The views this applies to: the camera's class. */
static int uwm_enter(const ppu_context* c)
{
    int st = s_uwm_state.load(std::memory_order_acquire);
    if (__builtin_expect(st < 0, 0)) st = uwm_init();
    if (!st || t_uwm_in) return 0;
    if (s_uwm_log) uwm_tick();
    if (vm_read32((uint32_t)c->gpr[3]) != k_vt_view) {
        if (s_uwm_log) s_uwm_other.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }
    return 1;
}
static inline float uwm_f(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }
static inline uint32_t uwm_u(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }

/* func_004E2FC0, CView::POS_SCREEN_FROM_WORLD(view r3, args r4) */
static int nat_pos_screen(ppu_context* c)
{
    if (!uwm_enter(c)) return 0;
    const uint32_t out = (uint32_t)c->gpr[4] & ~0xFu;
    t_uwm_in = 1;
    func_004E2FC0(c);
    t_uwm_in = 0;
    if (s_uwm_log) s_uwm_fwd.fetch_add(1, std::memory_order_relaxed);
    if (!s_uwm_adjust) return 1;
    /* {-1e6, -1e6, 0, 0}: the point is on the eye plane; a sentinel, kept. */
    if (vm_read32(out + 8) == 0) return 1;
    const uint32_t xb = vm_read32(out);
    const uint32_t nb = uwm_u(uwm_f(xb) / s_uwm_r);
    vm_write32(out, nb);
    /* v2 too, as the method leaves it (lane 0, big-endian bytes) */
    uint8_t* v = (uint8_t*)&c->vr[2];
    v[0] = (uint8_t)(nb >> 24); v[1] = (uint8_t)(nb >> 16); v[2] = (uint8_t)(nb >> 8); v[3] = (uint8_t)nb;
    if (s_uwm_log) { s_uwm_last_in.store(xb, std::memory_order_relaxed); s_uwm_last_out.store(nb, std::memory_order_relaxed); }
    return 1;
}

/* func_004E2E84, CView::POS_WORLD_FROM_SCREEN(view r3, args r4) */
static int nat_pos_world(ppu_context* c)
{
    if (!uwm_enter(c)) return 0;
    const uint32_t in = ((uint32_t)c->gpr[4] + 0x10u) & ~0xFu;
    const uint32_t xb = s_uwm_adjust ? vm_read32(in) : 0;
    if (s_uwm_adjust) vm_write32(in, uwm_u(uwm_f(xb) * s_uwm_r));
    t_uwm_in = 1;
    func_004E2E84(c);
    t_uwm_in = 0;
    if (s_uwm_adjust) vm_write32(in, xb);
    if (s_uwm_log) s_uwm_inv.fetch_add(1, std::memory_order_relaxed);
    return 1;
}

__attribute__((constructor)) static void ppu_native_init(void)
{
    for (const ppu_native_desc& d : s_funcs) ppu_native_register_desc(&d);
    /* Not a stand-in but a wrap around the lifted body, so nothing to compare
     * (no `lifted`): the compare mode just runs it. */
    static const ppu_native_desc setfov = { 0x0043E370u, "CView::SetFov", nat_setfov, nullptr, nullptr, 0 };
    ppu_native_register_desc(&setfov);
    static const ppu_native_desc pos_screen = { 0x004E2FC0u, "CView::POS_SCREEN_FROM_WORLD", nat_pos_screen,
                                                nullptr, nullptr, 0 };
    static const ppu_native_desc pos_world = { 0x004E2E84u, "CView::POS_WORLD_FROM_SCREEN", nat_pos_world,
                                               nullptr, nullptr, 0 };
    ppu_native_register_desc(&pos_screen);
    ppu_native_register_desc(&pos_world);
    if (getenv("PS3RECOMP_PPU_PROBE")) probe_register(std::make_index_sequence<N_PROBES>{});
    const char* e = getenv("PS3RECOMP_NATIVE_PPU");
    g_ppu_native_blocks = !e || atoi(e) != 0;
}

/* ---- compare mode ------------------------------------------------------- */

static __thread int t_in_compare;
static std::atomic<uint64_t> s_cmp_calls[PPU_NATIVE_MAX], s_cmp_diff[PPU_NATIVE_MAX],
    s_cmp_declined[PPU_NATIVE_MAX], s_cmp_skipped[PPU_NATIVE_MAX];

static int abi_diff(const ppu_context* a, const ppu_context* b, uint32_t ret, char* why, size_t wn)
{
    static const int gprs[] = {1, 2, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    for (int r : gprs)
        if (a->gpr[r] != b->gpr[r]) { snprintf(why, wn, "r%d lifted %llx native %llx", r, (unsigned long long)a->gpr[r], (unsigned long long)b->gpr[r]); return 1; }
    if ((ret & PPU_NATIVE_RET_R3) && a->gpr[3] != b->gpr[3]) {
        snprintf(why, wn, "r3 lifted %llx native %llx", (unsigned long long)a->gpr[3], (unsigned long long)b->gpr[3]);
        return 1;
    }
    if ((a->cr ^ b->cr) & 0x00FFF000u) { snprintf(why, wn, "cr2-4 lifted %08x native %08x", a->cr, b->cr); return 1; }
    for (int r = 14; r < 32; r++)
        if (memcmp(&a->fpr[r], &b->fpr[r], 8)) { snprintf(why, wn, "f%d", r); return 1; }
    for (int r = 20; r < 32; r++)
        if (memcmp(&a->vr[r], &b->vr[r], 16)) { snprintf(why, wn, "v%d", r); return 1; }
    if ((ret & PPU_NATIVE_RET_F1) && memcmp(&a->fpr[1], &b->fpr[1], 8)) { snprintf(why, wn, "f1"); return 1; }
    if ((ret & PPU_NATIVE_RET_V2) && memcmp(&a->vr[2], &b->vr[2], 16)) { snprintf(why, wn, "v2"); return 1; }
    return 0;
}

static void cmp_stats(void)
{
    static std::atomic<time_t> next;
    const time_t now = time(nullptr);
    time_t t = next.load(std::memory_order_relaxed);
    if (now < t || !next.compare_exchange_strong(t, now + 10, std::memory_order_relaxed)) return;
    if (!t) return;
    fprintf(stderr, "[ppu-native] compare:");
    for (int i = 0; i < s_n; i++)
        fprintf(stderr, " %s %llu (differ %llu, declined %llu, too big %llu)", s_tab[i].name,
                (unsigned long long)s_cmp_calls[i].load(), (unsigned long long)s_cmp_diff[i].load(),
                (unsigned long long)s_cmp_declined[i].load(), (unsigned long long)s_cmp_skipped[i].load());
    fprintf(stderr, "\n");
}

/* Both, on the same entry state: the lifted function first (whose result is
 * kept), then, on the bytes as they were, the native one. */
static int compare_run(int i, ppu_context* ctx)
{
    if (t_in_compare) return 0;     /* the lifted body below, through its own hook */
    const ppu_native_desc& d = s_tab[i];
    if (!d.lifted) return d.fn(ctx);
    cmp_stats();
    ppu_native_span sp[PPU_NATIVE_SPANS];
    const int ns = d.writes ? d.writes(ctx, sp) : 0;
    uint64_t total = 0;
    for (int k = 0; k < ns; k++) {
        if (!range_ok(sp[k].ea, sp[k].n)) return 0;
        total += sp[k].n;
    }
    if (total > (1u << 22)) { s_cmp_skipped[i]++; return 0; }
    static __thread std::vector<uint8_t>* pre, *post;
    if (!pre) { pre = new std::vector<uint8_t>; post = new std::vector<uint8_t>; }
    pre->resize(total); post->resize(total);
    uint64_t o = 0;
    for (int k = 0; k < ns; o += sp[k].n, k++) memcpy(pre->data() + o, vm_base + sp[k].ea, sp[k].n);
    const ppu_context in = *ctx;
    t_in_compare = 1;
    d.lifted(ctx);
    t_in_compare = 0;
    o = 0;
    for (int k = 0; k < ns; o += sp[k].n, k++) {
        memcpy(post->data() + o, vm_base + sp[k].ea, sp[k].n);
        memcpy(vm_base + sp[k].ea, pre->data() + o, sp[k].n);
    }
    ppu_context nat = in;
    const int took = d.fn(&nat);
    char why[160] = "";
    int bad = 0;
    if (took) {
        bad = abi_diff(ctx, &nat, d.ret, why, sizeof why);
        o = 0;
        for (int k = 0; k < ns && !bad; o += sp[k].n, k++)
            if (memcmp(post->data() + o, vm_base + sp[k].ea, sp[k].n)) {
                uint64_t j = 0;
                while (post->data()[o + j] == vm_base[sp[k].ea + j]) j++;
                snprintf(why, sizeof why, "byte at %08llx lifted %02x native %02x", (unsigned long long)(sp[k].ea + j),
                         post->data()[o + j], vm_base[sp[k].ea + j]);
                bad = 1;
            }
    }
    o = 0;
    for (int k = 0; k < ns; o += sp[k].n, k++) memcpy(vm_base + sp[k].ea, post->data() + o, sp[k].n);
    s_cmp_calls[i]++;
    if (!took) s_cmp_declined[i]++;
    if (bad && s_cmp_diff[i]++ < 20)
        fprintf(stderr, "[ppu-native] %s (0x%08X) differs: %s; r3 %llx r4 %llx r5 %llx r6 %llx lr %llx\n", d.name, d.addr,
                why, (unsigned long long)in.gpr[3], (unsigned long long)in.gpr[4], (unsigned long long)in.gpr[5],
                (unsigned long long)in.gpr[6], (unsigned long long)in.lr);
    return 1;
}
template <int I> static int compare_thunk(ppu_context* ctx) { return compare_run(I, ctx); }
template <int... I> struct thunks { static constexpr int (*tab[])(ppu_context*) = {compare_thunk<I>...}; };
static constexpr auto& s_thunks = thunks<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                          21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31>::tab;
static_assert(sizeof s_thunks / sizeof s_thunks[0] == PPU_NATIVE_MAX, "one compare thunk per slot");

/* ---- the hook's lookup -------------------------------------------------- */

extern "C" int (*ppu_native_slot(uint32_t addr))(ppu_context*)
{
    static std::atomic<int> once;
    if (s_mode < 0) {
        const char* e = getenv("PS3RECOMP_NATIVE_PPU");
        const int m = e ? atoi(e) : 1;
        int z = 0;
        if (once.compare_exchange_strong(z, 1))
            fprintf(stderr, "[ppu-native] %d native PPU function(s): %s\n", s_n,
                    m == 2 ? "compared with the lifted ones" : m ? "on" : "off");
        s_mode = m;
    }
    /* PS3RECOMP_NATIVE_PPU_OFF=<hex>,...: those entries stay lifted (A/B of one) */
    if (const char* off = getenv("PS3RECOMP_NATIVE_PPU_OFF"))
        for (const char* p = off; *p; ) {
            char* end;
            const unsigned long a = strtoul(p, &end, 16);
            if (end == p) break;
            if (a == addr) return nullptr;
            p = *end == ',' ? end + 1 : end;
        }
    for (int i = 0; i < s_n; i++)
        if (s_tab[i].addr == addr) return s_mode == 0 ? nullptr : s_mode == 2 ? s_thunks[i] : s_tab[i].fn;
    return nullptr;
}
