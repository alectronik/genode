TARGET = kernel-fiasco
LIBS   = kernel-fiasco

$(TARGET): sigma0 bootstrap fiasco

L4_BUILD_DIR = $(LIB_CACHE_DIR)/syscall-fiasco

fiasco:
	$(VERBOSE)ln -sf $(LIB_CACHE_DIR)/kernel-fiasco/build/fiasco

sigma0:
	$(VERBOSE)ln -sf $(L4_BUILD_DIR)/bin/x86_586/l4v2/sigma0

bootstrap:
	$(VERBOSE)ln -sf $(L4_BUILD_DIR)/bin/x86_586/bootstrap
