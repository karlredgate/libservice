
#ifndef _POOL_H_
#define _POOL_H_

#include <stdint.h>
#include <cstdlib>
#include "tcl.h"
#include "crc32.h"

namespace Service {
    class StringTable {
    private:
        class Storage {
        private:
            int size;
            Storage *next;
            char *dot;
            char d[1];
        public:
            Storage( Storage *next = 0 ) : next(next), dot(d) {}
            void * operator new ( size_t, size_t );
            // delete
            void grow();
            char * store( char * );
            char * data() const { return dot; }
            int remaining() {
                return (((char *)this) + size) - dot;
            }
            bool fits( int length ) { return remaining() > length; }
        } *storage;
        
        class Entry {
        public:
            char *s;
            uint16_t length;
            uint16_t count;
            Entry() {}
        } *table;
        
        int next;
        char *dot;
        int remaining;
        int size;
        void grow();
        bool equal( int, char * );
    public:
        StringTable( int size = 512 );
        int find( char * );
        int store( char * );
        void release( int );
        void release( char * );
        char * operator [] ( int );
    private:
        StringTable( StringTable& ) {}
    };

    bool Initialize( Tcl_Interp * );
}
#endif
