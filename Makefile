# Makefile for Custom V4L2 Capture Driver

# Name of the final kernel module
obj-m := ocd_cam.o

# Files that make up the module
our_cam-y := ocd_core.o ocd_cap.o ocd_hw.o

# Target kernel directory (defaults to current running kernel if not cross-compiling)
# Need to set up cross compiling

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
