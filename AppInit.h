
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

/** \file AppInit.h
 * \brief 
 *
 */

#ifndef _APPINIT_H_
#define _APPINIT_H_

#include "platform_tcl.h"

/**
 */
struct AppInit {
    struct AppInit *next;
    const char *name;
    bool (*function)( Tcl_Interp * );
};

void add_app_init_callback( struct AppInit * );
bool Tcl_CallAppInitChain( Tcl_Interp * );

#define app_init(callback) \
static void __attribute__ ((constructor)) __init__##callback(void) { \
    static struct AppInit init = { NULL, #callback, callback };                 \
    add_app_init_callback( &init );                                  \
}

#endif

/* vim: set autoindent expandtab sw=4 : */
