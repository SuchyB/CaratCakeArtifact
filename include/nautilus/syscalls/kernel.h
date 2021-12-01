#ifndef _SYSCALL_KERNEL
#define _SYSCALL_KERNEL

#ifndef __ASSEMBLER__

#include <nautilus/nautilus.h>

int nk_legacy_syscall_handler(excp_entry_t* excp, excp_vec_t vector,
                              void* state);

uint64_t nk_syscall_handler(struct nk_regs* r);

void nk_syscall_init();

void nk_syscall_init_ap();

void init_syscall_table();

#endif /* !__ASSEMBLER */

// TODO: sub before saving
#define SAVE_GPRS_SYSCALL() \
  movq % rax, -8(% rsp);    \
  movq % rbx, -16(% rsp);   \
  movq % rcx, -24(% rsp);   \
  movq % rdx, -32(% rsp);   \
  movq % rsi, -40(% rsp);   \
  movq % rdi, -48(% rsp);   \
  movq % rbp, -56(% rsp);   \
  movq % r8, -64(% rsp);    \
  movq % r9, -72(% rsp);    \
  movq % r10, -80(% rsp);   \
  movq % r11, -88(% rsp);   \
  movq % r12, -96(% rsp);   \
  movq % r13, -104(% rsp);  \
  movq % r14, -112(% rsp);  \
  movq % r15, -120(% rsp);  \
  subq $120, % rsp;

#define RESTORE_GPRS_EXCEPT_RAX() \
  movq (% rsp), % r15;             \
  movq 8(% rsp), % r14;           \
  movq 16(% rsp), % r13;          \
  movq 24(% rsp), % r12;          \
  movq 32(% rsp), % r11;          \
  movq 40(% rsp), % r10;          \
  movq 48(% rsp), % r9;           \
  movq 56(% rsp), % r8;           \
  movq 64(% rsp), % rbp;          \
  movq 72(% rsp), % rdi;          \
  movq 80(% rsp), % rsi;          \
  movq 88(% rsp), % rdx;          \
  movq 96(% rsp), % rcx;          \
  movq 104(% rsp), % rbx;         \
  addq $120, % rsp;

// 120 since last 8 is for RAX which we do not restore

#define SYSCALL_STACK_ALIGN 32

#endif // _SYSCALL_KERNEL
