#include <runtime.h>

extern char **environ;

// xxx - just a read only snapshot of the initial state
bag env_init()
{
    edb eb = (edb)create_edb(init, 0);
    uuid env = generate_uuid();
    for (char **x = environ; *x; x++) {
        unsigned char *y = *(unsigned char **)x;
        int len = cstring_length((char *)y);
        int j = 0;
        for (;(y[j] != '=') && (j < len); j++);
        edb_insert(eb, env, intern_string(y, j-1), intern_string(y + j + 1, len - j - 1), 0);
    }
    return (bag)eb;
}
