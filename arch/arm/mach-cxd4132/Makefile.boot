ifeq ($(CONFIG_CXD4132_DDR_RANK0_ABSENT),y)
base = 0x90000000
else
base = 0x80000000
endif
base := $(shell printf "%d" $(base))
offs := $(shell printf "%d" $(CONFIG_CXD4132_TEXT_OFFSET))
phys := $(shell expr $(base) + $(offs) - 32768)
   zreladdr-y	:= $(shell expr $(phys) + 32768)
params_phys-y	:= $(shell expr $(phys) + 256)
initrd_phys-y	:= $(shell expr $(phys) + 8388608)
