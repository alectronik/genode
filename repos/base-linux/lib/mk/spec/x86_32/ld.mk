BASE_LIBS += base-linux-common base-linux

include $(BASE_DIR)/lib/mk/ldso.inc

INC_DIR += $(DIR)/spec/x86_32
vpath %.s $(DIR)/spec/x86_32
