#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/syscalls/decl.h>
#include <nautilus/thread.h>
#include <nautilus/syscalls/proc.h>

#define SYSCALL_NAME "sys_exit"
#include "impl_preamble.h"

#define FUTEX_WAKE 1

uint64_t sys_exit(uint64_t exit_status) {

  // https://man7.org/linux/man-pages/man2/set_tid_address.2.html
  uint32_t* clear_child_tid = get_cur_thread()->clear_child_tid;
  if (clear_child_tid) {
    *(uint64_t*)clear_child_tid = 0;
    sys_futex(clear_child_tid, FUTEX_WAKE, 1, NULL, NULL, 0);
  }


#define PARSEC_TESTING 0
#if PARSEC_TESTING
  extern int parsec_started;
  if(parsec_started){
     extern uint64_t starting_cycles;
     extern uint64_t ending_cycles;
     ending_cycles = rdtsc();
     struct sys_info * sys = per_cpu_get(system);
     struct apic_dev *apic = sys->cpus[0]->apic;
     uint64_t time_duration = apic_cycles_to_realtime(apic,ending_cycles-starting_cycles);

     nk_vc_printf("Benchmark finished!\nThe totle # of cycles measured in internal nautilus is %llu, and the time duration is %llu, the cycles_per_us is : %llu,\n",ending_cycles-starting_cycles,time_duration,apic->cycles_per_us);
     //nk_vc_printf("The formular is 1000ULL*(cycles/(apic->cycles_per_us)), the cycles_per_us is : %llu, you can get the cycles in the printing above searching \"Benchmark\"\n", apic->cycles_per_us);
     
     fflush((void*)1UL);
     starting_cycles = 0;
     ending_cycles = 0;
     parsec_started = 0;
   }
#endif





  nk_thread_exit((void*)exit_status);
  panic("Execution past thread exit\n");
  return 0;
}
