# The following variables must contain relative paths
NK_VERSION=$(shell awk '/ version [0-9]/ {print $$NF}' netkit-version)
PUBLISH_DIR=/afs/vn.uniroma3.it/user/n/netkit/public/public_html/download/netkit/
MANPAGES_DIR=/afs/vn.uniroma3.it/user/n/netkit/public/public_html/man/

.PHONY: default help pack publish

default: help

help:
	@echo
	@echo -e "\e[1mAvailable targets are:\e[0m"
	@echo
	@echo -e "  \e[1mpack\e[0m       Create a distributable tarball of Netkit."
	@echo
	@echo -e "  \e[1mpublish\e[0m    Copy the Netkit tarball to the publicly accessible"
	@echo "             download directory. Also update the currently"
	@echo "             published readme, installation instructions, and"
	@echo "             changelog. Must be run from a machine with AFS"
	@echo "             access and by a user having a token with write"
	@echo "             permissions on the Netkit web site directory."
	@echo
	@echo -e "  \e[1mmanpublish\e[0m Publish the Netkit man pages on the Netkit web"
	@echo "             site, after converting them to HTML."
	@echo
	@echo "The above targets only affect the core Netkit distribution."
	@echo "In order to also package the kernel and/or filesystem, please"
	@echo "run the corresponding Makefile in the applicable directory."
	@echo

pack: ../netkit-$(NK_VERSION).tar.bz2
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

publish: netkit-$(NK_VERSION).tar.bz2
	cp "netkit-$(NK_VERSION).tar.bz2" CHANGES INSTALL README $(PUBLISH_DIR)

manpublish:
	for i in $(shell find man -type f ! -wholename "*svn*"); do \
		mkdir -p $(MANPAGES_DIR)/$$(dirname $$i); \
		man2html -r $$i > $(MANPAGES_DIR)/$$i.html; \
	done

