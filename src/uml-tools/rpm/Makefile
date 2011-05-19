RPMCMD = rpm -bb uml.spec

UML_BUILD_DIR = /tmp/uml_kit

all : uml.spec ztarball
	UML_BUILD_DIR=$(UML_BUILD_DIR) $(RPMCMD)

ztarball :
	cp $(ZTARBALL) .

uml.spec: uml.spec.in
	export UML_VERSION=$(UML_VERSION) ; \
	export CHANGELOG=$(CHANGELOG) ; \
	export ME="$(ME)" ; \
	awk "/%changelog/ \
	    { print; \
	      printf \"* %s $(ME)\n\", \"`date +'%a %b %e %Y'`\"; \
	      system(\"sed 's/^/- /' $(CHANGELOG)\"); \
	      print \"\"; \
	      next } \
	    { print }" < uml.spec.in > uml.spec.tmp ; \
	mv uml.spec.tmp uml.spec.in ; \
	echo "%define version $(UML_VERSION)" > uml.spec ; \
	echo "BuildRoot: $(UML_BUILD_DIR)" >> uml.spec ; \
	cat uml.spec.in >> uml.spec

clean:
	rm -f uml.spec
