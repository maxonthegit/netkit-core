# The following variables must contain relative paths
NK_VERSION=$(shell awk '/ version [0-9]/ {print $$NF}' netkit-version)
PUBLISH_DIR=/afs/vn.uniroma3.it/user/n/netkit/public/public_html/download/netkit/
MANPAGES_DIR=/afs/vn.uniroma3.it/user/n/netkit/public/public_html/man/

SRC_DIR=src
UML_TOOLS_DIR=$(SRC_DIR)/tools-20070815/
PATCHES_DIR=$(SRC_DIR)/patches/
BUILD_DIR=build
UML_TOOLS_BUILD_DIR=$(BUILD_DIR)/uml_tools/
NETKIT_BUILD_DIR=$(BUILD_DIR)/netkit/
UML_TOOLS_BIN_DIR=bin/uml_tools/


.PHONY: default help pack

default: help

help:
	@echo
	@echo -e "\e[1mAvailable targets are:\e[0m"
	@echo
	@echo -e "  \e[1mpack\e[0m       Create a distributable tarball of Netkit."
	@echo
	@echo -e "  \e[1mbuild\e[0m      Patch then build the uml tools."
	@echo
	@echo -e "  \e[1mclean\e[0m      Clean the building directories and the uml-tools binaries."
	@echo
	@echo "The above targets only affect the core Netkit distribution."
	@echo "In order to also package the kernel and/or filesystem, please"
	@echo "run the corresponding Makefile in the applicable directory."
	@echo

pack: ../netkit-$(NK_VERSION).tar.bz2 build
	mv ../netkit-$(NK_VERSION).tar.bz2 .

../netkit-$(NK_VERSION).tar.bz2: build
	mkdir $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_switch/uml_switch $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/port-helper/port-helper $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/tunctl/tunctl $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/mconsole/uml_mconsole $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/moo/uml_mkcow $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/moo/uml_moo $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_net/uml_net $(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_dump/uml_dump $(UML_TOOLS_BIN_DIR)
	cd bin; ln -s lstart lrestart; ln -s lstart ltest; find uml_tools -mindepth 1 -maxdepth 1 -type f -exec ln -s {} ';'
	mkdir -p $(NETKIT_BUILD_DIR)
	tar -C .. --owner=0 --group=0 -cjf "../netkit-$(NK_VERSION).tar.bz2" \
		--exclude=DONT_PACK --exclude=Makefile --exclude=fs --exclude=kernel \
		--exclude=awk --exclude=basename --exclude=date --exclude=dirname \
		--exclude=find --exclude=fuser --exclude=grep --exclude=head --exclude=id \
		--exclude=kill --exclude=ls --exclude=lsof --exclude=ps --exclude=wc \
		--exclude=getopt --exclude=netkit_commands.log --exclude=stresslabgen.sh \
		--exclude=build_tarball.sh --exclude="netkit-$(NK_VERSION).tar.bz2" --exclude=FAQ.old \
		--exclude=CVS --exclude=TODO \
                --exclude=netkit-filesystem-F* \
		--exclude=netkit-kernel-* \
                --exclude=fs \
		--exclude=kernel \
		--exclude=*.bz2 \
                --exclude=.* netkit/

build: clean
	mkdir $(BUILD_DIR)
	cp -rf $(UML_TOOLS_DIR) $(UML_TOOLS_BUILD_DIR)
	for PATCH in $(shell find $(PATCHES_DIR) -type f); do \
		cat $${PATCH} | patch -d $(UML_TOOLS_BUILD_DIR) -p1; \
	done
	(cd $(UML_TOOLS_BUILD_DIR) && $(MAKE) && cd -)


clean:
	cd bin; find . -mindepth 1 -maxdepth 1 -type l -exec unlink {} ";"
	rm -rf $(BUILD_DIR)
	rm -rf $(UML_TOOLS_BIN_DIR)
	rm -f netkit-$(NK_VERSION).tar.bz2
