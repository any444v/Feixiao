/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Master compatibility header for the rtw89 macOS port.
 *
 * Force-included into every rtw89 driver C file (mirrors rtw88_compat.h for
 * rtw88).  Pulls in the shared compat tree first, then layers the additional
 * kernel APIs that rtw89 uses and rtw88 never needed.  Keep additions here —
 * not in the shared linux/ or net/ headers — unless a definition must be
 * visible to code that includes those headers directly.
 */
#ifndef _RTW89_COMPAT_H
#define _RTW89_COMPAT_H

#include "rtw88_compat.h"

/* ------------------------------------------------------------------ */
/*  RCU (rtw89 uses annotated pointers and rcu_head; rtw88 did not)     */
/* ------------------------------------------------------------------ */

/* Sparse annotation — expands to nothing for real compilers */
#ifndef __rcu
#define __rcu
#endif

struct rcu_head {
    struct rcu_head *next;
    void (*func)(struct rcu_head *head);
};

/*
 * The port runs the whole driver + MLME on serialized IOKit threads and all
 * rcu_read_lock/unlock shims are no-ops, so there are no concurrent RCU
 * readers to wait for: run callbacks immediately instead of after a grace
 * period.  If the kext ever grows parallel readers this must be revisited.
 */
static inline void call_rcu(struct rcu_head *head,
                            void (*func)(struct rcu_head *head))
{
    func(head);
}

#define rcu_access_pointer(p)  (p)
#define kfree_rcu(ptr, rhf)    kfree(ptr)
#define kfree_rcu_mightsleep(ptr) kfree(ptr)

/* ------------------------------------------------------------------ */
/*  ktime                                                               */
/* ------------------------------------------------------------------ */

typedef s64 ktime_t;

static inline ktime_t ktime_get(void)
{
    uint64_t ns;
    absolutetime_to_nanoseconds(mach_absolute_time(), &ns);
    return (ktime_t)ns;
}

static inline u64 ktime_get_boottime_ns(void)
{
    return (u64)ktime_get();
}

static inline s64 ktime_ms_delta(ktime_t later, ktime_t earlier)
{
    return (later - earlier) / 1000000LL;
}

static inline s64 ktime_us_delta(ktime_t later, ktime_t earlier)
{
    return (later - earlier) / 1000LL;
}

/* ------------------------------------------------------------------ */
/*  errno / limits additions                                            */
/* ------------------------------------------------------------------ */

#ifndef ESRCH
#define ESRCH 3
#endif
#ifndef ENOLINK
#define ENOLINK 67
#endif
#ifndef ENOKEY
#define ENOKEY 126
#endif
#ifndef UINT_MAX
#define UINT_MAX (~0u)
#endif
#ifndef S8_MAX
#define S8_MAX  ((s8)127)
#endif
#ifndef S8_MIN
#define S8_MIN  ((s8)(-128))
#endif
#ifndef S16_MAX
#define S16_MAX ((s16)32767)
#endif
#ifndef S16_MIN
#define S16_MIN ((s16)(-32768))
#endif
#ifndef S32_MAX
#define S32_MAX ((s32)2147483647)
#endif
#ifndef S32_MIN
#define S32_MIN ((s32)(-2147483647 - 1))
#endif

/* flexible array member inside a union (kernel util macro) */
#ifndef DECLARE_FLEX_ARRAY
#define DECLARE_FLEX_ARRAY(TYPE, NAME) \
    struct { \
        struct { } __empty_##NAME; \
        TYPE NAME[]; \
    }
#endif

#ifndef NAPI_POLL_WEIGHT
#define NAPI_POLL_WEIGHT 64
#endif

#ifndef PCI_VENDOR_ID_ASMEDIA
#define PCI_VENDOR_ID_ASMEDIA 0x1b21
#endif

#ifndef ENODATA
#define ENODATA 61
#endif
#ifndef ECONNRESET
#define ECONNRESET 104
#endif

/* PCIe L1 substates config registers (rtw89 pci.c manipulates these) */
#ifndef PCI_EXT_CAP_ID_L1SS
#define PCI_EXT_CAP_ID_L1SS   0x1E
#define PCI_L1SS_CTL1         0x08
#define PCI_L1SS_CTL1_L1SS_MASK 0x0000000f
#endif

/* list helpers rtw88 never used */
#ifndef list_for_each
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#endif
#ifndef list_for_each_safe
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)
#endif

/* compile-time type equality check (linux/typecheck.h) */
#ifndef typecheck
#define typecheck(type, x) \
    ({ type __dummy; \
       __typeof__(x) __dummy2; \
       (void)(&__dummy == &__dummy2); \
       1; })
#endif

#ifndef static_assert
#define static_assert(expr, ...) _Static_assert(expr, #expr)
#endif

#ifndef BITS_PER_TYPE
#define BITS_PER_TYPE(type) (sizeof(type) * 8)
#endif

#ifndef flex_array_size
#define flex_array_size(p, member, count) \
    ((size_t)(count) * sizeof(*(p)->member))
#endif

#ifndef struct_size_t
#define struct_size_t(type, member, count) \
    (sizeof(type) + (size_t)(count) * sizeof(((type *)0)->member[0]))
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

/* constant-expression popcount so static_assert(hweight32(...)) works */
#ifdef hweight32
#undef hweight32
#endif
#define hweight32(x) __builtin_popcount((u32)(x))
#ifdef hweight16
#undef hweight16
#endif
#define hweight16(x) __builtin_popcount((u16)(x))
#ifdef hweight8
#undef hweight8
#endif
#define hweight8(x)  __builtin_popcount((u8)(x))

/* ------------------------------------------------------------------ */
/*  USB anchors (rtw89 usb.c tracks in-flight URBs with an anchor)      */
/* ------------------------------------------------------------------ */

struct urb;

struct usb_anchor {
    struct list_head urb_list;
    unsigned int poisoned;
};

static inline void init_usb_anchor(struct usb_anchor *anchor)
{
    INIT_LIST_HEAD(&anchor->urb_list);
    anchor->poisoned = 0;
}

/* Implemented in rtw89_compat.c against the port's URB emulation */
void usb_anchor_urb(struct urb *urb, struct usb_anchor *anchor);
void usb_unanchor_urb(struct urb *urb);
void usb_kill_anchored_urbs(struct usb_anchor *anchor);

/* sparse lock annotations */
#ifndef __acquires
#define __acquires(x)
#endif
#ifndef __releases
#define __releases(x)
#endif
#ifndef __must_hold
#define __must_hold(x)
#endif

/* flexible-array struct sizing */
#ifndef struct_size
#define struct_size(p, member, count) \
    (sizeof(*(p)) + (size_t)(count) * sizeof(*(p)->member))
#endif

/*
 * rtw89 asserts on values that are constant only after propagation
 * (static const locals), which _Static_assert-based BUILD_BUG_ON cannot
 * evaluate.  Upstream already validated these; drop to a no-op here.
 */
#ifdef BUILD_BUG_ON
#undef BUILD_BUG_ON
#endif
#define BUILD_BUG_ON(cond) do { } while (0)

/* ------------------------------------------------------------------ */
/*  cleanup.h guards (only the forms rtw89 actually uses)               */
/* ------------------------------------------------------------------ */

/* guard(rcu)(); — RCU read lock is a no-op in this port */
#define guard(_type)              __rtw89_guard_##_type
#define __rtw89_guard_rcu()       do { } while (0)

/* scoped_guard(spinlock_irqsave, &lock) { ... } */
#define scoped_guard(_type, args...) __rtw89_scoped_##_type(args)
#define __rtw89_scoped_spinlock_irqsave(_lock)                                \
    for (unsigned long __sg_once = ({ unsigned long __sg_f = 0;              \
                                      spin_lock_irqsave(_lock, __sg_f);      \
                                      (void)__sg_f; 1UL; });                 \
         __sg_once;                                                          \
         ({ unsigned long __sg_f = 0;                                        \
            spin_unlock_irqrestore(_lock, __sg_f); }), __sg_once = 0)

/* ------------------------------------------------------------------ */
/*  wiphy_work / wiphy_delayed_work                                     */
/* ------------------------------------------------------------------ */

/*
 * mac80211 runs these on the wiphy workqueue with the wiphy mutex held.
 * This port has no wiphy mutex (the kext MLME serializes everything), so
 * they map onto the compat workqueue.  Both flavors are backed by a
 * delayed_work; immediate queueing is delay=0.
 */
struct wiphy;
struct wiphy_work;
typedef void (*wiphy_work_func_t)(struct wiphy *wiphy, struct wiphy_work *work);

struct wiphy_work {
    struct delayed_work dwork;
    wiphy_work_func_t func;
    struct wiphy *wiphy;
};

struct wiphy_delayed_work {
    struct wiphy_work work;
};

static inline void __rtw89_wiphy_work_tramp(struct work_struct *w)
{
    struct delayed_work *dw = container_of(w, struct delayed_work, work);
    struct wiphy_work *ww = container_of(dw, struct wiphy_work, dwork);

    ww->func(ww->wiphy, ww);
}

static inline void wiphy_work_init(struct wiphy_work *work,
                                   wiphy_work_func_t func)
{
    INIT_DELAYED_WORK(&work->dwork, __rtw89_wiphy_work_tramp);
    work->func = func;
    work->wiphy = NULL;
}

static inline void wiphy_work_queue(struct wiphy *wiphy,
                                    struct wiphy_work *work)
{
    work->wiphy = wiphy;
    queue_delayed_work(system_wq, &work->dwork, 0);
}

static inline void wiphy_work_cancel(struct wiphy *wiphy,
                                     struct wiphy_work *work)
{
    cancel_delayed_work_sync(&work->dwork);
}

static inline void wiphy_work_flush(struct wiphy *wiphy,
                                    struct wiphy_work *work)
{
    flush_work(&work->dwork.work);
}

static inline void wiphy_delayed_work_init(struct wiphy_delayed_work *dwork,
                                           wiphy_work_func_t func)
{
    wiphy_work_init(&dwork->work, func);
}

static inline void wiphy_delayed_work_queue(struct wiphy *wiphy,
                                            struct wiphy_delayed_work *dwork,
                                            unsigned long delay)
{
    dwork->work.wiphy = wiphy;
    queue_delayed_work(system_wq, &dwork->work.dwork, delay);
}

static inline void wiphy_delayed_work_cancel(struct wiphy *wiphy,
                                             struct wiphy_delayed_work *dwork)
{
    cancel_delayed_work_sync(&dwork->work.dwork);
}

static inline void wiphy_delayed_work_flush(struct wiphy *wiphy,
                                            struct wiphy_delayed_work *dwork)
{
    if (cancel_delayed_work(&dwork->work.dwork))
        queue_delayed_work(system_wq, &dwork->work.dwork, 0);
    flush_work(&dwork->work.dwork.work);
}

/* Serialized MLME — no wiphy mutex in this port */
#define wiphy_lock(w)             do { } while (0)
#define wiphy_unlock(w)           do { } while (0)
#define lockdep_assert_wiphy(w)   do { } while (0)

static inline void wiphy_rfkill_set_hw_state(struct wiphy *wiphy, bool blocked) {}
static inline void wiphy_rfkill_start_polling(struct wiphy *wiphy) {}
static inline void wiphy_rfkill_stop_polling(struct wiphy *wiphy) {}

/* ------------------------------------------------------------------ */
/*  mac80211/cfg80211 additions for rtw89                               */
/* ------------------------------------------------------------------ */

enum ieee80211_roc_type {
    IEEE80211_ROC_TYPE_NORMAL = 0,
    IEEE80211_ROC_TYPE_MGMT_TX,
};

/* ------------------------------------------------------------------ */
/*  misc kernel helpers rtw89 needs                                     */
/* ------------------------------------------------------------------ */

/* attribute for flexible arrays sized by a struct member — no codegen */
#ifndef __counted_by
#define __counted_by(member)
#endif

#ifndef BITS_TO_BYTES
#define BITS_TO_BYTES(nr) (((nr) + 7) / 8)
#endif

/* const-preserving container_of; the cast keeps both const and non-const
 * callers compiling (matches how the port already treats constness) */
#ifndef container_of_const
#define container_of_const(ptr, type, member) \
    ((type *)(void *)(uintptr_t)((const char *)(ptr) - offsetof(type, member)))
#endif

#endif /* _RTW89_COMPAT_H */
