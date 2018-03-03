base = 0x80000000
base := $(shell printf "%d" $(base))
offs := $(shell printf "%d" $(CONFIG_CXD90014_TEXT_OFFSET))
phys := $(shell expr $(base) + $(offs) - 32768)
   zreladdr-y	:= $(shell expr $(phys) + 32768)
params_phys-y	:= $(shell expr $(phys) + 256)
initrd_phys-y	:= $(shell expr $(phys) + 8388608)
