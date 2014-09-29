
/** \file Thread.cc
 * \brief C++ class implementing a thread abstraction.
 *
 * This is a wrapper around a pthread library implementation that also
 * connects to a TCL library for debugging/configuration, etc.
 */

#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <tcl.h>
#include "TCL_Fixup.h"
#include "Thread.h"

namespace { Tcl_Interp *interpreter = NULL; }
Tcl_Interp *Thread::global_interp() { return interpreter; }

int
thread_cmd( ClientData data, Tcl_Interp *interp,
            int objc, Tcl_Obj * CONST *objv )
{
    Thread *thread = (Thread *)data;
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command ..." );
        return TCL_ERROR;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
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
    
    Svc_SetResult( interp, "Unknown command for thread object", TCL_STATIC );
    return TCL_ERROR;
    
}

void register_thread( Thread *thread ) {
    if ( thread->thread_name() == NULL )  return;
    char buffer[80];
    snprintf( buffer, sizeof(buffer), "Thread::%s", thread->thread_name() );
    Tcl_EvalEx( interpreter, "namespace eval Thread {}", -1, TCL_EVAL_GLOBAL );
    Tcl_CreateObjCommand( interpreter, buffer, thread_cmd, (ClientData)thread, NULL );
}

/**
 * The interpreter created here is not useful until it has been initialized
 * by the main program.
 */
class MainThread : public Thread {
public:
    MainThread() : Thread("main") {
        char oldname[32];
        char newname[48];
        prctl( PR_GET_NAME, oldname );
        sprintf( newname, "%s.main", oldname );
        prctl( PR_SET_NAME, newname );
        id = pthread_self();
        pid = ::getpid();
    }
    virtual ~MainThread() {}
    virtual void run() { }
    virtual bool start() { return false; }
};

pthread_key_t CurrentThread;

class ThreadList {
    Thread *thread;
    ThreadList *next;
public:
    ThreadList() {
        interpreter = Tcl_CreateInterp();
        pthread_key_create( &CurrentThread, NULL );
        thread = new MainThread;
        next = 0;
        pthread_setspecific( CurrentThread, thread );
        register_thread( thread );
    }
    ~ThreadList() {
        delete next;
        delete thread;
    }
    ThreadList( Thread *thread, ThreadList *next )
    : thread(thread), next(next) {
        register_thread( thread );
    }
    void add( Thread *that ) {
        ThreadList *entry = new ThreadList( that, next );
        next = entry;
    }
};
ThreadList threads;

#if 0
union SIGNAL *
receive( const SIGSELECT *segsel ) {
    Thread *self = (Thread *)pthread_getspecific(CurrentThread);
    // cycle through SignalQueue looking for the first
    // that matches the SIGSELECTLIST
}
#endif

static void *boot( void *data ) {
    Thread *thread = (Thread *)data;
    pthread_setspecific( CurrentThread, thread );
    pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
    thread->setpid();
    thread->running();
    prctl( PR_SET_NAME, thread->thread_name() );
    thread->run();
    return NULL;
}

bool Thread::start() {
    if ( pthread_create(&id, NULL, boot, this) ) {
        return false;
    }
    return true;
}

bool Thread::stop() {
    if ( pthread_cancel(id) != 0 ) {
        return false;
    }
    return true;
}

Thread::Thread( const char *_name ) : _thread_name(NULL) {
    thread_name( _name );
    status = "stop ready";
    threads.add( this );
}

/**
 * Since _thread_name might be static - we cannot free it.
 * Thread names do not change frequently enough to warrant
 * worrying about the leak.
 */
void Thread::thread_name( const char *_name ) {
    if ( _name == NULL )  return;
    // if ( _thread_name != NULL ) free(_thread_name);
    _thread_name = strdup(_name);
}

int
Thread::TclCommand( ClientData data, Tcl_Interp *interp,
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
    
    Svc_SetResult( interp, "Unknown command for thread object", TCL_STATIC );
    return TCL_ERROR;
    
}

/*
 * vim:autoindent
 * vim:expandtab
 */
