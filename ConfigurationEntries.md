# Introduction #

This page lists entries from `xenaccess.conf` that are known to be working.  You can use these to create a working configuration file for your system when you are getting started with XenAccess.  If you have any working entries that are not shown here, feel free to add them.

Note that the sysmap value in each entry should be omitted because this is a path and file name that will change for each installation of XenAccess.

# Configuration Entries #

### Windows Vista, PAE enabled ###
```
WinVista {    
    ostype = "Windows";
    win_tasks  = 0xa0;
    win_pdbase = 0x18;
    win_pid    = 0x9c;
    win_peb    = 0x188;
    win_iba    = 0x8;
    win_ph     = 0x18;
}
```

### Windows XP Professional, Service Pack 2, PAE enabled ###
```
WinXPProSP2 {
   ostype = "Windows";
   win_tasks   = 0x88;
   win_pdbase  = 0x18;
   win_pid     = 0x84;
   win_peb     = 0x1b0;
   win_iba     = 0x8;
   win_ph      = 0x18;
}
```

### Windows XP Professional, Service Pack 2, PAE disabled ###
```
WinXPProSP2 {
    ostype = "Windows";
    win_tasks   = 0x88;
    win_pdbase  = 0x18;
    win_pid     = 0x84;
    win_peb     = 0x1b0;
    win_iba     = 0x8;
    win_ph      = 0x18;
}
```

### HVM Debian Linux with 2.6.18-4-486 Kernel ###
```
Linux2-6-18-486{
    ostype = "Linux";
    linux_tasks = 108;
    linux_mm    = 132;
    linux_pid   = 168;
    linux_addr  = 120;
    linux_pgd   = 36;
}
```

### PV Linux shipped with Xen 3.0.4\_1 (2.6.16.33-xen) ###
```
Linux2-6-16-33-PV{
    ostype = "Linux";
    linux_tasks = 0x60;
    linux_mm    = 0x78;
    linux_pid   = 0x9c;
    linux_pgd   = 0x24;
    linux_addr  = 0x80;
}
```

### PV Linux shipped with Xen 3.1.0 (2.6.18-xen) ###
```
Linux2-6-18-PV {
    ostype = "Linux";
    linux_tasks = 0x82;
    linux_mm    = 0x9a;
    linux_pid   = 0xc0;
    linux_pgd   = 0x24;
    linux_addr  = 0xa0;
}
```

### PV fc8 2.6.21 with xen 3.1.0-13 ###
```
fedora{
    ostype = "Linux";
    linux_tasks= 0x7c;
    linux_mm = 0x84;
    linux_pid = 0xa8;
    linux_pgd = 0x28;
    linux_addr = 0x84;
}
```

### SLES 10 SP 2, with Xen 3.2.0 and PAE (2.6.16.60-0.21-xenpae) ###
```
sles10sp2 {
    ostype = "Linux";
    # linux_name = 452;
    linux_tasks= 120;
    linux_mm = 144;
    linux_pid = 180;
    linux_pgd = 36;
    linux_addr = 128;
}
```