
/** \file Thread.h
 * \brief C++ class defining a thread abstraction.
 *
 * This is a wrapper around a pthread library implementation that also
 * connects to a TCL library for debugging/configuration, etc.
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <tcl.h>
#include "Queue.h"

extern pthread_key_t CurrentThread;
extern void InitializeThreads();

/**
 * A class to wrap thread management.  It also connects the TCL interpreter
 * to an instance of the class for managing its state.
 */
class Thread {
protected:
    pthread_t id;
    pid_t pid;
    Queue q;
    char *_thread_name;
public:
    const char *status;
    Thread( const char * );
    virtual ~Thread() {} // TODO clean up name
    static Tcl_Interp *global_interp();
    void running() { status = "running"; }
    virtual void run() = 0;
    virtual bool start();
    virtual bool stop();
    void setpid() { pid = ::getpid(); }
    pid_t getpid() { return pid; }
    const char *thread_name() const { return _thread_name; }
    void thread_name( const char * );
    int TclCommand(ClientData, Tcl_Interp *, int, Tcl_Obj * CONST *);
};

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
