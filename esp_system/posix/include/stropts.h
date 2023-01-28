/****************************************************************************
  This file is part of UltraCore

  Copyright by UltraCreation Co Ltd 2018
-------------------------------------------------------------------------------
    The contents of this file are used with permission, subject to the Mozilla
  Public License Version 1.1 (the "License"); you may not use this file except
  in compliance with the License. You may  obtain a copy of the License at
  http://www.mozilla.org/MPL/MPL-1.1.html

    Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
  the specific language governing rights and limitations under the License.
****************************************************************************/
#ifndef __STR_OPTS_H
#define __STR_OPTS_H                    1

#include <features.h>
#include <sys/types.h>

/* **NON-STANDARD** request for ioctl */
    #define __OID                       ('O' << 8)
    #define OPT_RD_TIMEO                (__OID | 1)
    #define OPT_WR_TIMEO                (__OID | 2)

/* Macros used as `request' argument to `ioctl'. */
    #define __SID                       ('S' << 8)

    /* Counts the number of data bytes in the data block in the first message. */
    #define I_NREAD                     (__SID |  1)
    /* Push STREAMS module onto top of the current STREAM, just below the STREAM head. */
    #define I_PUSH                      (__SID |  2)
    /* Remove STREAMS module from just below the STREAM head. */
    #define I_POP                       (__SID |  3)
    /* Retrieve the name of the module just below the STREAM head and place it in a character string. */
    #define I_LOOK                      (__SID |  4)
    /* Flush all input and/or output. */
    #define I_FLUSH                     (__SID |  5)
    /* Sets the read mode. */
    #define I_SRDOPT                    (__SID |  6)
    /* Returns the current read mode setting. */
    #define I_GRDOPT                    (__SID |  7)
    /* Construct an internal STREAMS `ioctl' message and send that message downstream. */
    #define I_STR                       (__SID |  8)
    /* Inform the STREAM head that the process wants the SIGPOLL signal issued. */
    #define I_SETSIG                    (__SID |  9)
    /* Return the events for which the calling process is currently registered to be sent a SIGPOLL signal. */
    #define I_GETSIG                    (__SID | 10)
    /* Compares the names of all modules currently present in the STREAM to the name pointed to by `arg'. */
    #define I_FIND                      (__SID | 11)
    /* Connect two STREAMs. */
    #define I_LINK                      (__SID | 12)
    /* Disconnects the two STREAMs. */
    #define I_UNLINK                    (__SID | 13)
    /* Allows a process to retrieve the information in the first message on the STREAM head read queue
        without taking the message off the queue. */
    #define I_PEEK                      (__SID | 15)
    /* Create a message from the specified buffer(s), adds information about another STREAM, and send
        the message downstream. */
    #define I_FDINSERT                  (__SID | 16)
    /* Requests the STREAM associated with `fildes' to send a message, containing a file pointer, to the
        STREAM head at the other end of a STREAMS pipe. */
    #define I_SENDFD                    (__SID | 17)
    /* Non-EFT definition. */
    #define I_RECVFD                    (__SID | 14)
    /* Set the write mode. */
    #define I_SWROPT                    (__SID | 19)
    /* Return the current write mode setting. */
    #define I_GWROPT                    (__SID | 20)
    /* List all the module names on the STREAM, up to and including the topmost driver name. */
    #define I_LIST                      (__SID | 21)
    /* Connect two STREAMs with a persistent link. */
    #define I_PLINK                     (__SID | 22)
    /* Disconnect the two STREAMs that were connected with a persistent link. */
    #define I_PUNLINK                   (__SID | 23)
    /* Flush only band specified. */
    #define I_FLUSHBAND                 (__SID | 28)
    /* Check if the message of a given priority band exists on the STREAM head read queue. */
    #define I_CKBAND                    (__SID | 29)
    /* Return the priority band of the first message on the STREAM head read queue. */
    #define I_GETBAND                   (__SID | 30)
    /* See if the current message on the STREAM head read queue is "marked" by some module downstream. */
    #define I_ATMARK                    (__SID | 31)
    /* Set the time the STREAM head will delay when a STREAM is closing and there is data on the write queues. */
    #define I_SETCLTIME                 (__SID | 32)
    /* Get current value for closing timeout. */
    #define I_GETCLTIME                 (__SID | 33)
    /* Check if a certain band is writable. */
    #define I_CANPUT                    (__SID | 34)

/* Used in `I_LOOK' request. */
    /* compatibility w/UnixWare/Solaris. */
    #define FMNAMESZ                    8

/* Flush options. */
    /* Flush read queues. */
    #define FLUSHR                      0x01
    /* Flush write queues. */
    #define FLUSHW                      0x02
    /* Flush read and write queues. */
    #define FLUSHRW                     0x03
    /* Flush only specified band. */
    # define FLUSHBAND                  0x04

/* Possible arguments for `I_SETSIG'. */
    /* A message, other than a high-priority message, has arrived. */
    #define S_INPUT                     0x0001
    /* A high-priority message is present. */
    #define S_HIPRI                     0x0002
    /* The write queue for normal data is no longer full. */
    #define S_OUTPUT                    0x0004
    /* A STREAMS signal message that contains the SIGPOLL signal reaches the front of the STREAM head read queue. */
    #define S_MSG                       0x0008
    /* Notification of an error condition. */
    #define S_ERROR                     0x0010
    /* Notification of a hangup. */
    #define S_HANGUP                    0x0020
    /* A normal message has arrived. */
    #define S_RDNORM                    0x0040
    #define S_WRNORM                    S_OUTPUT
    /* A message with a non-zero priority has arrived. */
    #define S_RDBAND                    0x0080
    /* The write queue for a non-zero priority band is no longer full. */
    #define S_WRBAND                    0x0100
    /* When used in conjunction with S_RDBAND, SIGURG is generated instead of SIGPOLL when a priority
        message reaches the front of the STREAM head read queue. */
    #define S_BANDURG                   0x0200

/* Option for `I_PEEK'. */
    /* Only look for high-priority messages. */
    #define RS_HIPRI                    0x01

/* Options for `I_SRDOPT'. */
    /* Byte-STREAM mode, the default. */
    #define RNORM                       0x0000
    /* Message-discard mode. */
    #define RMSGD                       0x0001
    /* Message-nondiscard mode. */
    #define RMSGN                       0x0002
    /* Deliver the control part of a message as data. */
    #define RPROTDAT                    0x0004
    /* Discard the control part of a message, delivering any data part. */
    #define RPROTDIS                    0x0008
    /* Fail `read' with EBADMSG if a message containing a control part is at the front of the STREAM head read queue. */
    #define RPROTNORM                   0x0010
    /* The RPROT bits */
    #define RPROTMASK                   0x001C

/* Possible mode for `I_SWROPT'. */
    /* Send a zero-length message downstream when a `write' of 0 bytes occurs. */
    #define SNDZERO                     0x001
    /* Send SIGPIPE on write and putmsg if sd_werror is set. */
    #define SNDPIPE                     0x002

/* Arguments for `I_ATMARK'. */
    /* Check if the message is marked. */
    #define ANYMARK                     0x01
    /* Check if the message is the last one marked on the queue. */
    #define LASTMARK                    0x02

/* Argument for `I_UNLINK'. */
    /* Unlink all STREAMs linked to the STREAM associated with `fildes'. */
    #define MUXID_ALL                   (-1)

/* Macros for `getmsg', `getpmsg', `putmsg' and `putpmsg'. */
    /* Send/receive high priority message. */
    #define MSG_HIPRI                   0x01
    /* Receive any message. */
    #define MSG_ANY                     0x02
    /* Receive message from specified band. */
    #define MSG_BAND                    0x04

/* Values returned by getmsg and getpmsg */
    /* More control information is left in message. */
    #define MORECTL                     1
    /* More data is left in message. */
    #define MOREDATA                    2

    typedef int32_t                     t_scalar_t;
    typedef uint32_t                    t_uscalar_t;

    /* Structure used for the I_FLUSHBAND ioctl on streams. */
    struct bandinfo
    {
        unsigned char bi_pri;
        int bi_flag;
    };

    struct strbuf
    {
        int maxlen;                 /* Maximum buffer length. */
        int len;                    /* Length of data. */
        char *buf;                  /* Pointer to buffer. */
    };

    struct strpeek
    {
        struct strbuf ctlbuf;
        struct strbuf databuf;
        t_uscalar_t flags;          /* UnixWare/Solaris compatibility. */
    };

    struct strfdinsert
    {
        struct strbuf ctlbuf;
        struct strbuf databuf;
        t_uscalar_t flags;          /* UnixWare/Solaris compatibility. */
        int fildes;
        int offset;
    };

    struct strioctl
    {
        int ic_cmd;
        int ic_timout;
        int ic_len;
        char *ic_dp;
    };

    struct strrecvfd
    {
        int fd;
        uid_t uid;
        gid_t gid;
        char __fill[8];             /* UnixWare / Solaris compatibility */
    };

    struct str_mlist
    {
        char l_name[FMNAMESZ + 1];
    };

    struct str_list
    {
        int sl_nmods;
        struct str_mlist *sl_modlist;
    };

__BEGIN_DECLS

    /** Test whether FILDES is associated with a STREAM-based file.  */
extern __attribute__((nothrow))
    int isastream(int fd);

    /** Receive next message from a STREAMS file. */
extern __attribute__((nothrow))
    int getmsg(int fd, struct strbuf *restrict ctlptr, struct strbuf *restrict dataptr,
        int *restrict flagsptr);

    /** Receive next message from a STREAMS file, with *FLAGSP allowing to control which message */
extern __attribute__((nothrow))
    int getpmsg(int fd, struct strbuf *restrict ctlptr, struct strbuf *restrict dataptr,
        int *restrict __bandp, int *restrict flagsptr);

    /**
     *  Perform the I/O control operation specified by REQUEST on FD.
     *      One argument may follow; its presence and type depend on REQUEST.
     *      Return value depends on REQUEST.  Usually -1 indicates error.
     */
extern __attribute__((nothrow))
    int ioctl(int fd, unsigned long int request, ...);

    /** @Send a message on a STREAM. */
extern __attribute__((nothrow))
    int putmsg(int fd, struct strbuf const *ctlptr, struct strbuf const *dataptr, int flags);

    /** Send a message on a STREAM to the BAND.  */
extern __attribute__((nothrow))
    int putpmsg(int fd, struct strbuf const *ctlptr, struct strbuf const *dataptr, int __band, int flags);

    /** Attach a STREAMS-based file descriptor FILDES to a file PATH in the file system name space.  */
extern __attribute__((nothrow))
    int fattach(int fd, char const *path);

    /** Detach a name PATH from a STREAMS-based file descriptor.  */
extern __attribute__((nothrow))
    int fdetach(char const *path);

__END_DECLS
#endif
