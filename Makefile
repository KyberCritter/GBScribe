# GBScribe — GBA build (runs inside devkitpro/devkitarm container; see build.sh)

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

PREFIX   := $(DEVKITARM)/bin/arm-none-eabi-
CC       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy
GBAFIX   := $(DEVKITPRO)/tools/bin/gbafix
GRIT     := $(DEVKITPRO)/tools/bin/grit

TARGET   := GBScribe
BUILD    := build

SRCDIRS  := src src/ui src/core src/printer src/storage src/gfx
CFILES   := $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJS     := $(patsubst %.c,$(BUILD)/%.o,$(CFILES))

ARCH     := -mthumb -mthumb-interwork
CFLAGS   := $(ARCH) -mcpu=arm7tdmi -mtune=arm7tdmi -O2 -Wall -Wextra \
            -ffunction-sections -fdata-sections \
            -I$(DEVKITPRO)/libtonc/include -I$(DEVKITPRO)/libgba/include \
            -Isrc
LDFLAGS  := $(ARCH) -specs=gba.specs -Wl,-Map,$(BUILD)/$(TARGET).map -Wl,--gc-sections
LIBS     := -L$(DEVKITPRO)/libtonc/lib -ltonc \
            -L$(DEVKITPRO)/libgba/lib -lmm

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).gba: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tGBSCRIBE -cGBSC -mSR

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET).gba
