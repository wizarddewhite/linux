/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __TOOLS_LINUX_MM_TYPES_H
#define __TOOLS_LINUX_MM_TYPES_H

#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

struct mm_struct {
		spinlock_t page_table_lock;
};

struct vm_area_struct {
	/* For test purpose */
	int	index;
	/* VMA covers [vm_start; vm_end) addresses within mm */
	unsigned long vm_start;
	unsigned long vm_end;

	/* The address space we belong to. */
	struct mm_struct *vm_mm;

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_lock &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 *
	 */
	struct {
		struct rb_node rb;
		unsigned long rb_subtree_last;
	} shared;
#ifdef CONFIG_ANON_VMA_NAME
	/*
	 * For private and shared anonymous mappings, a pointer to a null
	 * terminated string containing the name given to the vma, or NULL if
	 * unnamed. Serialized by mmap_lock. Use anon_vma_name to access.
	 */
	struct anon_vma_name *anon_name;
#endif
};

#endif	/* __TOOLS_LINUX_MM_TYPES_H */
