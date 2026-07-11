#include <stdint.h>
#include <stdlib.h>
#include <mlibc/elf/startup.h>

extern "C" void __dlapi_enter(uintptr_t *);

extern char **environ;

static char *static_env[] = {
    (char *)"PATH=/bin:/",
    (char *)"HOME=/",
    (char *)"SHELL=/bin/bsh",
    (char *)"TERM=boredos",
    nullptr
};

extern "C" void
__mlibc_entry(uintptr_t *entry_stack, int (*main_fn)(int argc, char *argv[], char *env[])) {
    __dlapi_enter(entry_stack);

    if (!environ || !*environ) {
        environ = static_env;
    }

    auto result = main_fn(mlibc::entry_stack.argc, mlibc::entry_stack.argv, environ);
    exit(result);
}
