
/** \file Service.cc
 * \brief 
 *
 * \todo add "log" command -- like in house
 * \todo change command name in argv0
 *
 * \todo include SMBIOS information by default -- to provide UUID to all services.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <execinfo.h>
#include <glob.h>

#include <tcl.h>
#include "TCL_Fixup.h"

#include "util.h"
#include "Service.h"
#include "AppInit.h"

/**
 */
Service::Service( const char *_service_name ) : Thread("service") {
    service_name = strdup( _service_name );
    char buffer[1024];
    sprintf( buffer, "%s.service", service_name );
    thread_name( buffer );
    facility = LOG_DAEMON;
}

/**
 * A Service cannot be deleted and restarted - so don't bother with
 * memory recovery.
 */
Service::~Service() {
    /* free( service_name ); */
}

/**
 */
void
Service::set_facility( int syslog_facility ) {
    facility = syslog_facility;
}

/**
 */
Tcl_CmdInfo putsObjCmd;

/**
 * puts ?-nonewline? ?channelId? string
 */
static int
PutsSyslog_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc == 4 ) {
        return putsObjCmd.objProc(data, interp, objc, objv);
    }

    if ( objc == 3 ) {
        char *option = Tcl_GetStringFromObj( objv[1], NULL );
        if ( Tcl_StringMatch(option, "-nonewline") == 0 ) {
            return putsObjCmd.objProc(data, interp, objc, objv);
        }
        objc--; objv++;
    }

    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR;
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );

    syslog( LOG_NOTICE, "%s", message );
    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
SyslogWarn_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR;
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );

    syslog( LOG_WARNING, "%s", message );
    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
SyslogErr_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR;
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );

    syslog( LOG_ERR, "%s", message );
    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 * This is a convenience wrapper function that creates a TCL interpreter, performs
 * its basic initialization.
 *
 * - set the interpreters variables for the command line arguments of the service
 * - forces interactive to false
 * - hides some dangerous commands
 * - calls back the applications initialization function
 *
 * This interpreter acts as a remote command interpreter for other services that
 * need to call into this service.
 */
static Tcl_Interp* create_tcl_interp( int argc, char **argv ) {
    Tcl_Interp *interp;
    Tcl_FindExecutable( argv[0] );

    // interp = Thread::global_interp();
    interp = Tcl_CreateInterp();

    if ( interp == NULL ) {
        syslog( LOG_ERR, "failed to create interpreter" );
        exit( 1 );
    }   

    char *args = Tcl_Merge(argc-1, argv+1);
    Tcl_DString argString;
    Tcl_ExternalToUtfDString(NULL, args, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(args);
    Tcl_SetVar2Ex(interp, "argc", NULL, Tcl_NewIntObj(argc - 1), TCL_GLOBAL_ONLY);
    Tcl_SetVar2Ex(interp, "argv0", NULL, Tcl_NewStringObj(argv[0], -1), TCL_GLOBAL_ONLY);

    if ( getenv("INIT_VERSION") ) {
        Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
        char *run_level = getenv("RUNLEVEL");
        if ( run_level ) {
            Tcl_SetVar2Ex(interp, "run_level", NULL, Tcl_NewStringObj(run_level, -1), TCL_GLOBAL_ONLY);
        }
    } else {
        Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
    }

    // disable some dangerous commands
    Tcl_HideCommand( interp, "exit", "hiddenExit" );
    Tcl_GetCommandInfo(interp, "puts", &putsObjCmd);
    Tcl_CreateObjCommand(interp, "puts", PutsSyslog_cmd, (ClientData)0, NULL);
    Tcl_CreateObjCommand(interp, "ERROR", SyslogErr_cmd, (ClientData)0, NULL);
    Tcl_CreateObjCommand(interp, "WARN", SyslogWarn_cmd, (ClientData)0, NULL);

    if ( Channel_Initialize(interp) == false ) {
        syslog( LOG_ERR, "Channel_Initialize failed" );
    }

    if ( Tcl_CallAppInitChain(interp) == false ) {
	    // exit
    }

    return interp;
}

/**
 * create the rundir 
 * create the channel
 * create the interp
 * call the appinit
 *
 * I may want to read the systems UUID for all applications/services.
 */
bool
Service::initialize( int argc, char **argv ) {
    pid_t my_pid = getpid();
    char buffer[1024];

    /**
     * First redirect stderr, so we do not send errors to the console.
     */
    openlog( service_name, LOG_NDELAY, facility );
    syslog( LOG_NOTICE, " ======= Startup (%d) ======= ", my_pid );
    // \todo do not trap when interactive
    trap_error_signals();

    bool interactive = getenv("INIT_VERSION") == 0;

    if ( not interactive ) {
        freopen("/dev/null", "w", stdout);

        sprintf( buffer, "/var/log/%s-stderr.log", service_name );
        if ( freopen(buffer, "a", stderr) == NULL ) {
            syslog( LOG_WARNING, " failed to redirect stderr " );
        } else {
            setvbuf( stderr, NULL, _IONBF, 0 );
        }

        fprintf( stderr, "=== %s error log ===\n", service_name );
    }

    sprintf( argv[0], "service.%s ", service_name );

    /**
     * Create a rundir for this service and chdir to it, so temp files, etc
     * are created in one place.
     */
    sprintf( buffer, "/var/run/%s", service_name );
    mkdir( buffer, 0755 );
    chdir( buffer );
    channel = new Channel( this );

    /** * Enable core dumps for this service.
     *
     * The kernel configuration will need to be updated so the core dumps are
     * created in the current directory, or the filenames are not time based,
     * or we need to cause log/core file cleanup in one of the cron jobs.
     */
    struct rlimit core_dump;
    core_dump.rlim_cur = RLIM_INFINITY;
    core_dump.rlim_max = RLIM_INFINITY;
    if ( setrlimit(RLIMIT_CORE, &core_dump) < 0 ) {
        syslog( LOG_WARNING, "could not enable core dumps" );
    }

    /** Check if last run core dumped.
     */
    sprintf( buffer, "/var/core/%s*", service_name );
    glob_t core_dumps;
    memset( &core_dumps, 0, sizeof(core_dumps) );
    glob( buffer, GLOB_NOSORT, NULL, &core_dumps );

    for ( size_t i = 0 ; i < core_dumps.gl_pathc ; i++ ) {
        struct stat s;
        stat( core_dumps.gl_pathv[i], &s );
        syslog( LOG_WARNING, "WARNING: %s core dumped at %s", service_name,
                ctime_r(&s.st_mtime, buffer) );
        sprintf( buffer, "/var/core/saved-core-%s", service_name );
        rename( core_dumps.gl_pathv[i], buffer );
    }

    globfree( &core_dumps );

    /**
     * Create the TCL interpreter that manages this service and read the
     * service configuration script.
     */
    interp = create_tcl_interp( argc, argv );
    sprintf( buffer, "/etc/%s/%s.conf", service_name, service_name );
    if ( access(buffer, R_OK) == 0 ) {
        if ( Tcl_EvalFile(interp, buffer) == TCL_ERROR ) {
            syslog( LOG_ERR, "Failed to read %s initialization: %s", service_name, Tcl_GetStringResult(interp) );
        }
    }

    Tcl_EvalEx( interp, "proc clock {command} { namespace eval ::tcl::clock $command}", -1, TCL_EVAL_GLOBAL );
    Tcl_EvalEx( interp, "proc commands {} {namespace eval commands {info procs}}", -1, TCL_EVAL_GLOBAL );

    return true;
}

/**
 * In order to abstract (only marginally) the creation of commands for this
 * services interpreter, a method is provided that the service main routine
 * can call to add application specific commands to the interpreter.
 */
bool
Service::add_command( char *cmdName, Tcl_ObjCmdProc *proc, ClientData data ) {
    Tcl_Command command;
    command = Tcl_CreateObjCommand(interp, cmdName, proc, data, NULL);
    return (command != NULL);
}

/**
 * Load a command from a filename.  Use the basename of the file for the
 * command name.
 */
bool
Service::load_command( const char *filename ) {
    const char *name = strrchr(filename, '/');
    name = (name == NULL) ? filename : name+1 ;

    int retcode = Tcl_VarEval( interp,
    "namespace eval ", name, " {source ", filename, "}\n",
    "namespace eval commands {proc ", name, " {args} { namespace eval ::", name, " $args }}\n",
    "namespace eval commands {namespace export -clear *}\n",
    "namespace import -force commands::*\n",
    NULL );
    if ( retcode != TCL_OK ) return false;

    return true;
}

/**
 */
bool
Service::load_commands( const char *dir ) {
    char pattern[80];
    snprintf( pattern, sizeof(pattern), "%s/*", dir );

    glob_t paths;
    memset(&paths, 0, sizeof(paths));

    glob( pattern, GLOB_NOSORT, NULL, &paths );
    for ( size_t i = 0 ; i < paths.gl_pathc ; i++ ) {
        char *filename = paths.gl_pathv[i];

        syslog( LOG_NOTICE, "loading '%s'", filename );
        if ( access(filename, R_OK) == 0 ) {
            if ( load_command(filename) == false ) {
                syslog( LOG_WARNING, "error: %s", Tcl_GetStringResult(interp) );
            }
        }
    }
    globfree( &paths );

    return true;
}

/**
 */
bool
Service::load_directory( const char *dir ) {
    char pattern[80];
    snprintf( pattern, sizeof(pattern), "%s/*", dir );

    glob_t paths;
    memset(&paths, 0, sizeof(paths));

    glob( pattern, GLOB_NOSORT, NULL, &paths );
    for ( size_t i = 0 ; i < paths.gl_pathc ; i++ ) {
        char *filename = paths.gl_pathv[i];

        syslog( LOG_NOTICE, "loading '%s'", filename );
        if ( access(filename, R_OK) == 0 ) {
            if ( Tcl_EvalFile(interp, filename) != TCL_OK ) {
                syslog( LOG_WARNING, "error: %s", Tcl_GetStringResult(interp) );
            }
        }
    }
    globfree( &paths );

    return true;
}

/**
 */
bool
Service::load_file( const char *filename ) {
    if ( access(filename, R_OK) != 0 ) return false;
    syslog( LOG_NOTICE, "evaluating file '%s'", filename );
    if ( Tcl_EvalFile(interp, filename) != TCL_OK ) {
        // add TCL interp error string
        syslog( LOG_WARNING, "error evaluating file" );
        return false;
    }
    return true;
}

/**
 * A service object is a thread that waits for messages from a Channel, processes
 * them in a TCL interpreter and responds.
 */
void
Service::run() {
    syslog( LOG_NOTICE, "Channel listening" );
    char request[1024];
    char response[1024];
    for (;;) {
        long sender = channel->receive( request, sizeof(request) );
        if ( ::kill(sender, 0) < 0 ) {
            syslog( LOG_ERR, "client is dead: %s. Ignoring message", strerror(errno) );
            continue;
        }
        int result = Tcl_EvalEx( interp, request, -1, TCL_EVAL_GLOBAL );
        strlcpy( response, Tcl_GetStringResult(interp), sizeof(response) );
        channel->send( sender, result, response );
    }
}

/*
 * vim:autoindent
 * vim:expandtab
 */
