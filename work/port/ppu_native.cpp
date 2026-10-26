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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <atomic>
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

__attribute__((constructor)) static void ppu_native_init(void)
{
    for (const ppu_native_desc& d : s_funcs) ppu_native_register_desc(&d);
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
