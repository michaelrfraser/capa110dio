CONFIG_MODULE_SIG=n

obj-m += capa110dio.o

all:
	make -O -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -O -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
