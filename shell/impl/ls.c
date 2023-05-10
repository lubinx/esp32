#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "sh/ucsh.h"

__attribute__((weak))
int UCSH_ls(struct UCSH_env *env)
{
    static char const *_xlat_rwx[] =
    {
        "---",
        "--x",
        "-w-",
        "-wx",
        "r--",
        "r-x",
        "rw-",
        "rwx",
    };
    char const *pathname = ".";

    for (int i = 1; i < env->argc; i ++)
    {
        if (! CMD_param_isoptional(env->argv[i]))
        {
            pathname = env->argv[i];
            break;
        }
    }

    DIR *dir = opendir(pathname);

    if (dir)
    {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL)
        {
            /// https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxa500/lscmd.htm
            char fmt;

            if (S_ISBLK(ent->d_mode))
                fmt = 'b';
            else if (S_ISCHR(ent->d_mode))
                fmt = 'c';
            else if (S_ISDIR(ent->d_mode))
                fmt = 'd';
            else if (S_ISFIFO(ent->d_mode))
                fmt = 'p';
            else if (S_ISREG(ent->d_mode))
                fmt = '-';
            else if (S_ISLNK(ent->d_mode))
                fmt = 'l';
            else if (S_ISSOCK(ent->d_mode))
                fmt = 's';
            else
                fmt = '-';

            struct tm tv;
            localtime_r(&ent->d_modificaion_ts, &tv);

           UCSH_printf(env, "%c%s%s%s %d %s %s %8u %d/%02d/%02d %02d:%02d  %s\n",
                fmt,
                _xlat_rwx[(ent->d_mode >> 6) & 0x07],
                _xlat_rwx[(ent->d_mode >> 3) & 0x07],
                _xlat_rwx[ent->d_mode & 0x07],
                1, "root", "root",
                ent->d_size,
                1900 + tv.tm_year, tv.tm_mon + 1, tv.tm_mday, tv.tm_hour, tv.tm_min,
                ent->d_name
            );
        }

        return closedir(dir);
    }
    else
        return errno;
}
