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
#include <syscall.h>
#include <stdarg.h>

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404

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

// Check if fd is a TTY
int sys_isatty(int fd) {
    if (fd == 0 || fd == 1 || fd == 2) {
        return 0; // 0 means success (it is a TTY) in mlibc sysdeps
    }
    return ENOTTY;
}

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
    
    // Check path exists unless creating
    int exists = sys_exists(path);
    if ((flags & O_CREAT) && (flags & O_EXCL) && exists) {
        return EEXIST;
    }
    if (!(flags & O_CREAT) && !exists) {
        return ENOENT;
    }

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
    int is_a_tty = 0;
    if (sys_isatty(fd) == 0) {
        return ESPIPE;
    }

    long rc = syscall3(SYS_LSEEK, fd, offset, whence);
    if (rc < 0) {
        if (rc == -1) return ESPIPE;
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
    if (sys_exists(path)) {
        return EEXIST;
    }
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
    if (rc == -2) {
        // If we blocked and woke up, query the state non-blockingly (timeout = 0)
        // or block indefinitely if timeout was -1.
        int next_timeout = (timeout < 0) ? -1 : 0;
        while ((rc = syscall3(SYS_POLL, (uint64_t)fds, nfds, next_timeout)) == -2) {}
    }
    if (rc < 0) {
        return -rc;
    }
    *result = rc;
    return 0;
}

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
    long rc;
    
    // Check if the file descriptor has O_NONBLOCK set
    long fd_flags = syscall3(SYS_FCNTL, fd, F_GETFL, 0);
    bool nonblocking = (fd_flags >= 0 && (fd_flags & O_NONBLOCK));
    
    if (nonblocking) {
        rc = syscall3(SYS_ACCEPT, fd, (uint64_t)addr, (uint64_t)addrlen);
        if (rc == -2) return EAGAIN;
    } else {
        for (;;) {
            rc = syscall3(SYS_ACCEPT, fd, (uint64_t)addr, (uint64_t)addrlen);
            if (rc != -2) break;
            syscall0(SYS_SCHED_YIELD);
        }
    }
    
    if (rc < 0) {
        return -rc;
    }
    *newfd = rc;
    return 0;
}

// Sockets: Sendto
ssize_t sys_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, ssize_t *length) {
    long rc;
    if (dest_addr == nullptr) {
        rc = syscall3(SYS_WRITE, fd, (uint64_t)buf, len);
    } else {
        rc = syscall6(SYS_SENDTO, fd, (uint64_t)buf, len, flags, (uint64_t)dest_addr, addrlen);
    }
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *length = rc;
    return 0;
}

// Sockets: Recvfrom
ssize_t sys_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, ssize_t *length) {
    long rc;
    if (src_addr == nullptr) {
        rc = syscall3(SYS_READ, fd, (uint64_t)buf, len);
    } else {
        uint64_t temp_len = addrlen ? *addrlen : 0;
        rc = syscall6(SYS_RECVFROM, fd, (uint64_t)buf, len, flags, (uint64_t)src_addr, (uint64_t)&temp_len);
        if (rc >= 0 && addrlen) {
            *addrlen = temp_len;
        }
    }
    if (rc < 0) {
        if (rc == -2) return EAGAIN;
        return -rc;
    }
    *length = rc;
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
    if (!sys_exists(path)) {
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
        if (!sys_exists(path)) {
            return ENOENT;
        }
        
        FAT32_FileInfo info;
        if (sys_get_file_info(path, &info) == 0) {
            statbuf->st_size = info.size;
            if (info.is_directory) {
                statbuf->st_mode = S_IFDIR | 0755;
            } else {
                statbuf->st_mode = S_IFREG | 0644;
            }
            statbuf->st_blocks = (statbuf->st_size + 511) / 512;
            statbuf->st_ino = info.start_cluster;
        } else {
            // Fallback: try raw open/size/close
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
            } else {
                return -open_rc;
            }
        }
    } else if (fsfdt == fsfd_target::fd) {
        long size_rc = syscall1(SYS_SIZE, fd);
        if (size_rc < 0) {
            return -size_rc;
        }
        statbuf->st_size = size_rc;
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_blocks = (statbuf->st_size + 511) / 512;
    } else {
        return ENOSYS;
    }

    return 0;
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
