GFP_KERNEL

#define		APPTASK_LLC_HOOK 
#include    "llc-hook.h"



void d_fnstart(const char * fmt,...)
{
   ; 
}

void d_error(const char * fmt,...)
{
    ;
}
void d_printf(const char * fmt,...)
{
    ;
}
void d_fnend(const char * fmt,...)
{
    ;
}

void* kmalloc(int PoolSize, int mode)//GFP_KERNEL
{
    return malloc(PoolSize);
}

void d_init(void)
{
    ;
}

void d_exit(void)
{
    ;
}





