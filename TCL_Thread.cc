
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

/** \file TCL_Thread.cc
 * \brief TCL wrappers around thread classes
 *
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <tcl.h>
#include "tcl_util.h"
#include "Thread.h"
#include "PlatformThread.h"
#include "AppInit.h"

namespace { Tcl_Interp *interpreter = NULL; }
Tcl_Interp *Thread::global_interp() { return interpreter; }

/**
 */
class RegisterThreadWithTcl : public ThreadCallback {
    Tcl_Interp *interpreter;
public:
    RegisterThreadWithTcl( Tcl_Interp * );
    ~RegisterThreadWithTcl();
    virtual bool operator () ( Thread * );
};

/**
 */
RegisterThreadWithTcl::RegisterThreadWithTcl( Tcl_Interp *interpreter )
: interpreter(interpreter)
{
}

/**
 */
RegisterThreadWithTcl::~RegisterThreadWithTcl() {
}

static int
Thread_obj( ClientData data, Tcl_Interp *interp,
            int objc, Tcl_Obj * CONST *objv )
{
    Thread *thread = (Thread *)data;
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command ..." );
        return TCL_ERROR;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );

    if ( Tcl_StringMatch(command, "start") ) {
        thread->start();
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "pid") ) {
        Tcl_Obj *result = Tcl_NewIntObj( thread->getpid() );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "status") ) {
        Tcl_Obj *result = Tcl_NewStringObj( thread->status, -1 );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }

    Tcl_StaticSetResult( interp, "Unknown command for thread object" );
    return TCL_ERROR;
}

/**
 */
bool
RegisterThreadWithTcl::operator () ( Thread *thread ) {
    if ( thread->thread_name() == NULL )  return false;
    char buffer[80];
    snprintf( buffer, sizeof(buffer), "Thread::%s", thread->thread_name() );
    Tcl_EvalEx( interpreter, "namespace eval Thread {}", -1, TCL_EVAL_GLOBAL );
    Tcl_CreateObjCommand( interpreter, buffer, Thread_obj, (ClientData)thread, NULL );
    return true;
}

static bool
Thread_Module( Tcl_Interp *interp ) {
    thread_create_hook = new RegisterThreadWithTcl( interp );
    return true;
}

app_init( Thread_Module );

/* vim: set autoindent expandtab sw=4 : */
