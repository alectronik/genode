BASE_LIBS += base-nova-common base-nova

include $(BASE_DIR)/lib/mk/ldso.inc

INC_DIR += $(DIR)/spec/x86_64
vpath %.s $(DIR)/spec/x86_64
