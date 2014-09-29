
/** \file UUID.h
 * \brief 
 */

#ifndef _UUID_H_
#define _UUID_H_

#include <stdint.h>
#include <unistd.h>
#include <tcl.h>

/**
 * Without an arguement, create a new UUID.
 *
 * With an unsigned char* -- assume 16 bytes of raw uuid data to copy.
 * With a char* -- assume a string to be parsed.
 */
class UUID {
private:
    uint8_t data[16];
    char string[37];
    void parse();
    void format();
    uint8_t nybble( int );
public:
    UUID( UUID& );
    UUID();
    UUID( uint8_t * );
    UUID( char * );
    bool set( uint8_t * );
    bool set( char * );
    inline char *to_s() { return string; }
    inline uint8_t *raw() { return data; }
    bool operator == ( UUID& );
    bool operator != ( UUID& );
};

extern bool UUID_Initialize( Tcl_Interp * );

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
