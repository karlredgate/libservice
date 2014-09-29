/** \file 
 * \brief 
 *
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <glob.h>

/**
 * dst is the destination service id
 * src is the source service id
 */
struct channel_message {
    long dst;
    long src;
    long error;
    char body[2048];
};
#define MESSAGE_OK        0
#define MESSAGE_ERROR     1
#define MESSAGE_EXCEPTION 2

/** 
 * Look through list of processes to see if the named service is
 * alive.  We check by looking a process that has its executable
 * as /usr/sbin/[service_name].
 *
 * Note: readlink does not NULL terminate the strings it reads, so
 *   we must do it explicitly.
 */
static int
is_alive( char *service_name ) {
    int result = 0;
    glob_t processes;
    size_t i;
    char exe[256], path[80];

    sprintf( path, "/usr/sbin/%s", service_name );

    memset( &processes, 0, sizeof(processes) );
    glob( "/proc/*/exe", GLOB_NOSORT, NULL, &processes );

    for ( i = 0 ; i < processes.gl_pathc ; i++ ) {
        int count = readlink( processes.gl_pathv[i], exe, sizeof(exe) );
        if ( count == -1 ) continue;
        exe[count] = '\0';
        if ( strcmp(exe, path) == 0 ) {
            result = 1;
            break;
        }
    }

    globfree( &processes );
    return result;
}

/**
 */
key_t service_key( char *service_name ) {
    char path[80];
    sprintf( path, "/var/run/%s", service_name );
    key_t key = ftok( path, 'S' );
    if ( key == -1 ) {
        fprintf( stderr, "could not generate a key for '%s'\n", path );
        exit( 1 );
    }
    // send this to stdout if the tcl interpreter is interactive
    // syslog( LOG_NOTICE, "channel key for '%s' is 0x%08x", service_name, key );
    return key;
}

int Q( char *service_name ) {
    int q;
    key_t key = service_key(service_name);

    q = msgget( key, IPC_CREAT | 0777 );
    if ( q < 0 ) {
        fprintf( stderr, "could not create a msgQ for '%s'\n", service_name );
        exit( 1 );
    }
}

char *error_string( long error ) {
    switch (error) {
    case MESSAGE_OK:        return "OK";
    case MESSAGE_ERROR:     return "ERROR";
    case MESSAGE_EXCEPTION: return "EXCEPTION";
    default: return "UNKNOWN";
    }
}

/**
 */
void
peek( char *service_name ) {
    int body_length;
    int flags = 0; // IPC_NOWAIT | MSG_NOERROR
    struct channel_message message;
    int q = Q(service_name);
    int bytes = msgrcv( q, &message, sizeof(message), 0, flags );

    if ( bytes < 0 ) {
        // how to handle error
        perror( "Error peeking (msgrcv) at Channel" );
        exit( 1 );
    }

    body_length = strlen(message.body);
    fprintf( stderr, "body length is %d\n", body_length );
    if ( body_length > 1012 ) {
        fprintf( stderr, "body is too big for message\n" );
    }

    printf( "%d <- %d (%s) %s\n", message.dst, message.src,
                                  error_string(message.error), message.body );

    if ( bytes > 1024 ) bytes = 1024;
    if ( msgsnd(q, &message, bytes, flags) < 0 ) {
        perror( "failed to msgsnd" );
    }
}

int main( int argc, char **argv ) {
    peek( argv[1] );
}

/*
 * vim:autoindent
 * vim:expandtab
 */
