// MSVC stdatomic.h shim for rpmalloc.
// MSVC 19.x does not support C11 _Atomic in C mode; __STDC_NO_ATOMICS__ is
// permanently set and cannot be cleared even with /U.  This header provides
// the subset used by rpmalloc via Windows Interlocked intrinsics.
#pragma once

#ifndef __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

typedef enum memory_order
{
	memory_order_relaxed = 0,
	memory_order_consume = 1,
	memory_order_acquire = 2,
	memory_order_release = 3,
	memory_order_acq_rel = 4,
	memory_order_seq_cst = 5
} memory_order;

static __forceinline void _rp_fence(memory_order order)
{
	if (order == memory_order_seq_cst)
		MemoryBarrier();
	else
		_ReadWriteBarrier();
}
#define atomic_thread_fence(order) _rp_fence(order)

// Atomic types — plain volatile integers sized for Interlocked dispatch
typedef volatile LONG      atomic_int;
typedef volatile LONG      atomic_uint;
typedef volatile LONGLONG  atomic_llong;
typedef volatile ULONGLONG atomic_ullong;
typedef volatile SIZE_T    atomic_size_t;
typedef volatile ULONG_PTR atomic_uintptr_t;

// Internal helpers — always inline to avoid ODR issues across TUs
static __forceinline LONGLONG _rp_fetch_add64(volatile LONGLONG* p, LONGLONG v)
{
	return InterlockedExchangeAdd64(p, v);
}
static __forceinline LONG _rp_fetch_add32(volatile LONG* p, LONG v)
{
	return InterlockedExchangeAdd(p, v);
}
static __forceinline int _rp_cas64(volatile LONGLONG* p, LONGLONG* exp, LONGLONG des)
{
	LONGLONG old = InterlockedCompareExchange64(p, des, *exp);
	if (old == *exp) return 1;
	*exp = old;
	return 0;
}
static __forceinline int _rp_cas32(volatile LONG* p, LONG* exp, LONG des)
{
	LONG old = InterlockedCompareExchange(p, des, *exp);
	if (old == *exp) return 1;
	*exp = old;
	return 0;
}

// Load / Store — volatile read/write provides sequentially consistent visibility on x86/x64
#define atomic_load_explicit(obj, order) (*(obj))
#define atomic_load(obj)                 atomic_load_explicit(obj, memory_order_seq_cst)

#define atomic_store_explicit(obj, val, order) ((void)(*(obj) = (val)))
#define atomic_store(obj, val)                 atomic_store_explicit(obj, val, memory_order_seq_cst)

// Fetch-add: returns old value, cast back to caller's type via __typeof_unqual__
#define atomic_fetch_add_explicit(obj, val, order) \
	((__typeof_unqual__(*(obj)))(sizeof(*(obj)) == 8 \
		? _rp_fetch_add64((volatile LONGLONG*)(obj), (LONGLONG)(val)) \
		: (LONGLONG)_rp_fetch_add32((volatile LONG*)(obj), (LONG)(val))))
#define atomic_fetch_add(obj, val) atomic_fetch_add_explicit(obj, val, memory_order_seq_cst)

// Fetch-sub: negate and fetch-add
#define atomic_fetch_sub_explicit(obj, val, order) \
	atomic_fetch_add_explicit(obj, (0 - (val)), order)
#define atomic_fetch_sub(obj, val) atomic_fetch_sub_explicit(obj, val, memory_order_seq_cst)

// CAS (weak == strong on x86/x64 — no spurious failures)
#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail) \
	(sizeof(*(obj)) == 8 \
		? _rp_cas64((volatile LONGLONG*)(obj), (LONGLONG*)(expected), (LONGLONG)(desired)) \
		: _rp_cas32((volatile LONG*)(obj), (LONG*)(expected), (LONG)(desired)))
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail) \
	atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail)
#define atomic_compare_exchange_strong(obj, expected, desired) \
	atomic_compare_exchange_strong_explicit(obj, expected, desired, memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_weak(obj, expected, desired) \
	atomic_compare_exchange_strong(obj, expected, desired)

#define ATOMIC_VAR_INIT(val) (val)

#endif // __cplusplus
