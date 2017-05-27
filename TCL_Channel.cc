
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

/** \file TCL_Channel.cc
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
#include "tcl_util.h"

#include "util.h"
#include "Channel.h"
#include "Service.h"

#include "AppInit.h"

namespace {
    int debug = 0;
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
        Tcl_StaticSetResult( interp, "Channel" );
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

    Tcl_StaticSetResult( interp, "Unknown command for Channel object" );
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
bool Channel_Module( Tcl_Interp *interp ) {
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

app_init( Channel_Module );

/* vim: set autoindent expandtab sw=4 : */
