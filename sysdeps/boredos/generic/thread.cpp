#include <sys/mman.h>
#include <errno.h>
#include <mlibc/all-sysdeps.hpp>
#include <bits/ensure.h>
#include <mlibc/tcb.hpp>
#include <syscall.h>
#include <string.h>

namespace mlibc {

#define DEFAULT_THREAD_STACK 0x400000

int sys_prepare_stack(void **stack, void *entry, void *arg, void *tcb, size_t *stack_size, size_t *guard_size, void **stack_base) {
    *guard_size = 0;
    *stack_size = *stack_size ? *stack_size : DEFAULT_THREAD_STACK;

    if (!*stack) {
        *stack_base = mmap(NULL, *stack_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (*stack_base == MAP_FAILED) {
            return errno;
        }
    } else {
        *stack_base = *stack;
    }

    *stack = (void *)((char *)*stack_base + *stack_size);

    void **stack_it = (void **)*stack;

    *--stack_it = arg;
    *--stack_it = tcb;
    *--stack_it = entry;

    *stack = (void *)stack_it;

    return 0;
}

extern "C" void __mlibc_thread_entry();

int sys_clone(void *tcb, pid_t *tid_out, void *stack) {
    (void)tcb;
    long ret = syscall2(SYS_CLONE, (uintptr_t)__mlibc_thread_entry, (uintptr_t)stack);
    if (ret < 0) {
        return -ret;
    }
    *tid_out = (pid_t)ret;
    return 0;
}

void sys_yield() {
    syscall0(SYS_SCHED_YIELD);
}

int sys_gettid(pid_t *tid) {
    long ret = syscall0(SYS_GETTID);
    if (ret < 0) {
        return -ret;
    }
    *tid = (pid_t)ret;
    return 0;
}

[[noreturn]] void sys_thread_exit() {
    syscall1(SYS_EXIT, 0);
    __builtin_unreachable();
}

int sys_sysconf(int num, long *ret) {
    if (num == 84 || num == 83) { // _SC_NPROCESSORS_ONLN, _SC_NPROCESSORS_CONF
        int fd = -1;
        if (sys_open("/proc/cpuinfo", 0, 0, &fd) == 0 && fd >= 0) {
            char buf[2048];
            ssize_t bytes = 0;
            long count = 0;
            if (sys_read(fd, buf, sizeof(buf) - 1, &bytes) == 0 && bytes > 0) {
                buf[bytes] = '\0';
                const char *p = buf;
                while ((p = strstr(p, "processor"))) {
                    count++;
                    p += 9;
                }
            }
            sys_close(fd);
            if (count > 0) {
                *ret = count;
                return 0;
            }
        }
        *ret = 1;
        return 0;
    }
    return EINVAL;
}

} // namespace mlibc

extern "C" void __mlibc_thread_trampoline(void *(*fn)(void *), Tcb *tcb, void *arg) {
	if (mlibc::sys_tcb_set(tcb)) {
		__ensure(!"failed to set tcb for new thread");
	}

	while (__atomic_load_n(&tcb->tid, __ATOMIC_RELAXED) == 0) {
		mlibc::sys_futex_wait(&tcb->tid, 0, nullptr);
	}

	tcb->invokeThreadFunc(reinterpret_cast<void *>(fn), arg);

	__atomic_store_n(&tcb->didExit, 1, __ATOMIC_RELEASE);
	mlibc::sys_futex_wake(&tcb->didExit);

	mlibc::sys_thread_exit();
}
