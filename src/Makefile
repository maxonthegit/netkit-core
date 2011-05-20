UML_TOOLS_DIR=tools-20070815/
BUILD_DIR=build/
PATCHES_DIR=patches/

all: clean
	cp -rf $(UML_TOOLS_DIR) $(BUILD_DIR)
	for PATCH in $(shell find $(PATCHES_DIR) -type f); do \
	  cat $${PATCH} | patch -d $(UML_TOOLS_DIR) -p1; \
        done
	(cd $(BUILD_DIR) && $(MAKE) && cd -)

clean:
	rm -rf $(BUILD_DIR)

