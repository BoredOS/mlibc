// Copyright (c) 2023-2026 Christiaan (chris@boreddev.nl)
// This software is released under the GNU General Public License v3.0. See LICENSE file for details.
// This header needs to maintain in any file it is present in, as per the GPL license terms.
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Standard syscalls aligned with Linux numbers and BoredOS custom numbers
enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_STAT = 4,
    SYS_POLL = 7,
    SYS_LSEEK = 8,
    SYS_MMAP = 9,
    SYS_MUNMAP = 11,
    SYS_BRK = 12,
    SYS_RT_SIGACTION = 13,
    SYS_RT_SIGPROCMASK = 14,
    SYS_IOCTL = 16,
    SYS_PIPE = 22,
    SYS_SCHED_YIELD = 24,
    SYS_DUP = 32,
    SYS_DUP2 = 33,
    SYS_NANOSLEEP = 35,
    SYS_GETPID = 39,
    SYS_SOCKET = 41,
    SYS_CONNECT = 42,
    SYS_ACCEPT = 43,
    SYS_SENDTO = 44,
    SYS_RECVFROM = 45,
    SYS_BIND = 49,
    SYS_LISTEN = 50,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT = 60,
    SYS_WAIT4 = 61,
    SYS_KILL = 62,
    SYS_FCNTL = 72,
    SYS_RT_SIGPENDING = 73,
    SYS_GETCWD = 79,
    SYS_CHDIR = 80,
    SYS_MKDIR = 83,
    SYS_UNLINK = 87,
    SYS_ARCH_PRCTL = 158,
    SYS_FUTEX = 202,

    // Custom BoredOS system calls
    SYS_LIST_OFFSET = 300,
    SYS_SIZE = 301,
    SYS_TELL = 302,
    SYS_EXISTS = 303,
    SYS_FS_STATFS = 304,
    SYS_FS_MOUNT_COUNT = 305,
    SYS_FS_MOUNT_INFO = 306,
    SYS_TTY_CREATE = 307,
    SYS_TTY_READ_OUT = 308,
    SYS_TTY_WRITE_IN = 309,
    SYS_TTY_READ_IN = 310,
    SYS_TTY_DESTROY = 311,
    SYS_TTY_SET_FG = 312,
    SYS_TTY_GET_FG = 313,
    SYS_TTY_KILL_FG = 314,
    SYS_TTY_KILL_ALL = 315,
    SYS_TTY_GET_ID = 316,
    SYS_SPAWN = 317,
    SYS_PTY_CREATE = 320,
    SYS_PTY_DESTROY = 321,
    SYS_DISK_GET_COUNT = 322,
    SYS_DISK_GET_INFO = 323,
    SYS_DISK_WRITE_GPT = 324,
    SYS_DISK_WRITE_MBR = 325,
    SYS_DISK_MKFS_FAT32 = 326,
    SYS_DISK_MOUNT = 327,
    SYS_DISK_UMOUNT = 328,
    SYS_DISK_SYNC = 329,
    SYS_DISK_RESCAN = 330,
    SYS_REBOOT = 349,
    SYS_SHUTDOWN = 350
};

// Futex operations
enum {
    FUTEX_WAIT = 0,
    FUTEX_WAKE = 1
};

enum {
    SPAWN_FLAG_TERMINAL = 0x1,
    SPAWN_FLAG_INHERIT_TTY = 0x2,
    SPAWN_FLAG_TTY_ID = 0x4,
    SPAWN_FLAG_BACKGROUND = 0x8
};

// ELF app metadata (mirrors src/sys/elf.h, kept in sync manually)
#define BOREDOS_APP_METADATA_MAX_APP_NAME   64
#define BOREDOS_APP_METADATA_MAX_DESCRIPTION 192
#define BOREDOS_APP_METADATA_MAX_IMAGES     4
#define BOREDOS_APP_METADATA_MAX_IMAGE_PATH 160

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t image_count;
    uint16_t reserved;
    char app_name[BOREDOS_APP_METADATA_MAX_APP_NAME];
    char description[BOREDOS_APP_METADATA_MAX_DESCRIPTION];
    char images[BOREDOS_APP_METADATA_MAX_IMAGES][BOREDOS_APP_METADATA_MAX_IMAGE_PATH];
} boredos_app_metadata_t;

#ifdef __cplusplus
extern "C" {
#endif

// Internal assembly entry into Ring 0
extern uint64_t syscall0(uint64_t sys_num);
extern uint64_t syscall1(uint64_t sys_num, uint64_t arg1);
extern uint64_t syscall2(uint64_t sys_num, uint64_t arg1, uint64_t arg2);
extern uint64_t syscall3(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);
extern uint64_t syscall4(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
extern uint64_t syscall5(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
extern uint64_t syscall6(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// Public API
void sys_exit(int status);
int sys_read(int fd, void *buf, uint32_t len);
int sys_write(int fd, const void *buf, uint32_t len);
int sys_write_fs(int fd, const void *buf, uint32_t len);
int sys_open(const char *path, const char *mode);
void sys_close(int fd);
int sys_seek(int fd, int offset, int whence);
uint32_t sys_tell(int fd);
uint32_t sys_size(int fd);
int sys_delete(const char *path);
int sys_mkdir(const char *path);
int sys_exists(const char *path);
int sys_getcwd(char *buf, int size);
int sys_chdir(const char *path);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_pipe(int pipefd[2]);
int sys_fcntl(int fd, int cmd, int val);
void *sys_sbrk(int incr);

int sys_tty_create(void);
int sys_tty_read_out(int tty_id, char *buf, int len);
int sys_tty_write_in(int tty_id, const char *buf, int len);
int sys_tty_read_in(char *buf, int len);
int sys_tty_destroy(int tty_id);
int sys_tty_set_fg(int tty_id, int pid);
int sys_tty_get_fg(int tty_id);
int sys_tty_kill_fg(int tty_id);
int sys_tty_kill_all(int tty_id);
int sys_tty_get_id(void);
int sys_getpid(void);

int sys_spawn(const char *path, const char *args, uint64_t flags, uint64_t tty_id);
int sys_exec(const char *path, const char *args);
int sys_waitpid(int pid, int *status, int options);
int sys_kill_signal(int pid, int sig);
int sys_sigaction(int sig, const void *act, void *oldact);
int sys_sigprocmask(int how, const unsigned long *set, unsigned long *oldset);
int sys_sigpending(unsigned long *set);

int sys_poll(void *fds, int nfds, int timeout);

typedef struct {
    char name[256];
    uint32_t size;
    uint8_t is_directory;
    uint32_t start_cluster;
    uint16_t write_date;
    uint16_t write_time;
} FAT32_FileInfo;

int sys_list(const char *path, FAT32_FileInfo *entries, int max_entries);
int sys_list_offset(const char *path, FAT32_FileInfo *entries, int max_entries, int offset);
int sys_get_file_info(const char *path, FAT32_FileInfo *info);

typedef struct {
    uint32_t pid;
    char name[64];
    uint64_t ticks;
    size_t used_memory;
    uint32_t is_idle;
} ProcessInfo;

// Network API
typedef struct { uint8_t bytes[6]; } net_mac_address_t;
#ifndef NET_IPV4_ADDRESS_T_DEFINED
#define NET_IPV4_ADDRESS_T_DEFINED
typedef struct { uint8_t bytes[4]; } net_ipv4_address_t;
#endif

int sys_reboot(void);
int sys_shutdown(void);

int  sched_yield(void);
int  get_ticks(void);
int  rtc_get(int *dt);
int  rtc_set(int *dt);
void set_text_color(uint32_t color);

// Network helpers
int dns_lookup(const char *name, net_ipv4_address_t *out_ip);
int icmp_ping(const net_ipv4_address_t *dest_ip);


// FS API
typedef struct {
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint64_t block_size;
} vfs_statfs_t;

typedef struct {
    char path[256];
    char device[32];
    char fs_type[16];
} mount_info_t;

int sys_fs_statfs(const char *path, vfs_statfs_t *stat);
int sys_fs_mount_count(void);
int sys_fs_mount_info(int index, mount_info_t *info);

// Disk Management Syscalls
typedef struct {
    char     devname[16];
    char     label[32];
    uint32_t type;           
    uint32_t total_sectors;
    bool     is_partition;
    bool     is_fat32;
    bool     is_esp;
    uint32_t lba_offset;
} disk_info_t;

typedef struct {
    uint32_t lba_start;
    uint32_t sector_count;
    uint8_t  part_type;
    uint8_t  flags;
    char     label[36];
} partition_spec_t;

#define PART_FLAG_ESP       0x01
#define MIN_INSTALL_SECTORS 2097152  

int sys_disk_get_count(void);
int sys_disk_get_info(int index, disk_info_t *out);
int sys_disk_write_gpt(const char *devname, partition_spec_t *parts, int count);
int sys_disk_write_mbr(const char *devname, partition_spec_t *parts, int count);
int sys_disk_mkfs_fat32(const char *devname, const char *label);
int sys_disk_mount(const char *devname, const char *mountpoint);
int sys_disk_umount(const char *mountpoint);
int sys_disk_sync(const char *mountpoint);
int sys_disk_rescan(const char *devname);

#ifdef __cplusplus
}
#endif

#endif
