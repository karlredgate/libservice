
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
#include "Queue.h"

extern pthread_key_t CurrentThread;
extern void InitializeThreads();

class Thread;

class ThreadCallback {
public:
    ThreadCallback();
    virtual ~ThreadCallback();
    virtual bool operator () ( Thread * ) = 0;
};

extern ThreadCallback *thread_create_hook;

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

/* vim: set autoindent expandtab sw=4 : */
