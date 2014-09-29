
#ifndef _OBJECT_PRIMITIVES_H_
#define _OBJECT_PRIMITIVES_H_

#include <stdint.h>
#include <cstdlib>
#include "tcl.h"
#include "crc32.h"

namespace Service {

    class IntegerCoercion {
    public:
        virtual ~IntegerCoercion() {}
        virtual uint32_t operator() ( Context * ) = 0;
        virtual void destroy(Pool *);
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
    };

    class StringCoercion {
    public:
        virtual String * operator() ( Context * ) = 0;
        virtual ~StringCoercion() {}
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
        virtual void destroy(Pool *);
    };

    class IntegerIdentity : public IntegerCoercion {
        uint32_t value;
    public:
        IntegerIdentity( uint32_t value ) : value(value) { }
        virtual ~IntegerIdentity() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() ( Context *context ) {
            return value;
        }
    };

    class StringLength : public IntegerCoercion {
        StringCoercion *coercion;
    public:
        StringLength( StringCoercion *coercion )
        : coercion(coercion) { }
        virtual ~StringLength() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() (Context *);
    };

    class FieldLength : public IntegerCoercion {
        Field *dr;
    public:
        FieldLength( Field *dr ) : dr(dr) { }
        virtual ~FieldLength() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() ( Context *context ) {
            return dr->length;
        }
    };

    class FieldValue : public IntegerCoercion {
        Field *dr;
    public:
        FieldValue( Field *dr ) : dr(dr) { }
        virtual ~FieldValue() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() ( Context *context ) {
            if ( dr->count == 0 ) return 0;
            return dr->value;
        }
    };

    class FieldCRC32 : public IntegerCoercion {
        StringCoercion *input;
        bool annotated;
        uint32_t value;
    public:
        FieldCRC32( StringCoercion *input ) : input(input) { }
        virtual ~FieldCRC32() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() (Context *);
    };

    class IntegerFork : public IntegerCoercion {
        Predicate *predicate;
        IntegerCoercion *trueClause;
        IntegerCoercion *falseClause;
    public:
        IntegerFork(
            Predicate *predicate,
            IntegerCoercion *trueClause,
            IntegerCoercion *falseClause
        )
        : predicate(predicate),
          trueClause(trueClause),
          falseClause(falseClause) { }
    
        virtual ~IntegerFork() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() (Context *);
    };

    class StringIdentity : public StringCoercion {
        String *value;
    public:
        StringIdentity( String *value ) : value(value) {}
        virtual ~StringIdentity() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    };

    class FieldString : public StringCoercion {
        Field *dr;
    public:
        FieldString( Field *dr ) : dr(dr) { }
        virtual ~FieldString() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    };

    class hexdecode : public StringCoercion {
        StringCoercion *input;
        bool annotated;
        String annotation;
        static const char nybble[];
    public:
        hexdecode( StringCoercion *input )
        : input(input), annotated(false) {
            annotation.allocate( 1024 );
        }
        virtual ~hexdecode() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    };

    class s_lowercase : public StringCoercion {
        StringCoercion *input;
        bool annotated;
        String annotation;
    public:
        s_lowercase( StringCoercion *input )
        : input(input), annotated(false) {
            annotation.allocate( 1024 );
        }
        virtual ~s_lowercase() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    };

    class s_date : public StringCoercion {
        uint32_t delta;
        bool annotated;
        String annotation;
    public:
        s_date( uint32_t delta )
        : delta(delta), annotated(false) {
            // "Wdy, DD-Mon-YYYY HH:MM:SS UTC"
            annotation.allocate( 30 );
            annotation.count = 1;
        }
        virtual ~s_date() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    };

    class StringFork : public StringCoercion {
        Predicate *predicate;
        StringCoercion *trueClause;
        StringCoercion *falseClause;
    public:
        StringFork(
            Predicate *predicate,
            StringCoercion *trueClause,
            StringCoercion *falseClause
        )
        : predicate(predicate),
          trueClause(trueClause),
          falseClause(falseClause) { }
    
        virtual ~StringFork() {}
        virtual void destroy(Pool *);
        virtual String * operator() (Context *);
    
    };

    class ClientAddress : public IntegerCoercion {
    public:
        virtual ~ClientAddress() {}
        virtual void destroy(Pool *);
        virtual uint32_t operator() ( Context * );
    };

    bool Initialize( Tcl_Interp *, OPE * );
}
#endif
