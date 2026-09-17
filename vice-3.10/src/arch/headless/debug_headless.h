#ifndef HEADLESS_DEBUG
#define HEADLESS_DEBUG

#include <stdlib.h>

#define NOT_IMPLEMENTED() \
    fprintf(stderr, \
            "%s:%d: error: function %s() is not implemented yet, exiting\n", \
            __FILE__, __LINE__, __func__); \
    exit(1)

#endif
