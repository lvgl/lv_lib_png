CSRCS += adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c
CSRCS += inflate.c inftrees.c trees.c uncompr.c zutil.c


DEPPATH += --dep-path $(APNG_DIR)/$(APNG_DIR_NAME)/zlib
VPATH += :$(APNG_DIR)/$(APNG_DIR_NAME)/zlib

CFLAGS += "-I$(APNG_DIR)/$(APNG_DIR_NAME)/zlib"