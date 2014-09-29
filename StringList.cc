
/** \file StringList.cc
 * \brief 
 */

#include <stdlib.h>
#include <tcl.h>
#include "StringList.h"

/**
 */
StringList::StringList( char *data ) : strings(0) {
    parse( data, 0 );
}

/**
 */
StringList::~StringList() {
    if ( strings != 0 )  free( strings );
}

/**
 * start with n=0, increment with each recursion
 * at end of recursion set the string pointer
 */
void StringList::parse( const char *data, int n ) {
    if ( *data == '\0' ) { // end of strings
        _count = n;
        if ( _count != 0 ) {
            strings = (const char **)malloc( sizeof(char*) * n );
            _end = data + 1;
        } else {
            strings = 0;
            _end = data + 2;
        }
        return;
    }
    const char *p = data;
    while ( *p != '\0' ) p++;
    parse( p+1, n+1 );
    strings[n] = data;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
