#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include "nas_sincos.h"

static int sincos(char *__buf, void* __priv);

static struct shell_cmd_impl nas_sincos_impl = {
    .cmd      = "sc",
    .help_str = "sincos test",
    .handler  = sincos,
};
nk_register_shell_cmd(nas_sincos_impl);
/*--------------------------------------------------------------------
      program BT
c-------------------------------------------------------------------*/
static int sincos(char *__buf, void* __priv) {

  printf("sin : %lf, cos : %lf \n", sin(0.567),cos(9.087));

}

