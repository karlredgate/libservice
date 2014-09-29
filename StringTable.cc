
#include "tcl.h"
#include "Pool.h"

static int
stbl_obj( ClientData data,
        Tcl_Interp *interp,
        int objc, Tcl_Obj * CONST *objv )
{
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "command arg" );
        return TCL_ERROR;
    }
    
    using namespace ObjectProcessing;
    StringTable *table = (StringTable *)data;
    char *command = Tcl_GetStringFromObj( objv[1], NULL );
    if ( Tcl_StringMatch(command, "add") ) {
        char *s = Tcl_GetStringFromObj( objv[2], NULL );
        int index = table->store( s );
        Tcl_SetObjResult( interp, Tcl_NewLongObj(index) );
        return TCL_OK;
    }
    if ( Tcl_StringMatch(command, "find") ) {
        char *s = Tcl_GetStringFromObj( objv[2], NULL );
        int index = table->find( s );
        Tcl_SetObjResult( interp, Tcl_NewLongObj(index) );
        return TCL_OK;
    }
    if ( Tcl_StringMatch(command, "get") ) {
        int index;
        Tcl_GetIntFromObj( interp, objv[2], &index );
        char *s = (*table)[index];
        Tcl_Obj *result = Tcl_NewStringObj( s, -1 );
        Tcl_SetObjResult( interp, result );
        return TCL_OK;
    }
    
    Tcl_SetResult( interp, "unknown command", TCL_STATIC );
    return TCL_ERROR;
}
static int
stbl_cmd( ClientData data,
        Tcl_Interp *interp,
        int objc, Tcl_Obj * CONST *objv )
 {
    if ( objc != 3 ) {
        Tcl_ResetResult( interp );
        Tcl_WrongNumArgs( interp, 1, objv, "name entries" );
        return TCL_ERROR;
    }
    
    using namespace ObjectProcessing;
    char *name = Tcl_GetStringFromObj( objv[1], NULL );
    int size;
    Tcl_GetIntFromObj( interp, objv[2], &size );
    StringTable *table = new StringTable( size );
    Tcl_CreateObjCommand(
        interp, name, stbl_obj, (ClientData)table, 0
    );
    Tcl_SetResult( interp, name, TCL_VOLATILE );
    return TCL_OK;
}

namespace Service {

    void *
    StringTable::Storage::operator new ( size_t size, size_t space ) {
        void *address = allocateMmsMemory(
            space, 0, "Strings",
            current_process(), 0
        );
        if ( address == 0 ) {
            UERROR( "StringTable::Storage: MMS error allocating memory" << endl );
        }
        long *s = (long *)address;
        *s = size;
        return address;
    }
    char *
    StringTable::Storage::store( char *s ) {
        char *start = (char *)this;
        register int remaining = (start + size) - dot;
        char *current = dot;
        while ( remaining > 0 ) {
            remaining--;
            if ( (*dot++ = *s++) == '\0' ) break;
        }
        return current;
    }
    void
    StringTable::Storage::grow() {
        next = new (size) Storage;
        dot = next->data();
    }
    StringTable::StringTable( int size )
    : next(0), size(size) {
        table = (Entry *)allocateMmsMemory(
            (size * sizeof(Entry)),
            0, "StringTable",
            current_process(), 0
        );
        if ( table == 0 ) {
            UERROR( "StringTable: MMS error allocating memory" << endl );
        }
        storage = new (512 * 1024) Storage;
        // storage = new (256) Storage;
        remaining = 512 * 1024;
        dot = storage->data();
    }
    void StringTable::grow() {
        UINFO( 5, "Growing StringTable "
               << dec << size << " => " << (size*2) << endl );
        Entry *old = table;
        table = (Entry *)allocateMmsMemory(
            ( (size*2) * sizeof(Entry) ),
            0, "StringTable",
            current_process(), 0
        );
        if ( table == 0 ) {
            UERROR( "StringTable: MMS error allocating memory for grow()" << endl );
        }
        for ( int i = 0 ; i < next ; i++ ) {
            table[i] = old[i];
        }
        freeMmsMemory( (char *)old, current_process() );
        size *= 2;
    }
    bool StringTable::equal( int index, char *subject ) {
        register char *current = table[index].s;
        register char c;
        do {
            c = *current++;
            if ( c != *subject++ ) return false;
        } while ( c != '\0' );
        return true;
    }
    int StringTable::find( char *s ) {
        for ( int i = 0 ; i < next ; i++ ) {
            if ( equal(i, s) ) return i;
        }
        return -1;
    }
    int StringTable::store( char *s ) {
        int index = find( s );
        if ( index != -1 ) return index;
        if ( next >= size ) grow();
        table[next].s = dot;
        while ( remaining > 0 ) {
            remaining--;
            if ( (*dot++ = *s++) == '\0' ) break;
        }
        // check remaining
        return next++;
    }
    void StringTable::release( int index ) {
        if ( table[index].count < 1 ) {
            UERROR( "Decrement use count of StringTable entry that is 0" << endl );
        }
        table[index].count--;
    }
    void StringTable::release( char *s ) {
        int index = find( s );
        if ( index != -1 ) {
            UERROR( "Release a non-existent StringTable entry \"" 
                    << s << "\"" << endl );
            return;
        }
        release( index );
    }
    char *
    StringTable::operator [] ( int index ) {
        if ( index <    0 ) return NULL;
        if ( index < next ) return table[index].s;
        return NULL;
    }

    String::String( Tcl_Interp *interp, Tcl_Obj *obj ) {
        Tcl_CmdInfo info;
        if ( Tcl_GetCommandInfo(interp, "st1", &info) == 0 ) {
            UERROR( "StringTable internal error: "
                    << Tcl_GetStringResult(interp) << endl );
        }
        StringTable& table = *((StringTable *)(info.objClientData));
        int len;
        char *s = Tcl_GetStringFromObj( obj, &len );
        int index = table.store( s );
        length = len;
        start = table[index];
        allocated = false;
        count = 1;
    }
    String::~String() {
        if ( allocated ) {
            // Maybe this should be an error ...
            UINFO( 8, "String::~String: free( 0x" << hex << (uint32_t)start
                   << " )" << endl );
            // free( start );
        }
    }
    void *
    String::operator new ( std::size_t size, Pool *pool ) {
        UINFO( 9, "String: allocate new from pool with size=" << dec << size << endl );
        void *address = pool->allocate( size );
        return address;
    }
    void String::destroy( Pool *pool ) {
        UINFO( 8, "String: destroy" << endl );
        void *address = this;
        if ( allocated ) {
            UINFO( 8, "String: - string (0x" << hex << address
                   << ") is allocated on heap or stack,"
                   " free(start), don't destroy"
                   << endl );
            free( start );
            return;
        }
        pool->free( address );
    }

    bool Initialize( Tcl_Interp *interp, OPE *ope ) {
        Tcl_CreateObjCommand(interp, "stbl", stbl_cmd, (ClientData)(ope), NULL);
        
        char *stbl_script = 
        "stbl st1 16\n"
        "\n";

        int retcode = Tcl_EvalEx( interp, stbl_script, -1, TCL_EVAL_GLOBAL );
        if ( retcode != TCL_OK ) {
            UERROR( "StringTable failed to initialize" << endl );
        }

        return true;
    }

}
