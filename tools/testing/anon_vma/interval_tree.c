// SPDX-License-Identifier: GPL-2.0-or-later

#include "../../../mm/interval_tree.c"

#ifdef ANON_VMA_INTERVAL_TREE_DEBUG
enum child_dir {
	left_child,
	right_child,
	root_node
};

typedef void (*dump_node)(struct rb_node *node, int level);

void dump_avc(struct rb_node *node, int level)
{
	struct anon_vma_chain *avc;

	avc = rb_entry(node, struct anon_vma_chain, rb);

	printf(" -%02d [%lu, %lu] %lu\n", avc->vma->index,
		avc_start_pgoff(avc), avc_last_pgoff(avc),
		avc->rb_subtree_last);
}

void dump_rb_tree(struct rb_node *node, int level,
		  enum child_dir state, dump_node print)
{
	char prefix[40] = "                                        ";

	if (!node)
		return;

	dump_rb_tree(node->rb_right, level+1, right_child, print);

	if (state == left_child)
		printf("%.*s|\n", min_t(int, level * 2 + 2, ARRAY_SIZE(prefix)), prefix);

	printf("%02d%.*s", level, min_t(int, level * 2, ARRAY_SIZE(prefix)), prefix);
	(*print)(node, level);

	if (state == right_child)
		printf("%.*s|\n", min_t(int, level * 2 + 2, ARRAY_SIZE(prefix)), prefix);

	dump_rb_tree(node->rb_left, level+1, left_child, print);
}

void dump_anon_vma_interval_tree(struct anon_vma *anon_vma)
{
	dump_rb_tree(anon_vma->rb_root.rb_root.rb_node, 0, root_node, dump_avc);
}
#else
void dump_anon_vma_interval_tree(struct anon_vma *anon_vma) { return; }
#endif
