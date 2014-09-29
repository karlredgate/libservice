
/** \file Xen.cc
 * \brief 
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>  // for mlock

#include <stdint.h>
#include <stdlib.h>

/**
 * Lie about being a Xen tool...
 */
#define __XEN_TOOLS__
#include <xen/xen.h>
#include <xen/sys/privcmd.h>
#include <xen/sysctl.h>
#include <xen/domctl.h>

#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <tcl.h>
#include "TCL_Fixup.h"

#include "Hypercall.h"
#include "XenStore.h"

/**
 */
static int
physinfo_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 1 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "" );
        return TCL_ERROR;
    }

    Xen::GetPhysInfo physinfo_request;
    Xen::PhysInfo *physinfo = physinfo_request();

    Tcl_Obj *list = Tcl_NewListObj( 0, 0 );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("sockets", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->sockets()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("cores", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->cores()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("threads", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->threads()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("pages", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->pages()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("nr_cpus", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->nr_cpus()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("nr_nodes", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->nr_nodes()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("max_cpu_id", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->max_cpu_id()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("max_node_id", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->max_node_id()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("cpu_khz", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->cpu_khz()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("free_pages", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->free_pages()) );

    Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj("scrub_pages", -1) );
    Tcl_ListObjAppendElement( interp, list, Tcl_NewLongObj(physinfo->scrub_pages()) );

    Tcl_SetObjResult( interp, list );
    return TCL_OK;
}

/**
 */
static int
PauseDomain_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    using namespace Xen;
    PauseDomain *hypercall = (PauseDomain *)data;
    PauseDomain& pause_domain = *hypercall;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(hypercall)) );
        return TCL_OK;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Svc_SetResult( interp, "Xen::PauseDomain", TCL_STATIC );
        return TCL_OK;
    }

    // this should be in the delete proc
    if ( Tcl_StringMatch(command, "delete") ) {
        delete hypercall;
        Tcl_DeleteCommand( interp, Tcl_GetStringFromObj(objv[0],NULL) );
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    /**
     * Argument is a message
     */
    if ( Tcl_StringMatch(command, "call") ) {
        if ( objc != 2 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 1, objv, "call" );
            return TCL_ERROR;
        }

        pause_domain();
        Tcl_ResetResult( interp );
        return TCL_OK;
    }
    Svc_SetResult( interp, "Unknown command for RouteSocket object", TCL_STATIC );
    return TCL_ERROR;
}

/**
 */
static int
PauseDomain_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name domid" );
        return TCL_ERROR;
    }

    domid_t domid;
    void *p = (void *)&(domid); // avoid strict-aliasing breakage in compiler
    if ( Tcl_GetLongFromObj(interp,objv[2],(long*)p) != TCL_OK ) {
        return TCL_ERROR;
    }

    int len;
    char *name = Tcl_GetStringFromObj( objv[1], &len );

    using namespace Xen;
    PauseDomain *obj = new PauseDomain( domid );

    // should have a delete proc also
    Tcl_CreateObjCommand( interp, name, PauseDomain_obj, (ClientData)obj, 0 );
    Svc_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

/**
 */
static int
UnpauseDomain_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    using namespace Xen;
    UnpauseDomain *hypercall = (UnpauseDomain *)data;
    UnpauseDomain& pause_domain = *hypercall;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(hypercall)) );
        return TCL_OK;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Svc_SetResult( interp, "Xen::UnpauseDomain", TCL_STATIC );
        return TCL_OK;
    }

    // this should be in the delete proc
    if ( Tcl_StringMatch(command, "delete") ) {
        delete hypercall;
        Tcl_DeleteCommand( interp, Tcl_GetStringFromObj(objv[0],NULL) );
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    /**
     * Argument is a message
     */
    if ( Tcl_StringMatch(command, "call") ) {
        if ( objc != 2 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 1, objv, "call" );
            return TCL_ERROR;
        }

        pause_domain();
        Tcl_ResetResult( interp );
        return TCL_OK;
    }
    Svc_SetResult( interp, "Unknown command for RouteSocket object", TCL_STATIC );
    return TCL_ERROR;
}

/**
 */
static int
UnpauseDomain_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name domid" );
        return TCL_ERROR;
    }

    domid_t domid;
    void *p = (void *)&(domid); // avoid strict-aliasing breakage in compiler
    if ( Tcl_GetLongFromObj(interp,objv[2],(long*)p) != TCL_OK ) {
        return TCL_ERROR;
    }

    int len;
    char *name = Tcl_GetStringFromObj( objv[1], &len );

    using namespace Xen;
    UnpauseDomain *obj = new UnpauseDomain( domid );

    // should have a delete proc also
    Tcl_CreateObjCommand( interp, name, UnpauseDomain_obj, (ClientData)obj, 0 );
    Svc_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

/**
 */
static int
DestroyDomain_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    using namespace Xen;
    DestroyDomain *hypercall = (DestroyDomain *)data;
    DestroyDomain& call = *hypercall;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(hypercall)) );
        return TCL_OK;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Svc_SetResult( interp, "Xen::DestroyDomain", TCL_STATIC );
        return TCL_OK;
    }

    /**
     * Argument is a message
     */
    if ( Tcl_StringMatch(command, "call") ) {
        if ( objc != 2 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 1, objv, "call" );
            return TCL_ERROR;
        }

        call();
        Tcl_ResetResult( interp );
        return TCL_OK;
    }
    Svc_SetResult( interp, "Unknown command for RouteSocket object", TCL_STATIC );
    return TCL_ERROR;
}

/**
 */
static void
DestroyDomain_delete( ClientData data ) {
    using namespace Xen;
    DestroyDomain *hypercall = (DestroyDomain *)data;
    delete hypercall;
}

/**
 */
static int
DestroyDomain_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name domid" );
        return TCL_ERROR;
    }

    domid_t domid;
    void *p = (void *)&(domid); // avoid strict-aliasing breakage in compiler
    if ( Tcl_GetLongFromObj(interp,objv[2],(long*)p) != TCL_OK ) {
        return TCL_ERROR;
    }

    int len;
    char *name = Tcl_GetStringFromObj( objv[1], &len );

    using namespace Xen;
    DestroyDomain *obj = new DestroyDomain( domid );

    // should have a delete proc also
    Tcl_CreateObjCommand( interp, name, DestroyDomain_obj, (ClientData)obj, DestroyDomain_delete );
    Svc_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

/**
 */
static int
GetDomainInfoList_obj( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    using namespace Xen;
    GetDomainInfoList *hypercall = (GetDomainInfoList *)data;
    GetDomainInfoList& call = *hypercall;

    if ( objc == 1 ) {
        Tcl_SetObjResult( interp, Tcl_NewLongObj((long)(hypercall)) );
        return TCL_OK;
    }
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "type") ) {
        Svc_SetResult( interp, "Xen::GetDomainInfoList", TCL_STATIC );
        return TCL_OK;
    }

    if ( Tcl_StringMatch(command, "call") ) {
        if ( objc != 2 ) {
            Tcl_ResetResult( interp );
            Tcl_WrongNumArgs( interp, 1, objv, "call" );
            return TCL_ERROR;
        }

        size_t size = sizeof(xen_domctl_getdomaininfo_t) * 256;
        void *address = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if ( address == MAP_FAILED ) {
            Tcl_Obj *result = Tcl_NewStringObj( "failed to allocate memory (MAP_FAILED)", -1 );
            Tcl_SetObjResult( interp, result );
            return TCL_ERROR;
        }
        if ( mlock(address, size) == -1 ) {
            munmap( address, size );
            Tcl_Obj *result = Tcl_NewStringObj( "failed to mlock memory", -1 );
            Tcl_SetObjResult( interp, result );
            return TCL_ERROR;
        }

        xen_domctl_getdomaininfo_t *info = (xen_domctl_getdomaininfo_t *)address;
        Tcl_Obj *list = Tcl_NewListObj( 0, 0 );
        int count = call( info, 256 );

        if ( count == 0 ) {
            Tcl_Obj *result = Tcl_NewStringObj( "xen reports 0 domains", -1 );
            Tcl_SetObjResult( interp, result );
            return TCL_ERROR;
        }

        for ( int i = 0 ; i < count ; i++ ) {
            Tcl_Obj *element;
            Tcl_Obj *dominfo = Tcl_NewListObj( 0, 0 );

            element = Tcl_NewStringObj( "flags", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].flags );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            element = Tcl_NewStringObj( "tot_pages", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].tot_pages );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            element = Tcl_NewStringObj( "max_pages", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].max_pages );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            element = Tcl_NewStringObj( "cpu_time", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].cpu_time );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            element = Tcl_NewStringObj( "nr_online_vcpus", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].nr_online_vcpus );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            element = Tcl_NewStringObj( "max_vcpu_id", -1 );
            Tcl_ListObjAppendElement( interp, dominfo, element );
            element = Tcl_NewLongObj( info[i].max_vcpu_id );
            Tcl_ListObjAppendElement( interp, dominfo, element );

            Tcl_ListObjAppendElement( interp, list, dominfo );
        }

        munlock( address, size );
        munmap( address, size );

        Tcl_SetObjResult( interp, list );
        return TCL_OK;
    }
    Tcl_Obj *result = Tcl_NewStringObj( "Unknown command for GetDomainInfoList object", -1 );
    Tcl_SetObjResult( interp, result );
    return TCL_ERROR;
}

/**
 */
static void
GetDomainInfoList_delete( ClientData data ) {
    using namespace Xen;
    GetDomainInfoList *hypercall = (GetDomainInfoList *)data;
    delete hypercall;
}


/**
 */
static int
GetDomainInfoList_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name" );
        return TCL_ERROR;
    }

    int len;
    char *name = Tcl_GetStringFromObj( objv[1], &len );

    using namespace Xen;
    GetDomainInfoList *obj = new GetDomainInfoList();

    // should have a delete proc also
    Tcl_CreateObjCommand( interp, name, GetDomainInfoList_obj, (ClientData)obj, GetDomainInfoList_delete );
    Svc_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

/**
 */
static int
XenStoreRead_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "key" );
        return TCL_ERROR;
    }

    int len;
    char *key = Tcl_GetStringFromObj( objv[1], &len );

    Xen::Store store;
    char *value = store.read( key );
    Tcl_Obj *result = Tcl_NewStringObj( value, -1 );
    Tcl_SetObjResult( interp, result );
    free( value );
    return TCL_OK;
}

/**
 */
static int
XenStoreWrite_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "key value" );
        return TCL_ERROR;
    }

    int len;
    char *key   = Tcl_GetStringFromObj( objv[1], &len );
    char *value = Tcl_GetStringFromObj( objv[2], &len );

    Xen::Store store;
    if ( store.write(key, value) ) {
        Tcl_SetObjResult( interp, objv[2] );
        return TCL_OK;
    }

    Svc_SetResult( interp, (char *)store.error_message(), TCL_VOLATILE );
    char e[128];
    char *err;
    err = strerror_r( store.error(), e, sizeof(e) );
    Tcl_AppendResult( interp, err, NULL );
    return TCL_ERROR;
}

/**
 */
static int
XenStoreMkdir_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "key" );
        return TCL_ERROR;
    }

    int len;
    char *key = Tcl_GetStringFromObj( objv[1], &len );

    Xen::Store store;
    if ( store.mkdir(key) ) {
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    Svc_SetResult( interp, (char *)store.error_message(), TCL_VOLATILE );
    char e[128];
    char *err;
    err = strerror_r( store.error(), e, sizeof(e) );
    Tcl_AppendResult( interp, err, NULL );
    return TCL_ERROR;
}

/**
 */
static int
XenStoreRemove_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "key" );
        return TCL_ERROR;
    }

    int len;
    char *key = Tcl_GetStringFromObj( objv[1], &len );

    Xen::Store store;
    if ( store.remove(key) ) {
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    Svc_SetResult( interp, (char *)store.error_message(), TCL_VOLATILE );
    char e[128];
    char *err;
    err = strerror_r( store.error(), e, sizeof(e) );
    Tcl_AppendResult( interp, err, NULL );
    return TCL_ERROR;
}

/**
 */
static int
XenStoreList_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "key" );
        return TCL_ERROR;
    }

    int len;
    char *key = Tcl_GetStringFromObj( objv[1], &len );

    Tcl_Obj *list = Tcl_NewListObj( 0, 0 );

    Xen::Store store;
    Xen::StorePath *values = store.readdir( key );
    Xen::StorePath *child = values;
    while ( child != 0 ) {
        Tcl_ListObjAppendElement( interp, list, Tcl_NewStringObj(child->path, -1) );
        child = child->next;
    }
    // delete values;

    Tcl_SetObjResult( interp, list );
    return TCL_OK;
}

/**
 */
bool Xen::Initialize( Tcl_Interp *interp ) {
    Tcl_Command command;

    Tcl_Namespace *ns = Tcl_CreateNamespace(interp, "Xen", (ClientData)0, NULL);
    if ( ns == NULL ) {
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::physinfo", physinfo_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::PauseDomain", PauseDomain_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::UnpauseDomain", UnpauseDomain_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::DestroyDomain", DestroyDomain_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::GetDomainInfoList", GetDomainInfoList_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    ns = Tcl_CreateNamespace(interp, "Xen::Store", (ClientData)0, NULL);
    if ( ns == NULL ) {
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::Store::read", XenStoreRead_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::Store::write", XenStoreWrite_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::Store::mkdir", XenStoreMkdir_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::Store::remove", XenStoreRemove_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Xen::Store::list", XenStoreList_cmd, (ClientData)0, NULL);
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
