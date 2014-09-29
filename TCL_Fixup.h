
/** \file TCL_Fixup.h
 * \brief Layer some compatibility for TCL interoperability.
 *
 * TCL 8.4 doesn't export the Namespace management functions, but TCL 8.5
 * does.
 */

#ifndef _TCL_FIXUP_H_
#define _TCL_FIXUP_H_

#include <tcl.h>

EXTERN Tcl_Namespace *
Tcl_CreateNamespace _ANSI_ARGS_((Tcl_Interp * interp, CONST char * name,
                                 ClientData clientData, 
                                 Tcl_NamespaceDeleteProc * deleteProc));

EXTERN void
Tcl_DeleteNamespace _ANSI_ARGS_((Tcl_Namespace * nsPtr));

EXTERN Tcl_Namespace *
Tcl_FindNamespace _ANSI_ARGS_((Tcl_Interp * interp, CONST char * name,
                               Tcl_Namespace * contextNsPtr, int flags));

EXTERN Tcl_Var
Tcl_FindNamespaceVar _ANSI_ARGS_(( Tcl_Interp * interp, CONST char * name,
                                Tcl_Namespace * contextNsPtr, int flags));

EXTERN Tcl_Namespace *
Tcl_GetCurrentNamespace _ANSI_ARGS_((Tcl_Interp * interp));

EXTERN Tcl_Namespace *
Tcl_GetGlobalNamespace _ANSI_ARGS_((Tcl_Interp * interp));

inline void
Svc_SetResult( Tcl_Interp *interp, const char *message, Tcl_FreeProc *proc ) {
    Tcl_SetResult( interp, const_cast<char*>(message), proc );
}

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
