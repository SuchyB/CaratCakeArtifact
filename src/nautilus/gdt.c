#include <nautilus/gdt.h>

#include <nautilus/mm.h>
#include <nautilus/printk.h>
#include <nautilus/smp.h>

extern struct gdt_desc64 gdtr64;
extern uint64_t gdt64[];
extern struct tss64 tss64;

void set_tss_descriptor(struct tss64_descriptor* entry, void* base,
                        uint32_t limit, uint8_t flags, uint8_t access) {
  uint64_t* raw_entry = (uint64_t*)entry;

  *raw_entry = 0;
  *raw_entry |= (uint64_t)limit & 0xFFFF;
  *raw_entry |= ((uint64_t)base & 0xFFFFFF) << 16;
  *raw_entry |= (uint64_t)access << 40;
  *raw_entry |= ((uint64_t)limit & 0xF0000) << (48 - 16);
  *raw_entry |= ((uint64_t)flags & 0xF) << 52;
  *raw_entry |= ((uint64_t)base & 0xFF000000) << (56 - 24);
  *(raw_entry + 1) = ((uint64_t)base >> 32) & 0xFFFFFFFF;

  /* This should work if the struct is set up right, but it looks like it isn't
  entry->base_15_0 = base & 0xFFFF;
  entry->base_23_16 = (base & 0xFF0000) >> 16;
  entry->base_31_24 = (base & 0xFF000000) >> 24;

  entry->limit_15_0 = limit & 0xFFFF;
  entry->flags_limit_19_16 = ((limit & 0xFF0000) >> 16) | (flags << 8);

  entry->access_byte = access;
  */
}

void setup_tss(struct tss64* tss) {
  memset(tss, 0, sizeof(struct tss64));
  void* stack = kmem_sys_malloc_specific(IST_SIZE, 0, 0);
  if (!stack) {
    panic("Failed to allocate a stack for TSS\n");
  } else if (stack > (void*)0x100000000UL) {
    panic("Allocated interrupt stack outside of lower 4G\n");
  }
  tss->ist1 = (uint64_t)stack + IST_SIZE; // Must offset
  tss->iopb_offset = 0x68;                // Size does not include offset itself
}

void nk_gdt_init() {
  setup_tss(&tss64);
  set_tss_descriptor((struct tss64_descriptor*)&gdt64[3], (struct tss64*)&tss64, 0x67, 0, 0x89);
  lgdt64(&gdtr64);
  asm volatile("ltr %0" ::"r"((uint16_t)0x18));
}

void nk_gdt_init_ap(struct cpu* core) {
  memcpy(&core->gdt64_entries, &gdt64,
         3 * 8); /* The first 3 entries of the GDT are always the same */

  setup_tss(&core->tss);
  set_tss_descriptor((struct tss64_descriptor*)&core->gdt64_entries[3], (struct tss64*)&core->tss, 0x67, 0, 0x89);

  core->gdtr64.base = (uint64_t)&core->gdt64_entries;
  core->gdtr64.limit = sizeof(core->gdt64_entries) - 1;

  lgdt64(&core->gdtr64);
  asm volatile("ltr %0" ::"r"((uint16_t)0x18));
}