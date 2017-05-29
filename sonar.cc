
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

/** \file sonar.cc
 * \brief An echo-location service.
 *
 * Echo back what was sent with a timestamp and location.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>

#include <tcl.h>

#include <valgrind/memcheck.h>

#include "logger.h"

#include "util.h"
#include "UUID.h"
#include "Thread.h"
#include "Service.h"

/**
 */
UUID uuid;

void LoadUUID( Tcl_Interp *interp ) {
    FILE *f;
    int count;
    char buffer[128];
    static const char *config = "/etc/sonar/uuid";

    if ( access(config, R_OK) == 0 ) {
        f = fopen( config, "r" );
        count = fscanf(f, "%s", buffer);
        fclose( f );
        if ( count == 1 ) goto done;
    }

    log_notice( "UUID not configured yet, assigning UUID to netmgr" );
    f = fopen( "/proc/sys/kernel/random/uuid", "r" );
    count = fscanf(f, "%s", buffer);
    fclose( f );
    if ( count != 1 ) {
        log_err( "ERROR: failed to assign new UUID" );
        exit( 1 );
    }

    f = fopen(config, "w");
    if ( f == NULL ) {
        log_warn( "WARNING: Could not save netmgr UUID config" );
    } else {
        fprintf( f, "%s\n", buffer );
        fclose( f );
    }

done:
    uuid.set( buffer );
    log_notice( "UUID set to '%s'", uuid.to_s() );
    // could use LinKVar and make this readonly
    Tcl_SetVar(interp, "UUID", uuid.to_s(), TCL_GLOBAL_ONLY);
}

/**
 */
int advertise_interval = 3;

/**
 */
static int
AppInit( Tcl_Interp *interp ) {
    if ( Tcl_LinkVar(interp, "advertise_interval", (char *)&advertise_interval, TCL_LINK_INT) != TCL_OK ) {
        log_err( "failed to link advertise_interval" );
        exit( 1 );
    }

    return TCL_OK;
}

// Need an appinit

/**
 */
int main( int argc, char **argv ) {

    char buffer[16];

    gethostname( buffer, sizeof buffer );

    Service service( "sonar" );
    service.set_facility( LOG_LOCAL4 );
    service.initialize( argc, argv );

    // service.add_command( "set_access", set_access_cmd, (ClientData)&manager );

    service.start();

    int iter = 0;
    for (;;) {
        sleep( advertise_interval );
        if ( ++iter > 100 ) {
            VALGRIND_DO_LEAK_CHECK;
            iter = 0;
        }
    }
}

/* vim: set autoindent expandtab sw=4 : */
