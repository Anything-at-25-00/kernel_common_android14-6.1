/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM vmscan

#define TRACE_INCLUDE_PATH trace/hooks

#if !defined(_TRACE_HOOK_VMSCAN_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_VMSCAN_H

#include <trace/hooks/vendor_hooks.h>

DECLARE_RESTRICTED_HOOK(android_rvh_set_balance_anon_file_reclaim,
			TP_PROTO(bool *balance_anon_file_reclaim),
			TP_ARGS(balance_anon_file_reclaim), 1);
DECLARE_HOOK(android_vh_kswapd_per_node,
	TP_PROTO(int nid, bool *skip, bool run),
	TP_ARGS(nid, skip, run));
DECLARE_HOOK(android_vh_shrink_slab_bypass,
	TP_PROTO(gfp_t gfp_mask, int nid, struct mem_cgroup *memcg, int priority, bool *bypass),
	TP_ARGS(gfp_mask, nid, memcg, priority, bypass));
DECLARE_HOOK(android_vh_do_shrink_slab,
	TP_PROTO(struct shrinker *shrinker, long *freeable),
	TP_ARGS(shrinker, freeable));
DECLARE_HOOK(android_vh_do_shrink_slab_ex,
	TP_PROTO(struct shrink_control *shrinkctl, struct shrinker *shrinker,
	        long *freeable, int priority),
	TP_ARGS(shrinkctl, shrinker, freeable, priority));
DECLARE_HOOK(android_vh_shrink_node_memcgs,
	TP_PROTO(struct mem_cgroup *memcg, bool *skip),
	TP_ARGS(memcg, skip));
DECLARE_HOOK(android_vh_modify_scan_control,
	TP_PROTO(u64 *ext, unsigned long *nr_to_reclaim,
	struct mem_cgroup *target_mem_cgroup,
	bool *file_is_tiny, bool *may_writepage),
	TP_ARGS(ext, nr_to_reclaim, target_mem_cgroup, file_is_tiny, may_writepage));
DECLARE_HOOK(android_vh_should_continue_reclaim,
	TP_PROTO(u64 *ext, unsigned long *nr_to_reclaim,
	unsigned long *nr_reclaimed, bool *continue_reclaim),
	TP_ARGS(ext, nr_to_reclaim, nr_reclaimed, continue_reclaim));
DECLARE_HOOK(android_vh_file_is_tiny_bypass,
	TP_PROTO(bool file_is_tiny, bool *bypass),
	TP_ARGS(file_is_tiny, bypass));
DECLARE_HOOK(android_vh_check_folio_look_around_ref,
	TP_PROTO(struct folio *folio, int *skip),
	TP_ARGS(folio, skip));
enum scan_balance;
DECLARE_HOOK(android_vh_tune_scan_type,
	TP_PROTO(enum scan_balance *scan_type),
	TP_ARGS(scan_type));
DECLARE_HOOK(android_vh_tune_swappiness,
	TP_PROTO(int *swappiness),
	TP_ARGS(swappiness));
DECLARE_HOOK(android_vh_scan_abort_check_wmarks,
	TP_PROTO(bool *check_wmarks),
	TP_ARGS(check_wmarks));
DECLARE_HOOK(android_vh_vmscan_kswapd_done,
	TP_PROTO(int node_id, unsigned int highest_zoneidx, unsigned int alloc_order,
	        unsigned int reclaim_order),
	TP_ARGS(node_id, highest_zoneidx, alloc_order, reclaim_order));
DECLARE_RESTRICTED_HOOK(android_rvh_vmscan_kswapd_wake,
	TP_PROTO(int node_id, unsigned int highest_zoneidx, unsigned int alloc_order),
	TP_ARGS(node_id, highest_zoneidx, alloc_order), 1);
DECLARE_RESTRICTED_HOOK(android_rvh_vmscan_kswapd_done,
	TP_PROTO(int node_id, unsigned int highest_zoneidx, unsigned int alloc_order,
			unsigned int reclaim_order),
	TP_ARGS(node_id, highest_zoneidx, alloc_order, reclaim_order), 1);
DECLARE_HOOK(android_vh_handle_trylock_failed_folio,
	TP_PROTO(struct list_head *folio_list),
	TP_ARGS(folio_list));
DECLARE_HOOK(android_vh_folio_trylock_set,
	TP_PROTO(struct folio *folio),
	TP_ARGS(folio));
DECLARE_HOOK(android_vh_folio_trylock_clear,
	TP_PROTO(struct folio *folio),
	TP_ARGS(folio));
DECLARE_HOOK(android_vh_get_folio_trylock_result,
	TP_PROTO(struct folio *folio, bool *trylock_failed),
	TP_ARGS(folio, trylock_failed));
DECLARE_HOOK(android_vh_do_folio_trylock,
	TP_PROTO(struct folio *folio, struct rw_semaphore *sem,
		bool *got_lock, bool *skip),
	TP_ARGS(folio, sem, got_lock, skip));
DECLARE_HOOK(android_vh_page_referenced_check_bypass,
	TP_PROTO(struct folio *folio, unsigned long nr_to_scan, int lru, bool *bypass),
	TP_ARGS(folio, nr_to_scan, lru, bypass));
DECLARE_HOOK(android_vh_folio_referenced_check_bypass,
	TP_PROTO(struct folio *folio, s8 priority, unsigned long nr_to_scan, int lru, bool *bypass),
	TP_ARGS(folio, priority, nr_to_scan, lru, bypass));
DECLARE_HOOK(android_vh_shrink_node,
	TP_PROTO(pg_data_t *pgdat, struct mem_cgroup *memcg),
	TP_ARGS(pgdat, memcg));
DECLARE_HOOK(android_vh_should_memcg_bypass,
	TP_PROTO(struct mem_cgroup *memcg, int priority, bool *bypass),
	TP_ARGS(memcg, priority, bypass));
#endif /* _TRACE_HOOK_VMSCAN_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
