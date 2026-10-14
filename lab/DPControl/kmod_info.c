#include <mach/mach_types.h>
#include <libkern/OSKextLib.h>
extern kern_return_t _start(kmod_info_t*,void*);
extern kern_return_t _stop(kmod_info_t*,void*);
static kern_return_t dp_start(kmod_info_t*ki,void*data){(void)ki;(void)data;return KERN_SUCCESS;}
static kern_return_t dp_stop(kmod_info_t*ki,void*data){(void)ki;(void)data;return KERN_SUCCESS;}
KMOD_EXPLICIT_DECL(com.navi48.dpcontrol,"0.0.2",_start,_stop)
__private_extern__ kmod_start_func_t *_realmain=dp_start;
__private_extern__ kmod_stop_func_t *_antimain=dp_stop;
