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
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#ifndef __GDT_H__
#define __GDT_H__

#ifndef __ASSEMBLER__

#include <nautilus/intrinsics.h>
#include <nautilus/naut_types.h>

#define IST_SIZE 0x4000

struct gdt_desc64 {
    uint16_t limit;
    uint64_t base;
} __packed;

struct gdt_desc32 {
    uint16_t limit;
    uint32_t base;
} __packed;

struct tss64_descriptor {
    uint8_t base_31_24;
    uint8_t flags_limit_19_16;
    uint8_t access_byte;
    uint8_t base_23_16;
    uint16_t base_15_0;
    uint16_t limit_15_0;
    uint32_t base_63_32;
    uint32_t _reserved0;
} __packed;

struct tss64 {
    uint32_t _reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t _reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t _reserved2;
    uint16_t _reserved3;
    uint16_t iopb_offset;
} __packed;

static inline void
lgdt32 (const struct gdt_desc32 * g)
{
    asm volatile ("lgdt %0" :: "m" (*g));
}

static inline void
lgdt64 (const struct gdt_desc64 * g) 
{
    asm volatile ("lgdt %0" :: "m" (*g)); 
}

void set_tss_descriptor(struct tss64_descriptor* entry, void* base,
                        uint32_t limit, uint8_t flags, uint8_t access);

void setup_tss(struct tss64* tss);

void nk_gdt_init();

struct cpu;
void nk_gdt_init_ap(struct cpu* core);

#endif /* !__ASSEMBLER__ */

#define KERNEL_CS 8
#define KERNEL_DS 16
#define KERNEL_SS KERNEL_DS

#endif
