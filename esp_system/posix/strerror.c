#include <sys/errno.h>
#include "esp_err.h"

char *strerror(int err)
{
    // do not change order
    // TODO: use linkscript mapping table to lookup error messages
    if (err >= ESP_ERR_USER_BASE)
        return "esp user error";
    if (err >= ESP_ERR_MEMPROT_BASE)
        return "esp memory protect error";
    if (err >= ESP_ERR_HW_CRYPTO_BASE)
        return "esp crypto error";
    if (err >= ESP_ERR_FLASH_BASE)
        return "esp flash error";
    if (err >= ESP_ERR_MESH_BASE)
        return "esp mesh error";
    if (err >= ESP_ERR_WIFI_BASE)
        return "esp wifi error";

    switch (err)
    {
    default:
        if (err < ESP_ERR_BASE)
            return "Unknown error";
        else
            return "esp error";

/* esp error base */
    case ESP_ERR_INVALID_STATE:
        return "esp error invalid state";
    case ESP_ERR_INVALID_SIZE:
        return "esp error invalid size";
    case ESP_ERR_NOT_FOUND:
        return "esp error not found";
    case ESP_ERR_INVALID_RESPONSE:
        return "esp error invalid response";
    case ESP_ERR_INVALID_CRC:
        return "esp error invalid crc";
    case ESP_ERR_INVALID_VERSION:
        return "esp error invalid version";
    case ESP_ERR_INVALID_MAC:
        return "esp error invalid MAC";
    case ESP_ERR_NOT_FINISHED:
        return "esp error not finished";

/* linux errors */
    case EPERM:
        return "Operation not permitted";
    case ENOENT:
        return "No such file or directory";
    case ESRCH:
        return "No such process";
    case EINTR:
        return "Interrupted system call";
    case EIO:
        return "I/O error";
    case ENXIO:
        return "No such device or address";
    case E2BIG:
        return "Argument list too long";
    case ENOEXEC:
        return "Exec format error";
    case EBADF:
        return "Bad file descriptor.";
    case ECHILD:
        return "No child processes";
    case EAGAIN:
        return "Try again";
    case ENOMEM:
        return "Out of memory";
    case EACCES:
        return "Permission denied";
    case EFAULT:
        return "Bad address";
    case ENOTBLK:
        return "Block device required";
    case EBUSY:
        return "Device or resource busy";
    case EEXIST:
        return "File exists";
    case EXDEV:
        return "Cross-device link";
    case ENODEV:
        return "No such device";
    case ENOTDIR:
        return "Not a directory";
    case EISDIR:
        return "Is a directory";
    case EINVAL:
        return "Invalid argument.";
    case ENFILE:
        return "File table overflow";
    case EMFILE:
        return "Too many open files";
    case ENOTTY:
        return "Not a typewriter";
    case ETXTBSY:
        return "Text file busy";
    case EFBIG:
        return "File too large";
    case ENOSPC:
        return "No space left on device";
    case ESPIPE:
        return "Illegal seek";
    case EROFS:
        return "Read-only file system";
    case EMLINK:
        return "Too many links";
    case EPIPE:
        return "Broken pipe";
    case EDOM:
        return "Domain error";
    case ERANGE:
        return "Out of range";
    case EDEADLK:
        return "Resource deadlock would occur";
    case ENAMETOOLONG:
        return "Path name too long";
    case ENOLCK:
        return "No record locks available";
    case ENOSYS:
        return "Function not implemented";
    case ENOTEMPTY:
        return "Directory not empty";
    case ELOOP:
        return "Too many symbolic links encountered";
    case EILSEQ:
        return "Illegal byte sequence.";
    case ENOMSG:
        return "No message of desired type";
    case EIDRM:
        return "Identifier removed";
    case ECHRNG:
        return "Channel number out of range";
    case EL2NSYNC:
        return "Level 2 not synchronized";
    case EL3HLT:
        return "Level 3 halted";
    case EL3RST:
        return "Level 3 reset";
    case ELNRNG:
        return "Link number out of range";
    case EUNATCH:
        return "Protocol driver not attached";
    case ENOCSI:
        return "No CSI structure available";
    case EL2HLT:
        return "Level 2 halted";
    case EBADE:
        return "Invalid exchange";
    case EBADR:
        return "Invalid request descriptor";
    case EXFULL:
        return "Exchange full";
    case ENOANO:
        return "No anode";
    case EBADRQC:
        return "Invalid request code";
    case EBADSLT:
        return "Invalid slot";
    case EBFONT:
        return "Bad font file format";
    case ENOSTR:
        return "Device not a stream";
    case ENODATA:
        return "No data available";
    case ETIME:
        return "Timer expired";
    case ENOSR:
        return "Out of streams resources";
    case ENONET:
        return "Machine is not on the network";
    case ENOPKG:
        return "Package not installed";
    case EREMOTE:
        return "Object is remote";
    case ENOLINK:
        return "Link has been severed";
    case EADV:
        return "Advertise error";
    case ESRMNT:
        return "Srmount error";
    case ECOMM:
        return "Communication error on send";
    case EPROTO:
        return "Protocol error";
    case EMULTIHOP:
        return "Multihop attempted";
    case EDOTDOT:
        return "RFS specific error";
    case EBADMSG:
        return "Bad message";
    case EOVERFLOW:
        return "Value too large for defined data type";
    case ENOTUNIQ:
        return "Name not unique on network";
    case EBADFD:
        return "File descriptor in bad state";
    case EREMCHG:
        return "Remote address changed";
    case ELIBACC:
        return "Can not access a needed shared library";
    case ELIBBAD:
        return "Accessing a corrupted shared library";
    case ELIBSCN:
        return ".lib section in a.out corrupted";
    case ELIBMAX:
        return "Attempting to link in too many shared libraries";
    case ELIBEXEC:
        return "Cannot exec a shared library directly";
    case ERESTART:
        return "Interrupted system call should be restarted";
    case ESTRPIPE:
        return "Streams pipe error";
    case EUSERS:
        return "Too many users";
    case ENOTSOCK:
        return "Socket operation on non-socket";
    case EDESTADDRREQ:
        return "Destination address required";
    case EMSGSIZE:
        return "Message too long";
    case EPROTOTYPE:
        return "Protocol wrong type for socket";
    case ENOPROTOOPT:
        return "Protocol not available";
    case EPROTONOSUPPORT:
        return "Protocol not supported";
    case ESOCKTNOSUPPORT:
        return "Socket type not supported";
    case EOPNOTSUPP:
        return "Operation not supported on transport endpoint";
    case EPFNOSUPPORT:
        return "Protocol family not supported";
    case EAFNOSUPPORT:
        return "Address family not supported by protocol";
    case EADDRINUSE:
        return "Address already in use";
    case EADDRNOTAVAIL:
        return "Cannot assign requested address";
    case ENETDOWN:
        return "Network is down";
    case ENETUNREACH:
        return "Network is unreachable";
    case ENETRESET:
        return "Network dropped connection because of reset";
    case ECONNABORTED:
        return "Software caused connection abort";
    case ECONNRESET:
        return "Connection reset by peer";
    case ENOBUFS:
        return "No buffer space available";
    case EISCONN:
        return "Transport endpoint is already connected";
    case ENOTCONN:
        return "Transport endpoint is not connected";
    case ESHUTDOWN:
        return "Cannot send after transport endpoint shutdown";
    case ETOOMANYREFS:
        return "Too many references: cannot splice";
    case ETIMEDOUT:
        return "Connection timed out";
    case ECONNREFUSED:
        return "Connection refused";
    case EHOSTDOWN:
        return "Host is down";
    case EHOSTUNREACH:
        return "No route to host";
    case EALREADY:
        return "Operation already in progress";
    case EINPROGRESS:
        return "Operation now in progress";
    case ESTALE:
        return "Stale NFS file handle";
    case EUCLEAN:
        return "Structure needs cleaning";
    case ENOTNAM:
        return "Not a XENIX named type file";
    case ENAVAIL:
        return "No XENIX semaphores available";
    case EISNAM:
        return "Is a named type file";
    case EREMOTEIO:
        return "Remote I/O error";
    case EDQUOT:
        return "Quota exceeded";
    case ENOMEDIUM:
        return "No medium found";
    case EMEDIUMTYPE:
        return "Wrong medium type";
    case ECANCELED:
        return "Operation Canceled";
    case ENOKEY:
        return "Required key not available";
    case EKEYEXPIRED:
        return "Key has expired";
    case EKEYREVOKED:
        return "Key has been revoked";
    case EKEYREJECTED:
        return "Key was rejected by service";
    case EOWNERDEAD:
        return "Owner died";
    case ENOTRECOVERABLE:
        return "State not recoverable";
    case ERFKILL:
        return "Operation not possible due to RF-kill";
    case EHWPOISON:
        return "Memory page has hardware error";
    }
}

char const *esp_err_to_name(esp_err_t code)
    __attribute__((alias("strerror")));
