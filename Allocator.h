
/** \file Allocator.h
 * \brief 
 *
 */

#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

namespace Allocator {

    /**
     * The Injector object will be called once for each slab of allocated
     * data.  Each slab stores objects all of the same size.
     */
    class Injector {
    public:
        Injector() {}
        virtual ~Injector() {}
        virtual void operator () ( size_t objsize, int allocated, uint32_t available ) = 0;
    };

    void inject( Injector * );
}

#endif

/* vim: set autoindent expandtab sw=4 : */
