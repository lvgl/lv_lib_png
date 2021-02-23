CSRCS += png.c pngerror.c  pngget.c pngmem.c pngread.c pngrio.c pngrtran.c pngrutil.c pngset.c pngtrans.c 
CSRCS += pngwio.c pngwrite.c pngwtran.c pngwutil.c

DEPPATH += --dep-path $(APNG_DIR)/$(APNG_DIR_NAME)/libpng
VPATH += :$(APNG_DIR)/$(APNG_DIR_NAME)/libpng

CFLAGS += "-I$(APNG_DIR)/$(APNG_DIR_NAME)/libpng"