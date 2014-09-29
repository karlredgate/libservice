
/** \file BIOS.h
 * \brief 
 */

#ifndef _BIOS_H_
#define _BIOS_H_

#include <stdint.h>
#include <unistd.h>
#include <tcl.h>
#include <vector>
#include "UUID.h"
#include "StringList.h"

/**
 */
namespace SMBIOS {

    /**
     * Arguably the structure_type is not necessary since that information
     * is carried as part of the derived class, but it may be convenient.
     */
    class Structure {
    protected:
        uint8_t *data;
        StringList *strings;
        uint8_t structure_type;
        uint8_t header_length;
        const char *string( uint8_t ) const;
    public:
        Structure( void * );
        virtual ~Structure();

        static Structure *Factory( void *address ) {
            return new Structure( address );
        }

        const void *next() const { return strings->end(); }
        inline virtual bool is_processor() const { return false; }
        inline virtual bool is_system() const { return false; }
        inline virtual bool is_chassis() const { return false; }

        inline void *address() const { return data; }
    };

    /**
     */
    class BIOSInformation : public Structure {
    private:
        const char *_vendor, *_version, *_release_date;
    public:
        BIOSInformation( void * );
        virtual ~BIOSInformation() {}

        static Structure *Factory( void *address ) {
            return new BIOSInformation( address );
        }

        inline const char *vendor()        const { return _vendor; }
        inline const char *version()       const { return _version; }
        inline const char *release_date()  const { return _release_date; }
    };

    /**
     */
    class System : public Structure {
    private:
    public:
        System( void * );
        virtual ~System() {}

        static Structure *Factory( void *address ) {
            return new System( address );
        }

        inline virtual bool is_system() const { return true; }
        const char *manufacturer()  const;
        const char *product_name()  const;
        const char *serial_number() const;
        uint8_t * uuid_string() const;
        UUID *uuid();
    };

    /**
     * need to report features, and type
     */
    class BaseBoard : public Structure {
    private:
    public:
        BaseBoard( System * );
        BaseBoard( void * );
        virtual ~BaseBoard() {}

        static Structure *Factory( void *address ) {
            return new BaseBoard( address );
        }

        const char *  manufacturer()     const;
        const char *  product_name()     const;
        const char *  version()          const;
        const char *  serial_number()    const;
        const char *  asset_tag()        const;
        const char *  chassis_location() const;
        uint8_t features()         const;
        uint8_t type_id()          const;
    };

    /**
     * add chassis_state enum of
     * 1=other, 2=unknown 3=safe 4=warning 5=critical 6=non-recoverable
     * security state is
     * 1=other 2=unknown 3=none 4='external interface locked out' 5='external interface enabled'
     */
    class Chassis : public Structure {
    private:
    public:
        Chassis( void * );
        virtual ~Chassis() {}

        static Structure *Factory( void *address ) {
            return new Chassis( address );
        }

        inline virtual bool is_chassis() const { return true; }

        const char *  manufacturer()          const;
        const char *  version()               const;
        const char *  serial_number()         const;
        const char *  asset_tag()             const;
        uint8_t chassis_type_id()       const;
        uint8_t chassis_lock_id()       const;
        uint8_t bootup_state_id()       const;
        uint8_t power_supply_state_id() const;
        uint8_t thermal_state_id()      const;
        uint8_t security_state_id()     const;
        uint8_t power_cords()           const;

        const char * chassis_name() const;
    };

    /**
     */
    class Processor : public Structure {
    private:
        char *_socket_designation, *_manufacturer, *_version, *_serial_number, *_asset_tag, *_part_number;
        uint8_t voltage_id, max_freq_id, current_freq_id, upgrade_id;
        bool populated;
    public:
        Processor( void * );
        virtual ~Processor() {}
        static Structure *Factory( void *address ) {
            return new Processor( address );
        }
        inline virtual bool is_processor() const { return true; }
        const char *socket_designation()  const;
        const char *manufacturer()        const;
        const char *version()             const;
        const char *serial_number()       const;
        const char *asset_tag()           const;
        const char *part_number()         const;

        bool is_populated() const;
        bool is_enabled() const;
        bool is_user_disabled() const;
        bool is_bios_disabled() const;
        bool is_idle() const;

        inline bool is_disabled()   const { return is_user_disabled() or is_bios_disabled(); }
        inline bool not_populated() const { return not is_populated(); }
        inline bool not_enabled()   const { return not is_enabled(); }
        inline bool not_idle()      const { return not is_idle(); }
    };

    /**
     */
    class Table {
    private:
        void *address;
        Structure **dmi;
        uint16_t _count;
        std::vector<Processor*> sockets;
    public:
        Structure *bios, *system, *baseboard, *chassis;
        Table( uint8_t *, uint16_t, uint16_t );
        ~Table();
        inline std::vector<Processor*>& processors() { return sockets; }
    };

    /**
     */
    class Header {
    private:
        Table *table;
        uint8_t *dmi_address;
        uint16_t dmi_length;
        uint16_t dmi_number;
        uint16_t dmi_version;
        uint8_t major_version;
        uint8_t minor_version;
    protected:
    public:
        Header();
        ~Header();

        inline BIOSInformation& bios() const { return *( (BIOSInformation*)table->bios      ); }
        inline System&        system() const { return *(          (System*)table->system    ); }
        inline BaseBoard&  baseboard() const { return *(       (BaseBoard*)table->baseboard ); }
        inline Chassis&      chassis() const { return *(         (Chassis*)table->chassis   ); }

        inline std::vector<Processor*>& processors() const { return table->processors(); }

        void probe( uint8_t * );
        // static Header *locate();
        static uint8_t *locate();
    };

}

/**
 */
namespace BIOS {
    bool Initialize( Tcl_Interp * );
}

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
