
/** \file Allocator.cc
 * \brief 
 * \todo Limit quantity of pools of a specific size
 * \todo stats
 */

#include <sys/user.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <execinfo.h>

#include <exception>
#include <new>

#include <syslog.h>
#include <pthread.h>
#include <tcl.h>

#include "Allocator.h"
#include "AppInit.h"

namespace {
    int debug = 0;

    void
    print_stack() {
        void *pointers[256];

        int frame_count = backtrace( pointers, sizeof(pointers) );
        char **frames = backtrace_symbols( pointers, frame_count );
        for ( int i = 0 ; i < frame_count ; ++i ) {
            fprintf( stderr, "frame(%03d): %s\n", i, frames[i] );
        }
    }

}

class TclInjector : public Allocator::Injector {
    Tcl_Interp *interp;
    Tcl_Obj * result;
public:
    TclInjector( Tcl_Interp * );
    virtual ~TclInjector();
    virtual void operator () ( size_t, int, uint32_t );
    Tcl_Obj *get_result() { return result; }
};

TclInjector::TclInjector( Tcl_Interp *interp ) :
Allocator::Injector(), interp(interp) {
    result = Tcl_NewListObj( 0, 0 );
}

TclInjector::~TclInjector( ) {
    // The interpreter should free the objects
}

void
TclInjector::operator () ( size_t size, int count, uint32_t available_slots ) {
    Tcl_Obj *element = Tcl_NewListObj( 0, 0 );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("size", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(size) );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("capacity", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(count * size) );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("available", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(available_slots * size) );

    Tcl_ListObjAppendElement( interp, element, Tcl_NewStringObj("objects", -1) );
    Tcl_ListObjAppendElement( interp, element, Tcl_NewLongObj(count - available_slots) );

    Tcl_ListObjAppendElement( interp, result, element );
}

/**
 */
static int
stats_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 1 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "" );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
usage_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 1 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "" );
        return TCL_ERROR;
    }

    TclInjector *injector = new TclInjector( interp );
    Allocator::inject( injector );

    Tcl_SetObjResult( interp, injector->get_result() );
    return TCL_OK;
}

/**
 */
static bool
Allocator_Module( Tcl_Interp *interp ) {
    Tcl_Command command;

    Tcl_Namespace *ns = Tcl_CreateNamespace(interp, "Allocator", (ClientData)0, NULL);
    if ( ns == NULL ) {
        return false;
    }

    if ( Tcl_LinkVar(interp, "Allocator::debug", (char *)&debug, TCL_LINK_INT) != TCL_OK ) {
        syslog( LOG_ERR, "failed to link Allocator::debug" );
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Allocator::stats", stats_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Allocator::usage", usage_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        return false;
    }

    return true;
}

app_init( Allocator_Module );

/* vim: set autoindent expandtab sw=4 : */
