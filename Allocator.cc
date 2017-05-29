
/** \file Allocator.cc
 * \brief 
 * \todo Limit quantity of pools of a specific size
 * \todo stats
 */

#include <sys/user.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <execinfo.h>

#include <exception>
#include <new>

#include <syslog.h>
#include <pthread.h>
#include <tcl.h>

#include "Allocator.h"

namespace {
    int debug = 0;

    void
    print_stack() {
        void *pointers[256];

        int frame_count = backtrace( pointers, sizeof(pointers) );
        char **frames = backtrace_symbols( pointers, frame_count );
        for ( int i = 0 ; i < frame_count ; ++i ) {
            fprintf( stderr, "frame(%03d): %s\n", i, frames[i] );
        }
    }

}

class MemPoolException {
public:
    MemPoolException() {
        print_stack();
    }
};

class invalid_object : public MemPoolException { };
class double_free : public MemPoolException { };

/*
 */
class MemPool {
    static const int count = 512;
    static const int maps = count/32;
    size_t size;
    MemPool *next;
    uint8_t *start;
    uint32_t map[maps];
    void initialize( size_t );
public:
    MemPool() : size(0), next(NULL), start(NULL) {}
    ~MemPool() {}
    void *allocate( size_t );
    void free( void * );

    void usage( Tcl_Interp *, Tcl_Obj * );

    static void* operator new (size_t size); 
    static void operator delete (void *p);

    size_t get_size() { return size; }
    int get_count() { return count; }
    uint32_t available_slots();
    MemPool *next_pool() { return next; }
};

/**
 */
void *
MemPool::operator new ( size_t size ) {
    void *address = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0 );
    if ( address == MAP_FAILED ) {
        fprintf( stderr, "new operator failed to allocate %ld bytes\n", size );
        print_stack();
        throw std::bad_alloc();
    }
    return address;
}

/**
 */
void
MemPool::operator delete ( void *object ) {
    munmap( object, sizeof(MemPool) );
}

static const uint32_t m1  = 0x55555555;
static const uint32_t m2  = 0x33333333;
static const uint32_t m4  = 0x0F0F0F0F;
static const uint32_t m8  = 0x00FF00FF;
static const uint32_t m16 = 0x0000FFFF;

/**
 */
uint32_t
MemPool::available_slots() {
    uint32_t sum = 0;
    for ( int word = 0 ; word < maps ; word++ ) {
        uint32_t hw = map[word];
        hw -= (hw >> 1 ) & m1;
        hw = (hw & m2) + ((hw >> 2) & m2);
        hw = (hw + (hw >> 4)) & m4;
        hw += hw >> 8;
        hw += hw >> 16;
        hw &= 0x7f;
        sum += hw;
    }
    return sum;
}

/**
 */
void
MemPool::initialize( size_t object_size ) {
    size = object_size;
    size_t zone = size * count;

    fprintf( stderr, "Allocate %lu KB memory region for %lu byte objects\n", zone/1024, size );
    start = (uint8_t *)mmap( 0, zone, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0 );

    if ( start == MAP_FAILED ) {
        fprintf( stderr, "MemPool failed to allocate memory region for %lu byte objects\n", size );
        print_stack();
        throw std::bad_alloc();
    }

    for ( int i = 0 ; i < maps ; i++ ) {
        map[i] = 0xFFFFFFFF;
    }

    fprintf( stderr, "Creating new mempool\n" );
    next = new MemPool;
}

/**
 */
void
MemPool::usage( Tcl_Interp *interp, Tcl_Obj *result ) {
    if ( size == 0 ) return;

    Tcl_Obj *element = Tcl_NewListObj( 0, 0 );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("size", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(size) );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("capacity", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(count * size) );

    uint32_t sum = 0;
    for ( int word = 0 ; word < maps ; word++ ) {
        uint32_t hw = map[word];
        hw -= (hw >> 1 ) & m1;
        hw = (hw & m2) + ((hw >> 2) & m2);
        hw = (hw + (hw >> 4)) & m4;
        hw += hw >> 8;
        hw += hw >> 16;
        hw &= 0x7f;
        sum += hw;
    }

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("available", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(sum * size) );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("objects", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(count - sum) );

    Tcl_ListObjAppendElement( interp, result, element );
    next->usage( interp, result );
}

/**
 */
void *
MemPool::allocate( size_t object_size ) {
    if ( size == 0 )  initialize( object_size );

    if ( object_size != size ) {
        return next->allocate( object_size );
    }

    for ( int word = 0 ; word < maps ; word++ ) {
        if ( map[word] == 0 ) {
            continue;
        }

        for ( int bit = 0 ; bit < 32 ; bit++ ) {
            uint32_t mask = 1 << bit;
            if ( (map[word] & mask) == 0 ) {
                continue;
            }

            int entry = ((word * 32) + bit);
            fprintf( stderr, "allocate entry %d from %lu size table\n", entry, size );
            void *address = start + (entry * size);

            map[word] &= ~mask;
            return address;
        }
    }
    fprintf( stderr, "this table is full, looking for another\n" );

    return next->allocate( object_size );
}

/**
 */
void
MemPool::free( void *object ) {
    uint32_t length = count * size;
    uint8_t *end = (start + length) - 1;

    if ( (object < start) || (object > end) ) {
        if ( next == 0 )  throw invalid_object();
        next->free( object );
        return;
    }

    uint8_t *address = (uint8_t*)object;
    uint32_t offset = address - start;
    int entry = offset / size;
    int word  = entry / 32;
    int bit   = entry % 32;
    uint32_t mask = (1<<bit);

    if ( map[word] & mask ) {
        fprintf( stderr, "MemPool: map[%d] is 0x%08x\n", word, map[word] );
        fprintf( stderr, "MemPool: object 0x%p already freed\n", object );
        throw double_free();
    }

    fprintf( stderr, "free entry %d from %lu size table\n", entry, size );
    map[word] |= mask;
}

/** \brief the static primary MemPool object used by the
 * global allocators.
 */
MemPool heap;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 */
void* operator new (size_t size) throw(std::bad_alloc) {
    pthread_mutex_lock( &mutex );
    void *address = heap.allocate( size );
    pthread_mutex_unlock( &mutex );
    return address;
}

/**
 */
void operator delete ( void *address ) throw() {
    pthread_mutex_lock( &mutex );
    heap.free( address );
    pthread_mutex_unlock( &mutex );
}

/**
 */
static int
stats_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 1 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "" );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
usage_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 1 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "" );
        return TCL_ERROR;
    }

    Tcl_Obj *result = Tcl_NewListObj( 0, 0 );
    heap.usage( interp, result );

    Tcl_SetObjResult( interp, result );
    return TCL_OK;
}

/**
 */
void
Allocator::inject( Allocator::Injector *injector ) {
    Allocator::Injector &f = *injector;
    MemPool *pool = &heap;

    while ( pool != NULL ) {
        if ( pool->get_size() == 0 ) return;
        f( pool->get_size(), pool->get_count(), pool-> available_slots() );
        pool = pool->next_pool();
    }
}

/* vim: set autoindent expandtab sw=4 : */
