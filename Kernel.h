
/** \file Kernel.h
 * \brief A set of TCL extensions for syscalls
 *
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <tcl.h>

/**
 */
namespace Kernel {

    /**
     */
    bool Initialize( Tcl_Interp * );

}

#endif

/*
 * vim:autoindent:expandtab:
 */
