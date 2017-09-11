
/** \file Fiber.h
 * \brief C++ class defining a fiber abstraction.
 *
 * This is a wrapper around the POSIX ucontext implementation that also
 * connects to a TCL library for debugging/configuration, etc.
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <unistd.h>
// #include <ucontext.h>
#include <sys/ucontext.h>
#include "Queue.h"

typedef void (*coroutine)();

/**
 * A class to wrap fiber management.  It also connects the TCL interpreter
 * to an instance of the class for managing its state.
 */
class Fiber {
private:
    Fiber() {}
protected:
    ucontext_t context;
    char *_fiber_name;
public:
    enum { STOPPED, RUNNABLE, RUNNING, BLOCKED } state;
    Fiber( coroutine );
    virtual ~Fiber() {} // TODO clean up name
    void running() { state = RUNNING; }
    virtual void run() = 0;
    virtual bool start();
    virtual bool stop();
    const char *fiber_name() const { return _fiber_name; }
    void fiber_name( const char * );
};

class Scheduler {
private:
    ucontext_t dispatcher;
    ucontext_t *current;
public:
    Scheduler();
    ~Scheduler();
    void schedule();
    bool start( coroutine );
};

#endif

/* vim: set autoindent expandtab sw=4 : */
