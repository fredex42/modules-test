obj-m += hello-1.o
obj-m += hello-4.o
obj-m += bcm-gpitest.o
obj-m += bcm-gpitest2.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

