// Copyright (c) 2023-2026 Christiaan (chris@boreddev.nl)
// This software is released under the GNU General Public License v3.0. See LICENSE file for details.
// This header needs to maintain in any file it is present in, as per the GPL license terms.
#ifndef SYSCALL_USER_H
#define SYSCALL_USER_H

#include "syscall.h"
#include <stddef.h>

static inline void sys_serial_write(const char *str) {
    syscall2(8, 0, (uint64_t)str);
}

#endif
