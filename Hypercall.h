
/** \file Hypercall.h
 * \brief Interface to Dom-0 Hypercalls.
 *
 * \todo add namespace and initialization function
 */

#ifndef _HYPERCALL_H_
#define _HYPERCALL_H_

#include <stdint.h>
#include <unistd.h>
#include <tcl.h>

/**
 * Lie about being a Xen tool...
  */
#define __XEN_TOOLS__
#include <xen/xen.h>
#include <xen/sys/privcmd.h>
#include <xen/sysctl.h>
#include <xen/domctl.h>

/**
 */
namespace Xen {

    /**
     */
    class Hypercall {
    private:
    protected:
        privcmd_hypercall_t *hypercall;
        bool send();
        virtual bool lock() = 0;
        virtual bool unlock() = 0;
    public:
        Hypercall();
        virtual ~Hypercall();
    };

    /**
     * struct xen_sysctl is defined in xen/sysctl.h in the Dom-0 linux sparse tree.
     * It contains a command int, the interface version, and in/out structures for
     * the different request types.
     */
    class SysControl : public Hypercall {
    private:
    protected:
        struct xen_sysctl *request;
        virtual bool lock();
        virtual bool unlock();
    public:
        SysControl( uint32_t );
        virtual ~SysControl();
    };

    /**
     * I also want to add capabilities here
     */
    class PhysInfo {
    private:
        // struct xen_sysctl_physinfo info;
        uint32_t _nr_cpus;
        uint32_t _nr_nodes;
        uint32_t _max_cpu_id;
        uint32_t _max_node_id;
        uint32_t threads_per_core;
        uint32_t cores_per_socket;
        uint32_t _sockets;
        uint32_t _cpu_khz;
        uint64_t _total_pages;
        uint64_t _free_pages;
        uint64_t _scrub_pages;
    public:
        PhysInfo( struct xen_sysctl_physinfo * );
        uint32_t threads()     const { return threads_per_core; }
        uint32_t cores()       const { return cores_per_socket; }
        uint32_t sockets()     const { return _sockets; }
        uint32_t pages()       const { return _total_pages; }

        /* Not the main interface - provided for extra detail */
        uint32_t nr_cpus()     const { return _nr_cpus; }
        uint32_t nr_nodes()    const { return _nr_nodes; }
        uint32_t max_cpu_id()  const { return _max_cpu_id; }
        uint32_t max_node_id() const { return _max_node_id; }
        uint32_t cpu_khz()     const { return _cpu_khz; }
        uint32_t free_pages()  const { return _free_pages; }
        uint32_t scrub_pages() const { return _scrub_pages; }
    };

    /**
     */
    class GetPhysInfo : public SysControl {
    public:
        GetPhysInfo();
        virtual ~GetPhysInfo();
        PhysInfo* operator() ();
    };

    /**
     */
    class GetDomainInfoList : public SysControl {
    public:
        GetDomainInfoList();
        virtual ~GetDomainInfoList();
        int operator() ( xen_domctl_getdomaininfo_t*, int );
    };

    /**
     * Parent class for hypercalls that control domain state.
     */
    class DomControl : public Hypercall {
    protected:
        struct xen_domctl *request;
        virtual bool lock();
        virtual bool unlock();
    public:
        DomControl( domid_t, uint32_t );
        virtual ~DomControl();
    };

    /**
     * The base object adds the interface to call the functor
     */
    class DomainCommand : public DomControl {
    public:
        DomainCommand( domid_t, uint32_t );
        virtual ~DomainCommand();
        bool operator () ();
    };

    /**
     */
    class DestroyDomain : public DomainCommand {
    public:
        DestroyDomain( domid_t domain ) : DomainCommand(domain, XEN_DOMCTL_destroydomain) { }
        virtual ~DestroyDomain() {}
    };

    /**
     */
    class PauseDomain : public DomainCommand {
    public:
        PauseDomain( domid_t );
        virtual ~PauseDomain() { }
    };

    /**
     */
    class UnpauseDomain : public DomainCommand {
    public:
        UnpauseDomain( domid_t domain ) : DomainCommand(domain, XEN_DOMCTL_unpausedomain) { }
        virtual ~UnpauseDomain() {}
    };

    /**
     */
    class ResumeDomain : public DomainCommand {
    public:
        ResumeDomain( domid_t domain ) : DomainCommand(domain, XEN_DOMCTL_resumedomain) { }
        virtual ~ResumeDomain() {}
    };

    /**
     */
    class VcpuInfo {
    private:
        uint32_t _vcpu;
        uint64_t _cpu_time;
        uint32_t _cpu;
        bool _online;
        bool _blocked;
        bool _running;
    public:
        VcpuInfo( struct xen_domctl_getvcpuinfo * );
        uint64_t cpu_time() const { return _cpu_time; }
        void update( struct xen_domctl_getvcpuinfo * );
        uint32_t vcpu() const { return _vcpu; }
    };

    /**
     */
    class GetVcpuInfo : public DomControl {
    private:
    public:
        GetVcpuInfo( domid_t );
        virtual ~GetVcpuInfo();
        VcpuInfo* operator() ( int );
        VcpuInfo* operator() ( VcpuInfo* );
    };

    /**
     */
    class DomainInfo {
    private:
        domid_t domain;
        uint32_t flags;
        uint64_t tot_pages;
        uint64_t max_pages;
        uint64_t _cpu_time;
        uint32_t nr_online_vcpus;
        uint32_t max_vcpu_id;
        uint32_t ssidref;
        // typedef uint8_t xen_domain_handle_t[16];
        xen_domain_handle_t handle;
        int _vcpu_count;
        VcpuInfo **vcpu;
    public:
        DomainInfo( struct xen_domctl_getdomaininfo * );
        ~DomainInfo();
        void probe_vcpus();
        void update( struct xen_domctl_getdomaininfo * );
        inline uint64_t cpu_time() const { return _cpu_time; }
        inline int vcpu_count() const { return _vcpu_count; }
        uint64_t vcpu_time( uint32_t ) const;
    };

    /**
     */
    class GetDomainInfo : public DomControl {
    private:
    public:
        GetDomainInfo( domid_t );
        DomainInfo* operator() ( domid_t );
    };

    bool Initialize( Tcl_Interp * );

}

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
