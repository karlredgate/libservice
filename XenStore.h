
/** \file XenStore.h
 * \brief 
 *
 */

#ifndef _XENSTORE_H_
#define _XENSTORE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>
#include <unistd.h>
#include <tcl.h>

#include <errno.h>
#include <xen/xen.h>
#include <xen/io/xs_wire.h>

/**
 */
namespace Xen {

    /**
     */
    class StorePath {
    public:
        char *path;
        StorePath *next;
        StorePath( char *, int );
    };

    /**
     */
    class Store {
    private:
        int _error;
        char error_string[256];
    public:
        Store();
        char* read( char * );
        StorePath* readdir( char * );
        bool write( char *, char * );
        bool mkdir( char * );
        bool remove( char * );
        inline char * operator [] ( char *key ) {
            return read(key);
        }
        const char *error_message() const { return error_string; }
        int error() const { return _error; }
    };

}

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
