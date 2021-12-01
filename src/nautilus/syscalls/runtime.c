#include <nautilus/nautilus.h>

uint64_t syscall_int80(uint64_t num, ...) {

  uint64_t rc;

  va_list args;
  va_start(args, num);
  uint64_t l[6];
  int i = 0;
  while (i < 6) {
    l[i] = va_arg(args, uint64_t);
    i++;
  }

  uint64_t a1 = l[0];
  uint64_t a2 = l[1];
  uint64_t a3 = l[2];
  uint64_t a4 = l[3];
  uint64_t a5 = l[4];
  uint64_t a6 = l[5];

  va_end(args);

  __asm__ __volatile__(
      "movq %1, %%rax; "
      "movq %2, %%rdi; "
      "movq %3, %%rsi; "
      "movq %4, %%rdx; "
      "movq %5, %%r10; "
      "movq %6, %%r8; "
      "movq %7, %%r9; "
      "int $0x80; "
      "movq %%rax, %0; "
      : "=m"(rc)
      : "m"(num), "m"(a1), "m"(a2), "m"(a3), "m"(a4), "m"(a5), "m"(a6)
      : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "%r11");
  return rc;
}


uint64_t syscall_syscall(uint64_t num, ...) {
  uint64_t rc;

  va_list args;
  va_start(args, num);
  uint64_t l[6];
  int i = 0;
  while (i < 6) {
    l[i] = va_arg(args, uint64_t);
    i++;
  }

  uint64_t a1 = l[0];
  uint64_t a2 = l[1];
  uint64_t a3 = l[2];
  uint64_t a4 = l[3];
  uint64_t a5 = l[4];
  uint64_t a6 = l[5];

  va_end(args);

  __asm__ __volatile__(
      "movq %1, %%rax; "
      "movq %2, %%rdi; "
      "movq %3, %%rsi; "
      "movq %4, %%rdx; "
      "movq %5, %%r10; "
      "movq %6, %%r8; "
      "movq %7, %%r9; "
      //"movq $123, %%rcx;"
      //"movq $123, %%rcx;"
      //"movq $123, %%rcx;"
      "syscall; "
      "movq %%rax, %0; "
      //"retq;"
      : "=m"(rc)
      : "m"(num), "m"(a1), "m"(a2), "m"(a3), "m"(a4), "m"(a5), "m"(a6)
      : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "%r11");

  return rc;
}