# Makefile for Custom V4L2 Capture Driver

# Name of the final kernel module
obj-m := ocd_cam.o

# Files that make up the module
ocd_cam-y := ocd_core.o ocd_cap.o ocd_hw.o

# Default to native architecture if not specified
ARCH ?= $(shell uname -m)
ifeq ($(ARCH),x86_64)
	# Native Fedora build
	KDIR ?= /lib/modules/$(shell uname -r)/build
	CROSS_COMPILE ?= 
else ifeq ($(ARCH),arm64)
	# Cross-compiling for i.MX8
	# UPDATE THIS PATH to wherever your i.MX kernel source tree is located on your Fedora host!
	KDIR ?= /home/krs/imx-linux-source/build 
	CROSS_COMPILE ?= aarch64-linux-gnu-
endif

PWD := $(shell pwd)

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean
