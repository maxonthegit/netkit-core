# The following variables must contain relative paths
NK_VERSION=$(shell awk '/ version [0-9]/ {print $$NF}' netkit-version)

.PHONY: default help pack

default: help

help:
	@echo
	@echo -e "\e[1mAvailable targets are:\e[0m"
	@echo
	@echo -e "  \e[1mpack\e[0m       Create a distributable tarball of Netkit."
	@echo
	@echo "The above targets only affect the core Netkit distribution."
	@echo "In order to also package the kernel and/or filesystem, please"
	@echo "run the corresponding Makefile in the applicable directory."
	@echo

pack: ../netkit-$(NK_VERSION).tar.bz2 build
	mv ../netkit-$(NK_VERSION).tar.bz2 .

../netkit-$(NK_VERSION).tar.bz2:
	cd bin; ln -s lstart lrestart; ln -s lstart ltest; find uml_tools -mindepth 1 -maxdepth 1 -type f -exec ln -s {} ';'
	tar -C .. --owner=0 --group=0 -cjf "../netkit-$(NK_VERSION).tar.bz2" \
		--exclude=DONT_PACK --exclude=Makefile --exclude=fs --exclude=kernel \
		--exclude=awk --exclude=basename --exclude=date --exclude=dirname \
		--exclude=find --exclude=fuser --exclude=grep --exclude=head --exclude=id \
		--exclude=kill --exclude=ls --exclude=lsof --exclude=ps --exclude=wc \
		--exclude=getopt --exclude=netkit_commands.log --exclude=stresslabgen.sh \
		--exclude=build_tarball.sh --exclude="netkit-$(NK_VERSION).tar.bz2" --exclude=FAQ.old \
		--exclude=CVS --exclude=TODO --exclude=netkit-filesystem-F* \
		--exclude=netkit-kernel-* --exclude=.* netkit/

build:
	(cd src && $(MAKE) && cd -)
	mkdir bin/uml_tools/
	cp src/build/uml_switch/uml_switch bin/uml_tools/
	cp src/build/port-helper/port-helper bin/uml_tools/
	cp src/build/tunctl/tunctl bin/uml_tools/
	cp src/build/mconsole/uml_mconsole bin/uml_tools/
	cp src/build/moo/uml_mkcow bin/uml_tools/
	cp src/build/moo/uml_moo bin/uml_tools/
	cp src/build/uml_net/uml_net bin/uml_tools/
	

clean:
	(cd src && $(MAKE) clean && cd -)
	rm -rf bin/uml_tools/
