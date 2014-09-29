
/** \file Hypercall.cc
 * \brief 
 *
 */

#define _XOPEN_SOURCE 600

#include <sys/user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>  // for mlock

#include <stdint.h>
#include <stdlib.h>

/**
 * Lie about being a Xen tool...
 */
#define __XEN_TOOLS__
#include <xen/xen.h>
#include <xen/sys/privcmd.h>
#include <xen/sysctl.h>
#include <xen/domctl.h>

#include <errno.h>
#include <string.h>
#include <syslog.h>

#include "Hypercall.h"

/**
 */
static int
xen_memalign( void **memptr, size_t alignment, size_t size ) {
    void *address = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0 );
    if ( address == MAP_FAILED ) return ENOMEM;
    *memptr = address;
    return 0;
}

/**
 */
static int
xen_free( void *address, size_t size ) {
    return munmap( address, size );
}

/**
 */
Xen::Hypercall::Hypercall() {
    xen_memalign( (void**)&hypercall, PAGE_SIZE, sizeof(*hypercall) );
    memset( hypercall, 0, sizeof(*hypercall) );
}

/**
 */
Xen::Hypercall::~Hypercall() {
    if ( hypercall == NULL ) {
        syslog( LOG_ERR, "trying to destruct a NULL hypercall" );
        abort();
    }
    xen_free( hypercall, sizeof(*hypercall) );
    hypercall = NULL;
}

/**
 * Call this hypercall with no arguments
 */
bool Xen::Hypercall::send() {
    char buffer[80];
    // open file
    int fd = open("/proc/xen/privcmd", O_RDWR);
    if ( fd == -1 ) {
        syslog( LOG_ERR, "failed to open hypervisor interface : %s",
               strerror_r(errno, buffer, sizeof(buffer)) );
        exit( errno );
    }
    if ( mlock(hypercall, sizeof(*hypercall)) == -1 ) {
        perror( "mlock SysControl" );
    }
    lock();
    fflush( stdout );
    // send request
    int result = ioctl( fd, IOCTL_PRIVCMD_HYPERCALL, (unsigned long)hypercall );
    if ( result == -1 ) {
        /*
         * \todo we get permission denied in Rio
         */
        syslog( LOG_WARNING, "hypercall failed: %s",
               strerror_r(errno, buffer, sizeof(buffer)) );
    }
    // more than once?
    unlock();
    if ( munlock(hypercall, sizeof(*hypercall)) == -1 ) {
        perror( "mlock SysControl" );
    }
    if ( close(fd) == -1 ) {
        perror( "close privcmd" );
    }
    return (result == -1) ? false : true;
}

/**
 */
Xen::SysControl::SysControl( uint32_t cmd ) {
    xen_memalign( (void**)&request, PAGE_SIZE, sizeof(*request) );
    memset( request, 0, sizeof(*request) );

    request->cmd = cmd;
    request->interface_version = XEN_SYSCTL_INTERFACE_VERSION;

    hypercall->op = __HYPERVISOR_sysctl;
    hypercall->arg[0] = (unsigned long)request;
}

/**
 */
Xen::SysControl::~SysControl() {
    if ( request == NULL ) {
        syslog( LOG_ERR, "trying to destruct a NULL SysControl" );
        abort();
    }
    xen_free( request, sizeof(*request) );
    request = NULL;
}

/**
 */
bool Xen::SysControl::lock() {
    if ( mlock(request, sizeof(*request)) == -1 ) {
        perror( "mlock SysControl" );
        exit( 1 );
    }
    return true;
}

/**
 */
bool Xen::SysControl::unlock() {
    if ( munlock(request, sizeof(*request)) == -1 ) {
        perror( "munlock SysControl" );
        exit( 1 );
    }
    return true;
}

/**
 */
Xen::DomControl::DomControl( domid_t domain, uint32_t cmd ) {
    xen_memalign( (void**)&request, PAGE_SIZE, sizeof(*request) );
    memset( request, 0, sizeof(*request) );

    request->cmd = cmd;
    request->domain = domain;
    request->interface_version = XEN_DOMCTL_INTERFACE_VERSION;

    hypercall->op = __HYPERVISOR_domctl;
    hypercall->arg[0] = (unsigned long)request;
}

/**
 */
Xen::DomControl::~DomControl() {
    if ( request == NULL ) {
        syslog( LOG_ERR, "trying to destruct a NULL DomControl" );
        abort();
    }
    xen_free( request, sizeof(*request) );
    request = NULL;
}

/**
 */
bool Xen::DomControl::lock() {
    return mlock(request, sizeof(*request)) != -1;
}

/**
 */
bool Xen::DomControl::unlock() {
    return munlock(request, sizeof(*request)) != -1;
}

/**
 */
Xen::PhysInfo::PhysInfo( struct xen_sysctl_physinfo *_in ) {
    _nr_cpus         = _in->nr_cpus;
    _nr_nodes        = _in->nr_nodes;
    _max_cpu_id      = _in->max_cpu_id;
    _max_node_id     = _in->max_node_id;

    threads_per_core = _in->threads_per_core;
    cores_per_socket = _in->cores_per_socket;
    _sockets         = _nr_cpus / (threads_per_core * cores_per_socket);
    _cpu_khz = _in->cpu_khz;

    _total_pages = _in->total_pages;
    _free_pages  = _in->free_pages;
    _scrub_pages = _in->scrub_pages;
}

/**
 */
Xen::GetPhysInfo::GetPhysInfo() :
Xen::SysControl::SysControl(XEN_SYSCTL_physinfo) {
}

/**
 */
Xen::GetPhysInfo::~GetPhysInfo() {
    // syslog( LOG_NOTICE, "destruct GetPhysInfo" );
}

/**
 */
Xen::PhysInfo* Xen::GetPhysInfo::operator() ( ) {
    if ( Xen::SysControl::send() == false ) {
        syslog( LOG_ERR, "GetPhysInfo failed" );
    }
    Xen::PhysInfo *info = new Xen::PhysInfo( &(request->u.physinfo) );
    return info;
}

/**
 */
Xen::GetDomainInfoList::GetDomainInfoList() :
Xen::SysControl::SysControl(XEN_SYSCTL_getdomaininfolist) {
    // syslog( LOG_NOTICE, "construct GetDomainInfoList" );
}

/**
 */
Xen::GetDomainInfoList::~GetDomainInfoList() {
    // syslog( LOG_NOTICE, "destruct GetDomainInfoList" );
}

/**
 */
int Xen::GetDomainInfoList::operator() ( xen_domctl_getdomaininfo_t *domains, int size ) {
    request->u.getdomaininfolist.first_domain = 0;
    request->u.getdomaininfolist.max_domains = size;
    set_xen_guest_handle( request->u.getdomaininfolist.buffer, domains );
    if ( Xen::SysControl::send() == false ) {
        syslog( LOG_ERR, "GetDomainInfoList failed" );
    }
    int count = request->u.getdomaininfolist.num_domains;
    return count;
}

/**
 */
Xen::VcpuInfo::VcpuInfo( struct xen_domctl_getvcpuinfo *info ) {
    update( info );
}

/**
 */
void Xen::VcpuInfo::update( struct xen_domctl_getvcpuinfo *info ) {
    _vcpu     = info->vcpu;
    _cpu_time = info->cpu_time;
    _cpu      = info->cpu;
    _online   = (info->online  != 0);
    _blocked  = (info->blocked != 0);
    _running  = (info->running != 0);
}

/**
 */
Xen::GetVcpuInfo::GetVcpuInfo( domid_t domain ) :
Xen::DomControl::DomControl(domain, XEN_DOMCTL_getvcpuinfo) {
}

/**
 */
Xen::GetVcpuInfo::~GetVcpuInfo() {
    // syslog( LOG_NOTICE, "destruct GetVcpuInfo" );
}

/**
 */
Xen::VcpuInfo* Xen::GetVcpuInfo::operator() ( int vcpu ) {
    request->u.getvcpuinfo.vcpu = vcpu;
    if ( Xen::DomControl::send() == false ) {
        syslog( LOG_ERR, "GetVcpuInfo failed" );
    }
    Xen::VcpuInfo *info = new Xen::VcpuInfo( &(request->u.getvcpuinfo) );
    return info;
}

/**
 */
Xen::VcpuInfo* Xen::GetVcpuInfo::operator() ( VcpuInfo *info ) {
    request->u.getvcpuinfo.vcpu = info->vcpu();
    if ( Xen::DomControl::send() == false ) {
        syslog( LOG_ERR, "GetVcpuInfo failed" );
    }
    info->update( &(request->u.getvcpuinfo) );
    return info;
}

/**
 */
Xen::DomainCommand::DomainCommand( domid_t domain, uint32_t command )
: DomControl(domain, command) {
}

/**
 */
Xen::DomainCommand::~DomainCommand() {
    // syslog( LOG_NOTICE, "destruct DomainCommand" );
}

/**
 * Should determine errors here.
 */
bool Xen::DomainCommand::operator() () {
    if ( Xen::DomControl::send() == false ) {
        syslog( LOG_ERR, "DomainCommand failed" );
    }
    return true;
}

/**
 */
Xen::PauseDomain::PauseDomain( domid_t domain )
: DomainCommand(domain, XEN_DOMCTL_pausedomain) {
}

/**
 */
Xen::DomainInfo::DomainInfo( struct xen_domctl_getdomaininfo *_in ) {
    domain          = _in->domain;
    flags           = _in->flags;
    tot_pages       = _in->tot_pages;
    max_pages       = _in->max_pages;
    _cpu_time        = _in->cpu_time;
    nr_online_vcpus = _in->nr_online_vcpus;
    max_vcpu_id     = _in->max_vcpu_id;
    memcpy(handle, _in->handle, sizeof(xen_domain_handle_t));
    _vcpu_count = max_vcpu_id + 1;

    size_t size = sizeof(Xen::VcpuInfo*) * _vcpu_count;
    void *address = mmap( 0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0 );
    vcpu = (Xen::VcpuInfo **)address;
}

/**
 */
Xen::DomainInfo::~DomainInfo() {
    /* THIS IS THE DOUBLE FREE */
    /*
    for ( int i = 0 ; i < _vcpu_count ; i++ ) {
        if ( vcpu[i] != 0 ) delete vcpu[i];
    }
    */
    size_t size = sizeof(Xen::VcpuInfo*) * _vcpu_count;
    munmap( vcpu, size );
    vcpu = NULL;
}

/**
 * for each vcpu ID get the VCPU info
 */
void Xen::DomainInfo::probe_vcpus() {
    Xen::GetVcpuInfo request(domain);
    for ( int i = 0 ; i < _vcpu_count ; i++ ) {
        if ( vcpu[i] == NULL ) {
            vcpu[i] = request( i );
        } else {
            // if the pointers are already valid,
            // then update instead of creating new ones
            request( vcpu[i] );
            // if vcpu[i].vcpu() != i  ???err
        }
    }
}

/**
 */
void Xen::DomainInfo::update( struct xen_domctl_getdomaininfo *info ) {
    flags           = info->flags;
    tot_pages       = info->tot_pages;
    max_pages       = info->max_pages;
    _cpu_time        = info->cpu_time;
    nr_online_vcpus = info->nr_online_vcpus;
    max_vcpu_id     = info->max_vcpu_id;
    _vcpu_count = max_vcpu_id + 1;
    probe_vcpus();
/*
    printf( "DOMAIN UUID: " );
    printf( "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
             handle[0],  handle[1],  handle[2],  handle[3],
             handle[4],  handle[5],  handle[6],  handle[7],
             handle[8],  handle[9], handle[10], handle[11],
            handle[12], handle[13], handle[14], handle[15] );
*/
}

/**
 */
uint64_t Xen::DomainInfo::vcpu_time( uint32_t id ) const {
    if ( id > max_vcpu_id ) return 0;
    return vcpu[id]->cpu_time();
}

/**
 */
Xen::GetDomainInfo::GetDomainInfo( domid_t domain ) :
Xen::DomControl::DomControl(domain, XEN_DOMCTL_getdomaininfo) {
}

/**
 */
Xen::DomainInfo* Xen::GetDomainInfo::operator() ( domid_t domain ) {
    request->u.getdomaininfo.domain = domain;
    if ( Xen::DomControl::send() == false ) {
        syslog( LOG_ERR, "GetDomainInfo failed" );
    }
    Xen::DomainInfo *info = new Xen::DomainInfo( &(request->u.getdomaininfo) );
    return info;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
