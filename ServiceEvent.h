
/** \file ServiceEvent.h
 * \brief 
 *
 */

#ifndef _SERVICE_EVENT_H_
#define _SERVICE_EVENT_H_

#include <tcl.h>

/**
 */
namespace Service {

    /**
     */
    class Event {
    protected:
        char buffer[2048];
        char *event_name;
    public:
        Event( char * );
        virtual ~Event();
        virtual bool send();
    };

    /**
     */
    class LinkUp : public Event {
    private:
    public:
        LinkUp( char * );
        virtual ~LinkUp() {}
    };

    /**
     */
    bool Initialize( Tcl_Interp * );

}

#endif

/*
 * vim:autoindent:expandtab:
 */
