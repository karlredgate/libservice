
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

/** \file Channel.cc
 * \brief 
 *
 * The problem with SYSV message queues for bidirectional messages, is that
 * if the message is read it is deleted.  If the process reading the message
 * fails in some manner, then the message is not processed and the sender
 * does not get a response and requires mitigation.
 *
 * Consider new method where the message is read and consumed as separate
 * steps.  Perhaps a message can be peeked with SYS MQ.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <glob.h>

#include "string_util.h"
#include "Channel.h"
#include "Service.h"

namespace { int debug = 0; }

/**
 * dst is the destination service id
 * src is the source service id
 */
struct channel_message {
    long dst;
    long src;
    long error;
    char body[1012];
};
#define MESSAGE_OK        0
#define MESSAGE_ERROR     1
#define MESSAGE_EXCEPTION 2

/** 
 * Look through list of processes to see if the named service is
 * alive.  We check by looking a process that has its executable
 * as /usr/sbin/[service_name].
 *
 * Note: readlink does not NULL terminate the strings it reads, so
 *   we must do it explicitly.
 */
static int
is_alive( char *service_name ) {
    int result = 0;
    glob_t processes;
    size_t i;
    char exe[256], path[80];

    sprintf( path, "/usr/sbin/%s", service_name );

    memset( &processes, 0, sizeof(processes) );
    glob( "/proc/*/exe", GLOB_NOSORT, NULL, &processes );

    for ( i = 0 ; i < processes.gl_pathc ; i++ ) {
        int count = readlink( processes.gl_pathv[i], exe, sizeof(exe) );
        if ( count == -1 ) continue;
        exe[count] = '\0';
        if ( strcmp(exe, path) == 0 ) {
            result = 1;
            break;
        }
    }

    globfree( &processes );
    return result;
}

/**
 */
key_t service_key( const char *service_name ) {
    key_t key = *( (key_t*)service_name );
    if ( key == -1 ) {
        syslog( LOG_ERR, "could not generate a key for '%s'", service_name );
        exit( 1 );
    }
    // send this to stdout if the tcl interpreter is interactive
    // syslog( LOG_NOTICE, "channel key for '%s' is 0x%08x", service_name, key );
    return key;
}

/**
 * will need a path name to point the mq at
 *
 * Channel needs to be a thread that reads the Q and
 * calls the interpreter when it gets the message
 *
 * The channel cannot accept messages until the interpreter is
 * fully initialized.  Also, the channel should be the primary
 * (only?) evaluator in the interpreter.
 *
 * Should make the Interpreter be a base class and have a 
 * TclInterpreter and a RubyInterpreter?  The Interpreter
 * should provide an evalute method ...
 *
 *
 * There will be a race here ... where the client of a service
 * will not be able to start until the server.  However, if they
 * talk to each other then they are both clients and servers
 * so neither could start.  So, the client needs to be able to
 * create the server's directory also.
 */
Channel::Channel( Service *service )
: service(service) {
    key_t key = service_key( service->name() );
    syslog( LOG_NOTICE, "msgQ id = 0x%08x", key );

    // Eventually change this so only root can send...
    q = msgget( key, IPC_CREAT | 0777 );
    if ( q < 0 ) {
        syslog( LOG_ERR, "could not create a msgQ for '%s'", service->name() );
        exit( 1 );
    }
}

/**
 * \todo should use IPC_NOWAIT -- and try again if EAGAIN
 * \todo fix message length calculation
 */
void
Channel::send( long dst, int result, char *message ) {
    int flags = 0; // IPC_NOWAIT | MSG_NOERROR
    struct channel_message m;
    m.dst = dst;
    m.src = 1;
    m.error = result;
    int bytes = strlcpy( m.body, message, sizeof(m.body) );
    if ( bytes > sizeof(m.body) ) {
        syslog( LOG_WARNING, "message body truncated" );
        bytes = sizeof(m.body) - 1;
    }
    if ( msgsnd(q, &m, bytes + sizeof(char) + sizeof(long) + sizeof(long), flags) < 0 ) {
        syslog( LOG_ERR, "failed to msgsnd" );
    }
}

/**
 * block ... read from Q and process msg
 * \todo add retry logic when we get a msgrcv err on Channel
 */
long
Channel::receive( char *buffer, int length ) {
    int flags = 0; // IPC_NOWAIT | MSG_NOERROR
    struct channel_message request;
    int bytes = msgrcv( q, &request, sizeof(request), 1, flags );
    if ( bytes < 0 ) {
        // how to handle error
        syslog( LOG_ERR, "Error receiving Channel request" );
    }
    if ( debug > 0 ) syslog( LOG_NOTICE, "request '%s'", request.body );
    strlcpy( buffer, request.body, length );
    return request.src;
}

/**
 */
ChannelClient::ChannelClient( char *service_name ) {
    strlcpy( service, service_name, sizeof(service) );
    key_t key = service_key( service_name );
    // Eventually change this so only root can send...
    q = msgget( key, IPC_CREAT | 0777 );
    if ( q < 0 ) {
        syslog( LOG_ERR, "could not create a msgQ for '%s'", service_name );
        exit( 1 );
    }
}

/**
 * \todo should use IPC_NOWAIT -- and try again if EAGAIN
 * \todo fix message length calculation
 */
void
ChannelClient::send( char *message ) {
    int flags = 0; // IPC_NOWAIT | MSG_NOERROR
    struct channel_message m;
    m.dst = 1;
    m.src = getpid();
    m.error = 0;
    int bytes = strlcpy( m.body, message, sizeof(m.body) );
    if ( bytes > sizeof(m.body) ) {
        syslog( LOG_WARNING, "message body truncated" );
        bytes = sizeof(m.body) - 1;
    }
    if ( msgsnd(q, &m, bytes + sizeof(char) + sizeof(long) + sizeof(long), flags) < 0 ) {
        syslog( LOG_ERR, "failed to msgsnd" );
    }
}

/**
 * block ... read from Q and process msg
 */
int
ChannelClient::receive( char *buffer, int length ) {
    int flags = 0; // IPC_NOWAIT | MSG_NOERROR
    struct channel_message request;

    int bytes = msgrcv( q, &request, sizeof(request), getpid(), flags );
    if ( bytes < 0 ) {
        // how to handle error -- try again
    }
    strlcpy( buffer, request.body, length );
    return request.error;
}

/**
 */
int
ChannelClient::receive( char *buffer, int length, time_t time_limit ) {
    struct channel_message request;
    time_t elapsed = 0;
    struct timespec delay = { 1, 0 };
    pid_t pid = getpid();

    do {
        int bytes = msgrcv( q, &request, sizeof(request), pid, IPC_NOWAIT );

        if ( bytes > 0 ) {
            strlcpy( buffer, request.body, length );
            return request.error;
        }

#if 0 // where is valgrind on OSX
        /**
         * We only want this error correction when we are not running under
         * valgrind, since valgrind changes the name of the program that is
         * running and all messaging would fail since we can not detect process
         * liveness after the program name change.
         */
        if ( RUNNING_ON_VALGRIND == 0 ) {
            if ( is_alive(service) == false ) return MESSAGE_EXCEPTION;
        }
#endif
        nanosleep( &delay, NULL );
    } while ( ++elapsed < time_limit );

    syslog( LOG_ERR, "ERROR channel recv timed out" );
    return MESSAGE_EXCEPTION;
}

/* vim: set autoindent expandtab sw=4 : */
