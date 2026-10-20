# GBScribe — GBA build (runs inside devkitpro/devkitarm container; see build.sh)

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

PREFIX   := $(DEVKITARM)/bin/arm-none-eabi-
CC       := $(PREFIX)gcc
OBJCOPY  := $(PREFIX)objcopy
GBAFIX   := $(DEVKITPRO)/tools/bin/gbafix
GRIT     := $(DEVKITPRO)/tools/bin/grit
MMUTIL   := $(DEVKITPRO)/tools/bin/mmutil
BIN2S    := $(DEVKITPRO)/tools/bin/bin2s

TARGET   := GBScribe
BUILD    := build

AUDIO    := assets/theme.mod assets/click.wav assets/ding.wav

SRCDIRS  := src src/ui src/core src/storage src/gfx lib/AdvancePrint/src
CFILES   := $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJS     := $(patsubst %.c,$(BUILD)/%.o,$(CFILES)) $(BUILD)/soundbank.o

ARCH     := -mthumb -mthumb-interwork
CFLAGS   := $(ARCH) -mcpu=arm7tdmi -mtune=arm7tdmi -O2 -Wall -Wextra \
            -ffunction-sections -fdata-sections -MMD -MP \
            -I$(DEVKITPRO)/libtonc/include -I$(DEVKITPRO)/libgba/include \
            -Isrc -I$(BUILD) -Ilib/AdvancePrint/include
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

# audio: mod/wav -> soundbank (mmutil emits .bin and .h together)
$(BUILD)/soundbank.h: $(AUDIO)
	@mkdir -p $(BUILD)
	$(MMUTIL) $(AUDIO) -o$(BUILD)/soundbank.bin -h$(BUILD)/soundbank.h
$(BUILD)/soundbank.bin: $(BUILD)/soundbank.h ;

$(BUILD)/soundbank.o: $(BUILD)/soundbank.bin
	$(BIN2S) -a 4 $< > $(BUILD)/soundbank.s
	$(CC) $(ARCH) -x assembler-with-cpp -c $(BUILD)/soundbank.s -o $@

$(BUILD)/src/sound.o: $(BUILD)/soundbank.h

# header dependencies emitted by -MMD (stale .o after a .h edit otherwise)
-include $(OBJS:.o=.d)

clean:
	rm -rf $(BUILD) $(TARGET).gba
