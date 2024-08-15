XBE_TITLE = StellarAttachISO
GEN_XISO = $(XBE_TITLE).iso
NXDK_SDL = n
NXDK_CXX = n
NXDK_NET = n
DEBUG=n

#
XBE_XTIMAGE = $(CURDIR)/resources/logo.xpr

#
SRCS += \
	$(CURDIR)/source/attach.c \
	$(CURDIR)/source/helpers.c \
	$(CURDIR)/source/resources.packed.obj

CFLAGS += \
	-I$(CURDIR)/3rdparty \
	-I$(CURDIR)/source \
	-DWIN32

# minIni
SRCS += \
	$(CURDIR)/3rdparty/minIni/dev/minIni.c
CFLAGS += \
	-I$(CURDIR)/3rdparty/minIni/dev

# Optimize build by removing unused symbols
CFLAGS += \
	-ffunction-sections \
	-fdata-sections
CXXFLAGS += \
	-ffunction-sections \
	-fdata-sections

include $(NXDK_DIR)/Makefile

$(OBJS): $(CURDIR)/source/resources.obj
$(CURDIR)/source/resources.packed.obj: $(CURDIR)/source/resources.obj
	objcopy $< --long-section-names=enable --update-section 'XTIMAGE=$(XBE_XTIMAGE)' $@
