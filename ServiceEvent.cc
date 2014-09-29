
/** \file ServiceEvent.cc
 * \brief 
 *
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include <tcl.h>
#include "TCL_Fixup.h"

#include "ServiceEvent.h"

/**
 */
Service::Event::Event( char *name ) {
    event_name = strdup( name );
}

namespace {
    char *eventfile = "/var/run/service_events";
}

/**
 */
bool Service::Event::send() {
    int q;
    int bytes;
    key_t key;
    /* struct msgbuf *message = (struct msgbuf *)buffer; */
    struct event_message message;

    if ( argc < 2 ) {
        fprintf( stderr, "usage: genevent EventName [args]\n" );
        exit( 1 );
    }

    key = ftok(eventfile, 'S');
    if ( key == -1 ) {
        perror("Cannot create key");
        exit(errno);
    }
    q = msgget( key, 0 );
    if ( q == -1 ) {
        perror("Cannot open message queue");
        exit(errno);
    }

    message.mtype = 1; /* positive integer */;

    argv++;
    strcpy( message.event, *argv );
    argv++;
    while ( *argv ) {
        strcat( message.event, " " );
        strcat( message.event, *argv );
        argv++;
    }

    bytes = strlen( message.event ) + 1;
    if ( msgsnd( q, &message, bytes, 0 ) < 0 ) {
        perror("msgsnd"); 
        exit(errno);
    }

    {
        pid_t peer_service_pid;
        struct msqid_ds stats;
        if ( msgctl(q, IPC_STAT, &stats) < 0 ) {
            /* skip kill */
            exit( 0 );
        }
        peer_service_pid = stats.msg_lrpid;
        kill( peer_service_pid, SIGRTMIN );
    }

    return true;
}

/**
 */
Service::LinkUp::LinkUp( char *device ) : Service::Event("LinkUp") {
}

/**
 */
static int
LinkUp_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    using namespace Service;
    LinkUp *object = (LinkUp *)data;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(object)) );
        return TCL_OK;
    }

    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Tcl_SetResult( interp, "Service::LinkUp", TCL_STATIC );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "send") ) {
        Tcl_SetResult( interp, "send not implemented", TCL_STATIC );
        return TCL_ERROR;

        Tcl_ResetResult( interp );
        return TCL_OK;
    }
    Tcl_SetResult( interp, "Unknown command for LinkUp object", TCL_STATIC );
    return TCL_ERROR;
}

/**
 */
static void
LinkUp_delete( ClientData data ) {
    using namespace Service;
    LinkUp *message = (LinkUp *)data;
    delete message;
}

/**
 */
static int
LinkUp_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name" );
        return TCL_ERROR; 
    }

    char *name = Tcl_GetStringFromObj( objv[1], NULL );
    using namespace Service;
    LinkUp *object = new LinkUp();
    Tcl_CreateObjCommand( interp, name, LinkUp_obj, (ClientData)object, LinkUp_delete );
    Tcl_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

namespace { int debug = 0; }

/**
 */
bool Service::Initialize( Tcl_Interp *interp ) {
    Tcl_Command command;

    Tcl_Namespace *ns = Tcl_CreateNamespace(interp, "Service", (ClientData)0, NULL);
    if ( ns == NULL ) {
        return false;
    }
    if ( Tcl_LinkVar(interp, "Service::debug", (char *)&debug, TCL_LINK_INT) != TCL_OK ) {
        syslog( LOG_ERR, "failed to link Service::debug" );
        exit( 1 );
    }

    command = Tcl_CreateObjCommand(interp, "Service::LinkUp", LinkUp_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    return true;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
