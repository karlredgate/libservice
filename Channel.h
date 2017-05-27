
/*
 * Copyright (c) 2012 Karl N. Redgate
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

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

/* vim: set autoindent expandtab sw=4 : */
