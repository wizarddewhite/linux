/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __MM_ANON_VMA_INTERNAL_H
#define __MM_ANON_VMA_INTERNAL_H

#define CONFIG_MMU

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/bug.h>

#define SLAB_TYPESAFE_BY_RCU	0
#define SLAB_ACCOUNT		0

static inline void might_sleep(void)
{
}

static inline void mmap_assert_locked(struct mm_struct *)
{
}

/*
 * rwsem_is_locked() is only used in anon_vma_free() to synchronize against
 * folio_lock_anon_vma_read(), which is not used in current tests.
 *
 * So just return 0 for simplification.
 */
static inline int rwsem_is_locked(struct rw_semaphore *sem)
{
	return 0;
}

static inline struct anon_vma *find_mergeable_anon_vma(struct vm_area_struct *vma)
{
	return NULL;
}

#ifndef pgoff_t
#define pgoff_t unsigned long
#endif
static inline void vma_set_range(struct vm_area_struct *vma,
					  unsigned long start, unsigned long end,
					  pgoff_t pgoff)
{
	vma->vm_start = start;
	vma->vm_end = end;
	vma->vm_pgoff = pgoff;
}

void dump_anon_vma_interval_tree(struct anon_vma *anon_vma);
extern int nr_allocated;
#endif // __MM_ANON_VMA_INTERNAL_H
