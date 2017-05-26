
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

/** \file Thread.cc
 * \brief C++ class implementing a thread abstraction.
 *
 * This is a wrapper around a pthread library implementation that also
 * connects to a TCL library for debugging/configuration, etc.
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "Thread.h"
#include "PlatformThread.h"
#include "AppInit.h"

ThreadCallback *thread_create_hook;

/**
 */
ThreadCallback::ThreadCallback() {
}

/**
 */
ThreadCallback::~ThreadCallback() {
}

/**
 * The interpreter created here is not useful until it has been initialized
 * by the main program.
 */
class MainThread : public Thread {
public:
    MainThread() : Thread("main") {
        set_main_thread_name();
        id = pthread_self();
        pid = ::getpid();
    }
    virtual ~MainThread() {}
    virtual void run() { }
    virtual bool start() { return false; }
};

pthread_key_t CurrentThread;

class ThreadList {
    Thread *thread;
    ThreadList *next;
public:
    ThreadList() {
        pthread_key_create( &CurrentThread, NULL );
        thread = new MainThread;
        next = 0;
        pthread_setspecific( CurrentThread, thread );

        if ( thread_create_hook != NULL ) {
            ThreadCallback& callback = *thread_create_hook;
            callback( thread );
        }
    }
    ~ThreadList() {
        delete next;
        delete thread;
    }
    ThreadList( Thread *thread, ThreadList *next )
    : thread(thread), next(next) {
        if ( thread_create_hook != NULL ) {
            ThreadCallback& callback = *thread_create_hook;
            callback( thread );
        }
    }
    void add( Thread *that ) {
        ThreadList *entry = new ThreadList( that, next );
        next = entry;
    }
};
ThreadList threads;

/**
 */
static void *boot( void *data ) {
    Thread *thread = (Thread *)data;
    pthread_setspecific( CurrentThread, thread );
    pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
    thread->setpid();
    thread->running();
    set_thread_name( thread->thread_name() );
    thread->run();
    return NULL;
}

/**
 */
bool Thread::start() {
    if ( pthread_create(&id, NULL, boot, this) ) {
        return false;
    }
    return true;
}

/**
 */
bool Thread::stop() {
    if ( pthread_cancel(id) != 0 ) {
        return false;
    }
    return true;
}

/**
 */
Thread::Thread( const char *_name ) : _thread_name(NULL) {
    thread_name( _name );
    status = "stop ready";
    threads.add( this );
}

/**
 * Since _thread_name might be static - we cannot free it.
 * Thread names do not change frequently enough to warrant
 * worrying about the leak.
 */
void Thread::thread_name( const char *_name ) {
    if ( _name == NULL )  return;
    // if ( _thread_name != NULL ) free(_thread_name);
    _thread_name = strdup(_name);
}

/* vim: set autoindent expandtab sw=4 : */
