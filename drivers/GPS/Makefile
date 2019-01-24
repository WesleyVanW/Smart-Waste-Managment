include $(RIOTBASE)/Makefile.base

CFLAGS += -D_POSIX_C_SOURCE=199309L -D_BSD_SOURCE -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE

# see minmea README.md
CFLAGS += -Dtimegm=mktime
USEMODULE += printf_float
SRC := minmea.c
