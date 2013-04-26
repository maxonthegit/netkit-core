TUNCTL = $(shell [ -e /usr/include/linux/if_tun.h ] && echo tunctl)

SUBDIRS = lib jail jailtest humfsify mconsole moo port-helper $(TUNCTL) \
	uml_net uml_switch watchdog

CFLAGS = -g -Wall -static -O2 -m$(SIZE_ARCH)

export CFLAGS

all install: 
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done

clean:
	rm -rf *~
	set -e ; for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done

spec:	
	sed -e 's/__UMLVER__/$(UMLVER)/' < uml_util.spec.in > uml_util.spec
