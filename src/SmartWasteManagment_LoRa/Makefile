# name of your application
APPLICATION = SmartWasteManagment

DRIVER ?= sht31
DRIVER ?= lsm303agr
DRIVER ?= GPS
BOARD ?= octa

USEMODULE += sht31
USEMODULE += lsm303agr
USEMODULE += shell
USEMODULE += saul_default
USEMODULE += shell_commands
USEMODULE += ps  
USEMODULE += xtimer
USEMODULE += periph_gpio_irq 
USEMODULE += srf04
USEMODULE += GPS
USEMODULE += printf_float

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../RIOT

# Uncomment these lines if you want to use platform support from external
# repositories:
#RIOTCPU ?= $(CURDIR)/../../RIOT/thirdparty_cpu
#RIOTBOARD ?= $(CURDIR)/../../RIOT/thirdparty_boards

# Uncomment this to enable scheduler statistics for ps:
#USEMODULE += schedstatistics

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

EXTERNAL_MODULE_DIRS += $(RIOTBASE)/../riot-oss7-modem/drivers/oss7_modem
USEMODULE += oss7_modem

INCLUDES += -I$(RIOTBASE)/../riot-oss7-modem/drivers/oss7_modem/include

CFLAGS += -DDEBUG_ASSERT_VERBOSE

include $(RIOTBASE)/Makefile.include