#include <nautilus/aspace.h>
#include <nautilus/nautilus.h>
#include <nautilus/thread.h>


int paging_wrapper(
    char * _buf, 
    void *_priv,
    int (*fptr)(char *, void *)
);