obj-m += prism.o

prism-y := prism_drv.o prism_dev.o

KDIR := /home/charain/Project/qemu_vm/driver/linux-6.17.3/

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

compile_commands:
	$(KDIR)/scripts/clang-tools/gen_compile_commands.py -d $(KDIR)
