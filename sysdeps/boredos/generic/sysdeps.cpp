#include <bits/ensure.h>
#include <abi-bits/errno.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/stat.h>
#include <abi-bits/termios.h>
#include <abi-bits/poll.h>
#include <abi-bits/signal.h>
#include <sys/mman.h>
#include <errno.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/tcb.hpp>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>
#include <stdarg.h>
#include <stdio.h>

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404

static inline uint64_t sys_call0(uint64_t num) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call1(uint64_t num, uint64_t a1) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call2(uint64_t num, uint64_t a1, uint64_t a2) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call3(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call4(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = a4;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call5(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = a4;
    register uint64_t r8 asm("r8") = a5;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return ret;
}
static inline uint64_t sys_call6(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = a4;
    register uint64_t r8 asm("r8") = a5;
    register uint64_t r9 asm("r9") = a6;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return ret;
}

#undef syscall0
#undef syscall1
#undef syscall2
#undef syscall3
#undef syscall4
#undef syscall5
#undef syscall6
#define syscall0 sys_call0
#define syscall1 sys_call1
#define syscall2 sys_call2
#define syscall3 sys_call3
#define syscall4 sys_call4
#define syscall5 sys_call5
#define syscall6 sys_call6

namespace mlibc {

// Panic implementation
void sys_libc_panic() {
    sys_libc_log("!!! mlibc panic !!!\n");
    sys_exit(-1);
    __builtin_trap();
}

// Log implementation
void sys_libc_log(const char *msg) {
    syscall3(SYS_WRITE, 2, (uint64_t)msg, strlen(msg));
}

#define TIOCGWINSZ 0x5413

// Check if fd is a TTY
int sys_isatty(int fd) {
    struct winsize ws;
    int result = 0;
    if (sys_ioctl(fd, TIOCGWINSZ, &ws, &result) == 0) {
        return 0; // 0 means success (it is a TTY) in mlibc sysdeps
    }
    if (fd == 0 || fd == 1 || fd == 2) {
        return 0;
    }
    return ENOTTY;
}

#ifndef MLIBC_BUILDING_RTLD
int sys_ptsname(int fd, char *buffer, size_t length) {
    int index = 0;
    int result = 0;
    if (int e = sys_ioctl(fd, 0x80045430 /* TIOCGPTN */, &index, &result); e) {
        return e;
    }
    snprintf(buffer, length, "/dev/pts/%d", index);
    return 0;
}

int sys_unlockpt(int fd) {
    int unlock = 0;
    int result = 0;
    return sys_ioctl(fd, 0x40045431 /* TIOCSPTLCK */, &unlock, &result);
}
#endif

// Standard file descriptor write
int sys_write(int fd, void const *buf, size_t size, ssize_t *ret) {
    long rc = syscall3(SYS_WRITE, fd, (uint64_t)buf, size);
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *ret = rc;
    return 0;
}

// Standard file descriptor read
int sys_read(int fd, void *buf, size_t size, ssize_t *ret) {
    long rc = syscall3(SYS_READ, fd, (uint64_t)buf, size);
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *ret = rc;
    return 0;
}

// Helper to convert flags to mode string for BoredOS sys_open
static const char *mode_from_flags(int flags) {
    int accmode = flags & O_ACCMODE;
    if (accmode == O_RDONLY) {
        return "rb";
    }
    if (accmode == O_RDWR) {
        if (flags & O_TRUNC) return "w+";
        if (flags & O_APPEND) return "a+";
        return "r+";
    }
    if (flags & O_APPEND) {
        return "ab";
    }
    if (flags & O_TRUNC) {
        return "wb";
    }
    return "wb";
}

// Open file
int sys_open(const char *path, int flags, mode_t mode, int *fd) {
    (void)mode;
    long rc = syscall2(SYS_OPEN, (uint64_t)path, (uint64_t)mode_from_flags(flags));
    if (rc < 0) {
        return -rc;
    }
    *fd = rc;
    return 0;
}

// Close file
int sys_close(int fd) {
    long rc = syscall1(SYS_CLOSE, fd);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Seek file
int sys_seek(int fd, off_t offset, int whence, off_t *ret) {
    long rc = syscall3(SYS_LSEEK, fd, offset, whence);
    if (rc < 0) {
        if (rc == -29 || rc == -22 || rc == -9 || rc == -1) return ESPIPE;
        return -rc;
    }
    *ret = rc;
    return 0;
}

// Exit process
void sys_exit(int status) {
    syscall1(SYS_EXIT, status);
    while (true) {}
}

// Sleep
int sys_sleep(time_t *secs, long *nanos) {
    long ms = (*secs * 1000) + (*nanos / 1000000);
    long rc = syscall1(SYS_NANOSLEEP, ms);
    if (rc < 0) {
        return -rc;
    }
    *secs = 0;
    *nanos = 0;
    return 0;
}

// Anonymous memory allocation using sys_mmap
int sys_anon_allocate(size_t size, void **pointer) {
    long rc = syscall6(SYS_MMAP, 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (rc == (long)MAP_FAILED || rc < 0) {
        return ENOMEM;
    }
    *pointer = (void *)rc;
    return 0;
}

// Anonymous memory free
int sys_anon_free(void *ptr, size_t size) {
    long rc = syscall2(SYS_MUNMAP, (uint64_t)ptr, size);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// VM Map
int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
    long rc = syscall6(SYS_MMAP, (uint64_t)hint, size, prot, flags, fd, offset);
    if (rc == (long)MAP_FAILED || rc < 0) {
        return ENOMEM;
    }
    *window = (void *)rc;
    return 0;
}

// VM Unmap
int sys_vm_unmap(void *ptr, size_t size) {
    long rc = syscall2(SYS_MUNMAP, (uint64_t)ptr, size);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// VM Protect
int sys_vm_protect(void *ptr, size_t size, int prot) {
    long rc = syscall3(SYS_MPROTECT, (uint64_t)ptr, size, prot);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Helper time functions
static int is_leap_year(int year) {
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static int days_in_month(int year, int month) {
    static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 1 && is_leap_year(year)) {
        return 29;
    }
    return mdays[month];
}

static long long days_before_year(int year) {
    long long y = (long long)year - 1;
    return y * 365 + y / 4 - y / 100 + y / 400;
}

static long long days_since_epoch(int year, int month, int day) {
    long long days = days_before_year(year) - days_before_year(1970);
    for (int m = 0; m < month - 1; m++) {
        days += days_in_month(year, m);
    }
    days += (day - 1);
    return days;
}

static time_t seconds_from_ymdhms(int year, int month, int day, int hour, int minute, int second) {
    long long days = days_since_epoch(year, month, day);
    return (time_t)(days * 86400LL + hour * 3600LL + minute * 60LL + second);
}

#ifndef MLIBC_BUILDING_RTLD
// Clock Get
int sys_clock_get(int clock, time_t *secs, long *nanos) {
    if (clock == CLOCK_REALTIME) {
        int dt[6] = {1970, 1, 1, 0, 0, 0};
        if (rtc_get(dt) == 0) {
            *secs = seconds_from_ymdhms(dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
        } else {
            *secs = 0;
        }
        *nanos = 0;
        return 0;
    } else if (clock == CLOCK_MONOTONIC) {
        int ticks = get_ticks();
        *secs = ticks / 1000;
        *nanos = (ticks % 1000) * 1000000;
        return 0;
    }
    return EINVAL;
}
#endif

// Futex wait
int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
    (void)time; // Ignore timeout for simplicity
    long rc = syscall4(SYS_FUTEX, (uint64_t)pointer, FUTEX_WAIT, expected, 0);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Futex wake
int sys_futex_wake(int *pointer) {
    long rc = syscall3(SYS_FUTEX, (uint64_t)pointer, FUTEX_WAKE, 0x7fffffff);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Fork
int sys_fork(pid_t *out_child) {
    long rc = syscall0(SYS_FORK);
    if (rc < 0) {
        return -rc;
    }
    *out_child = rc;
    return 0;
}

// Execve
int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    (void)envp;
    
    // Join arguments into a single space-separated string (as expected by BoredOS sys_exec)
    char args_buf[512] = {0};
    size_t used = 0;
    
    if (argv && argv[0]) {
        for (int i = 1; argv[i]; i++) {
            const char *a = argv[i];
            size_t len = strlen(a);
            if (used && used + 1 < sizeof(args_buf)) {
                args_buf[used++] = ' ';
            }
            for (size_t j = 0; j < len && used + 1 < sizeof(args_buf); j++) {
                args_buf[used++] = a[j];
            }
        }
    }
    if (used < sizeof(args_buf)) {
        args_buf[used] = '\0';
    }

    long rc = syscall2(SYS_EXECVE, (uint64_t)path, (uint64_t)(args_buf[0] ? args_buf : nullptr));
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Waitpid
int sys_waitpid(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
    (void)ru;
    long rc = syscall3(SYS_WAIT4, pid, (uint64_t)status, flags);
    if (rc < 0) {
        return -rc;
    }
    *ret_pid = rc;
    return 0;
}

// GetPid
pid_t sys_getpid() {
    return syscall0(SYS_GETPID);
}

// GetCwd
int sys_getcwd(char *buffer, size_t size) {
    long rc = syscall2(SYS_GETCWD, (uint64_t)buffer, size);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Chdir
int sys_chdir(const char *path) {
    long rc = syscall1(SYS_CHDIR, (uint64_t)path);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Mkdir
int sys_mkdir(const char *path, mode_t mode) {
    (void)mode;
    long rc = syscall1(SYS_MKDIR, (uint64_t)path);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Unlinkat
int sys_unlinkat(int dirfd, const char *path, int flags) {
    (void)dirfd;
    (void)flags;
    long rc = syscall1(SYS_UNLINK, (uint64_t)path);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Dup
int sys_dup(int fd, int flags, int *newfd) {
    (void)flags;
    long rc = syscall1(SYS_DUP, fd);
    if (rc < 0) {
        return -rc;
    }
    *newfd = rc;
    return 0;
}

// Dup2
int sys_dup2(int fd, int flags, int newfd) {
    (void)flags;
    long rc = syscall2(SYS_DUP2, fd, newfd);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Pipe
int sys_pipe(int pipefd[2], int flags) {
    (void)flags;
    long rc = syscall1(SYS_PIPE, (uint64_t)pipefd);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Fcntl
int sys_fcntl(int fd, int cmd, va_list args, int *result) {
    uint64_t arg = va_arg(args, uint64_t);
    long rc = syscall3(SYS_FCNTL, fd, cmd, arg);
    if (rc < 0) {
        return -rc;
    }
    *result = rc;
    return 0;
}

// Poll
int sys_poll(struct pollfd *fds, nfds_t nfds, int timeout, int *result) {
    long rc = syscall3(SYS_POLL, (uint64_t)fds, nfds, timeout);
    while (rc == -2) {
        rc = syscall3(SYS_POLL, (uint64_t)fds, nfds, 0);
    }
    if (rc < 0) {
        return -rc;
    }
    *result = rc;
    return 0;
}

#ifndef MLIBC_BUILDING_RTLD
// Pselect
int sys_pselect(
    int nfds,
    fd_set *read_set,
    fd_set *write_set,
    fd_set *except_set,
    const struct timespec *timeout,
    const sigset_t *sigmask,
    int *num_events
) {
    (void)sigmask;
    if (nfds <= 0) {
        if (read_set) FD_ZERO(read_set);
        if (write_set) FD_ZERO(write_set);
        if (except_set) FD_ZERO(except_set);
        *num_events = 0;
        return 0;
    }

    struct pollfd stack_fds[64];
    struct pollfd *fds = stack_fds;
    if (nfds > 64) {
        fds = (struct pollfd *)calloc(nfds, sizeof(struct pollfd));
        if (!fds) return ENOMEM;
    } else {
        memset(stack_fds, 0, sizeof(struct pollfd) * nfds);
    }

    for (int i = 0; i < nfds; i++) {
        struct pollfd *fd = &fds[i];
        if (read_set && FD_ISSET(i, read_set)) fd->events |= POLLIN;
        if (write_set && FD_ISSET(i, write_set)) fd->events |= POLLOUT;
        if (except_set && FD_ISSET(i, except_set)) fd->events |= POLLPRI;

        if (!fd->events) {
            fd->fd = -1;
            continue;
        }
        fd->fd = i;
    }

    int timeout_ms = -1;
    if (timeout) {
        timeout_ms = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000;
    }

    int poll_result = 0;
    int e = sys_poll(fds, nfds, timeout_ms, &poll_result);
    if (e != 0) {
        if (fds != stack_fds) free(fds);
        return e;
    }

    int count = 0;
    fd_set res_read_set, res_write_set, res_except_set;
    FD_ZERO(&res_read_set);
    FD_ZERO(&res_write_set);
    FD_ZERO(&res_except_set);

    for (int i = 0; i < nfds; i++) {
        struct pollfd *fd = &fds[i];
        if (read_set && FD_ISSET(i, read_set) && (fd->revents & (POLLIN | POLLERR | POLLHUP))) {
            FD_SET(i, &res_read_set);
            count++;
        }
        if (write_set && FD_ISSET(i, write_set) && (fd->revents & (POLLOUT | POLLERR | POLLHUP))) {
            FD_SET(i, &res_write_set);
            count++;
        }
        if (except_set && FD_ISSET(i, except_set) && (fd->revents & POLLPRI)) {
            FD_SET(i, &res_except_set);
            count++;
        }
    }

    if (read_set) *read_set = res_read_set;
    if (write_set) *write_set = res_write_set;
    if (except_set) *except_set = res_except_set;

    if (fds != stack_fds) free(fds);
    *num_events = count;
    return 0;
}
#endif

// Ioctl
int sys_ioctl(int fd, unsigned long request, void *arg, int *result) {
    // Intercept Termios get/set attributes since BoredOS kernel does not back termios
    if (request == TCGETS) {
        struct termios *termios_p = (struct termios *)arg;
        if (!termios_p) return EINVAL;
        termios_p->c_iflag = ICRNL | IXON;
        termios_p->c_oflag = OPOST | ONLCR;
        termios_p->c_cflag = CS8 | CREAD | CLOCAL;
        termios_p->c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN;
        termios_p->c_cc[VMIN] = 1;
        termios_p->c_cc[VTIME] = 0;
        if (result) *result = 0;
        return 0;
    }
    if (request == TCSETS || request == TCSETSW || request == TCSETSF) {
        if (result) *result = 0;
        return 0;
    }

    long rc = syscall3(SYS_IOCTL, fd, request, (uint64_t)arg);
    if (rc < 0) {
        return -rc;
    }
    if (result) *result = rc;
    return 0;
}

// Sockets: Socket
int sys_socket(int domain, int type, int protocol, int *fd) {
    long rc = syscall3(SYS_SOCKET, domain, type, protocol);
    if (rc < 0) {
        return -rc;
    }
    *fd = rc;
    return 0;
}

// Sockets: Connect
int sys_connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    long rc = syscall3(SYS_CONNECT, fd, (uint64_t)addr, addrlen);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Sockets: Bind
int sys_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    long rc = syscall3(SYS_BIND, fd, (uint64_t)addr, addrlen);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Sockets: Listen
int sys_listen(int fd, int backlog) {
    long rc = syscall2(SYS_LISTEN, fd, backlog);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Sockets: Accept
int sys_accept(int fd, int *newfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    (void)flags;
    long rc = syscall3(SYS_ACCEPT, fd, (uint64_t)addr, (uint64_t)addrlen);
    if (rc == -2) {
        return EAGAIN;
    }
    if (rc < 0) {
        return -rc;
    }
    *newfd = rc;
    return 0;
}

// Sockets: Sendto
ssize_t sys_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, ssize_t *length) {
    long rc = syscall6(SYS_SENDTO, fd, (uint64_t)buf, len, flags,
                       (uint64_t)dest_addr, dest_addr ? (uint64_t)addrlen : 0);
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *length = rc;
    return 0;
}

// Sockets: Recvfrom
ssize_t sys_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, ssize_t *length) {
    uint64_t temp_len = (src_addr && addrlen) ? (uint64_t)*addrlen : 0;
    long rc = syscall6(SYS_RECVFROM, fd, (uint64_t)buf, len, flags,
                       (uint64_t)src_addr, src_addr ? (uint64_t)&temp_len : 0);
    if (rc >= 0 && src_addr && addrlen) {
        *addrlen = (socklen_t)temp_len;
    }
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *length = rc;
    return 0;
}

// Sockets: Setsockopt
int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
    long rc = syscall5(SYS_SETSOCKOPT, fd, layer, number, (uint64_t)buffer, size);
    if (rc < 0) return -rc;
    return 0;
}

// Sockets: Getsockopt
int sys_getsockopt(int fd, int layer, int number, void *buffer, socklen_t *size) {
    long rc = syscall5(SYS_GETSOCKOPT, fd, layer, number, (uint64_t)buffer, (uint64_t)size);
    if (rc < 0) return -rc;
    return 0;
}

// Sockets: Socketpair
int sys_socketpair(int domain, int type, int protocol, int *sv) {
    long rc = syscall4(SYS_SOCKETPAIR, domain, type, protocol, (uint64_t)sv);
    if (rc < 0) return -rc;
    return 0;
}

// Sockets: Getsockname
int sys_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    long rc = syscall3(SYS_GETSOCKNAME, fd, (uint64_t)addr, (uint64_t)addrlen);
    if (rc < 0) return -rc;
    return 0;
}

// Sockets: Getpeername
int sys_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen) {
    long rc = syscall3(SYS_GETPEERNAME, fd, (uint64_t)addr, (uint64_t)addrlen);
    if (rc < 0) return -rc;
    return 0;
}

// Sockets: Sendmsg
int sys_msg_send(int fd, const struct msghdr *msg, int flags, ssize_t *length) {
    long rc = syscall3(SYS_SENDMSG, fd, (uint64_t)msg, flags);
    if (rc < 0) return -rc;
    if (length) *length = rc;
    return 0;
}

// Sockets: Recvmsg
int sys_msg_recv(int fd, struct msghdr *msg, int flags, ssize_t *length) {
    long rc = syscall3(SYS_RECVMSG, fd, (uint64_t)msg, flags);
    if (rc < 0) return -rc;
    if (length) *length = rc;
    return 0;
}

// Signals: Sigaction
int sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oact) {
    long rc = syscall3(SYS_RT_SIGACTION, sig, (uint64_t)act, (uint64_t)oact);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Signals: Sigprocmask
int sys_sigprocmask(int how, const sigset_t *set, sigset_t *o_set) {
    long rc = syscall3(SYS_RT_SIGPROCMASK, how, (uint64_t)set, (uint64_t)o_set);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Signals: Sigpending
int sys_sigpending(sigset_t *set) {
    long rc = syscall1(SYS_RT_SIGPENDING, (uint64_t)set);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Signals: Kill
int sys_kill(pid_t pid, int sig) {
    long rc = syscall2(SYS_KILL, pid, sig);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Thread Set TCB (Stub for static compiler/runtime)
int sys_tcb_set(void *pointer) {
    // In BoredOS, thread base is set via FS register using arch_prctl
    // SYS_ARCH_PRCTL (158) with ARCH_SET_FS (0x1002)
    long rc = syscall2(SYS_ARCH_PRCTL, 0x1002, (uint64_t)pointer);
    if (rc < 0) {
        return -rc;
    }
    return 0;
}

// Access check
int sys_access(const char *path, int mode) {
    (void)mode;
    long rc = syscall1(SYS_EXISTS, (uint64_t)path);
    if (rc <= 0) {
        return ENOENT;
    }
    return 0;
}

// Stat (file metadata)
int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
    (void)flags;
    
    // Initialize statbuf to default empty values
    statbuf->st_dev = 0;
    statbuf->st_ino = 0;
    statbuf->st_mode = 0;
    statbuf->st_nlink = 1;
    statbuf->st_uid = 0;
    statbuf->st_gid = 0;
    statbuf->st_rdev = 0;
    statbuf->st_size = 0;
    statbuf->st_blksize = 512;
    statbuf->st_blocks = 0;
    statbuf->st_atime = 0;
    statbuf->st_mtime = 0;
    statbuf->st_ctime = 0;

    if (fsfdt == fsfd_target::path || fsfdt == fsfd_target::fd_path) {
        if (!path || path[0] == '\0') {
            return EINVAL;
        }
        
        long open_rc = syscall2(SYS_OPEN, (uint64_t)path, (uint64_t)"rb");
        if (open_rc >= 0) {
            int open_fd = open_rc;
            long size_rc = syscall1(SYS_SIZE, open_fd);
            if (size_rc >= 0) {
                statbuf->st_size = size_rc;
            }
            statbuf->st_mode = S_IFREG | 0644;
            statbuf->st_blocks = (statbuf->st_size + 511) / 512;
            syscall1(SYS_CLOSE, open_fd);
            return 0;
        } else {
            return ENOENT;
        }
    } else if (fsfdt == fsfd_target::fd) {
        if (fd < 0) return EBADF;
        if (fd == 0 || fd == 1 || fd == 2) {
            statbuf->st_mode = S_IFCHR | 0666;
            statbuf->st_rdev = 1;
            return 0;
        }
        long size_rc = syscall1(SYS_SIZE, fd);
        if (size_rc < 0) {
            statbuf->st_mode = S_IFCHR | 0666;
            return 0;
        }
        statbuf->st_size = size_rc;
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_blocks = (statbuf->st_size + 511) / 512;
        return 0;
    }
    return EINVAL;
}

// Termios tcgetattr
int sys_tcgetattr(int fd, struct termios *attr) {
    int result = 0;
    return sys_ioctl(fd, TCGETS, attr, &result);
}

// Termios tcsetattr
int sys_tcsetattr(int fd, int optional_action, const struct termios *attr) {
    int req;
    switch (optional_action) {
        case TCSANOW: req = TCSETS; break;
        case TCSADRAIN: req = TCSETSW; break;
        case TCSAFLUSH: req = TCSETSF; break;
        default: return EINVAL;
    }
    int result = 0;
    return sys_ioctl(fd, req, const_cast<struct termios *>(attr), &result);
}

int perform_ioctl(int fd, unsigned long request, void *arg, int *result) {
    return sys_ioctl(fd, request, arg, result);
}

} // namespace mlibc

#undef errno
extern "C" int errno = 0;

extern "C" int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *arg = va_arg(args, void *);
    va_end(args);

    int result = 0;
    int error = mlibc::perform_ioctl(fd, request, arg, &result);
    if (error) {
        errno = error;
        return -1;
    }
    return result;
}
