
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

/** \file tcl_util.h
 * \brief Layer some compatibility for TCL interoperability.
 *
 * TCL 8.4 doesn't export the Namespace management functions, but TCL 8.5
 * does.
 */

#ifndef _TCL_UTIL_H_
#define _TCL_UTIL_H_

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

void Tcl_StaticSetResult( Tcl_Interp *interp, const char *message );

#endif

/* vim: set autoindent expandtab sw=4 : */
