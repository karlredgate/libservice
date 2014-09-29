
/** \file Kernel.cc
 * \brief 
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/reboot.h>

#include <time.h>

#include <tcl.h>
#include "TCL_Fixup.h"

#include "Kernel.h"

namespace {
    int debug = 0;
    char *static_envp[] = {
        const_cast<char*>("HOME=/root"),
        NULL
    };
}

/**
 */
static int
tracing_enabled( Tcl_Interp *interp ) {
    int enabled = 1;

    Tcl_Obj *obj = Tcl_GetVar2Ex( interp, "trace", NULL, 0 );
    if ( obj == NULL ) return enabled;

    if ( Tcl_GetBooleanFromObj(interp, obj, &enabled) != TCL_OK ) {
        return 1;
    }

    return enabled;
}

/**
 * Kernel::daemonize /path/to/command arg1 arg2 ...
 */
static int
daemonize_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command [args]" );
        return TCL_ERROR; 
    }

    char *command = Tcl_GetStringFromObj( objv[1], NULL );

    if ( access(command, X_OK) < 0 ) {
        Svc_SetResult( interp, "cannot execute command", TCL_STATIC );
        return TCL_ERROR;
    }

    char *argv[30];
    int argc = 1;
    argv[0] = command;

    for ( int i = 2 ; i < objc ; i++ ) {
        argv[argc++] = Tcl_GetStringFromObj( objv[i], NULL );
    }
    argv[argc] = NULL;

    int pid = fork();
    if ( pid != 0 ) {
        Tcl_SetObjResult( interp, Tcl_NewIntObj((pid)) );
        return TCL_OK;
    }
    close( 0 );
    close( 1 );
    close( 2 );
    if ( fork() != 0 ) { _exit(0); }

    if ( setsid() < 0 ) {
        syslog( LOG_NOTICE, "failed to setsid()" );
    }

    freopen( "/dev/null", "r", stdin );
    freopen( "/dev/null", "w", stdout );
    freopen( "/dev/null", "w", stderr );

    openlog( "(house:background)", LOG_PERROR, LOG_USER );
    syslog( LOG_NOTICE, "spawning '%s'", command );

    if ( execve(command, argv, static_envp) < 0 ) {
        syslog( LOG_NOTICE, "failed to execve('%s')", command );
        _exit(0);
    }

    return TCL_ERROR;
}

/**
 * Look in /proc/filesystems for valid fs types -- then look for modules if it is not loaded
 *
 * type defaults to ext3
 * flags should be defined constants: Kernel::MS_BIND
 *
 * Kernel::mount /dev/drbd5 /shared ext3 {nodev dirsync}
 */
static int
mount_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc < 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "source target [type] [flags]" );
        return TCL_ERROR; 
    }

    unsigned long flags = 0;
    char *source = Tcl_GetStringFromObj( objv[1], NULL );
    char *target = Tcl_GetStringFromObj( objv[2], NULL );

    const char *fstype = "ext3";
    if ( objc > 3 ) {
        fstype = Tcl_GetStringFromObj( objv[3], NULL );
    }

    // change this to an array
    if ( objc > 4 ) {
        for ( int i = 4 ; i < objc ; i++ ) {
            char *flag = Tcl_GetStringFromObj( objv[i], NULL );
            if ( Tcl_StringMatch(flag, "nodev") ) { flags |= MS_NODEV; continue; }
            if ( Tcl_StringMatch(flag, "dirsync") ) { flags |= MS_NODEV; continue; }
            if ( Tcl_StringMatch(flag, "bind") ) { flags |= MS_BIND; continue; }
        }
    }

    if ( mount(source, target, fstype, flags, "") < 0 ) {
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
umount_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "target" );
        return TCL_ERROR; 
    }

    char *target = Tcl_GetStringFromObj( objv[1], NULL );
    int result = ::umount( target );
    if ( result == 0 ) {
        Tcl_ResetResult( interp );
        return TCL_OK;
    }

    // \todo set result to perror value
    Svc_SetResult( interp, "Unknown command for EchoRequest object", TCL_STATIC );
    return TCL_OK;
}

/**
 */
static int
reboot_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR; 
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );
    syslog( LOG_NOTICE, "reboot: %s", message );
    sync();
    int result = ::reboot( RB_AUTOBOOT );
    if ( result < 0 ) {
        Tcl_ResetResult( interp );
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
halt_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR; 
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );
    syslog( LOG_NOTICE, "reboot: %s", message );
    sync();
    int result = ::reboot( RB_HALT_SYSTEM );
    if ( result < 0 ) {
        Tcl_ResetResult( interp );
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
poweroff_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "message" );
        return TCL_ERROR; 
    }

    char *message = Tcl_GetStringFromObj( objv[1], NULL );
    syslog( LOG_NOTICE, "reboot: %s", message );
    sync();
    int result = ::reboot( RB_POWER_OFF );
    if ( result < 0 ) {
        Tcl_ResetResult( interp );
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static int
salute_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "bool" );
        return TCL_ERROR; 
    }

    int cad;
    if ( Tcl_GetBooleanFromObj(interp, objv[1], &cad) != TCL_OK ) {
        Svc_SetResult( interp, "invalid boolean for arg", TCL_STATIC );
        return TCL_ERROR;
    }
    int result;
    if ( cad ) {
        result = ::reboot( RB_ENABLE_CAD );
    } else {
        result = ::reboot( RB_DISABLE_CAD );
    }

    if ( result < 0 ) {
        Tcl_ResetResult( interp );
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );
    return TCL_OK;
}

/**
 */
static pid_t
spawn_child( Tcl_Interp *interp, const char *command, int objc, Tcl_Obj * CONST *objv, int *out ) {

    char *argv[30];
    int argc = 1;
    argv[0] = const_cast<char*>(command);

    for ( int i = 0 ; i < objc ; i++ ) {
        argv[argc++] = Tcl_GetStringFromObj( objv[i], NULL );
    }
    argv[argc] = NULL;

    int fd[2];

    /*
     * fd[0] is for reading - parent input fd
     * fd[1] is for writing - child stdout
     */
    if ( pipe(fd) < 0 ) {
        syslog( LOG_NOTICE, "failed to generate pipe for '%s'", command );
        return -1;
    }

    pid_t child = fork();

    /*
     * if child < 0 this is an error
     * if child > 0 this is the parent
     */
    if ( child != 0 ) {
        close( fd[1] );
        *out = fd[0];
        return child;
    }

    /*
     * Redirect stdout and stderr to the pipe
     */
    close( fd[0] );
    close( 1 );
    dup2( fd[1], 1 );
    close( 2 );
    dup2( fd[1], 2 );

    if ( execve(command, argv, static_envp) < 0 ) {
        syslog( LOG_NOTICE, "failed to execve '%s'", command );
        _exit( 0 );
    }
    syslog( LOG_NOTICE, "THIS MESSAGE SHOULD NEVER HAPPEN - exec failed" );
    _exit( 0 );
}

/**
 */
static void
sleep_for_second() {
    struct timespec delay = { 1, 0 };
    struct timespec remaining = { 0, 0 };

    while ( nanosleep(&delay, &remaining) < 0 ) {
        if ( errno != EINTR ) return;
        delay.tv_sec = remaining.tv_sec;
        delay.tv_nsec = remaining.tv_nsec;
    }
}

/**
 */
static int
wait_for_child( pid_t child, int timeout ) {
    int status;
    int pid;

    struct timespec delay = { 1, 0 };
    struct timespec remaining = { 0, 0 };

    if ( child == 0 ) {
        syslog( LOG_NOTICE, "asked to wait for an invalid pid" );
        return 0;
    }

    while ( timeout > 0 ) {
        pid = waitpid( child, &status, WNOHANG );
        if ( pid == child ) {
            if ( WIFEXITED(status) ) {
                if ( WEXITSTATUS(status) != 0 ) {
                    syslog( LOG_NOTICE, "%d exited with status %d", pid, WEXITSTATUS(status) );
                }
                return child; 
            }
            if ( WIFSIGNALED(status) ) {
                syslog( LOG_NOTICE, "%d killed by signal %d", pid, WTERMSIG(status) );
                return child;
            }
        }
        if ( pid == 0 ) {
            sleep_for_second();
            --timeout;
            continue;
        }
        if ( pid < 0 ) {
            switch (errno) {
            case EINTR: continue;
            case EINVAL:
                syslog( LOG_NOTICE, "waitpid() with an invalid argument" );
                break;
            case ECHILD:
                syslog( LOG_NOTICE, "waitpid() for an invalid pid" );
                break;
            default:
                syslog( LOG_NOTICE, "unknown errno from waitpid()" );
                break;
            }
        }
        syslog( LOG_NOTICE,
                "waitpid() returned an invalid value with %d seconds remaining",
                timeout );
        break;
    }

    if ( debug ) syslog( LOG_NOTICE, "timed out, killing pid %d", child );
    kill( child, SIGKILL );

    /* pid = waitpid( child, &status, WNOHANG ); */
    pid = waitpid( child, &status, 0 );
    return -1;
}

/**
 */
static int
timeout( Tcl_Interp *interp, int time_limit,
              const char *command, int objc, Tcl_Obj * CONST *objv )
{
    int tracing = tracing_enabled(interp);

    if ( access(command, X_OK) < 0 ) {
        Svc_SetResult( interp, "cannot execute command", TCL_STATIC );
        return TCL_ERROR;
    }

    Tcl_Obj *obj = Tcl_ConcatObj( objc, objv );
    char *arg_string = Tcl_GetStringFromObj( obj, NULL );

    int out;
    pid_t child = spawn_child( interp, command, objc, objv, &out );

    if ( child < 0 ) {
        syslog( LOG_NOTICE, "fork failed for '%s %s'", command, arg_string );
        Tcl_ResetResult( interp );
        char buffer[128];
        char *err = strerror_r(errno, buffer, sizeof(buffer));
        Svc_SetResult( interp, err, TCL_VOLATILE );
        return TCL_ERROR;
    }

    if ( tracing ) {
        syslog( LOG_NOTICE, "spawned '%s %s' (%d)", command, arg_string, child );
    }
    int result = wait_for_child( child, time_limit );

    if ( result < 0 ) {
        close( out );
        syslog( LOG_NOTICE, "timed out waiting for '%s' (%d)", command, child );
        Svc_SetResult( interp, "timed out waiting for child", TCL_STATIC );
        return TCL_ERROR;
    }

    Tcl_ResetResult( interp );

    char buffer[128];
    ssize_t bytes;

    for (;;) {
        bytes = read( out, buffer, sizeof(buffer)-1 );
        if ( bytes == 0 ) break;
        if ( bytes < 0 ) {
            syslog( LOG_NOTICE, "error reading from child - skipping rest" );
            break;
        }
        buffer[bytes] = '\0';
        Tcl_AppendResult( interp, buffer, NULL );
    }
    close( out );

    if ( bytes < 0 ) {
        Svc_SetResult( interp, "error reading output from child", TCL_STATIC );
        return TCL_ERROR;
    }

    return TCL_OK;
}

/**
 * timeout <seconds> command arg1..argN
 */
static int
timeout_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc < 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "seconds command arg1..argN" );
        return TCL_ERROR; 
    }

    int time_limit;
    if ( Tcl_GetIntFromObj(interp,objv[1],&time_limit) != TCL_OK ) {
        Svc_SetResult( interp, "invalid timeout", TCL_STATIC );
        return TCL_ERROR;
    }

    char *command = Tcl_GetStringFromObj( objv[2], NULL );

    return timeout( interp, time_limit, command, objc-3, objv+3 );
}

/**
 */
static int
determine_timeout( Tcl_Interp *interp ) {
    int time_limit = 20;

    Tcl_Obj *obj = Tcl_GetVar2Ex( interp, "timeout", NULL, 0 );
    if ( obj == NULL ) return time_limit;

    if ( Tcl_GetIntFromObj(interp,obj,&time_limit) != TCL_OK ) {
        return 20;
    }

    return time_limit;
}

/**
 * set ipmi_timeout 30
 * ipmitool command arg1..argN
 */
static int
ipmitool_cmd( ClientData data, Tcl_Interp *interp,
             int objc, Tcl_Obj * CONST *objv )
{
    if ( objc < 2 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command arg1..argN" );
        return TCL_ERROR; 
    }

    return timeout( interp, determine_timeout(interp),
                    "/usr/bin/ipmitool", objc-1, objv+1 );
}

/**
 */
bool Kernel::Initialize( Tcl_Interp *interp ) {
    Tcl_Command command;

    Tcl_Namespace *ns = Tcl_CreateNamespace(interp, "Kernel", (ClientData)0, NULL);
    if ( ns == NULL ) {
        return false;
    }

    if ( Tcl_LinkVar(interp, "Kernel::debug", (char *)&debug, TCL_LINK_INT) != TCL_OK ) {
        syslog( LOG_ERR, "failed to link Kernel::debug" );
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::daemonize", daemonize_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::mount", mount_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::umount", umount_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::reboot", reboot_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::halt", halt_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::poweroff", poweroff_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::salute", salute_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::timeout", timeout_cmd, (ClientData)0, NULL);
    if ( command == NULL ) {
        // syslog ?? want to report TCL Error
        return false;
    }

    command = Tcl_CreateObjCommand(interp, "Kernel::ipmitool", ipmitool_cmd, (ClientData)0, NULL);
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
