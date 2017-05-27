
/** \file Service.h
 * \brief 
 *
 */

#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <tcl.h>
#include "Thread.h"
#include "Channel.h"

/**
 * run() should not be able to execute unless initialized
 * or maybe when run -- initialize is called -- then it could 
 * be private
 */
class Service : public Thread {
private:
	// this could potentially be completley internal
    Tcl_Interp *interp;
    const char *service_name;
    char logfilename[80];
    char rundir[80];
    Channel *channel;
    int facility;
public:
    Service( const char * );
    virtual ~Service();
    bool initialize( int, char ** );
    virtual void run();
    const char *name() const { return service_name; }
    void set_facility( int );
    bool add_command( char *, Tcl_ObjCmdProc *, ClientData );
    bool load_command( const char * );
    bool load_commands( const char * );
    bool load_directory( const char * );
    bool load_file( const char * );
    // this is here ONLY temporarily while I transition ekg/interface to the service
    // interface -- it calls the tcl interp directly -- which I do not want
    Tcl_Interp *interpreter() { return interp; }
};

#endif

/* vim: set autoindent expandtab sw=4 : */
