
/** \file Fiber.cc
 * \brief C++ class implementing a fiber abstraction.
 *
 * This is a wrapper around a pthread library implementation that also
 * connects to a TCL library for debugging/configuration, etc.
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <tcl.h>
#include "TCL_Fixup.h"
#include "Fiber.h"

static void
noop() {
    // do nothing - yield ??
}

/* I don't think I want this */
class MainFiber : public Fiber {
public:
    MainFiber() : Fiber(noop) {
    }
    virtual ~MainFiber() {}
    virtual void run() {}
    virtual bool start() { return false; }
};

void
finish_fiber() {
    // get scheduler for this thread?
    // get current context from scheduler
    // call destructor
}

// namespace { Tcl_Interp *interpreter = NULL; }
// Tcl_Interp *Fiber::global_interp() { return interpreter; }

int
fiber_cmd( ClientData data, Tcl_Interp *interp,
            int objc, Tcl_Obj * CONST *objv )
{
    Fiber *fiber = (Fiber *)data;
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command ..." );
        return TCL_ERROR;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
#if 0
    if ( Tcl_StringMatch(command, "pid") ) {
        Tcl_Obj *result = Tcl_NewIntObj( fiber->getpid() );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }
#endif
    if ( Tcl_StringMatch(command, "state") ) {
        const char *s = "UNKNOWN";
        switch ( fiber->state ) {
            case Fiber::STOPPED:  s = "STOPPED";
            case Fiber::RUNNABLE: s = "RUNNABLE";
            case Fiber::RUNNING:  s = "RUNNING";
            case Fiber::BLOCKED:  s = "BLOCKED";
        }
        Tcl_Obj *result = Tcl_NewStringObj( s, -1 );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }
    
    Svc_SetResult( interp, "Unknown command for fiber object", TCL_STATIC );
    return TCL_ERROR;
    
}

void register_fiber( Fiber *fiber ) {
    if ( fiber->fiber_name() == NULL )  return;
    char buffer[80];
    snprintf( buffer, sizeof(buffer), "Fiber::%s", fiber->fiber_name() );
    // Tcl_EvalEx( interpreter, "namespace eval Fiber {}", -1, TCL_EVAL_GLOBAL );
    // Tcl_CreateObjCommand( interpreter, buffer, fiber_cmd, (ClientData)fiber, NULL );
}

class FiberList {
    Fiber *fiber;
    FiberList *next;
public:
    FiberList() {
        fiber = new MainFiber();
        next = 0;
        register_fiber( fiber );
    }
    ~FiberList() {
        delete next;
        delete fiber;
    }
    FiberList( Fiber *fiber, FiberList *next )
    : fiber(fiber), next(next) {
        register_fiber( fiber );
    }
    void add( Fiber *that ) {
        FiberList *entry = new FiberList( that, next );
        next = entry;
    }
};
FiberList fibers;

#if 0
union SIGNAL *
receive( const SIGSELECT *segsel ) {
    Fiber *self = (Fiber *)pthread_getspecific(CurrentFiber);
    // cycle through SignalQueue looking for the first
    // that matches the SIGSELECTLIST
}
#endif

static void *boot( void *data ) {
    Fiber *fiber = (Fiber *)data;
    pthread_setspecific( CurrentFiber, fiber );
    pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
    fiber->running();
    fiber->run();
    return NULL;
}

bool Fiber::start() {
#if 0
    if ( pthread_create(&id, NULL, boot, this) ) {
        return false;
    }
#endif
    return true;
}

bool Fiber::stop() {
#if 0
    if ( pthread_cancel(id) != 0 ) {
        return false;
    }
#endif
    return true;
}

Fiber::Fiber( coroutine f ) {
    fiber_name( "foo" );
    fibers.add( this );
    getcontext( &context );
    context.uc_link = X;
    makecontext( &context, f );
}

/**
 * Since _fiber_name might be static - we cannot free it.
 * Fiber names do not change frequently enough to warrant
 * worrying about the leak.
 */
void Fiber::fiber_name( const char *_name ) {
    if ( _name == NULL )  return;
    // if ( _fiber_name != NULL ) free(_fiber_name);
    _fiber_name = strdup(_name);
}

#if 0
int
Fiber::TclCommand( ClientData data, Tcl_Interp *interp,
                    int objc, Tcl_Obj * CONST *objv )
{
    Fiber *fiber = (Fiber *)data;
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command ..." );
        return TCL_ERROR;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "start") ) {
        fiber->start();
        Tcl_ResetResult( interp );
        return TCL_OK;
    }
    if ( Tcl_StringMatch(command, "pid") ) {
        Tcl_Obj *result = Tcl_NewIntObj( fiber->getpid() );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }
    if ( Tcl_StringMatch(command, "status") ) {
        Tcl_Obj *result = Tcl_NewStringObj( fiber->status, -1 );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }
    
    Svc_SetResult( interp, "Unknown command for fiber object", TCL_STATIC );
    return TCL_ERROR;
    
}
#endif

Scheduler::Scheduler() {
}

Scheduler::~Scheduler() {
}

/**
 * What about yield() - how will execution flow back through the scheduler?
 */
void
Scheduler::yield() {
    // find another context
    ucontext_t *context;
    context->uc_link = &dispatcher;
    swapcontext( &dispatcher, context );
}

/**
 * Maybe scheduler is a thread - and run is the schedule loop
 */
void
Scheduler::schedule() {
    while ( true ) {
        yield();
    }
}

bool
Scheduler::start( coroutine f ) {
    Fiber *fiber = new Fiber( f );
    return false;
}

/* vim: set autoindent expandtab sw=4 : */
