#pragma once

#include <features.h>

#include <stdint.h>
#include <stddef.h>
#include <sys/dir.h>

#include <sys/types.h>
#include_next <unistd.h>

/**
 *  Integer value indicating version of this standard (C-language binding) to which the implementation
 *      conforms. For implementations conforming to POSIX.1-2017, the value shall be 200809L.
 */
#define _POSIX_VERSION                  (200809L)
/**
 *  Integer value indicating version of the Shell and Utilities volume of POSIX.1 to which the implementation
 *      conforms. For implementations conforming to POSIX.1-2017, the value shall be 200809L.
 *  For profile implementations that define _POSIX_SUBPROFILE (see Section 2.1.5.1) in <unistd.h>,
 *      _POSIX2_VERSION may be left undefined or be defined with the value −1 to indicate that the Shell and
 *      Utilities volume of POSIX.1 is not supported. In this case, a call to sysconf(_SC_2_VERSION) shall
 *      return either 200809L or −1 indicating that the Shell and Utilities volume of POSIX.1 is or is not,
 *      respectively, supported at runtime.
 */
#define _POSIX2_VERSION                 _POSIX_VERSION
/**
 *  Integer value indicating version of the X/Open Portability Guide to which the implementation conforms.
 *      The value shall be 700.
 */
#define _XOPEN_VERSION                  (700)

/**
 *  The implementation supports the Advisory Information option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either
 *  be −1 or 200809L
 */
#define _POSIX_ADVISORY_INFO            (-1)

/**
 *  The implementation supports asynchronous input and output. This symbol shall always be set to the
 *      value 200809L.
 */
#define _POSIX_ASYNCHRONOUS_IO          _POSIX_VERSION

/**
 *  The implementation supports asynchronous input and output. This symbol shall always be set to the
 *      value 200809L.
 */
// #define _POSIX_BARRIERS                 _POSIX_VERSION

/**
 *  The use of chown() and fchown() is restricted to a process with appropriate privileges, and to changing
 *      the group ID of a file only to the effective group ID of the process or to one of its supplementary
 *      group IDs. This symbol shall be defined with a value other than −1.
 */
#define _POSIX_CHOWN_RESTRICTED         (-1)

/**
 *  The implementation supports clock selection. This symbol shall always be set to the value 200809L.
 */
// REVIEW: defined in include_next<features.h>
// #define _POSIX_CLOCK_SELECTION          _POSIX_VERSION

/**
 *  The implementation supports the Process CPU-Time Clocks option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_CPUTIME                  (-1)

/**
 *  The implementation supports the File Synchronization option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_FSYNC                    (-1)

/**
 *  The implementation supports the IPv6 option. If this symbol is defined in <unistd.h>, it shall be defined
 *  to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
#define _POSIX_IPV6                     (-1)

/**
 *  The implementation supports job control. This symbol shall always be set to a value greater than zero
 */
#define _POSIX_JOB_CONTROL              (0)

/**
 *  The implementation supports memory mapped Files. This symbol shall always be set to the
 *      value 200809L.
 */
#define _POSIX_MAPPED_FILES             _POSIX_VERSION

/**
 *  The implementation supports the Process Memory Locking option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_MEMLOCK                  (-1)
#define _POSIX_MEMLOCK_RANGE            _POSIX_MEMLOCK

/**
 *  The implementation supports memory protection. This symbol shall always be set to the value 200809L.
 */
#define _POSIX_MEMORY_PROTECTION        _POSIX_VERSION

/**
 *  The implementation supports the Message Passing option. If this symbol is defined in <unistd.h>, it shall be
 *      defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
#define _POSIX_MESSAGE_PASSING          (-1)

/**
 *  The implementation supports the Monotonic Clock option. If this symbol is defined in <unistd.h>, it shall be
 *      defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
// REVIEW: defined in include_next<features.h>
// #define _POSIX_MONOTONIC_CLOCK          (-1)

/**
 *  Pathname components longer than {NAME_MAX} generate an error. This symbol shall be defined with a value other than −1.
 */
#define _POSIX_NO_TRUNC                 (-1)

/**
 *  The implementation supports the Prioritized Input and Output option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_PRIORITIZED_IO           (-1)

/**
 *  The implementation supports the Process Scheduling option. If this symbol is defined in <unistd.h>, it shall be
 *      defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either be −1 or 200809L.
 */
#define _POSIX_PRIORITY_SCHEDULING      (-1)

/**
 *  The implementation supports the Raw Sockets option. If this symbol is defined in <unistd.h>, it shall be
 *      defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either be −1 or 200809L.
 */
#define _POSIX_RAW_SOCKETS              _POSIX_VERSION

/**
 *  The implementation supports read-write locks. This symbol shall always be set to the value 200809L.
 */
// REVIEW: pthread/CMakeLists.txt defined this
// #define _POSIX_READER_WRITER_LOCKS      _POSIX_VERSION

/**
 *  The implementation supports realtime signals. This symbol shall always be set to the value 200809L.
 */
#define _POSIX_REALTIME_SIGNALS         _POSIX_VERSION

/**
 *  The implementation supports the Regular Expression Handling option. This symbol shall always be set to a
 *      value greater than zero.
 */
#define _POSIX_REGEXP                   _POSIX_VERSION

/**
 *  Each process has a saved set-user-ID and a saved set-group-ID. This symbol shall always be set to a value
 *      greater than zero.
 */
#define _POSIX_SAVED_IDS                _POSIX_VERSION

/**
 *  The implementation supports semaphores. This symbol shall always be set to the value 200809L.
 */
#define _POSIX_SEMAPHORES               _POSIX_VERSION

/**
 *  The implementation supports the Shared Memory Objects option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_SHARED_MEMORY_OBJECTS    (-1)

/**
 *  The implementation supports the POSIX shell. This symbol shall always be set to a value greater than zero.
 */
#define _POSIX_SHELL                    _POSIX_VERSION

/**
 *  The implementation supports the Spawn option. If this symbol is defined in <unistd.h>, it shall be defined
 *      to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either be −1 or 200809L.
 */
#define _POSIX_SPAWN                    (-1)

/**
 *  The implementation supports spin locks. This symbol shall always be set to the value 200809L.
 */
// TODO: _POSIX_SPIN_LOCKS: freertos implemented pthread_spinlock_t, need to introduce into sys/pthread.h
// #define _POSIX_SPIN_LOCKS               _POSIX_VERSION

/**
 *  The implementation supports the Process Sporadic Server option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_SPORADIC_SERVER          (-1)
#define _POSIX_THREAD_SPORADIC_SERVER   (-1)

/**
 *  The implementation supports the Synchronized Input and Output option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_SYNCHRONIZED_IO          _POSIX_VERSION

/**
 *  The implementation supports the Thread Stack Address Attribute option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_THREAD_ATTR_STACKADDR    _POSIX_VERSION
#define _POSIX_THREAD_ATTR_STACKSIZE    _POSIX_VERSION

/**
 *  The implementation supports the Thread CPU-Time Clocks option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_THREAD_CPUTIME           (-1)

/**
 *  The implementation supports the Non-Robust Mutex Priority Inheritance option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by syscon() shall either
 *      be −1 or 200809L.
 */
#define _POSIX_THREAD_PRIO_INHERIT      (-1)
#define _POSIX_THREAD_PRIO_PROTECT      _POSIX_THREAD_PRIO_INHERIT

/**
 *  The implementation supports the Thread Execution Scheduling option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
#define _POSIX_THREAD_PRIORITY_SCHEDULING   (-1)

/**
 *  The implementation supports the Thread Process-Shared Synchronization option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
#define _POSIX_THREAD_PROCESS_SHARED    (-1)

/**
 *  The implementation supports the Robust Mutex Priority Inheritance option. If this symbol is defined in <unistd.h>,
 *      it shall be defined to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */

#define _POSIX_THREAD_ROBUST_PRIO_INHERIT   (-1)
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT   _POSIX_THREAD_ROBUST_PRIO_INHERIT

/**
 *  The implementation supports thread-safe functions. This symbol shall always be set to the value 200809L.
 */
#define _POSIX_THREAD_SAFE_FUNCTIONS    _POSIX_VERSION

/**
 *  The implementation supports threads. This symbol shall always be set to the value 200809L.
 */
// REVIEW: defined in include_next<features.h>
// #define _POSIX_THREADS                  _POSIX_VERSION

/**
 *  The implementation supports timeouts. This symbol shall always be set to the value 200809L.
 */
// REVIEW: defined in include_next<features.h>
// #define _POSIX_TIMEOUTS                 _POSIX_VERSION

/**
 *  The implementation supports timers. This symbol shall always be set to the value 200809L.
 */
// REVIEW: defined in include_next<features.h>
// #define _POSIX_TIMERS                   _POSIX_VERSION

/**
 *  The implementation supports the Trace option. If this symbol is defined in <unistd.h>, it shall be defined
 *      to be −1, 0, or 200809L. The value of this symbol reported by sysconf() shall either be −1 or 200809L.
 */
#define _POSIX_TRACE                    (-1)
#define _POSIX_TRACE_EVENT_FILTER       _POSIX_TRACE
#define _POSIX_TRACE_INHERIT            _POSIX_TRACE
#define _POSIX_TRACE_LOG

// should be defined in stdio.h
#ifndef SEEK_SET
    #define SEEK_SET                    (0)
#endif
#ifndef SEEK_CUR
    #define SEEK_CUR                    (1)
#endif
#ifndef SEEK_END
    #define SEEK_END                    (2)
#endif

__BEGIN_DECLS

extern __attribute__((nothrow))
    unsigned int alarm(unsigned int seconds);
extern __attribute__((nothrow))
    useconds_t ualarm(useconds_t useconds, useconds_t interval);

extern __attribute__((nothrow, deprecated))
    int brk(void *addr);
extern __attribute__((nothrow, deprecated))
    void *sbrk(intptr_t incr);

extern __attribute__((nothrow))
    size_t confstr(int name, char *buf, size_t len);

extern __attribute__((nothrow))
    char *crypt(char const *key, char const *salt);
extern __attribute__((nothrow))
    void encrypt(char *block, int edflag); // void encrypt(char block[64], int edflag);

extern __attribute__((nothrow))
    char *ctermid(char *s);

extern __attribute__((nothrow, deprecated))
    char *cuserid(char *s);

extern __attribute__((nothrow))
    pid_t fork(void);

extern __attribute__((nothrow, deprecated))
    int getdtablesize(void);

extern __attribute__((nothrow))
    gid_t getegid(void);
extern __attribute__((nothrow))
    uid_t geteuid(void);
extern __attribute__((nothrow))
    gid_t getgid(void);
extern __attribute__((nothrow))
    int getgroups(int gidsetsize, gid_t grouplist[]);

extern __attribute__((nothrow))
    pid_t getpgid(pid_t pid);
extern __attribute__((nothrow))
    int setpgid(pid_t pid, pid_t pgid);

extern __attribute__((nothrow))
    pid_t getpgrp(void);
extern __attribute__((nothrow))
    pid_t setpgrp(void);

extern __attribute__((nothrow))
    pid_t tcgetpgrp(int fildes);
extern __attribute__((nothrow))
    int tcsetpgrp(int fildes, pid_t pgid_id);

extern __attribute__((nothrow))
    pid_t getpid(void);
extern __attribute__((nothrow))
    int setgid(gid_t gid);

extern __attribute__((nothrow))
    pid_t getppid(void);

extern __attribute__((nothrow))
    pid_t getsid(pid_t pid);
extern __attribute__((nothrow))
    pid_t setsid(void);

extern __attribute__((nothrow))
    uid_t getuid(void);
extern __attribute__((nothrow))
    int setuid(uid_t uid);

extern __attribute__((nothrow))
    long gethostid(void);

extern __attribute__((nothrow))
    int setregid(gid_t rgid, gid_t egid);

extern __attribute__((nothrow))
    int setreuid(uid_t ruid, uid_t euid);

extern __attribute__((nothrow))
    char *getlogin(void);
extern __attribute__((nothrow))
    int getlogin_r(char *name, size_t namesize);

extern char *optarg;
extern int optind, opterr, optopt;

extern __attribute__((nothrow))
    int getopt(int argc, char *const argv[], char const *optstring);

extern __attribute__((nothrow, deprecated))
    int getpagesize(void);

extern __attribute__((nothrow, deprecated))
    char *getpass(char const *prompt);

extern __attribute__((nothrow))
    int nice(int incr);

extern __attribute__((nothrow))
    void swab(void const *src, void *dest, ssize_t nbytes);

extern __attribute__((nothrow))
    pid_t tcgetpgrp(int fildes);

/**
 *  file system
*/
extern __attribute__((nothrow))
    int access(char const *path, int amode);

extern __attribute__((nothrow))
    int chdir(char const *path);
extern __attribute__((nothrow))
    int fchdir(int fildes);
extern __attribute__((nothrow, deprecated))
    int chroot(char const *path);

extern __attribute__((nothrow))
    int rmdir(char const *path);

extern __attribute__((nothrow))
    char *getcwd(char *buf, size_t size);
extern __attribute__((nothrow))
    char *getwd(char *path_name);

extern __attribute__((nothrow))
    int link(char const *path1, char const *path2);
extern __attribute__((nothrow))
    int symlink(char const *path1, char const *path2);
extern __attribute__((nothrow))
    int unlink(char const *path);

extern __attribute__((nothrow))
    int chown(char const *path, uid_t owner, gid_t group);
extern __attribute__((nothrow))
    int fchown(int fildes, uid_t owner, gid_t group);
extern __attribute__((nothrow))
    int lchown(char const *path, uid_t owner, gid_t group);

extern __attribute__((nothrow))
    long int pathconf(char const *path, int name);

extern __attribute__((nothrow))
    int readlink(char const *path, char *buf, size_t bufsize);

extern __attribute__((nothrow))
    long int sysconf(int name);

extern __attribute__((nothrow))
    long int fpathconf(int fildes, int name);
extern __attribute__((nothrow))
    long int pathconf(char const *path, int name);

extern __attribute__((nothrow))
    int ftruncate(int fildes, off_t length);
extern __attribute__((nothrow))
    int truncate(char const *path, off_t length);

/**
 *  file descriptors
*/
extern __attribute__((nothrow))
    int close(int fildes);

extern __attribute__((nothrow))
    off_t lseek(int fildes, off_t offset, int whence);

extern __attribute__((nothrow))
    ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset);
extern __attribute__((nothrow))
    ssize_t read(int fildes, void *buf, size_t nbyte);

extern __attribute__((nothrow))
    ssize_t pwrite(int fildes, void const *buf, size_t nbyte, off_t offset);
extern __attribute__((nothrow))
    ssize_t write(int fildes, void const *buf, size_t nbyte);

extern __attribute__((nothrow))
    int isatty(int fildes);

extern __attribute__((nothrow))
    int dup(int fildes);
extern __attribute__((nothrow))
    int dup2(int fildes, int fildes2);

extern __attribute__((nothrow))
    int lockf(int fildes, int function, off_t size);

extern __attribute__((nothrow))
    int pipe(int fildes[2]);

extern __attribute__((nothrow))
    int fdatasync(int fildes);
extern __attribute__((nothrow))
    int fsync(int fildes);
extern __attribute__((nothrow))
    void sync(void);

extern __attribute__((nothrow))
    char *ttyname(int fildes);
extern __attribute__((nothrow))
    int ttyname_r(int fildes, char *name, size_t namesize);

/**
 *  threads
*/
extern __attribute__((nothrow))
    int pause(void);

extern __attribute__((nothrow))
    unsigned int sleep(unsigned int seconds);
extern __attribute__((nothrow))
    int usleep(useconds_t useconds);

/**
 *  process
*/
extern __attribute__((nothrow))
    void _exit(int status);

extern __attribute__((nothrow))
    pid_t vfork(void);

extern char **environ;

extern __attribute__((nothrow))
    int execl(char const *path, char const *arg0, ... /*, (char *)0 */);
extern __attribute__((nothrow))
    int execv(char const *path, char *const argv[]);
extern __attribute__((nothrow))
    int execle(char const *path, char const *arg0, ... /*, (char *)0, char *const envp[]*/);
extern __attribute__((nothrow))
    int execve(char const *path, char *const argv[], char *const envp[]);
extern __attribute__((nothrow))
    int execlp(char const *file, char const *arg0, ... /*, (char *)0 */);
extern __attribute__((nothrow))
    int execvp(char const *file, char *const argv[]);

/**
 *  no stdandard function
*/
    /**
     *  msleep(): suspend execution for millisecond intervals
     */
extern __attribute__((nothrow))
    int msleep(uint32_t msec);

    /**
     *  tell(): get current read/write file offset
     */
extern __attribute__((nothrow))
    off_t tell(int fd);

    /**
     *  readbuf(): guarantee to read buf size, or error
     */
extern __attribute((nonnull, nothrow))
    ssize_t readbuf(int fd, void *buf, size_t bufsize);

    /**
     *  writebuf(): guarantee to write buf, or error
     */
extern __attribute((nonnull, nothrow))
    ssize_t writebuf(int fd, void const *buf, size_t count);

    /**
     *  readln(): read until linebreak
     */
extern __attribute((nonnull, nothrow))
    ssize_t readln(int fd, char *buf, size_t bufsize);

    /**
     *  writeln(): write with linebreak
     */
extern __attribute__((nothrow))
    ssize_t writeln(int fd, char const *buf, size_t count);

__END_DECLS
