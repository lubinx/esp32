#include <rtos/nvm.h>
#include "sh/ucsh.h"

static bool NVM_enum_callback(struct NVM_attr const *attr, void const *data, void *arg)
{
    struct UCSH_env *env = (struct UCSH_env *)arg;

    if (NVM_TAG_KEY_ID != attr->key)
    {
        char *ch = (char *)&attr->key;
        UCSH_printf(env, "key: 0x%08X(%c%c%c%c)  size: %d\n", (unsigned)attr->key, ch[0], ch[1], ch[2], ch[3], attr->data_size);
    }
    else
        UCSH_printf(env, "tag name \"%s\"\n", (char *)data);

    return true;
}

__attribute__((weak))
int UCSH_nvm(struct UCSH_env *env)
{
    NVM_enum(NVM_enum_callback, env);
    writeln(env->fd, "", 0);

    return 0;
}
