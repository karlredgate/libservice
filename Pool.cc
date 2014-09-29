
#include "tcl.h"
#include "Pool.h"

namespace Service {

    void
    Pool::initialize( uint32_t object_size ) {
        UINFO( 5, "initialize pool for size "
               << dec << object_size << endl );
        size = object_size;
        // Check if object_size is word aligned
        start = (uint8_t *)allocateMmsMemory( size * count, 0,
             "ObjectPolicyPool", current_process(), 0
        );
        if ( start == 0 ) {
            UERROR( "Pool: MMS error allocating memory" << endl );
        }
        for ( int i = 0 ; i < MAPS ; i++ ) {
            map[i] = 0xFFFFFFFF;
        }
        next = new Pool;
    }

    void *
    Pool::allocate( uint32_t object_size ) {
        if ( size == 0 )  initialize( object_size );
        if ( object_size != size ) {
            UINFO( 9, "Pool: current=" << dec << size
                   << " need=" << dec << object_size
                   << " : skipping to next pool" << endl );
            return next->allocate( object_size );
        }
        UINFO( 9, "Pool: - found size=" << dec << size
               << " : starts at 0x" << hex << (uint32_t)start << endl );
        for ( int word = 0 ; word < MAPS ; word++ ) {
            if ( map[word] == 0 ) {
                UINFO( 9, "Pool: -- map[" << dec << word << "] is empty" << endl );
                continue;
            }
            UINFO( 9, "Pool: -- map[" << dec << word << "] has space" << endl );
            for ( int bit = 0 ; bit < 32 ; bit++ ) {
                uint32_t mask = 1 << bit;
                if ( (map[word] & mask) == 0 ) {
                    UINFO( 9, "Pool: --- bit " << dec << bit << " is used" << endl );
                    continue;
                }
                int entry = ((word * 32) + bit);
                void *address = start + (entry * size);
            
                UINFO( 9, "Pool: --- map[" << dec << word << "] == 0x"
                       << hex << map[word] << " using bit " << dec << bit
                       << " [mask=0x" << hex << mask << "]"
                       << endl );
            
                UINFO( 9, "Pool: --- allocate " << dec << size
                       << " byte entry #" << dec << entry
                       << " at offset 0x" << hex << (entry*size)
                       << " [addr=0x" << hex << address << "]"
                       << endl );
            
                map[word] &= ~mask;
                UINFO( 6, "Pool: --- map[" << dec << word << "] is now 0x"
                       << hex << (map[word]) << endl );
                return address;
            }
            
        }
        UINFO( 9, "Pool: - current pool is fully allocated,"
                  " looking in the next pool" << endl );
        return next->allocate( object_size );
        
    }

    void
    Pool::free( void *object ) {
        uint32_t length = count * size;
        uint8_t *end = (start + length) - 1;
        UINFO( 9, "Pool: find 0x" << hex << (uint32_t)object
               << " in pool [0x" << hex << (uint32_t)start
                  << "..0x" << hex << (uint32_t)end << "]" << endl );
    
        if ( (object < start) || (object > end) ) {
            if ( next == 0 ) {
                UERROR( "Pool: Object 0x"
                    << hex << (uint32_t)object
                    << " not in pool" << endl );
                return;
            }
            UINFO( 9, "Pool: - object not in current pool, try next" << endl );
            next->free( object );
            return;
        }
    
        uint8_t *address = (uint8_t*)object;
        uint32_t offset = address - start;
        int entry = offset / size;
        int word  = entry / 32;
        int bit   = entry % 32;
        uint32_t mask = (1<<bit);
    
        UINFO( 9, "Pool: - found entry " << dec << entry
               << " at offset 0x" << hex << offset
               << " (set bit " << dec << bit << " of map[" << dec << word << "])"
               << endl );
        UINFO( 9, "Pool: -- mask 0x" << hex << mask << endl );
    
        if ( map[word] & mask ) {
            UINFO( 9, "Pool: map[" << dec << word << "] is 0x"
                   << hex << (map[word]) << endl );
            UINFO( 9, "Pool: mask is 0x"
                   << hex << mask << endl );
            UERROR( "Pool: object 0x"
                    << hex << (uint32_t)object
                    << " already freed" << endl );
        }
        map[word] |= mask;
        UINFO( 5, "Pool: - object 0x" << hex << (uint32_t)object
               << " freed - map[" << dec << word << "] is now 0x"
               << hex << (map[word])
               << endl );
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
