# devkitPro's ds_rules pattern; original DS ARM9 build
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM to the installed devkitARM path)
endif
include $(DEVKITARM)/ds_rules
TARGET := my-baby-ai
BUILD := build
SOURCES := source
INCLUDES := include
ARCH := -march=armv5te -mtune=arm946e-s -mthumb
CFLAGS := -g -O2 -Wall -Wextra -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lfat -lnds9
LIBDIRS := $(LIBNDS) $(PORTLIBS)
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export LD := $(CC)
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: $(BUILD) clean
$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).nds
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).nds : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)
-include $(DEPENDS)
endif
