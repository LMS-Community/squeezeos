ifeq ($(CONFIG_MACH_I30030EVB),y)
   zreladdr-y		:= 0x80008000
   params_phys-y	:= 0x80000100
   initrd_phys-y	:= 0x80800000
endif

ifeq ($(CONFIG_MACH_MXC30030EVB),y)
   zreladdr-y		:= 0x80008000
   params_phys-y	:= 0x80000100
   initrd_phys-y	:= 0x80800000
endif

ifeq ($(CONFIG_MACH_I30030ADS),y)
   zreladdr-y		:= 0x90008000
   params_phys-y	:= 0x90000100
   initrd_phys-y	:= 0x90800000
endif

ifeq ($(CONFIG_MACH_MXC30030ADS),y)
   zreladdr-y		:= 0x90008000
   params_phys-y	:= 0x90000100
   initrd_phys-y	:= 0x90800000
endif
