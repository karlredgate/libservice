
/** \file StringList.h
 * \brief 
 */

#ifndef _STRINGLIST_H_
#define _STRINGLIST_H_

#include <stdint.h>
#include <unistd.h>
#include <tcl.h>

/**
 */
class StringList {
private:
    int _count;
    const char **strings;
    const char *_end;
    void parse( const char *, int );
public:
    StringList( char * );
    ~StringList();
    const int count() const { return _count; }
    const char *end() const { return _end; }
    const char * operator [] ( int index ) const { return strings[index]; }
};

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
