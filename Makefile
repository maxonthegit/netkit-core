# The following variables must contain relative paths
NK_VERSION=$(shell awk '/ version [0-9]/ {print $$NF}' netkit-version)

SRC_DIR=src
UML_TOOLS_DIR=$(SRC_DIR)/tools-20070815/
PATCHES_DIR=$(SRC_DIR)/patches/
BUILD_DIR=build
UML_TOOLS_BUILD_DIR=$(BUILD_DIR)/uml_tools/
NETKIT_BUILD_DIR=$(BUILD_DIR)/netkit/
UML_TOOLS_BIN_DIR=bin/uml_tools/

DEBIAN_VERSION=`cat /etc/debian_version | cut -c 1`

.PHONY: default help pack

default: help

check:
	@echo
	@echo -e "Checking \e[1mdebian\e[0m"
	cat /etc/debian_version
	@echo -e "Checking debian version \e[1m(6.X.X)\e[0m"
	test  $(DEBIAN_VERSION) = "6"
	@echo -e "Checking package \e[1mlibreadline6\e[0m"
	dpkg -s libreadline6 > /dev/null 2> /dev/null
	@echo -e "Checking package \e[1mlibreadline6-dev\e[0m"
	dpkg -s libreadline6-dev > /dev/null 2> /dev/null
	@echo -e "Checking package \e[1mlibfuse2\e[0m"
	dpkg -s libfuse2 > /dev/null 2> /dev/null
	@echo -e "Checking package \e[1mlibfuse-dev\e[0m"
	dpkg -s libfuse-dev > /dev/null 2> /dev/null
	@echo -e "Checking \e[1mTUNTAP\e[0m include file"
	test -e /usr/include/linux/if_tun.h
	

help:
	@echo
	@echo -e "\e[1mAvailable targets are:\e[0m"
	@echo
	@echo -e "  \e[1mpackage\e[0m    Create a distributable tarball of Netkit."
	@echo
	@echo -e "  \e[1mbuild\e[0m      Patch then build the uml tools."
	@echo
	@echo -e "  \e[1mclean\e[0m      Clean the building directories and the uml-tools binaries."
	@echo
	@echo "The above targets only affect the core Netkit distribution."
	@echo "In order to also package the kernel and/or filesystem, please"
	@echo "run the corresponding Makefile in the applicable directory."
	@echo

package: build
	mkdir $(NETKIT_BUILD_DIR)
	cp -r bin  CHANGES check_configuration.d  check_configuration.sh  COPYING  INSTALL  man  netkit.conf  Netkit-konsole.profile  netkit-version  README $(NETKIT_BUILD_DIR)
	mkdir  $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_switch/uml_switch $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/port-helper/port-helper $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/tunctl/tunctl $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/mconsole/uml_mconsole $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/moo/uml_mkcow $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/moo/uml_moo $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_net/uml_net $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)
	cp $(UML_TOOLS_BUILD_DIR)/uml_dump/uml_dump $(NETKIT_BUILD_DIR)$(UML_TOOLS_BIN_DIR)

	(cd $(NETKIT_BUILD_DIR)bin &&  ln -s lstart lrestart; ln -s lstart ltest; find uml_tools -mindepth 1 -maxdepth 1 -type f -exec ln -s {} ';' && cd -)
	tar -C $(BUILD_DIR) --owner=0 --group=0 -cjf "../netkit-$(NK_VERSION).tar.bz2" netkit/

build: clean check
	mkdir $(BUILD_DIR)
	cp -rf $(UML_TOOLS_DIR) $(UML_TOOLS_BUILD_DIR)
	for PATCH in $(shell find $(PATCHES_DIR) -type f); do \
		cat $${PATCH} | patch -d $(UML_TOOLS_BUILD_DIR) -p1; \
	done
	(cd $(UML_TOOLS_BUILD_DIR) && $(MAKE) && cd -)


clean:
	cd bin; find . -mindepth 1 -maxdepth 1 -type l -exec unlink {} ";"
	rm -rf $(BUILD_DIR)
	rm -f ../netkit-$(NK_VERSION).tar.bz2
