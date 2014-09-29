
/** \file Service.h
 * \brief 
 *
 */

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <time.h>
#include <tcl.h>
#include "Thread.h"

class Service;

/**
 * The Channel is a singleton for a service.  It is the
 * communication mechanism to the service.  The receive method
 * accepts requests from a client and populates a buffer with
 * the request.  The return value is the id of the client.
 * The response is sent to this id.
 */
class Channel {
private:
    Service *service;
    long id;
    int q;
public:
    Channel( Service * );
    void send( long, int, char * );
    long receive( char *, int );
};

/**
 * A channel client is passed the name of the service to which
 * we are connecting.  All requests sent from this object go
 * to this service.  The receive method is passed a buffer to
 * populate with the response, and return an int error value.
 * 0 means no error.
 */
class ChannelClient {
private:
    int q;
    char service[80];
public:
    ChannelClient( char * );
    void send( char * );
    int receive( char *, int );
    int receive( char *, int, time_t );
};

bool Channel_Initialize( Tcl_Interp * );

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
