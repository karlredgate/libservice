
#ifndef _OBJECT_PRIMITIVES_H_
#define _OBJECT_PRIMITIVES_H_

#include <stdint.h>
#include <cstdlib>
#include "tcl.h"
#include "crc32.h"

namespace Service {

    class Predicate {
    public:
        virtual ~Predicate() {}
        virtual bool operator () ( Context * ) = 0;
        void * operator new ( std::size_t, Pool * );
        void operator delete ( void * ) {}
        virtual void destroy(Pool *);
    };

    class AnnotatedPredicate {
    public:
        struct Annotation {
            bool evaluated : 1;
            bool result    : 1;
        } annotation;
        virtual ~AnnotatedPredicate() {}
        virtual bool operator () ( Context * ) = 0;
    };

    class T : public Predicate {
    public:
        T() { }
        virtual ~T() {}
        virtual void destroy(Pool *pool) {}
        virtual bool
        operator () ( Context *context ) { return true; }
    };

    class F : public Predicate {
    public:
        F() { }
        virtual ~F() {}
        virtual void destroy(Pool *pool) {}
        virtual bool
        operator () ( Context *context ) { return false; }
    };

    class Matches : public Predicate {
        StringCoercion *ff;
        Glob *speglob;
    public:
        Matches( StringCoercion *ff, Glob *glob )
        : ff(ff), speglob(glob) {}
        virtual ~Matches() {}
        virtual bool operator() ( Context * );
        virtual void destroy(Pool *);
    };

    class present : public Predicate {
        StringCoercion *coercion;
    public:
        present( StringCoercion *coercion ) : coercion(coercion) {}
        virtual ~present() { }
        virtual bool operator() ( Context * ) ;
        virtual void destroy( Pool * );
    };

    class absent : public Predicate {
        StringCoercion *coercion;
    public:
        absent( StringCoercion *coercion ) : coercion(coercion) {}
        virtual ~absent() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context * );
    };

    class i_eq_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_eq_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_eq_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_eq_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) == (*rhs)(context) );
        }
    };

    class i_eq_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_eq_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_eq_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_eq_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) == rhs );
        }
    };
    
    class i_ne_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_ne_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_ne_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_ne_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) != (*rhs)(context) );
        }
    };

    class i_ne_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_ne_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_ne_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_ne_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) != rhs );
        }
    };
    
    class i_lt_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_lt_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_lt_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_lt_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) < (*rhs)(context) );
        }
    };

    class i_lt_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_lt_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_lt_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_lt_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) < rhs );
        }
    };
    
    class i_gt_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_gt_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_gt_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_gt_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) > (*rhs)(context) );
        }
    };

    class i_gt_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_gt_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_gt_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_gt_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) > rhs );
        }
    };
    
    class i_le_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_le_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_le_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_le_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) <= (*rhs)(context) );
        }
    };

    class i_le_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_le_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_le_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_le_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) <= rhs );
        }
    };
    
    class i_ge_r_r : public Predicate {
        IntegerCoercion *lhs;
        IntegerCoercion *rhs;
    public:
        i_ge_r_r( IntegerCoercion *lhs, IntegerCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_ge_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::i_ge_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) >= (*rhs)(context) );
        }
    };

    class i_ge_r_i : public Predicate {
        IntegerCoercion *lhs;
        uint32_t rhs;
    public:
        i_ge_r_i( IntegerCoercion *lhs, uint32_t rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~i_ge_r_i() { }
        virtual void destroy(Pool *pool) {
            UINFO( 8, "Predicate::i_ge_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            return ( (*lhs)(context) >= rhs );
        }
    };
    
    class s_eq_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_eq_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_eq_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_eq_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->eq(b) );
        }
    };

    class s_eq_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_eq_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_eq_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_eq_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->eq(rhs) );
        }
    };
    
    class s_ne_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_ne_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_ne_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_ne_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->ne(b) );
        }
    };

    class s_ne_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_ne_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_ne_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_ne_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->ne(rhs) );
        }
    };
    
    class s_lt_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_lt_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_lt_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_lt_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->lt(b) );
        }
    };

    class s_lt_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_lt_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_lt_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_lt_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->lt(rhs) );
        }
    };
    
    class s_gt_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_gt_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_gt_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_gt_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->gt(b) );
        }
    };

    class s_gt_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_gt_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_gt_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_gt_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->gt(rhs) );
        }
    };
    
    class s_le_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_le_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_le_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_le_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->le(b) );
        }
    };

    class s_le_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_le_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_le_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_le_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->le(rhs) );
        }
    };
    
    class s_ge_r_r : public Predicate {
        StringCoercion *lhs;
        StringCoercion *rhs;
    public:
        s_ge_r_r( StringCoercion *lhs, StringCoercion *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_ge_r_r() {}
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_ge_r_r: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            String *b = (*rhs)(context);
            return ( a->ge(b) );
        }
    };

    class s_ge_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_ge_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_ge_r_i() { }
        virtual void destroy( Pool *pool ) {
            UINFO( 8, "Predicate::s_ge_r_i: destroy" << endl );
            if ( lhs ) lhs->destroy( pool );
            if ( rhs ) rhs->destroy( pool );
            Predicate::destroy( pool );
        }
        virtual bool operator() ( Context *context ) {
            String *a = (*lhs)(context);
            return ( a->ge(rhs) );
        }
    };
    
    class s_prefix_r_i : public Predicate {
        StringCoercion *lhs;
        String *rhs;
    public:
        s_prefix_r_i( StringCoercion *lhs, String *rhs )
        : lhs(lhs), rhs(rhs)  {}
        virtual ~s_prefix_r_i() {}
        virtual void destroy( Pool * );
        virtual bool operator() ( Context * );
    };

    class Contains : public Predicate {
        IntegerCoercion *coercion;
        uint32_t mask;
    public:
        Contains( IntegerCoercion *coercion, uint32_t mask )
        : coercion(coercion), mask(mask) {}
        virtual ~Contains() {}
        virtual void destroy( Pool * );
        virtual bool operator() ( Context * );
    };

    class BinaryLogic : public Predicate {
    protected:
        Predicate *lhs;
        Predicate *rhs;
    public:
        BinaryLogic( Predicate *lhs, Predicate *rhs )
        : lhs(lhs), rhs(rhs) {}
        virtual ~BinaryLogic() {}
        virtual void destroy(Pool *);
    };

    class OR : public BinaryLogic {
    public:
        OR( Predicate *lhs, Predicate *rhs )
        : BinaryLogic(lhs, rhs) {}
        virtual bool operator() ( Context * );
        virtual void destroy(Pool *);
    };

    class NOR : public BinaryLogic {
    public:
        NOR( Predicate *lhs, Predicate *rhs )
        : BinaryLogic(lhs, rhs) {}
        virtual void destroy(Pool *);
        virtual bool operator() ( Context * );
    };

    class AND : public BinaryLogic {
    public:
        AND( Predicate *lhs, Predicate *rhs )
        : BinaryLogic(lhs, rhs) {}
        virtual void destroy(Pool *);
        virtual bool operator() ( Context * );
    };

    class NAND : public BinaryLogic {
    public:
        NAND( Predicate *lhs, Predicate *rhs )
        : BinaryLogic(lhs, rhs) {}
        virtual void destroy(Pool *);
        virtual bool operator() ( Context * );
    };

    class NOT : public Predicate {
        Predicate *operand;
    public:
        NOT( Predicate *operand ) : operand(operand) { }
        virtual ~NOT() {}
        virtual bool operator() ( Context * );
        virtual void destroy(Pool *);
    };

    class AddressMatches : public Predicate {
        uint32_t matchAddress;
        uint32_t mask;
    public:
        AddressMatches( uint32_t addr, uint32_t bits ) {
            mask = 0xffffffff << (32-bits);
            matchAddress = (addr & mask);
        }
        virtual ~AddressMatches() {}
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class LocationMatchAllPorts : public Predicate {
        Field *field;
    public:
        LocationMatchAllPorts( Field *field ) : field(field) { }
        virtual ~LocationMatchAllPorts() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class LocationMatchOnePortNeeded : public Predicate {
        Field *field;
    public:
        LocationMatchOnePortNeeded( Field *field ) : field(field) { }
        virtual ~LocationMatchOnePortNeeded() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class ConnectionDelete : public Predicate {
    public:
        ConnectionDelete() { }
        virtual ~ConnectionDelete() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class ConnectionInsert : public Predicate {
    public:
        ConnectionInsert() { }
        virtual ~ConnectionInsert() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class SetCookieInsert : public Predicate {
    public:
        SetCookieInsert() { }
        virtual ~SetCookieInsert() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class IsPassive : public Predicate {
    public:
        IsPassive() { }
        virtual ~IsPassive() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };

    class SSLCipherInsert : public Predicate {
    public:
        SSLCipherInsert() { }
        virtual ~SSLCipherInsert() { }
        virtual void destroy(Pool *);
        virtual bool operator() ( Context *context );
    };
    
    bool Initialize( Tcl_Interp *, OPE * );
}
#endif
