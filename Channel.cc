
/** \file Channel.cc
 * \brief 
 *
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

#include <tcl.h>
#include "TCL_Fixup.h"
#include <valgrind/memcheck.h>

#include "util.h"
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
    if ( msgsnd(q, &m, bytes + 1 + 8, flags) < 0 ) {
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
    if ( msgsnd(q, &m, bytes + 1 + 8, flags) < 0 ) {
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

        /**
         * We only want this error correction when we are not running under
         * valgrind, since valgrind changes the name of the program that is
         * running and all messaging would fail since we can not detect process
         * liveness after the program name change.
         */
        if ( RUNNING_ON_VALGRIND == 0 ) {
            if ( is_alive(service) == false ) return MESSAGE_EXCEPTION;
        }
        nanosleep( &delay, NULL );
    } while ( ++elapsed < time_limit );

    syslog( LOG_ERR, "ERROR channel recv timed out" );
    return MESSAGE_EXCEPTION;
}

/**
 */
static int
Channel_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    ChannelClient *channel = (ChannelClient *)data;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(channel)) );
        return TCL_OK;
    }

    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Svc_SetResult( interp, "Channel", TCL_STATIC );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "send") ) {
        if ( objc != 3 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 2, objv, "request" );
            return TCL_ERROR;
        }
        char *request = Tcl_GetStringFromObj( objv[2], NULL );
        channel->send( request );
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "receive") ) {
        if ( objc != 2 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 2, objv, "" );
            return TCL_ERROR;
        }
        char buffer[1024];
        int result = channel->receive( buffer, sizeof(buffer) );
        Tcl_SetObjResult( interp, Tcl_NewStringObj(buffer, -1) );
        return result;
    }

    if ( Tcl_StringMatch(command, "ask") ) {
        if ( objc != 3 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 2, objv, "request" );
            return TCL_ERROR;
        }
        char *request = Tcl_GetStringFromObj( objv[2], NULL );
        channel->send( request );
        char buffer[1024];
        int result = channel->receive( buffer, sizeof(buffer) );
        Tcl_SetObjResult( interp, Tcl_NewStringObj(buffer, -1) );
        return result;
    }

    if ( Tcl_StringMatch(command, "tell") ) {
        if ( objc != 3 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 2, objv, "request" );
            return TCL_ERROR;
        }
        char *request = Tcl_GetStringFromObj( objv[2], NULL );
        if ( fork() != 0 ) {
            Tcl_ResetResult( interp );
            return TCL_OK;
        }
        setsid();
        openlog( "(background:channel)", LOG_PERROR, LOG_USER );
        syslog( LOG_NOTICE, "send '%s'", request );
        channel->send( request );
        char buffer[1024];
        int result = channel->receive( buffer, sizeof(buffer), 60 );
        if ( result != TCL_OK ) {
            syslog( LOG_ERR, "channel failed" );
        }
        _exit( 0 );
    }

    Svc_SetResult( interp, "Unknown command for Channel object", TCL_STATIC );
    return TCL_ERROR;
}

/**
 */
static void
Channel_delete( ClientData data ) {
    ChannelClient *channel = (ChannelClient *)data;
    delete channel;
}

/**
 * Gotta fix this ...
 */
static int
Channel_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "service" );
        return TCL_ERROR;
    }

    char *name = Tcl_GetStringFromObj( objv[1], NULL );
    // Should check if the service is really there
    ChannelClient *object = new ChannelClient( name );
    Tcl_CreateObjCommand( interp, name, Channel_obj, (ClientData)object, Channel_delete );
    Svc_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

/**
 */
bool Channel_Initialize( Tcl_Interp *interp ) {
    Tcl_Command command;

    Tcl_Namespace *ns = Tcl_CreateNamespace(interp, "Channel", (ClientData)0, NULL);
    if ( ns == NULL )  return false;

    if ( Tcl_LinkVar(interp, "Channel::debug", (char *)&debug, TCL_LINK_INT) != TCL_OK ) {
        syslog( LOG_ERR, "failed to link Channel::debug" );
        exit( 1 );
    }

    command = Tcl_CreateObjCommand(interp, "Channel", Channel_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        return false;
    }

    return true;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
