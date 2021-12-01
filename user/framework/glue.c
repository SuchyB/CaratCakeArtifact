#include <stdio.h>

void* __gcc_personality_v0;

#if 0
__attribute__((used))
void _Unwind_Resume() {
  printf("Call to fake _Unwind_Resume\n");
  return;
}

__attribute__((used))
void _Unwind_GetCFA() {
  return;
}

__attribute__((used))
void _Unwind_ForcedUnwind() {
  return;  
}

__attribute__((used))
double __unordtf2() {
  printf("Call to fake __unordtf2\n");
  return 0;
}
#endif
