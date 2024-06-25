ifeq ($(KERNELRELEASE),)

X86_KERNEL_BUILD = /lib/modules/$(shell uname -r)/build
ARM_KERNEL_BUILD = /home/linux/fs4412/kernel/linux-3.14
MODULE_PATH  = $(shell pwd)


arm_module:
	make -C $(ARM_KERNEL_BUILD) M=$(MODULE_PATH) modules ARCH=arm CROSS_COMPILE=arm-linux-
	cp test.ko /home/linux/fs4412/fs/rootfs
	arm-linux-gcc test-app.c  -o test-app
	cp test-app /home/linux/fs4412/fs/rootfs

clean:
	make -C $(X86_KERNEL_BUILD) M=$(MODULE_PATH) clean

else
	obj-m = test.o
endif

