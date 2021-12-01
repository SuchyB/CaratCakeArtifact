#include <asm/lowlevel.h>
#include <nautilus/syscalls/kernel.h>

/*
Entry point for syscall handler
*/

.section .text
.code64

// SYSRET sets ring mode to 0. Workaround is manually storing restoring flags and jumping to the return address
// Interesting to see if register can directly be moved to RFLAGS

ENTRY(syscall_entry)
    leaq -256(%rsp), %rsp // To avoid mangling redzone
    pushq %r11 // syscall instruction saves the caller's RFLAGS in R11
    SAVE_GPRS_SYSCALL()
    movq %rsp, %rdi
    movq %rsp, %r13
    subq $SYSCALL_STACK_ALIGN, %rsp
    andq $-SYSCALL_STACK_ALIGN, %rsp
    testq $(SYSCALL_STACK_ALIGN-1), %rsp
    jnz bad
    callq nk_syscall_handler
    mov %r13, %rsp
    RESTORE_GPRS_EXCEPT_RAX()
    popf
    leaq 256(%rsp), %rsp
    jmp *%rcx
bad:
    movabs $syscall_err_str, %rdi
    movabs $panic, %rax
    callq *%rax
loop1:
    hlt
    jmp loop1

syscall_err_str:
.asciz "Not 16 byte aligned stack %u\12"