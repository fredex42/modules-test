obj-m += hello-1.o
obj-m += hello-4.o
obj-m += bcm-gpitest.o
obj-m += bcm-gpitest2.o
obj-m += bcm-gpitest3.o
obj-m += bcm-gpitest4.o
obj-m += timertest1.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

