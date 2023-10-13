XBE_TITLE = StellarAttachISO
GEN_XISO = $(XBE_TITLE).iso
NXDK_SDL = n
NXDK_CXX = n
NXDK_NET = n
NXDK_DISABLE_AUTOMOUNT_D=y
DEBUG=n

#
SRCS += \
	$(CURDIR)/source/attach.c

CFLAGS += \
	-I$(CURDIR)/3rdparty \
	-I$(CURDIR)/source \
	-DWIN32

include $(NXDK_DIR)/Makefile
