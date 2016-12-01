BASE_LIBS += base-linux-common base-linux

include $(BASE_DIR)/lib/mk/ldso.inc

INC_DIR += $(DIR)/spec/arm
vpath %.s $(DIR)/spec/arm
