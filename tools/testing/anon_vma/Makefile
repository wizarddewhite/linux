# SPDX-License-Identifier: GPL-2.0-or-later

.PHONY: default clean

default: anon_vma

include ../shared/shared.mk

CFLAGS += -D CONFIG_PHYS_ADDR_T_64BIT
ifeq ($(DEBUG), 1)
	CFLAGS += -D ANON_VMA_INTERVAL_TREE_DEBUG -g
endif

OFILES = $(LIBS) linux.o anon_vma.o rbtree-shim.o \
	 interval_tree-shim.o interval_tree.o
TARGETS = anon_vma

interval_tree.o: ../../../mm/interval_tree.c
anon_vma.o: anon_vma.c anon_vma_internal.h ../../../mm/anon_vma.c \
	 ../../../include/linux/anon_vma.h

anon_vma: $(OFILES)

clean:
	$(RM) $(TARGETS) *.o generated/*
