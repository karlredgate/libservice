
#include <stdint.h>
#include "tcl.h"
#include "crc32.h"

#ifndef _OBJECTPOLICY_H_
#define _OBJECTPOLICY_H_

namespace Service {
    // using namespace ObjectProcessing;

    class Verb {
    protected:
        Verb *next;
    public:
        Verb( Verb *_next ) : next(_next) { }
        virtual ~Verb() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context& ) = 0;
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
    };

    class NullVerb : public Verb {
    public:
        NullVerb() : Verb(0) { }
        virtual ~NullVerb() {}
        virtual void destroy(Pool *);
        virtual void operator() (Context&);
    };

    class IfVerb : public Verb {
        Predicate *predicate;
        Verb      *block;
    public:
        IfVerb( Predicate *predicate, Verb *block, Verb *next )
        : predicate(predicate), block(block), Verb(next) { }
        virtual ~IfVerb() {}
        virtual void destroy(Pool *);
        virtual void operator() (Context&);
    };

    class Selection {
    public:
        Predicate *predicate;
        Verb      *block;
        Selection *next;
    
        Selection( Predicate *predicate, Verb *block, Selection *next )
        : predicate(predicate), block(block), next(next) { }
        ~Selection() {}
        void destroy(Pool *);
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
    };

    class Cond : public Verb {
        Selection *selection;
    public:
        Cond( Selection *selection, Verb *next )
        : selection(selection), Verb(next) { }
        virtual ~Cond() {}
        virtual void destroy(Pool *);
        virtual void operator() (Context&);
    };

    class fpID : public Verb {
        uint16_t id;
    public:
        fpID( uint16_t id, Verb *next )
        : Verb(next), id(id) { }
        virtual ~fpID() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context & );
    };

    class tunnel : public Verb {
    public:
        tunnel( Verb *verb ) : Verb(verb) { }
        virtual ~tunnel() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context & );
    };

    class cookiePersist : public Verb {
    public:
        ObjectProcessing::Persistence::Policy *persistence;
        cookiePersist(
            ObjectProcessing::Persistence::Policy *persistence,
            Verb *verb
        ) : persistence(persistence), Verb(verb) { }
        virtual ~cookiePersist() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context & );
    };

    class dont_retry : public Verb {
    public:
        dont_retry( Verb *verb ) : Verb(verb) { }
        virtual ~dont_retry() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context & );
    };

    class closeOptim : public Verb {
    public:
        closeOptim( Verb *verb ) : Verb(verb) { }
        virtual ~closeOptim() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context & );
    };

    class stickyVariable : public Verb {
        IntegerCoercion *coercion;
    public:
        stickyVariable( IntegerCoercion *coercion, Verb *verb )
        : Verb(verb), coercion(coercion) { }
        virtual ~stickyVariable() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context& );
    };

    class cookieNOOP : public Verb {
    public:
        cookieNOOP( Verb *verb ) : Verb(verb) { }
        virtual ~cookieNOOP() {}
        virtual void destroy(Pool *);
        virtual void operator() ( Context &state ) { (*next)( state ); }
    };

    bool Initialize( Tcl_Interp *, OPE * );
}
#endif
