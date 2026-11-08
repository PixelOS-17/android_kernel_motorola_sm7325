/* SPDX-License-Identifier: GPL-2.0 OR Apache-2.0 */
/*
 * include/linux/ashmem.h
 *
 * Copyright 2008 Google Inc.
 * Author: Robert Love
 */

#ifndef _LINUX_ASHMEM_H
#define _LINUX_ASHMEM_H

#include <linux/limits.h>
#include <linux/ioctl.h>
#include <linux/compat.h>

#include "uapi/ashmem.h"

#define ASHMEM_NAME_PREFIX "dev/ashmem/"
#define ASHMEM_NAME_PREFIX_LEN (sizeof(ASHMEM_NAME_PREFIX) - 1)
#define ASHMEM_FULL_NAME_LEN (ASHMEM_NAME_LEN + ASHMEM_NAME_PREFIX_LEN)

/* support of 32bit userspace on 64bit platforms */
#ifdef CONFIG_COMPAT
enum {
	COMPAT_ASHMEM_SET_SIZE		=	_IOW(__ASHMEMIOC, 3, compat_size_t),
	COMPAT_ASHMEM_SET_PROT_MASK	=	_IOW(__ASHMEMIOC, 5, unsigned int),
	COMPAT_ASHMEM_GET_FILE_ID	=	_IOR(__ASHMEMIOC, 11, compat_uptr_t),
};
#endif

bool is_ashmem_file(struct file *file);

long ashmem_memfd_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif	/* _LINUX_ASHMEM_H */
