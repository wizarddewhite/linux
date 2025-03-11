// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/bitmap.h>

#include "anon_vma_internal.h"
#include "../../../mm/anon_vma.c"

#define ASSERT_TRUE(_expr)						\
	do {								\
		if (!(_expr)) {						\
			fprintf(stderr,					\
				"Assert FAILED at %s:%d:%s(): %s is FALSE.\n", \
				__FILE__, __LINE__, __func__, #_expr); \
			return false;					\
		}							\
	} while (0)
#define ASSERT_FALSE(_expr) ASSERT_TRUE(!(_expr))
#define ASSERT_EQ(_val1, _val2) ASSERT_TRUE((_val1) == (_val2))
#define ASSERT_NE(_val1, _val2) ASSERT_TRUE((_val1) != (_val2))

static struct mm_struct dummy_mm = {
	.page_table_lock = __SPIN_LOCK_UNLOCKED(dummy_mm.page_table_lock),
};

#define NUM_VMAS 50
static int vmas_idx;
static struct vm_area_struct *vmas[NUM_VMAS];
static struct kmem_cache *vm_area_cachep;

static void vma_ctor(void *data)
{
	struct vm_area_struct *vma;

	vma = data;
	INIT_LIST_HEAD(&vma->anon_vma_chain);
	vma->vm_mm = &dummy_mm;
	vma->anon_vma = NULL;
}

void vma_cache_init(void)
{
	/* vma's kmem_cache for test */
	vm_area_cachep = kmem_cache_create("vm_area_struct",
			sizeof(struct vm_area_struct), 0,
			SLAB_PANIC|SLAB_ACCOUNT, vma_ctor);
}

struct vm_area_struct *alloc_vma(unsigned long start, unsigned long end, pgoff_t pgoff)
{
	if (vmas_idx >= NUM_VMAS)
		return NULL;

	vmas[vmas_idx] = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL);
	vmas[vmas_idx]->index = vmas_idx;
	vma_set_range(vmas[vmas_idx], start, end, pgoff);

	return vmas[vmas_idx++];
}

void vm_area_free(struct vm_area_struct *vma)
{
	kmem_cache_free(vm_area_cachep, vma);
}

void cleanup(void)
{
	int i;

	for (i = 0; i < NUM_VMAS; i++) {
		if (!vmas[i])
			continue;

		unlink_anon_vmas(vmas[i]);
		vm_area_free(vmas[i]);
		vmas[i] = NULL;
	}

	vmas_idx = 0;
}

static bool test_simple_fault(void)
{
	struct vm_area_struct *root_vma;
	struct anon_vma_chain *avc;

	root_vma = alloc_vma(0x3000, 0x5000, 3);
	/* First fault on anonymous vma would create its anon_vma. */
	__anon_vma_prepare(root_vma);
	ASSERT_NE(NULL, root_vma->anon_vma);
	dump_anon_vma_interval_tree(root_vma->anon_vma);

	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */

	anon_vma_interval_tree_foreach(avc, &root_vma->anon_vma->rb_root, 3, 4) {
		/* Expect to find itself from anon_vma interval tree */
		ASSERT_EQ(avc->vma, root_vma);
	}

	cleanup();

	ASSERT_EQ(0, nr_allocated);
	return true;
}

static bool test_simple_fork(void)
{
	struct vm_area_struct *root_vma, *child_vma;
	struct anon_vma_chain *avc;
	DECLARE_BITMAP(expected, 10);
	DECLARE_BITMAP(found, 10);

	bitmap_zero(expected, 10);
	bitmap_zero(found, 10);

	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */

	root_vma = alloc_vma(0x3000, 0x5000, 3);
	/* First fault on parent anonymous vma. */
	__anon_vma_prepare(root_vma);
	ASSERT_NE(NULL, root_vma->anon_vma);
	bitmap_set(expected, root_vma->index, 1);

	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *                \
	 *                 \   child_vma
	 *                  \  +-----------+
	 *                   > |           |
	 *                     +-----------+
	 */

	child_vma = alloc_vma(0x3000, 0x5000, 3);
	/* Fork child will link it to parent and may create its own anon_vma. */
	anon_vma_fork(child_vma, root_vma);
	ASSERT_NE(NULL, child_vma->anon_vma);
	bitmap_set(expected, child_vma->index, 1);
	/* Parent/Root is root_vma->anon_vma */
	ASSERT_EQ(child_vma->anon_vma->parent, root_vma->anon_vma);
	ASSERT_EQ(child_vma->anon_vma->root, root_vma->anon_vma);
	dump_anon_vma_interval_tree(root_vma->anon_vma);

	anon_vma_interval_tree_foreach(avc, &root_vma->anon_vma->rb_root, 3, 4) {
		bitmap_set(found, avc->vma->index, 1);
	}

	/* Expect to find all vma including the forked one. */
	ASSERT_TRUE(bitmap_equal(expected, found, 10));

	cleanup();

	ASSERT_EQ(0, nr_allocated);
	return true;
}

static bool test_fork_two(void)
{
	struct vm_area_struct *root_vma, *vma1, *vma2;
	struct anon_vma_chain *avc;
	DECLARE_BITMAP(expected, 10);
	DECLARE_BITMAP(found, 10);

	bitmap_zero(expected, 10);
	bitmap_zero(found, 10);

	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */

	root_vma = alloc_vma(0x3000, 0x5000, 3);
	/* First fault on parent anonymous vma. */
	__anon_vma_prepare(root_vma);
	ASSERT_NE(NULL, root_vma->anon_vma);
	bitmap_set(expected, root_vma->index, 1);

	/* First fork */
	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *                \
	 *                 \   vma1
	 *                  \  +-----------+
	 *                   > |           |
	 *                     +-----------+
	 */
	vma1 = alloc_vma(0x3000, 0x5000, 3);
	anon_vma_fork(vma1, root_vma);
	ASSERT_NE(NULL, vma1->anon_vma);
	bitmap_set(expected, vma1->index, 1);
	/* Parent/Root is root_vma->anon_vma */
	ASSERT_EQ(vma1->anon_vma->parent, root_vma->anon_vma);
	ASSERT_EQ(vma1->anon_vma->root, root_vma->anon_vma);

	/* Second fork */
	/*
	 *  anon_vma           root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *               \
	 *                \------------------+
	 *                 \   vma1           \   vma2
	 *                  \  +-----------+   \  +-----------+
	 *                   > |           |    > |           |
	 *                     +-----------+      +-----------+
	 */
	vma2 = alloc_vma(0x3000, 0x5000, 3);
	anon_vma_fork(vma2, root_vma);
	ASSERT_NE(NULL, vma2->anon_vma);
	bitmap_set(expected, vma2->index, 1);
	/* Parent/Root is root_vma->anon_vma */
	ASSERT_EQ(vma2->anon_vma->parent, root_vma->anon_vma);
	ASSERT_EQ(vma2->anon_vma->root, root_vma->anon_vma);
	dump_anon_vma_interval_tree(root_vma->anon_vma);

	anon_vma_interval_tree_foreach(avc, &root_vma->anon_vma->rb_root, 3, 4) {
		bitmap_set(found, avc->vma->index, 1);
	}

	/* Expect to find all vma including the forked one. */
	ASSERT_TRUE(bitmap_equal(expected, found, 10));

	/*
	 *  vma1->anon_vma     vma1
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */
	anon_vma_interval_tree_foreach(avc, &vma1->anon_vma->rb_root, 3, 4) {
		/* Expect to find only itself from its anon_vma interval tree */
		ASSERT_EQ(avc->vma, vma1);
	}

	/*
	 *  vma2->anon_vma     vma2
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */
	anon_vma_interval_tree_foreach(avc, &vma2->anon_vma->rb_root, 3, 4) {
		/* Expect to find only itself from its anon_vma interval tree */
		ASSERT_EQ(avc->vma, vma2);
	}

	cleanup();

	ASSERT_EQ(0, nr_allocated);
	return true;
}

static bool test_fork_grand_child(void)
{
	struct vm_area_struct *root_vma, *grand_vma, *vma1, *vma2;
	struct anon_vma_chain *avc;
	struct anon_vma *root_anon_vma;
	DECLARE_BITMAP(expected, 10);
	DECLARE_BITMAP(found, 10);

	bitmap_zero(expected, 10);
	bitmap_zero(found, 10);

	/*
	 *  root_anon_vma      root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 */

	root_vma = alloc_vma(0x3000, 0x5000, 3);
	/* First fault on parent anonymous vma. */
	__anon_vma_prepare(root_vma);
	root_anon_vma = root_vma->anon_vma;
	ASSERT_NE(NULL, root_anon_vma);
	bitmap_set(expected, root_vma->index, 1);

	/* First fork */
	/*
	 *  root_anon_vma      root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *                \
	 *                 \   vma1
	 *                  \  +-----------+
	 *                   > |           |
	 *                     +-----------+
	 */
	vma1 = alloc_vma(0x3000, 0x5000, 3);
	anon_vma_fork(vma1, root_vma);
	ASSERT_NE(NULL, vma1->anon_vma);
	bitmap_set(expected, vma1->index, 1);
	/* Parent/Root is root_vma->anon_vma */
	ASSERT_EQ(vma1->anon_vma->parent, root_vma->anon_vma);
	ASSERT_EQ(vma1->anon_vma->root, root_vma->anon_vma);

	/* Second fork */
	/*
	 *  root_anon_vma      root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *               \
	 *                \------------------+
	 *                 \   vma1           \   vma2
	 *                  \  +-----------+   \  +-----------+
	 *                   > |           |    > |           |
	 *                     +-----------+      +-----------+
	 */
	vma2 = alloc_vma(0x3000, 0x5000, 3);
	anon_vma_fork(vma2, root_vma);
	ASSERT_NE(NULL, vma2->anon_vma);
	bitmap_set(expected, vma2->index, 1);
	/* Parent/Root is root_vma->anon_vma */
	ASSERT_EQ(vma2->anon_vma->parent, root_vma->anon_vma);
	ASSERT_EQ(vma2->anon_vma->root, root_vma->anon_vma);
	dump_anon_vma_interval_tree(root_vma->anon_vma);

	/* Fork grand child from second child */
	/*
	 *  root_anon_vma      root_vma
	 *  +-----------+      +-----------+
	 *  |           | ---> |           |
	 *  +-----------+      +-----------+
	 *               \
	 *                \------------------+
	 *                |\   vma1           \   vma2
	 *                | \  +-----------+   \  +-----------+
	 *                |  > |           |    > |           |
	 *                |    +-----------+      +-----------+
	 *                \
	 *                 \   grand_vma
	 *                  \  +-----------+
	 *                   > |           |
	 *                     +-----------+
	 */
	grand_vma = alloc_vma(0x3000, 0x5000, 3);
	anon_vma_fork(grand_vma, vma2);
	ASSERT_NE(NULL, grand_vma->anon_vma);
	bitmap_set(expected, grand_vma->index, 1);
	/* Root is root_vma->anon_vma */
	ASSERT_EQ(grand_vma->anon_vma->root, root_vma->anon_vma);
	/* Parent is vma2->anon_vma */
	ASSERT_EQ(grand_vma->anon_vma->parent, vma2->anon_vma);

	/* Expect to find only vmas from second fork */
	anon_vma_interval_tree_foreach(avc, &vma2->anon_vma->rb_root, 3, 4) {
		ASSERT_TRUE(avc->vma == vma2 || avc->vma == grand_vma);
	}

	anon_vma_interval_tree_foreach(avc, &root_vma->anon_vma->rb_root, 3, 4) {
		bitmap_set(found, avc->vma->index, 1);
	}
	/* Expect to find all vma including child and grand child. */
	ASSERT_TRUE(bitmap_equal(expected, found, 10));

	/* Root process exit or unmap root_vma. */
	/*
	 *  root_anon_vma
	 *  +-----------+
	 *  |           |
	 *  +-----------+
	 *               \
	 *                \------------------+
	 *                |\   vma1           \   vma2
	 *                | \  +-----------+   \  +-----------+
	 *                |  > |           |    > |           |
	 *                |    +-----------+      +-----------+
	 *                \
	 *                 \   grand_vma
	 *                  \  +-----------+
	 *                   > |           |
	 *                     +-----------+
	 */
	bitmap_clear(expected, root_vma->index, 1);
	unlink_anon_vmas(root_vma);
	ASSERT_EQ(0, root_anon_vma->num_active_vmas);

	bitmap_zero(found, 10);
	anon_vma_interval_tree_foreach(avc, &root_anon_vma->rb_root, 3, 4) {
		bitmap_set(found, avc->vma->index, 1);
	}
	/* Expect to find all vmas even root_vma released. */
	ASSERT_TRUE(bitmap_equal(expected, found, 10));

	cleanup();

	ASSERT_EQ(0, nr_allocated);
	return true;
}

int main(void)
{
	int num_tests = 0, num_fail = 0;

	vma_cache_init();
	anon_vma_init();

#define TEST(name)							\
	do {								\
		num_tests++;						\
		if (!test_##name()) {					\
			num_fail++;					\
			fprintf(stderr, "Test " #name " FAILED\n");	\
		}							\
	} while (0)

	TEST(simple_fault);
	TEST(simple_fork);
	TEST(fork_two);
	TEST(fork_grand_child);

#undef TEST

	printf("%d tests run, %d passed, %d failed.\n",
	       num_tests, num_tests - num_fail, num_fail);

	return num_fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
