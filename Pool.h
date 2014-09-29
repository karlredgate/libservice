
#ifndef _POOL_H_
#define _POOL_H_

#include <stdint.h>
#include <cstdlib>
#include "tcl.h"
#include "crc32.h"

namespace Service {

    class Pool {
        static const int count = 1024;
        static const int MAPS = count/32;
        int size;
        Pool *next;
        uint8_t *start;
        uint32_t map[MAPS];
        void initialize( uint32_t );
    public:
        Pool()
        : size(0), next(0), start(0) {}
        void *allocate( uint32_t );
        void free( void * );
    };

    bool Initialize( Tcl_Interp * );
}
#endif
