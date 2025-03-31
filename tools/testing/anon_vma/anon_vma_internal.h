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

extern struct vm_area_struct *mergeable_vma;

static int anon_vma_compatible(struct vm_area_struct *a, struct vm_area_struct *b)
{
	return a->vm_end == b->vm_start &&
		b->vm_pgoff == a->vm_pgoff + ((b->vm_start - a->vm_start) >> PAGE_SHIFT);
}

static struct anon_vma *reusable_anon_vma(struct vm_area_struct *old,
					  struct vm_area_struct *a,
					  struct vm_area_struct *b)
{
	if (anon_vma_compatible(a, b)) {
		struct anon_vma *anon_vma = READ_ONCE(old->anon_vma);

		if (anon_vma && list_is_singular(&old->anon_vma_chain))
			return anon_vma;
	}
	return NULL;
}

static inline struct anon_vma *find_mergeable_anon_vma(struct vm_area_struct *vma)
{
	struct anon_vma *anon_vma = NULL;

	if (!mergeable_vma)
		return NULL;

	/* Try next first. */
	if (mergeable_vma->vm_start >= vma->vm_end) {
		anon_vma = reusable_anon_vma(mergeable_vma, vma, mergeable_vma);
		if (anon_vma)
			return anon_vma;
	}

	return reusable_anon_vma(mergeable_vma, mergeable_vma, vma);
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
