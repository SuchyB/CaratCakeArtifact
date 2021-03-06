/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2018, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2018, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/cpuid.h>
#include <nautilus/cpu.h>
#include <nautilus/msr.h>
#include <nautilus/numa.h>
#include <nautilus/errno.h>
#include <nautilus/list.h>
#include <nautilus/percpu.h>
#include <nautilus/math.h>
#include <nautilus/paging.h>
#include <nautilus/mm.h>
#include <nautilus/atomic.h>
#include <nautilus/naut_string.h>
#include <nautilus/multiboot2.h>
#include <dev/apic.h>


#ifndef NAUT_CONFIG_DEBUG_NUMA
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define NUMA_PRINT(fmt, args...) printk("NUMA: " fmt, ##args)
#define NUMA_DEBUG(fmt, args...) DEBUG_PRINT("NUMA: " fmt, ##args)
#define NUMA_ERROR(fmt, args...) ERROR_PRINT("NUMA: " fmt, ##args)
#define NUMA_WARN(fmt, args...)  WARN_PRINT("NUMA: " fmt, ##args)


static inline uint32_t 
next_pow2 (uint32_t v)
{
    v--;
    v |= v>>1;
    v |= v>>2;
    v |= v>>4;
    v |= v>>8;
    return ++v;
}

/* logical processor count */
static uint8_t
get_max_id_per_pkg (void)
{
    cpuid_ret_t ret;
    cpuid(1, &ret);
    return ((ret.b >> 16) & 0xff);
}


static uint8_t
amd_get_cmplegacy (void)
{
    cpuid_ret_t ret;
    cpuid(CPUID_AMD_FEATURE_INFO, &ret);
    return ret.c & 0x2;
}

static uint8_t
has_leafb (void)
{
    cpuid_ret_t ret;
    cpuid_sub(11, 0, &ret);
    return (ret.b != 0);
}

static uint8_t 
has_htt (void)
{
    cpuid_ret_t ret;
    cpuid(1, &ret);
    return !!(ret.d & (1<<28));
}


static void
intel_probe_with_leafb (struct nk_topo_params *tp)
{
	cpuid_ret_t ret;
	unsigned subleaf = 0;
	unsigned smt_width = 0;
	unsigned smtcore_width = 0;
	unsigned level_width = 0;
	uint8_t level_type = 0;
	uint8_t found_core = 0;
	uint8_t found_smt = 0;

	// iterate through the 0xb subleafs until EBX == 0
	do {
		cpuid_sub(0xb, subleaf, &ret);
		
		// this is an invalid subleaf
		if (ret.b == 0) {
			break;
		}

		level_type  = (ret.c >> 8) & 0xff;
		level_width = ret.a & 0xf;

		switch (level_type) {
			case 0x1: // level type is SMT, which means level_width holds the SMT mask width
				smt_width = level_width;
				found_smt = 1;
				break;
			case 0x2: // level type is SMT + core, which means level_width holds the width of both
				smtcore_width = level_width;
				found_core = 1;
				break;
			default:
				break;
		}
		subleaf++;
	} while (1);
	
	// we saw both core and smt levels, we have hyperthreads
	if (found_core && found_smt) {
		tp->core_bits = smtcore_width;
		tp->smt_bits  = smt_width;
	} else if (!found_core && found_smt) { // only saw threads, 
		tp->core_bits = 0;
		tp->smt_bits  = smt_width;
	} else {
		NUMA_WARN("Intel chip has strange topology...\n");
	}
}

static void
intel_probe_with_leaves14 (struct nk_topo_params * tp)
{
    cpuid_ret_t ret;
    if (cpuid_leaf_max() >= 0x4) {
        NUMA_DEBUG("Intel probing topo with leaves 1 and 4\n");
        cpuid_sub(4, 0, &ret);
        uint32_t eax_bits = (ret.a >> 26) & 0xff;
        tp->smt_bits  = next_pow2(get_max_id_per_pkg());
        tp->core_bits = next_pow2(eax_bits >> tp->smt_bits);
    } else { 
        NUMA_DEBUG("Intel probing topo using leaf 1 only\n");
        tp->smt_bits  = next_pow2(get_max_id_per_pkg());
        tp->core_bits = 0;
    }
}

static void
amd_get_topo_secondary (struct nk_topo_params * tp)
{
    cpuid_ret_t ret;
    NUMA_DEBUG("Attempting AMD topo probe\n");

    cpuid(CPUID_AMD_BASIC_INFO, &ret);

    unsigned num_cores;
    unsigned max_num_cores;
    unsigned threads_per_core;
    unsigned cores_per_proc;

    if (ret.a >= CPUID_AMD_EXT_INFO) {
        NUMA_DEBUG("AMD probing topo using CPUID fn 8000_0008\n");

        cpuid(CPUID_AMD_EXT_INFO, &ret);

        tp->core_bits = (ret.c >> 12) & 0xf;

        num_cores = (ret.c & 0xff) + 1;

        if (tp->core_bits) {
            max_num_cores  = 1 << tp->core_bits;
        } else {
            max_num_cores = num_cores;
        }

        cores_per_proc   = num_cores;
        threads_per_core = amd_get_cmplegacy() ? 1 : 
                                                 get_max_id_per_pkg()/cores_per_proc;
        NUMA_DEBUG("%u Cores per proc, %u threads per core (max logical count=%u)\n", cores_per_proc, threads_per_core, 
                get_max_id_per_pkg());

        tp->smt_bits = ilog2(threads_per_core);

    } else {
        NUMA_DEBUG("AMD probing topo using CPUID FN 0x0000_0001\n");
        tp->core_bits = next_pow2(get_max_id_per_pkg());
        tp->smt_bits  = 0;
    }
}


static void 
intel_get_topo_secondary (struct nk_topo_params * tp)
{
	if (cpuid_leaf_max() >= 0xb && has_leafb()) {
		intel_probe_with_leafb(tp);
	}  else {
		intel_probe_with_leaves14(tp);
	}
}

static inline void
get_topo_secondary (struct nk_topo_params * tp)
{
    if (nk_is_amd()) {
        amd_get_topo_secondary(tp);
    } else {
        intel_get_topo_secondary(tp);
    }
}


static void
get_topo_params (struct nk_topo_params * tp)
{
    if (has_htt()) {

        get_topo_secondary(tp);

    } else {
        NUMA_DEBUG("No HTT support, using default values\n");
        /* no HTT support, 1 core per package */
        tp->core_bits = 0;
        tp->smt_bits  = 0;
    }


    NUMA_DEBUG("Finished topo probe...\n");
    NUMA_DEBUG("\tcore_bits=0x%x\n", tp->core_bits);
    NUMA_DEBUG("\tsmt_bits =0x%x\n", tp->smt_bits);
}

static void 
assign_core_coords (struct cpu * me, struct nk_cpu_coords * coord, struct nk_topo_params *tp)
{
    uint32_t my_apic_id;

	/* 
	 * use the x2APIC ID if it's supported 
	 * and this is the right environment
	 */
	if (!nk_is_amd() && 
		cpuid_leaf_max() >= 0xb &&
		has_leafb()) {

		cpuid_ret_t ret;
		cpuid_sub(0x1, 0, &ret);
		NUMA_DEBUG("Using x2APIC for ID\n");
		my_apic_id = ret.d;

	} else {
		my_apic_id = apic_get_id(per_cpu_get(apic));
	}
		
    coord->smt_id  = my_apic_id & ((1 << tp->smt_bits) - 1);
    coord->core_id = (my_apic_id >> tp->smt_bits) & ((1 << tp->core_bits) - 1);
    coord->pkg_id  = my_apic_id & ~((1 << (tp->smt_bits + tp->core_bits)) - 1);

    NUMA_DEBUG("Core OS ID: %u (APIC ID=0x%x):\n", me->id, my_apic_id);
    NUMA_DEBUG("\tLogical Core ID:  %u\n", coord->smt_id);
    NUMA_DEBUG("\tPhysical Core ID: %u\n", coord->core_id);
    NUMA_DEBUG("\tPhysical Chip ID: %u\n", coord->pkg_id);
}


/* 
 *
 * to be called by all cores
 *
 */
int
nk_cpu_topo_discover (struct cpu * me)
{
    struct nk_cpu_coords * coord = NULL;
    struct nk_topo_params * tp = NULL;

    coord = (struct nk_cpu_coords*)malloc(sizeof(struct nk_cpu_coords));
    if (!coord) {
        NUMA_ERROR("Could not allocate coord struct for CPU %u\n", my_cpu_id());
        return -1;
    }
    memset(coord, 0, sizeof(struct nk_cpu_coords));

    tp = (struct nk_topo_params*)malloc(sizeof(struct nk_topo_params));
    if (!tp) {
        NUMA_ERROR("Could not allocate param struct for CPU %u\n", my_cpu_id());
        goto out_err;
    }
    memset(tp, 0, sizeof(struct nk_topo_params));

    get_topo_params(tp);

    /* now we can determine the CPU coordinates */
    assign_core_coords(me, coord, tp);

    me->tp    = tp;
    me->coord = coord;

    return 0;

out_err:
    free(coord);
    return -1;
}



static void 
dump_mem_regions (struct numa_domain * d)
{
    struct mem_region * reg = NULL;
    unsigned i = 0;

    list_for_each_entry(reg, &(d->regions), entry) {
        printk("\tMemory Region %u:\n", i);
        printk("\t\tRange:         %p - %p\n", (void*)reg->base_addr, (void*)(reg->base_addr + reg->len));
        printk("\t\tSize:          %lu.%lu MB\n", reg->len / 1000000, reg->len % 1000000);
        printk("\t\tEnabled:       %s\n", reg->enabled ? "Yes" : "No");
        printk("\t\tHot Pluggable: %s\n", reg->hot_pluggable ? "Yes" : "No");
        printk("\t\tNon-volatile:  %s\n", reg->nonvolatile ? "Yes" : "No");
        i++;
    }
}


static void
dump_adj_list (struct numa_domain * d)
{
    struct domain_adj_entry * ent = NULL;

    if (list_empty(&d->adj_list)) {
        printk("[no entries in adjacency list]\n");
        return;
    }
    printk("[ ");
    list_for_each_entry(ent, &(d->adj_list), list_ent) {
        printk("%u ", ent->domain->id);
    }
    printk("]\n");
}


static void
dump_domain_cpus (struct numa_domain * d)
{
    struct sys_info * sys = &(nk_get_nautilus_info()->sys);
    int i;

    printk("[ ");

    for (i = 0; i < sys->num_cpus; i++) {
        if (sys->cpus[i]->domain == d) {
            printk("%u ", i);
        }
    }

    printk(" ]\n");
}


static void
dump_numa_domains (struct nk_locality_info * numa)
{
    unsigned i;

    printk("%u NUMA Domains:\n", numa->num_domains);
    printk("----------------\n");

    for (i = 0; i < numa->num_domains; i++) {

        struct numa_domain * domain = numa->domains[i];

        printk("Domain %u (Total Size = %lu.%lu MB)\n",
                domain->id,
                domain->addr_space_size / 1000000,
                domain->addr_space_size % 1000000);

        printk("Adjacency List (ordered by distance): ");
        dump_adj_list(domain);

        printk("Associated CPUs: ");
        dump_domain_cpus(domain);

        printk("%u constituent region%s:\n", domain->num_regions, domain->num_regions > 1 ? "s" : "");

        dump_mem_regions(domain);
        printk("\n");
    }
}


static void 
dump_numa_matrix (struct nk_locality_info * numa)
{
    unsigned i,j;

    if (!numa || !numa->numa_matrix) {
        return;
    }

    printk("NUMA Distance Matrix:\n");
    printk("---------------------\n");
    printk("   ");

    for (i = 0; i < numa->num_domains; i++) {
        printk("%02u ", i);
    }
    printk("\n");

    for (i = 0; i < numa->num_domains; i++) {
        printk("%02u ", i);
        for (j = 0; j < numa->num_domains; j++) {
            printk("%02u ", *(numa->numa_matrix + i*numa->num_domains + j));
        }
        printk("\n");
    }

}


static void 
__associate_domains_slit (struct nk_locality_info * loc)
{
    unsigned i, j;

    NUMA_DEBUG("Associate Domains Using SLIT\n");
    
    for (i = 0; i < loc->num_domains; i++) {
        for (j = 0; j < loc->num_domains; j++) {
            if (j == i) continue;

            struct domain_adj_entry * new_dom_ent = mm_boot_alloc(sizeof(struct domain_adj_entry));
            new_dom_ent->domain = loc->domains[j];
            if (list_empty(&(loc->domains[i]->adj_list))) {
                list_add(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
            } else {
                struct domain_adj_entry * ent = NULL;
                list_for_each_entry(ent, &(loc->domains[i]->adj_list), list_ent) {
                    uint8_t dist_to_j = *(loc->numa_matrix + i*loc->num_domains + j);
                    uint8_t dist_to_other = *(loc->numa_matrix + i*loc->num_domains + ent->domain->id);

                    if (dist_to_j < dist_to_other) {
                        list_add_tail(&(new_dom_ent->list_ent), &(ent->list_ent));
                        continue;
                    }
                }
                list_add_tail(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
            }

        }
    }
}


static void
__associate_domains_adhoc (struct nk_locality_info * loc)
{
    unsigned i, j;
    
    NUMA_WARN("Associate Domains using ADHOC METHOD\n");

    for (i = 0; i < loc->num_domains; i++) {
        for (j = 0; j < loc->num_domains; j++) {
            if (j == i) continue;

            struct domain_adj_entry * new_dom_ent = mm_boot_alloc(sizeof(struct domain_adj_entry));
	    if (!new_dom_ent) {
		NUMA_ERROR("Unable to allocate domain entry - skipping association\n");
		continue;
	    }
	    INIT_LIST_HEAD(&new_dom_ent->list_ent);
            new_dom_ent->domain = loc->domains[j];
            NUMA_DEBUG("Adding domain %u to adjacency list of domain %u\n", j, i);
            list_add(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
        }
    }
}


static void
__coalesce_regions (struct numa_domain * d)
{
    struct mem_region * mem = NULL;
    unsigned i;

    NUMA_DEBUG("Coalesce Regions for domain id=%lu\n",d->id);

restart_scan:
    NUMA_DEBUG("Restart Scan\n");
    i = 0;
    list_for_each_entry(mem, &d->regions, entry) {

        if (mem->entry.next != &d->regions) {

            NUMA_DEBUG("Examining region:  base=%p, len=0x%lx, domain=0x%lx enabled=%d hot_plug=%d nv=%d\n",mem->base_addr, mem->len, mem->domain_id, mem->enabled, mem->hot_pluggable, mem->nonvolatile);
            struct mem_region * next_mem = list_entry(mem->entry.next, struct mem_region, entry);
            ASSERT(next_mem);

            NUMA_DEBUG("Next region is: base=%p, len=0x%lx, domain=0x%lx, enabled=%d, hot_plug=%d nv=%d\n",next_mem->base_addr, next_mem->len, next_mem->domain_id, next_mem->enabled, next_mem->hot_pluggable, next_mem->nonvolatile);

            if (next_mem->base_addr == mem->base_addr + mem->len) {

                NUMA_DEBUG("Coalescing adjacent regions %u and %u in domain %u\n", i, i+1, d->id);
                NUMA_DEBUG("First  region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n",mem->base_addr, mem->len, mem->domain_id, mem->enabled);
                NUMA_DEBUG("Second region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n", next_mem->base_addr, next_mem->len, next_mem->domain_id, next_mem->enabled);
                struct mem_region * new_reg = mm_boot_alloc(sizeof(struct mem_region));
                if (!new_reg) {
                    panic("Could not create new coalesced memory region\n");
                }

                memset(new_reg, 0, sizeof(struct mem_region));

                /* NOTE: we ignore the flags, e.g. nonvolatile, hot removable etc */
                new_reg->base_addr = mem->base_addr;
                new_reg->len       = mem->len + next_mem->len;
                new_reg->domain_id = mem->domain_id;
                new_reg->enabled   = mem->enabled;

                NUMA_DEBUG("Combined Region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n", new_reg->base_addr, new_reg->len, new_reg->domain_id, new_reg->enabled);

                /* stitch the coalesced region into the domain's region list */
                list_add_tail(&new_reg->entry, &mem->entry);

                /* get rid of the old ones */
                list_del(&mem->entry);
                list_del(&next_mem->entry);

                --d->num_regions;
                goto restart_scan;

            }

        } 
        i++;
    }
}


/*
 * sometimes we end up with non-overlapping, adjacent regions
 * that, for our purposes, should be treated as one range of memory.
 * We coalesce them here to avoid overhead in the page allocator
 * later on.
 */
static void
coalesce_regions (struct nk_locality_info * loc)
{
    unsigned i;

    NUMA_DEBUG("Coalescing adjacent memory regions\n");
    for (i = 0; i < loc->num_domains; i++) {
        NUMA_DEBUG("...coalescing for domain %d\n", i);
        __coalesce_regions(loc->domains[i]);
    }
}


static void
associate_domains (struct nk_locality_info * loc)
{
    NUMA_DEBUG("Associating NUMA domains to each other using adjacency info\n");
    if (loc->numa_matrix) {
        __associate_domains_slit(loc);
    } else {
        __associate_domains_adhoc(loc);
    }
}


struct mem_region *
nk_get_base_region_by_num (unsigned num)
{
    struct nk_locality_info * loc = &(nautilus_info.sys.locality_info);

    ASSERT(num < loc->num_domains);

    struct list_head * first = loc->domains[num]->regions.next;
    return list_entry(first, struct mem_region, entry);
}


struct mem_region * 
nk_get_base_region_by_cpu (cpu_id_t cpu)
{
    struct numa_domain * domain = nk_get_nautilus_info()->sys.cpus[cpu]->domain;
    struct list_head * first = domain->regions.next;
    return list_entry(first, struct mem_region, entry);
}


unsigned
nk_my_numa_node (void)
{
    return per_cpu_get(domain)->id;
}


void
nk_dump_numa_info (void)
{
    struct nk_locality_info * numa = &(nk_get_nautilus_info()->sys.locality_info);

    printk("===================\n");
    printk("     NUMA INFO:\n");
    printk("===================\n");
    
    dump_numa_domains(numa);

    dump_numa_matrix(numa);

    printk("\n");
    printk("======================\n");
    printk("     END NUMA INFO:\n");
    printk("======================\n");
}

unsigned 
nk_get_num_domains (void)
{
    struct nk_locality_info * l = &(nk_get_nautilus_info()->sys.locality_info);

    return l->num_domains;
}


struct numa_domain *
nk_numa_domain_create (struct sys_info * sys, unsigned id)
{
    struct numa_domain * d = NULL;

    d = (struct numa_domain *)mm_boot_alloc(sizeof(struct numa_domain));
    if (!d) {
        ERROR_PRINT("Could not allocate NUMA domain\n");
        return NULL;
    }
    memset(d, 0, sizeof(struct numa_domain));

    d->id = id;

    INIT_LIST_HEAD(&(d->regions));
    INIT_LIST_HEAD(&(d->adj_list));

    if (id != (sys->locality_info.num_domains + 1)) { 
        NUMA_DEBUG("Memory regions are not in expected domain order, but that should be OK\n");
    }

    sys->locality_info.domains[id] = d;
    sys->locality_info.num_domains++;

    return d;
}


/*
 *
 * only called by BSP once
 *
 */
int
nk_numa_init (void)
{
    struct sys_info * sys = &(nk_get_nautilus_info()->sys);

    NUMA_DEBUG("NUMA ARCH INIT\n");

    if (arch_numa_init(sys) != 0) {
        NUMA_ERROR("Error initializing arch-specific NUMA\n");
        return -1;
    }

    /* if we didn't find any of the relevant ACPI tables, we need
     * to coalesce the regions given to us at boot-time into a single
     * NUMA domain */
    if (nk_get_num_domains() == 0) {

        NUMA_DEBUG("No domains in ACPI locality info - making it up\n");

        int i;
        struct numa_domain * domain = nk_numa_domain_create(sys, 0);

        if (!domain) {
            NUMA_ERROR("Could not create main NUMA domain\n");
            return -1;
        }

        for (i = 0; i < mm_boot_num_regions(); i++) {
            struct mem_map_entry * m = mm_boot_get_region(i);
            struct mem_region * region = NULL;
            if (!m) {
                NUMA_ERROR("Couldn't get memory map region %u\n", i);
                return -1;
            }

            if (m->type != MULTIBOOT_MEMORY_AVAILABLE) {
                continue;
            }

            region = mm_boot_alloc(sizeof(struct mem_region));
            if (!region) {
                NUMA_ERROR("Couldn't allocate memory region\n");
                return -1;
            }
            memset(region, 0, sizeof(struct mem_region));
            
            region->domain_id     = 0;
            region->base_addr     = m->addr;
            region->len           = m->len;
            region->enabled       = 1;
            region->hot_pluggable = 0;
            region->nonvolatile   = 0;
        
            domain->num_regions++;
            domain->addr_space_size += region->len;

            /* add the mem region in order */
            if (list_empty(&domain->regions)) {
                list_add(&region->entry, &domain->regions);
            } else {
                struct mem_region * ent = NULL;
                list_for_each_entry(ent, &domain->regions, entry) {
                    if (region->base_addr < ent->base_addr) {
                        list_add_tail(&region->entry, &ent->entry);
                        break;
                    }
                }
                if (ent != (struct mem_region *)&domain->regions) {
                    list_add_tail(&region->entry, &domain->regions);
                }
            }

        }

        /* this domain is now every CPU's local domain */
        for (i = 0; i < sys->num_cpus; i++) {
            sys->cpus[i]->domain = domain;
        }
    }


    // now deal with weird situations where they start counting domains at 1
    // should ideally find all "missing" domains and create fakes for them
    if (!sys->locality_info.domains[0]) {
	NUMA_WARN("Faking domain 0 because of crack monkeys\n");
	struct numa_domain *n = nk_numa_domain_create(sys,0);
	if (!n) {
	    NUMA_ERROR("Cannot allocate fake domain 0\n");
	    return -1;
	}
	// domain 0 should be zero size, 0 regions, no adjacency
    }

    
    /* we now construct a list of adjacent domains for each domain, 
     * ordered by distances obtained in the SLIT. If we didn't see a SLIT,
     * we add domains arbitrarily */
    associate_domains(&sys->locality_info);

    /* combine non-overlapping adjacent regions */
    coalesce_regions(&sys->locality_info);

#ifdef NAUT_CONFIG_DEBUG_NUMA
    nk_dump_numa_info();
#endif

    return 0;
}

