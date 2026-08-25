// Copyright (c) 2023-2026 Christiaan (chris@boreddev.nl)
// This software is released under the GNU General Public License v3.0. See LICENSE file for details.
// This header needs to maintain in any file it is present in, as per the GPL license terms.
#include "syscall.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations for helpers defined later in this file */
int get_ticks(void);
int sched_yield(void);

extern "C" {

uint64_t syscall0(uint64_t sys_num) {
    uint64_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num)
                 : "rcx", "r11", "memory");
    return ret;
}

uint64_t syscall1(uint64_t sys_num, uint64_t arg1) {
    uint64_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1)
                 : "rcx", "r11", "memory");
    return ret;
}

uint64_t syscall2(uint64_t sys_num, uint64_t arg1, uint64_t arg2) {
    uint64_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1), "S"(arg2)
                 : "rcx", "r11", "memory");
    return ret;
}

uint64_t syscall3(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1), "S"(arg2), "d"(arg3)
                 : "rcx", "r11", "memory", "r10", "r8", "r9");
    return ret;
}

uint64_t syscall4(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = arg4;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                 : "rcx", "r11", "memory", "r8", "r9");
    return ret;
}

uint64_t syscall5(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = arg4;
    register uint64_t r8  asm("r8") = arg5;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
                 : "rcx", "r11", "memory", "r9");
    return ret;
}

uint64_t syscall6(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    uint64_t ret;
    register uint64_t r10 asm("r10") = arg4;
    register uint64_t r8  asm("r8") = arg5;
    register uint64_t r9  asm("r9") = arg6;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(sys_num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8), "r"(r9)
                 : "rcx", "r11", "memory");
    return ret;
}


void sys_exit(int status) {
    syscall1(SYS_EXIT, (uint64_t)status);
    while (1); 
}

int sys_read(int fd, void *buf, uint32_t len) {
    return (int)syscall3(SYS_READ, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

int sys_write(int fd, const void *buf, uint32_t len) {
    return (int)syscall3(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}

int sys_write_fs(int fd, const void *buf, uint32_t len) {
    return sys_write(fd, buf, len);
}

void *sys_sbrk(int incr) {
    uintptr_t cur = (uintptr_t)syscall1(SYS_BRK, 0);
    if (incr == 0) return (void *)cur;
    uintptr_t next = (uintptr_t)syscall1(SYS_BRK, (uint64_t)(cur + incr));
    if (next == cur && incr > 0) return (void *)-1;
    return (void *)cur;
}

int sys_open(const char *path, const char *mode) {
    return (int)syscall2(SYS_OPEN, (uint64_t)path, (uint64_t)mode);
}

void sys_close(int fd) {
    syscall1(SYS_CLOSE, (uint64_t)fd);
}

int sys_seek(int fd, int offset, int whence) {
    return (int)syscall3(SYS_LSEEK, (uint64_t)fd, (uint64_t)offset, (uint64_t)whence);
}

uint32_t sys_tell(int fd) {
    return (uint32_t)syscall1(SYS_TELL, (uint64_t)fd);
}

uint32_t sys_size(int fd) {
    return (uint32_t)syscall1(SYS_SIZE, (uint64_t)fd);
}

int sys_list(const char *path, FAT32_FileInfo *entries, int max_entries) {
    return sys_list_offset(path, entries, max_entries, 0);
}

int sys_list_offset(const char *path, FAT32_FileInfo *entries, int max_entries, int offset) {
    return (int)syscall4(SYS_LIST_OFFSET, (uint64_t)path, (uint64_t)entries, (uint64_t)max_entries, (uint64_t)offset);
}

int sys_get_file_info(const char *path, FAT32_FileInfo *info) {
    return (int)syscall2(SYS_STAT, (uint64_t)path, (uint64_t)info);
}

int sys_delete(const char *path) {
    return (int)syscall1(SYS_UNLINK, (uint64_t)path);
}

int sys_mkdir(const char *path) {
    return (int)syscall1(SYS_MKDIR, (uint64_t)path);
}

int sys_exists(const char *path) {
    return (int)syscall1(SYS_EXISTS, (uint64_t)path);
}

int sys_getcwd(char *buf, int size) {
    return (int)syscall2(SYS_GETCWD, (uint64_t)buf, (uint64_t)size);
}

int sys_chdir(const char *path) {
    return (int)syscall1(SYS_CHDIR, (uint64_t)path);
}

int sys_dup(int oldfd) {
    return (int)syscall1(SYS_DUP, (uint64_t)oldfd);
}

int sys_dup2(int oldfd, int newfd) {
    return (int)syscall2(SYS_DUP2, (uint64_t)oldfd, (uint64_t)newfd);
}

int sys_pipe(int pipefd[2]) {
    return (int)syscall1(SYS_PIPE, (uint64_t)pipefd);
}

int sys_fcntl(int fd, int cmd, int val) {
    return (int)syscall3(SYS_FCNTL, (uint64_t)fd, (uint64_t)cmd, (uint64_t)val);
}

int sys_fs_statfs(const char *path, vfs_statfs_t *stat) {
    return (int)syscall2(SYS_FS_STATFS, (uint64_t)path, (uint64_t)stat);
}

int sys_fs_mount_count(void) {
    return (int)syscall0(SYS_FS_MOUNT_COUNT);
}

int sys_fs_mount_info(int index, mount_info_t *info) {
    return (int)syscall2(SYS_FS_MOUNT_INFO, (uint64_t)index, (uint64_t)info);
}

int sys_poll(void *fds, int nfds, int timeout) {
    int rc;
    if (timeout > 0) {
        uint32_t start_ticks = (uint32_t)get_ticks();
        int remaining = timeout;
        while (1) {
            rc = (int)syscall3(SYS_POLL, (uint64_t)fds, (uint64_t)nfds, (uint64_t)remaining);
            if (rc != -2) {
                break;
            }
            uint32_t now = (uint32_t)get_ticks();
            int elapsed = (int)(now - start_ticks);
            if (elapsed >= timeout) {
                rc = 0;
                break;
            }
            remaining = timeout - elapsed;
            if (remaining <= 0) {
                rc = 0;
                break;
            }
        }
        if (rc == 0) {
            // Perform one final non-blocking poll call to clean up wait queues in the kernel
            syscall3(SYS_POLL, (uint64_t)fds, (uint64_t)nfds, 0);
        }
    } else {
        while ((rc = (int)syscall3(SYS_POLL, (uint64_t)fds, (uint64_t)nfds, (uint64_t)timeout)) == -2);
    }
    return rc;
}

int sys_spawn(const char *path, const char *args, uint64_t flags, uint64_t tty_id) {
    return (int)syscall4(SYS_SPAWN, (uint64_t)path, (uint64_t)args, flags, (uint64_t)tty_id);
}

int sys_exec(const char *path, const char *args) {
    return (int)syscall2(SYS_EXECVE, (uint64_t)path, (uint64_t)args);
}

int sys_waitpid(int pid, int *status, int options) {
    return (int)syscall3(SYS_WAIT4, (uint64_t)pid, (uint64_t)status, (uint64_t)options);
}

int sys_kill_signal(int pid, int sig) {
    return (int)syscall2(SYS_KILL, (uint64_t)pid, (uint64_t)sig);
}

int sys_sigaction(int sig, const void *act, void *oldact) {
    return (int)syscall3(SYS_RT_SIGACTION, (uint64_t)sig, (uint64_t)act, (uint64_t)oldact);
}

int sys_sigprocmask(int how, const unsigned long *set, unsigned long *oldset) {
    return (int)syscall3(SYS_RT_SIGPROCMASK, (uint64_t)how, (uint64_t)set, (uint64_t)oldset);
}

int sys_ioctl(int fd, unsigned long request, void *arg) {
    return (int)syscall3(SYS_IOCTL, (uint64_t)fd, (uint64_t)request, (uint64_t)arg);
}

int sys_sigpending(unsigned long *set) {
    return (int)syscall1(SYS_RT_SIGPENDING, (uint64_t)set);
}

void sys_kill(int pid) {
    sys_kill_signal(pid, 9); // Terminate immediately
}




void set_text_color(uint32_t color) {
    char seq[64];
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    sprintf(seq, "\x1B[38;2;%d;%d;%dm", r, g, b);
    sys_write(1, seq, strlen(seq));
}

int dns_lookup(const char *name, net_ipv4_address_t *out_ip) {
    if (!name || !*name || !out_ip) return -1;
    if (strcmp(name, "localhost") == 0 || strcmp(name, "localhost.localdomain") == 0) {
        out_ip->bytes[0] = 127;
        out_ip->bytes[1] = 0;
        out_ip->bytes[2] = 0;
        out_ip->bytes[3] = 1;
        return 0;
    }
    uint32_t dns_servers[4];
    int num_dns = 0;

    int fd = sys_open("/etc/resolv.conf", 0 /* O_RDONLY */);
    if (fd >= 0) {
        char buf[256];
        int bytes = sys_read(fd, buf, sizeof(buf) - 1);
        sys_close(fd);
        if (bytes > 0) {
            buf[bytes] = '\0';
            char *line = buf;
            while (line && *line && num_dns < 4) {
                char *ns = strstr(line, "nameserver");
                if (!ns) break;
                ns += 10;
                while (*ns == ' ' || *ns == '\t') ns++;
                int ip[4];
                if (sscanf(ns, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
                    uint32_t s = (ip[0] & 0xFF) | ((ip[1] & 0xFF) << 8) | ((ip[2] & 0xFF) << 16) | ((ip[3] & 0xFF) << 24);
                    int exists = 0;
                    for (int k = 0; k < num_dns; k++) {
                        if (dns_servers[k] == s) { exists = 1; break; }
                    }
                    if (!exists) dns_servers[num_dns++] = s;
                }
                line = strchr(ns, '\n');
                if (line) line++;
            }
        }
    }
    if (num_dns == 0) return -1;

    int sockfd = (int)syscall3(SYS_SOCKET, 2 /* AF_INET */, 2 /* SOCK_DGRAM */, 0);
    if (sockfd < 0) return -1;

    // Set non-blocking
    int flags = (int)syscall3(SYS_FCNTL, (uint64_t)sockfd, 3 /* F_GETFL */, 0);
    syscall3(SYS_FCNTL, (uint64_t)sockfd, 4 /* F_SETFL */, (uint64_t)(flags | 0x800 /* O_NONBLOCK */));

    uint8_t query[512];
    memset(query, 0, sizeof(query));
    query[0] = 0x12; query[1] = 0x34; // Transaction ID
    query[2] = 0x01; query[3] = 0x00; // Flags: Standard query
    query[4] = 0x00; query[5] = 0x01; // Questions: 1

    int q_idx = 12;
    const char *src = name;
    while (*src) {
        const char *dot = strchr(src, '.');
        int len = dot ? (dot - src) : (int)strlen(src);
        query[q_idx++] = len;
        memcpy(query + q_idx, src, len);
        q_idx += len;
        src = dot ? (dot + 1) : (src + len);
    }
    query[q_idx++] = 0; // Terminate name

    query[q_idx++] = 0x00; query[q_idx++] = 0x01; // Type: A
    query[q_idx++] = 0x00; query[q_idx++] = 0x01; // Class: IN

    int query_len = q_idx;

    struct sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        uint32_t sin_addr;
        char sin_zero[8];
    } dest;

    for (int s_idx = 0; s_idx < num_dns; s_idx++) {
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = 2; // AF_INET
        dest.sin_port = ((53 & 0xFF) << 8) | ((53 >> 8) & 0xFF); // htons(53)
        dest.sin_addr = dns_servers[s_idx];
        syscall6(SYS_SENDTO, (uint64_t)sockfd, (uint64_t)query, (uint64_t)query_len, 0, (uint64_t)&dest, sizeof(dest));
    }

    uint8_t resp[1024];
    int resp_len = -1;
    int start_ticks = get_ticks();
    while (get_ticks() - start_ticks < 1500) {
        struct sockaddr_in src_addr;
        uint64_t addr_len = sizeof(src_addr);
        resp_len = (int)syscall6(SYS_RECVFROM, (uint64_t)sockfd, (uint64_t)resp, sizeof(resp), 0, (uint64_t)&src_addr, (uint64_t)&addr_len);
        if (resp_len > 0) {
            break;
        }
        sched_yield();
    }

    sys_close(sockfd);
    if (resp_len < 12) return -1;

    if (resp[0] != 0x12 || resp[1] != 0x34) return -1;
    if ((resp[3] & 0x0F) != 0) return -1;

    int answers = (resp[6] << 8) | resp[7];
    if (answers == 0) return -1;

    int idx = 12;
    int questions = (resp[4] << 8) | resp[5];
    for (int q = 0; q < questions; q++) {
        while (resp[idx]) {
            if ((resp[idx] & 0xC0) == 0xC0) {
                idx += 2;
                goto skipped_q_name;
            }
            idx += resp[idx] + 1;
        }
        idx++;
    skipped_q_name:
        idx += 4;
    }

    for (int a = 0; a < answers; a++) {
        while (resp[idx]) {
            if ((resp[idx] & 0xC0) == 0xC0) {
                idx += 2;
                goto skipped_a_name;
            }
            idx += resp[idx] + 1;
        }
        idx++;
    skipped_a_name:;
        uint16_t type = (resp[idx] << 8) | resp[idx + 1];
        uint16_t dns_class = (resp[idx + 2] << 8) | resp[idx + 3];
        uint16_t rdlen = (resp[idx + 8] << 8) | resp[idx + 9];
        idx += 10;
        if (type == 0x0001 && dns_class == 0x0001 && rdlen == 4) {
            out_ip->bytes[0] = resp[idx];
            out_ip->bytes[1] = resp[idx + 1];
            out_ip->bytes[2] = resp[idx + 2];
            out_ip->bytes[3] = resp[idx + 3];
            return 0;
        }
        idx += rdlen;
    }

    return -1;
}





int sys_disk_get_count(void) {
    return (int)syscall0(SYS_DISK_GET_COUNT);
}

int sys_disk_get_info(int index, disk_info_t *out) {
    return (int)syscall2(SYS_DISK_GET_INFO, (uint64_t)index, (uint64_t)out);
}

int sys_disk_mount(const char *devname, const char *mountpoint) {
    return (int)syscall2(SYS_DISK_MOUNT, (uint64_t)devname, (uint64_t)mountpoint);
}

int sys_disk_umount(const char *mountpoint) {
    return (int)syscall1(SYS_DISK_UMOUNT, (uint64_t)mountpoint);
}

int sys_disk_sync(const char *mountpoint) {
    return (int)syscall1(SYS_DISK_SYNC, (uint64_t)mountpoint);
}

int sys_disk_rescan(const char *devname) {
    return (int)syscall1(SYS_DISK_RESCAN, (uint64_t)devname);
}


int rtc_get(int *dt) {
    int fd = sys_open("/dev/rtc", "r");
    if (fd < 0) return -1;
    char buf[64];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    
    int y = 0;
    for (int i = 0; i < 4; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            y = y * 10 + (buf[i] - '0');
        } else {
            return -1;
        }
    }
    int m = (buf[5] - '0') * 10 + (buf[6] - '0');
    int d = (buf[8] - '0') * 10 + (buf[9] - '0');
    int h = (buf[11] - '0') * 10 + (buf[12] - '0');
    int min = (buf[14] - '0') * 10 + (buf[15] - '0');
    int s = (buf[17] - '0') * 10 + (buf[18] - '0');
    
    if (dt) {
        dt[0] = y;
        dt[1] = m;
        dt[2] = d;
        dt[3] = h;
        dt[4] = min;
        dt[5] = s;
    }
    return 0;
}

int rtc_set(int *dt) {
    if (!dt) return -1;
    int fd = sys_open("/dev/rtc", "w");
    if (fd < 0) return -1;
    char buf[64];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d\n", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
    sys_write(fd, buf, strlen(buf));
    sys_close(fd);
    return 0;
}

int sys_reboot(void) {
    return (int)syscall0(SYS_REBOOT);
}

int sys_shutdown(void) {
    return (int)syscall0(SYS_SHUTDOWN);
}


int get_ticks(void) {
    static int fd = -1;
    if (fd < 0) {
        fd = sys_open("/sys/kernel/ticks", "r");
        if (fd < 0) return 0;
    }
    char buf[32];
    sys_seek(fd, 0, 0);
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        sys_close(fd);
        fd = sys_open("/sys/kernel/ticks", "r");
        if (fd < 0) return 0;
        n = sys_read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) return 0;
    }
    buf[n] = '\0';
    int val = 0;
    for (int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++) {
        val = val * 10 + (buf[i] - '0');
    }
    return val;
}



int sys_getpid(void) {
    return (int)syscall0(SYS_GETPID);
}

}

