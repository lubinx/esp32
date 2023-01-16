#pragma once

#include <features.h>
#include <sys/reent.h>

// NOTE: do not use esp-idf errno defines
/*
    bcuz its defines different IDs compare to linux
    most porting 3-party like lwip using linux error defines
*/
// #include_next <sys/errno.h>

#define errno                           (*__errno())
#define error_r(reent)                  (*_errno_r(reent))
#define __errno_r(reent)                (*_errno_r(reent))

typedef int esp_err_t;

#define ESP_OK                          0
#define ESP_ERR_BASE                    0x1000
    /* esp_err_t remap to std error */
    #define ESP_FAIL                    EFAULT
    #define ESP_ERR_NO_MEM              ENOMEM
    #define ESP_ERR_INVALID_ARG         EINVAL
    #define ESP_ERR_NOT_SUPPORTED       EOPNOTSUPP
    #define ESP_ERR_TIMEOUT             ETIMEDOUT
    /* esp errors */
    #define ESP_ERR_INVALID_STATE       0x1003      /*!< Invalid state */
    #define ESP_ERR_INVALID_SIZE        0x1004      /*!< Invalid size */
    #define ESP_ERR_NOT_FOUND           0x1005      /*!< Requested resource not found */
    #define ESP_ERR_INVALID_RESPONSE    0x1008      /*!< Received response was invalid */
    #define ESP_ERR_INVALID_CRC         0x1009      /*!< CRC or checksum was invalid */
    #define ESP_ERR_INVALID_VERSION     0x100A      /*!< Version was invalid */
    #define ESP_ERR_INVALID_MAC         0x100B      /*!< MAC address was invalid */
    #define ESP_ERR_NOT_FINISHED        0x100C      /*!< There are items remained to retrieve */

#define ESP_ERR_WIFI_BASE               0x3000      /*!< Starting number of WiFi error codes */
#define ESP_ERR_MESH_BASE               0x4000      /*!< Starting number of MESH error codes */
#define ESP_ERR_FLASH_BASE              0x6000      /*!< Starting number of flash error codes */
#define ESP_ERR_HW_CRYPTO_BASE          0xc000      /*!< Starting number of HW cryptography module error codes */
#define ESP_ERR_MEMPROT_BASE            0xd000      /*!< Starting number of Memory Protection API error codes */

#define ESP_ERR_USER_BASE               0xe000


__BEGIN_DECLS

extern __attribute__((nothrow, const))
    int *__errno(void);
static inline
    int *_errno_r(struct _reent *r)    { return &r->_errno; }

extern __attribute__((nothrow, pure))
    int __set_errno_r_neg(struct _reent *r, int err, char const *__function__);
#define __set_errno_neg(r, err)         __set_errno_r_neg(r, err, __func__)

extern __attribute__((nothrow, pure))
    void *__set_errno_r_nullptr(struct _reent *r, int err, char const *__function__);
#define __set_errno_nullptr(r, err)     __set_errno_r_nullptr(r, err, __func__)

__END_DECLS

/* asm-generic/errno-base.h */

#ifndef EPERM
    #define EPERM                       (1 /* Operation not permitted */)
#endif

#ifndef ENOENT
    #define ENOENT                      (2 /* No such file or directory */)
#endif

#ifndef ENOFILE
    #define ENOFILE                     ENOENT
#endif

#ifndef ESRCH
    #define ESRCH                       (3 /* No such process */)
#endif

#ifndef EINTR
    #define EINTR                       (4 /* Interrupted system call */)
#endif

#ifndef EIO
    #define EIO                         (5 /* I/O error */)
#endif

#ifndef ENXIO
    #define ENXIO                       (6 /* No such device or address */)
#endif

#ifndef E2BIG
    #define E2BIG                       (7 /* Argument list too long */)
#endif

#ifndef ENOEXEC
    #define ENOEXEC                     (8 /* Exec format error */)
#endif

#ifndef EBADF
    #define EBADF                       (9 /* Bad file descriptor. */)
#endif

#ifndef ECHILD
    #define ECHILD                      (10 /* No child processes */)
#endif

#ifndef EAGAIN
    #define EAGAIN                      (11 /* Try again */)
#endif

#ifndef EWOULDBLOCK
    #define EWOULDBLOCK                 EAGAIN   /* Operation would block */
#endif

#ifndef ENOMEM
    #define ENOMEM                      (12 /* Out of memory */)
#endif

#ifndef EACCES
    #define EACCES                      (13 /* Permission denied */)
#endif

#ifndef EFAULT
    #define EFAULT                      (14 /* Bad address */)
#endif

#ifndef ENOTBLK
    #define ENOTBLK                     (15 /* Block device required */)
#endif

#ifndef EBUSY
    #define EBUSY                       (16 /* Device or resource busy */)
#endif

#ifndef EEXIST
    #define EEXIST                      (17 /* File exists */)
#endif

#ifndef EXDEV
    #define EXDEV                       (18 /* Cross-device link */)
#endif

#ifndef ENODEV
    #define ENODEV                      (19 /* No such device */)
#endif

#ifndef ENOTDIR
    #define ENOTDIR                     (20 /* Not a directory */)
#endif

#ifndef EISDIR
    #define EISDIR                      (21 /* Is a directory */)
#endif

#ifndef EINVAL
    #define EINVAL                      (22 /* Invalid argument. */)
#endif

#ifndef ENFILE
    #define ENFILE                      (23 /* File table overflow */)
#endif

#ifndef EMFILE
    #define EMFILE                      (24 /* Too many open files */)
#endif

#ifndef ENOTTY
    #define ENOTTY                      (25 /* Not a typewriter */)
#endif

#ifndef ETXTBSY
    #define ETXTBSY                     (26 /* Text file busy */)
#endif

#ifndef EFBIG
    #define EFBIG                       (27 /* File too large */)
#endif

#ifndef ENOSPC
    #define ENOSPC                      (28 /* No space left on device */)
#endif

#ifndef ESPIPE
    #define ESPIPE                      (29 /* Illegal seek */)
#endif

#ifndef EROFS
    #define EROFS                       (30 /* Read-only file system */)
#endif

#ifndef EMLINK
    #define EMLINK                      (31 /* Too many links */)
#endif

#ifndef EPIPE
    #define EPIPE                       (32 /* Broken pipe */)
#endif

#ifndef EDOM
    /**
     * If a mathematical function suffers a domain error (an input
     * argument is outside the domain over which the mathematical
     * function is defined, e.g. log of a negative number) the integer
     * expression errno acquires the value of the macro EDOM.
     *
     * EDOM is also returned by ftell, fgetpos and fsetpos when they
     * fail.
     */
    #define EDOM                        (33 /* Domain error */)
#endif

#ifndef ERANGE
    /**
     * If a mathematical function suffers a range error (the result of
     * the function is too large or too small to be accurately
     * represented in the output floating-point format), the integer
     * expression errno acquires the value of the macro ERANGE.
     *
     * ERANGE is used by functions in math.h and complex.h, and also by
     * the strto* and wcsto* family of decimal-to-binary conversion
     * functions (both floating and integer) and by floating-point
     * conversions in scanf.
     */
    #define ERANGE                      (34 /* Out of range */)
#endif

/* asm-generic/errno.h */

#ifndef EDEADLK
    #define EDEADLK                     (35 /* Resource deadlock would occur */)
#endif

#ifndef ENAMETOOLONG
    #define ENAMETOOLONG                (36 /* Path name too long */)
#endif

#ifndef ENOLCK
    #define ENOLCK                      (37 /* No record locks available */)
#endif

#ifndef ENOSYS
    /**
     * This error code is special: arch syscall entry code will return
     * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
     * failures of syscalls that really do exist distinguishable from
     * failures due to attempts to use a nonexistent syscall, syscall
     * implementations should refrain from returning -ENOSYS.
     */
    #define ENOSYS                      (38 /* Function not implemented */)
#endif

#ifndef ENOTEMPTY
    #define ENOTEMPTY                   (39 /* Directory not empty */)
#endif

#ifndef ELOOP
    #define ELOOP                       (40 /* Too many symbolic links encountered */)
#endif

#ifndef EILSEQ
    #define EILSEQ                      (41 /* Illegal byte sequence. */)
#endif

#ifndef ENOMSG
    #define ENOMSG                      (42 /* No message of desired type */)
#endif

#ifndef EIDRM
    #define EIDRM                       (43 /* Identifier removed */)
#endif

#ifndef ECHRNG
    #define ECHRNG                      (44 /* Channel number out of range */)
#endif

#ifndef EL2NSYNC
    #define EL2NSYNC                    (45 /* Level 2 not synchronized */)
#endif

#ifndef EL3HLT
    #define EL3HLT                      (46 /* Level 3 halted */)
#endif

#ifndef EL3RST
    #define EL3RST                      (47 /* Level 3 reset */)
#endif

#ifndef ELNRNG
    #define ELNRNG                      (48 /* Link number out of range */)
#endif

#ifndef EUNATCH
    #define EUNATCH                     (49 /* Protocol driver not attached */)
#endif

#ifndef ENOCSI
    #define ENOCSI                      (50 /* No CSI structure available */)
#endif

#ifndef EL2HLT
    #define EL2HLT                      (51 /* Level 2 halted */)
#endif

#ifndef EBADE
    #define EBADE                       (52 /* Invalid exchange */)
#endif

#ifndef EBADR
    #define EBADR                       (53 /* Invalid request descriptor */)
#endif

#ifndef EXFULL
    #define EXFULL                      (54 /* Exchange full */)
#endif

#ifndef ENOANO
    #define ENOANO                      (55 /* No anode */)
#endif

#ifndef EBADRQC
    #define EBADRQC                     (56 /* Invalid request code */)
#endif

#ifndef EBADSLT
    #define EBADSLT                     (57 /* Invalid slot */)
#endif

#ifndef EDEADLOCK
    #define EDEADLOCK                   EDEADLK
#endif

#ifndef EBFONT
    #define EBFONT                      (59 /* Bad font file format */)
#endif

#ifndef ENOSTR
    #define ENOSTR                      (60 /* Device not a stream */)
#endif

#ifndef ENODATA
    #define ENODATA                     (61 /* No data available */)
#endif

#ifndef ETIME
    #define ETIME                       (62 /* Timer expired */)
#endif

#ifndef ENOSR
    #define ENOSR                       (63 /* Out of streams resources */)
#endif

#ifndef ENONET
    #define ENONET                      (64 /* Machine is not on the network */)
#endif

#ifndef ENOPKG
    #define ENOPKG                      (65 /* Package not installed */)
#endif

#ifndef EREMOTE
    #define EREMOTE                     (66 /* Object is remote */)
#endif

#ifndef ENOLINK
    #define ENOLINK                     (67 /* Link has been severed */)
#endif

#ifndef EADV
    #define EADV                        (68 /* Advertise error */)
#endif

#ifndef ESRMNT
    #define ESRMNT                      (69 /* Srmount error */)
#endif

#ifndef ECOMM
    #define ECOMM                       (70 /* Communication error on send */)
#endif

#ifndef EPROTO
    #define EPROTO                      (71 /* Protocol error */)
#endif

#ifndef EMULTIHOP
    #define EMULTIHOP                   (72 /* Multihop attempted */)
#endif

#ifndef EDOTDOT
    #define EDOTDOT                     (73 /* RFS specific error */)
#endif

#ifndef EBADMSG
    #define EBADMSG                     (74 /* Bad message */)
#endif

#ifndef EOVERFLOW
    #define EOVERFLOW                   (75 /* Value too large for defined data type */)
#endif

#ifndef ENOTUNIQ
    #define ENOTUNIQ                    (76 /* Name not unique on network */)
#endif

#ifndef EBADFD
    #define EBADFD                      (77 /* File descriptor in bad state */)
#endif

#ifndef EREMCHG
    #define EREMCHG                     (78 /* Remote address changed */)
#endif

#ifndef ELIBACC
    #define ELIBACC                     (79 /* Can not access a needed shared library */)
#endif

#ifndef ELIBBAD
    #define ELIBBAD                     (80 /* Accessing a corrupted shared library */)
#endif

#ifndef ELIBSCN
    #define ELIBSCN                     (81 /* .lib section in a.out corrupted */)
#endif

#ifndef ELIBMAX
    #define ELIBMAX                     (82 /* Attempting to link in too many shared libraries */)
#endif

#ifndef ELIBEXEC
    #define ELIBEXEC                    (83 /* Cannot exec a shared library directly */)
#endif

#ifndef ERESTART
    #define ERESTART                    (85 /* Interrupted system call should be restarted */)
#endif

#ifndef ESTRPIPE
    #define ESTRPIPE                    (86 /* Streams pipe error */)
#endif

#ifndef EUSERS
    #define EUSERS                      (87 /* Too many users */)
#endif

#ifndef ENOTSOCK
    #define ENOTSOCK                    (88 /* Socket operation on non-socket */)
#endif

#ifndef EDESTADDRREQ
    #define EDESTADDRREQ                (89 /* Destination address required */)
#endif

#ifndef EMSGSIZE
    #define EMSGSIZE                    (90 /* Message too long */)
#endif

#ifndef EPROTOTYPE
    #define EPROTOTYPE                  (91 /* Protocol wrong type for socket */)
#endif

#ifndef ENOPROTOOPT
    #define ENOPROTOOPT                 (92 /* Protocol not available */)
#endif

#ifndef EPROTONOSUPPORT
    #define EPROTONOSUPPORT             (93 /* Protocol not supported */)
#endif

#ifndef ESOCKTNOSUPPORT
    #define ESOCKTNOSUPPORT             (94 /* Socket type not supported */)
#endif

#ifndef EOPNOTSUPP
    #define EOPNOTSUPP                  (95 /* Operation not supported */)
#endif

#ifndef ENOTSUP
    #define ENOTSUP                     EOPNOTSUPP
#endif

#ifndef EPFNOSUPPORT
    #define EPFNOSUPPORT                (96 /* Protocol family not supported */)
#endif

#ifndef EAFNOSUPPORT
    #define EAFNOSUPPORT                (97 /* Address family not supported by protocol */)
#endif

#ifndef EADDRINUSE
    #define EADDRINUSE                  (98 /* Address already in use */)
#endif

#ifndef EADDRNOTAVAIL
    #define EADDRNOTAVAIL               (99 /* Cannot assign requested address */)
#endif

#ifndef ENETDOWN
    #define ENETDOWN                    (100 /* Network is down */)
#endif

#ifndef ENETUNREACH
    #define ENETUNREACH                 (101 /* Network is unreachable */)
#endif

#ifndef ENETRESET
    #define ENETRESET                   (102 /* Network dropped connection because of reset */)
#endif

#ifndef ECONNABORTED
    #define ECONNABORTED                (103 /* Software caused connection abort */)
#endif

#ifndef ECONNRESET
    #define ECONNRESET                  (104 /* Connection reset by peer */)
#endif

#ifndef ENOBUFS
    #define ENOBUFS                     (105 /* No buffer space available */)
#endif

#ifndef EISCONN
    #define EISCONN                     (106 /* Transport endpoint is already connected */)
#endif

#ifndef ENOTCONN
    #define ENOTCONN                    (107 /* Transport endpoint is not connected */)
#endif

#ifndef ESHUTDOWN
    #define ESHUTDOWN                   (108 /* Cannot send after transport endpoint shutdown */)
#endif

#ifndef ETOOMANYREFS
    #define ETOOMANYREFS                (109 /* Too many references: cannot splice */)
#endif

#ifndef ETIMEDOUT
    #define ETIMEDOUT                   (110 /* Connection timed out */)
#endif

#ifndef ECONNREFUSED
    #define ECONNREFUSED                (111 /* Connection refused */)
#endif

#ifndef EHOSTDOWN
    #define EHOSTDOWN                   (112 /* Host is down */)
#endif

#ifndef EHOSTUNREACH
    #define EHOSTUNREACH                (113 /* No route to host */)
#endif

#ifndef EALREADY
    #define EALREADY                    (114 /* Operation already in progress */)
#endif

#ifndef EINPROGRESS
    #define EINPROGRESS                 (115 /* Operation now in progress */)
#endif

#ifndef ESTALE
    #define ESTALE                      (116 /* Stale NFS file handle */)
#endif

#ifndef EUCLEAN
    #define EUCLEAN                     (117 /* Structure needs cleaning */)
#endif

#ifndef ENOTNAM
    #define ENOTNAM                     (118 /* Not a XENIX named type file */)
#endif

#ifndef ENAVAIL
    #define ENAVAIL                     (119 /* No XENIX semaphores available */)
#endif

#ifndef EISNAM
    #define EISNAM                      (120 /* Is a named type file */)
#endif

#ifndef EREMOTEIO
    #define EREMOTEIO                   (121 /* Remote I/O error */)
#endif

#ifndef EDQUOTEDQUOT
    #define EDQUOT                      (122 /* Quota exceeded */)
#endif

#ifndef ENOMEDIUM
    #define ENOMEDIUM                   (123 /* No medium found */)
#endif

#ifndef EMEDIUMTYPE
    #define EMEDIUMTYPE                 (124 /* Wrong medium type */)
#endif

#ifndef ECANCELED
    #define ECANCELED                   (125 /* Operation Canceled */)
#endif

#ifndef ENOKEY
    #define ENOKEY                      (126 /* Required key not available */)
#endif

#ifndef EKEYEXPIRED
    #define EKEYEXPIRED                 (127 /* Key has expired */)
#endif

#ifndef EKEYREVOKED
    #define EKEYREVOKED                 (128 /* Key has been revoked */)
#endif

#ifndef EKEYREJECTED
    #define EKEYREJECTED                (129 /* Key was rejected by service */)
#endif

/* for robust mutexes */

#ifndef EOWNERDEAD
    #define EOWNERDEAD                  (130 /* Owner died */)
#endif

#ifndef ENOTRECOVERABLE
    #define ENOTRECOVERABLE             (131 /* State not recoverable */)
#endif

#ifndef ERFKILL
    #define ERFKILL                     (132 /* Operation not possible due to RF-kill */)
#endif

#ifndef EHWPOISON
    #define EHWPOISON                    (133 /* Memory page has hardware error */)
#endif
