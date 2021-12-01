#include <linux/futex.h>
#include <nautilus/nautilus.h>
#include <nautilus/waitqueue.h>

#define SYSCALL_NAME "sys_futex"
#include "impl_preamble.h"

#define NUM_FUTEXES 128

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#define FUTEX_PRIVATE_FLAG 128

struct futex {
  int* uaddr; // search key; == 0 implies not in use
  int val;    // for wait check
  nk_wait_queue_t* waitq;
};

static int futex_inited = 0;
static spinlock_t futex_lock;

static struct futex futex_pool[NUM_FUTEXES];

static int futex_init() {
  if (!futex_inited) {
    int i;
    char buf[80];
    spinlock_init(&futex_lock);
    for (i = 0; i < NUM_FUTEXES; i++) {
      futex_pool[i].uaddr = 0;
      futex_pool[i].val = 0;
      sprintf(buf, "futex%d-waitq", i);
      futex_pool[i].waitq = nk_wait_queue_create(buf);
      if (!futex_pool[i].waitq) {
        // ERROR
        return -1;
      }
    }
    futex_inited = 1;
  }
  return 0;
}

static struct futex* futex_find(int* uaddr) {
  int i;
  for (i = 0; i < NUM_FUTEXES; i++) {
    if (futex_pool[i].uaddr == uaddr) {
      return &futex_pool[i];
    }
  }
  return 0;
}

static struct futex* futex_allocate(int* uaddr) {
  int i;
  for (i = 0; i < NUM_FUTEXES; i++) {
    if (__sync_bool_compare_and_swap(&futex_pool[i].uaddr, 0, uaddr)) {
      return &futex_pool[i];
    }
  }
  return 0;
}

static void futex_free(int* uaddr) {
  struct futex* f = futex_find(uaddr);
  if (f) {
    __sync_fetch_and_and(&f->uaddr, 0);
  }
}

static int futex_check(void* state) {
  struct futex* f = (struct futex*)state;

  return *(f->uaddr) != f->val;
}

uint64_t sys_futex(uint32_t* uaddr, int op, uint32_t val,
                   /*(struct timespec*)*/ void* utime, uint32_t* uaddr2,
                   uint32_t val3) {
  // DEBUG("Called with args:\n0: %p\n1: %d\n2: %d\n3: %p\n4: %p\n5: %d\n",
  //             uaddr, op, val, utime, uaddr2, val3);

  struct futex* f;

  futex_init();
  spin_lock(&futex_lock);

  if (!(op & FUTEX_PRIVATE_FLAG)) {
    // This may break things ???
    // But we continue anyway just in case it's ok
    DEBUG("Non-private futex.\n");
  }

  op &= ~FUTEX_PRIVATE_FLAG;

  switch (op) {
  case FUTEX_WAIT:
    if (utime) {
      DEBUG("timeout unsupported\n");
      spin_unlock(&futex_lock);
      return -1;
    }
    f = futex_find(uaddr);
    if (!f) {
      f = futex_allocate(uaddr);
    }
    if (!f) {
      DEBUG("cannot find or allocate futex\n");
      spin_unlock(&futex_lock);
      return -1;
    }

    DEBUG("Starting futex wait on %p %p %d %d\n", f, f->uaddr, *f->uaddr, val);
    f->val = val;
    spin_unlock(&futex_lock);
    nk_wait_queue_sleep_extended(f->waitq, futex_check, f);
    DEBUG("Finished futex wait on %p %p %d %d\n", f, f->uaddr, *f->uaddr, val);

    return 0;

    break;

  case FUTEX_WAKE: {
    uint64_t awoken_threads = 0;
    f = futex_find(uaddr);
    if (!f) {
      spin_unlock(&futex_lock);
      DEBUG("Futex for uaddr=%p does not exist\n", uaddr);
      return 0; // no one to wake - probably race with a FUTEX_WAIT
    }

    DEBUG("Starting futex wake on %p %p %d (waking %d%s)\n", f, f->uaddr,
          *f->uaddr, val, val == INT_MAX ? " ALL" : "");

    if (val != INT_MAX) {
      int i;
      for (i = 0; i < val; i++) {
        awoken_threads += nk_wait_queue_wake_one(f->waitq);
      }
    } else {
      awoken_threads += nk_wait_queue_wake_all(f->waitq);
    }
    spin_unlock(&futex_lock);
    return awoken_threads;
    break;
  }
  default:
    DEBUG("Unsupported FUTEX OP\n");
    spin_unlock(&futex_lock);
    return -1;
  }
}