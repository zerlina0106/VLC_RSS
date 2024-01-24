obj-m := rssicatcher.o

KSRC ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi) 

all:
	make -C $(KSRC) SUBDIRS=$(PWD) modules
clean:
	make -C $(KSRC) M=$(PWD) clean

