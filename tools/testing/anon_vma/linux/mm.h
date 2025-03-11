/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __TOOLS_LINUX_MM_H
#define __TOOLS_LINUX_MM_H

#include <linux/kernel.h>
#include <linux/mm_types.h>
#include <linux/interval_tree_generic.h>
#include <linux/types.h>
#include "../../include/linux/mm.h"

#define VM_WARN_ON(_expr) (WARN_ON(_expr))
#define VM_BUG_ON(_expr) (BUG_ON(_expr))
#define VM_BUG_ON_VMA(_expr, _vma) (BUG_ON(_expr))

static inline unsigned long vma_pages(struct vm_area_struct *vma)
{
	return (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
}

#include <linux/anon_vma.h>

#define vma_interval_tree_foreach(vma, root, start, last)		\
	for (vma = vma_interval_tree_iter_first(root, start, last);	\
	     vma; vma = vma_interval_tree_iter_next(vma, start, last))

void anon_vma_interval_tree_insert(struct anon_vma_chain *node,
				   struct rb_root_cached *root);
void anon_vma_interval_tree_remove(struct anon_vma_chain *node,
				   struct rb_root_cached *root);
struct anon_vma_chain *
anon_vma_interval_tree_iter_first(struct rb_root_cached *root,
				  unsigned long start, unsigned long last);
struct anon_vma_chain *anon_vma_interval_tree_iter_next(
	struct anon_vma_chain *node, unsigned long start, unsigned long last);
#ifdef CONFIG_DEBUG_VM_RB
void anon_vma_interval_tree_verify(struct anon_vma_chain *node);
#endif

#define anon_vma_interval_tree_foreach(avc, root, start, last)		 \
	for (avc = anon_vma_interval_tree_iter_first(root, start, last); \
	     avc; avc = anon_vma_interval_tree_iter_next(avc, start, last))

#endif /* __TOOLS_LINUX_MM_H */
