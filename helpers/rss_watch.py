"""RSS of the translator's isolated process group via public libproc APIs."""
import ctypes, errno
lib = ctypes.CDLL('/usr/lib/libproc.dylib', use_errno=True)
lib.proc_listpids.argtypes = [ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_int]
lib.proc_pidinfo.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_uint64, ctypes.c_void_p, ctypes.c_int]

def group_rss_kb(pgid):
    # SDK sys/proc_info.h: PROC_PGRP_ONLY=2, PROC_PIDTASKINFO=4;
    # proc_taskinfo starts with uint64 virtual_size, uint64 resident_size.
    pids = (ctypes.c_int * 2048)()
    n = lib.proc_listpids(2, pgid, pids, ctypes.sizeof(pids))
    if n <= 0 or n >= ctypes.sizeof(pids): return None
    rss = 0; seen = 0
    for pid in pids[:n//ctypes.sizeof(ctypes.c_int)]:
        if pid <= 0: continue
        data = ctypes.create_string_buffer(256)
        count = lib.proc_pidinfo(pid, 4, 0, data, len(data))
        if count >= 16:
            rss += ctypes.c_uint64.from_buffer(data, 8).value; seen += 1
        elif ctypes.get_errno() != errno.ESRCH:
            return None
    return (rss + 1023)//1024 if seen else None
