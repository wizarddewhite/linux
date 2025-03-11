/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __TOOLS_LINUX_ATOMIC_H
#define __TOOLS_LINUX_ATOMIC_H

#include <urcu/uatomic.h>

#define atomic_t int32_t
#define atomic_inc(x) uatomic_inc(x)
#define atomic_read(x) uatomic_read(x)
#define atomic_set(x, y) uatomic_set(x, y)

static inline bool atomic_dec_and_test(atomic_t *v)
{
	return uatomic_sub_return(v, 1) == 0;
}

#endif	/* __TOOLS_LINUX_ATOMIC_H */
