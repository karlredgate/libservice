
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

/** \file AppInit.cc
 * \brief 
 *
 */

#include "AppInit.h"

static struct AppInit *head = NULL;

/**
 */
void
add_app_init_callback( struct AppInit *init ) {
    init->next = head;
    head = init;
}

bool
Tcl_CallAppInitChain( Tcl_Interp *interp ) {
    int interactive = 1;
    Tcl_Obj *interactive_obj;
    interactive_obj = Tcl_GetVar2Ex( interp, "tcl_interactive", NULL, TCL_GLOBAL_ONLY );

    if ( interactive_obj != NULL ) {
        Tcl_GetIntFromObj( interp, interactive_obj, &interactive );
    }

    bool result = true;
    struct AppInit *current = head;

    while ( current != NULL ) {
        bool r = current->function(interp);
        result &= r;
        if ( r == true ) {
            if ( interactive ) printf( "%s initialized\n", current->name );
        }
        current = current->next;
    }

    return result;
}

/* vim: set autoindent expandtab sw=4 : */
