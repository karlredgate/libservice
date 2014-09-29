#include <int_types.h>
#include "tcl.h"
#include "crc32.h"
#include "MFP.h"

#ifndef _OBJECTPOLICY_H_
#define _OBJECTPOLICY_H_

namespace Service {
    using namespace ObjectProcessing;

    class Policy {
    public:
        Predicate *predicate;
        Verb *verb;
        Policy *next;
        Policy( Predicate *predicate, Verb *verb, Policy *next  )
        : predicate(predicate), verb(verb), next(next) { }
        virtual void destroy(Pool *);
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
    };

    class Program {
    protected:
        Field *TRANSFER_ENCODING;
        Field *CONTENT_LENGTH;
        Field *Expect;
        Field *Range;
        Field *CONNECTION;
        Field *KeepAlive;
        Field *RXDATA;
        Field *EoH;
    
        Predicate *connection_has_close;
        Predicate *connection_has_keepalive;
    
        uint32_t receiveLimit;
    public:
        uint32_t contentLength;
    
        Program()
        : receiveLimit( (20 * 1024) - (4 * 1024) ),
        TRANSFER_ENCODING(NULL), CONTENT_LENGTH(NULL), Expect(NULL), Range(NULL),
        CONNECTION(NULL), KeepAlive(NULL), RXDATA(NULL), EoH(NULL),
        connection_has_close(NULL), connection_has_keepalive(NULL)
        { }
        virtual ~Program() {}
        void * operator new ( std::size_t, Pool * );
        virtual void destroy( Pool * );
        void operator delete (void *) {}
    
        virtual void operator () ( Context *context ) = 0;
    
        void set_TRANSFER_ENCODING( Field *ff ) { TRANSFER_ENCODING = ff; }
        void set_CONTENT_LENGTH( Field *ff ) { CONTENT_LENGTH = ff; }
        void set_Expect( Field *ff ) { Expect = ff; }
        void set_Range( Field *ff ) { Range = ff; }
        void set_CONNECTION( Field *ff ) { CONNECTION = ff; }
        void set_KeepAlive( Field *ff ) { KeepAlive = ff; }
        void set_RXDATA( Field *ff ) { RXDATA = ff; }
        void set_EoH( Field *ff ) { EoH = ff; }
        void set_connection_has_close( Predicate *ff ) { connection_has_close = ff; }
        void set_connection_has_keepalive( Predicate *ff ) { connection_has_keepalive = ff; }

        void setReceiveLimit( uint32_t value ) {
            receiveLimit = value;
        }

        void adjustReceiveLimit( int32_t value ) {
            receiveLimit += value;
        }
    
        Context::HTTPVersion
        httpVersion( Field *field ) {
            if ( field->count == 0 ) return Context::HTTP_0_9;
    
            char *s    = (char *)(field->start);
            int length = field->length;
    
            if ( length != 8 ) return Context::HTTP_0_9;
            char c = *(s+7);
            if ( c == '1' ) return Context::HTTP_1_1;
            if ( c == '0' ) return Context::HTTP_1_0;
            return Context::HTTP_0_9;
        }
    };

    class RequestProgram : public Program {
        Field *METHOD;
        Field *REQUEST_VERSION;
        Predicate *method_is_head;
    
        Verb *block;
    public:
        char *host;
        char *portString;
        uint32_t port;
        RequestProgram( Verb *block )
        : Program(), block(block),
          host(0), portString(0), port(80),
          METHOD(NULL), REQUEST_VERSION(NULL), method_is_head(NULL)
        { }
        virtual ~RequestProgram() {}
        virtual void destroy(Pool *);
    
        virtual void operator () (Context *);
        void set_METHOD( Field *ff ) { METHOD = ff; }
        void set_REQUEST_VERSION( Field *ff ) { REQUEST_VERSION = ff; }
        void set_method_is_head( Predicate *ff ) { method_is_head = ff; }

        void setHost( char *_host ) {
            host = (char *)malloc( strlen(_host)+1 );
            strcpy( host, _host );
        }

        void setPortString( char *_portString ) {
            portString = (char *)malloc( strlen(_portString)+1 );
            strcpy( portString, _portString );
        }

        void setPort( uint32_t _port ) {
            port = _port;
        }
    };

    class ResponseProgram : public Program {
        Field *Upgrade;
        Field *RESPONSE_VERSION;
        Field *RESPONSE_CODE;
    
        Verb *block;
    
    public:
        ResponseProgram( Verb *block )
        : Program(), block(block),
        Upgrade(NULL), RESPONSE_VERSION(NULL), RESPONSE_CODE(NULL)
        { }
        virtual ~ResponseProgram() {}
        virtual void destroy(Pool *);
        void set_Upgrade( Field *ff ) { Upgrade = ff; }
        void set_RESPONSE_VERSION( Field *ff ) { RESPONSE_VERSION = ff; }
        void set_RESPONSE_CODE( Field *ff ) { RESPONSE_CODE = ff; }

        virtual void operator () (Context *);
    };

    bool Initialize( Tcl_Interp *, OPE * );
}
#endif
