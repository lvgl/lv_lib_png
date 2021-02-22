CSRCS += lv_apng.c
CSRCS += apng_test.c

include $(APNG_DIR)/$(APNG_DIR_NAME)/libpng/libpng.mk
include $(APNG_DIR)/$(APNG_DIR_NAME)/zlib/zlib.mk

DEPPATH += --dep-path $(APNG_DIR)/$(APNG_DIR_NAME)/
VPATH += :$(APNG_DIR)/$(APNG_DIR_NAME)/

CFLAGS += "-I$(APNG_DIR)/$(APNG_DIR_NAME)/"