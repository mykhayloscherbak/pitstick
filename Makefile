# 
# Based on tech02_3 application build recipe
#
# Copyright Oleksiy Mikoyan 2018.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# Project name
PROJECT := pitstick

# Sources root folder
SRC_DIR := sources

# Root folder for all output files, empty prohibited
OUTPUT_DIR := output

# Subfolder for object files
OBJ_DIR := obj

# Subfolder for elf and bin files
EXE_DIR := exe

# Subfolder for map, listing and size
LST_DIR := exe

# Output files
OUTPUT_BIN := $(PROJECT).bin
OUTPUT_HEX := $(PROJECT).hex
OUTPUT_ELF := $(PROJECT).elf
OUTPUT_MAP := $(PROJECT).map
OUTPUT_LST := $(PROJECT).lst
OUTPUT_SIZ := $(PROJECT).siz

# Linker script
LDSCRIPT := scripts/STM32F103C8_FLASH.ld
#LDSCRIPT := scripts/STM32F103RBTx_FLASH.ld

# Application stack sizes
STACK_SIZE := 0x400

# Tools and their command-line options
AS      := arm-none-eabi-gcc
CC      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJDUMP := arm-none-eabi-objdump
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
MKDIR   := mkdir
RM      := rm

ARCH_OPTS := -mcpu=cortex-m3
ARCH_OPTS += -mthumb

CFLAGS := -std=c11
CFLAGS += -ffreestanding

CFLAGS += -Og
#CFLAGS += -freorder-blocks-algorithm=stc

#CFLAGS += -fno-tree-switch-conversion
#CFLAGS += -fno-jump-tables
#CFLAGS += -fno-inline
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -fno-common
#CFLAGS += -pedantic
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-declarations
CFLAGS += -Wredundant-decls
CFLAGS += -Wnested-externs
CFLAGS += -Wvla
CFLAGS += -Werror
#CFLAGS += -Wunused-macros
#CFLAGS += -Wconversion
CFLAGS += -Winit-self
CFLAGS += -Wlogical-op
CFLAGS += -g3
CFLAGS += -gdwarf-2
CFLAGS += -gstrict-dwarf

CFLAGS += -DSTM32F103xB

LDFLAGS := -nostdlib
LDFLAGS += -lgcc 
LDFLAGS += -Xlinker --gc-sections --specs=nano.specs --specs=nosys.specs -Wl,--print-memory-usage
LDFLAGS += -Wl,--undefined=uxTopUsedPriority


########### End of configuration section ###########
# Everything below are derived variables and targets

# Helper function to build list of non-empty sub-folders
get_sub_dirs = $(foreach DIR, $(filter $(1)/%, $(wildcard $(1)/*)), $(if $(wildcard $(DIR)/*), $(DIR) $(call get_sub_dirs, $(DIR))))

# Build list of folders that contain either files or sub-folders or both
SRC_SUB_DIRS := $(call get_sub_dirs, $(SRC_DIR))

# Build full list of source files
SRCS := $(foreach DIR, $(SRC_DIR) $(SRC_SUB_DIRS), $(wildcard $(DIR)/*.c))

#Build full list of asm files
ASMS := $(foreach DIR, $(SRC_DIR) $(SRC_SUB_DIRS), $(wildcard $(DIR)/*.S))

# Build list of source folders
SRC_DIRS := $(foreach DIR, $(SRC_DIR) $(SRC_SUB_DIRS), $(if $(filter $(DIR)%, $(SRCS) $(ASMS)), $(DIR)))

# Build list of folders to look for headers
# When "iquote" is used all discovered header paths
# have to be added manually to "CDT User Settings Entries" -
# "iquote" is not recognized by build output parser
# Another option is to use "I" instead of "iquote"
INC_DIRS := $(foreach DIR, $(SRC_DIR) $(SRC_SUB_DIRS), $(if $(wildcard $(DIR)/*.h), $(DIR)))
#INC_OPTS := $(strip $(patsubst %, -iquote%, $(INC_DIRS)))
INC_OPTS := $(strip $(patsubst %, -I%, $(INC_DIRS)))

# Build list of output folders
OUTPUT_OBJ_DIR := $(OUTPUT_DIR)/$(OBJ_DIR)
OUTPUT_EXE_DIR := $(OUTPUT_DIR)/$(EXE_DIR)
OUTPUT_LST_DIR := $(OUTPUT_DIR)/$(LST_DIR)
OUTPUT_OBJ_DIRS := $(patsubst $(SRC_DIR)%, $(OUTPUT_OBJ_DIR)%, $(SRC_DIRS))
OUTPUT_DIRS := $(sort $(OUTPUT_OBJ_DIRS) $(OUTPUT_EXE_DIR) $(OUTPUT_LST_DIR))

# Targets
OBJSC := $(strip $(patsubst $(SRC_DIR)/%.c, $(OUTPUT_OBJ_DIR)/%.o, $(SRCS)))
OBJSASM := $(strip $(patsubst $(SRC_DIR)/%.S, $(OUTPUT_OBJ_DIR)/%.o, $(ASMS)))
OBJS := $(OBJSC) $(OBJSASM)

# Dependencies
DEPSC   := $(strip $(patsubst $(SRC_DIR)/%.c, $(OUTPUT_OBJ_DIR)/%.d, $(SRCS)))
DEPSASM := $(strip $(patsubst $(SRC_DIR)/%.S, $(OUTPUT_OBJ_DIR)/%.d, $(ASMS)))
DEPS    := $(DEPSC) $(DEPSASM)
ifeq ($(MAKECMDGOALS), all_with_deps)
# Include dependency files for further reference
include $(DEPS)
endif

# Create a list of initally empty cleanup targets
CLEANUP :=
# Build list of all object folders
OLD_OBJ_DIRS    := $(OUTPUT_OBJ_DIR) $(call get_sub_dirs, $(OUTPUT_OBJ_DIR))
# Build list of orphan object folders
ORPHAN_OBJ_DIRS := $(filter-out $(OUTPUT_OBJ_DIRS), $(OLD_OBJ_DIRS))
# Build list of all object files on filesystem
OLD_OBJS        := $(foreach DIR, $(OLD_OBJ_DIRS), $(wildcard $(DIR)/*.o))
# Build list of all orphan object files
ORPHAN_OBJS     := $(filter-out $(OBJS), $(OLD_OBJS))
# Build list of all dependency files on filesystem
OLD_DEPS        := $(foreach DIR, $(OLD_OBJ_DIRS), $(wildcard $(DIR)/*.d))
# Build list of all orphan dependency files
ORPHAN_DEPS     := $(filter-out $(DEPS), $(OLD_DEPS))
# 
# Add cleanup targets if necessary
ifneq ($(ORPHAN_OBJS), )
CLEANUP += remove_orphan_objs
endif
ifneq ($(ORPHAN_DEPS), )
CLEANUP += remove_orphan_deps
endif
ifneq ($(strip $(CLEANUP)), )
# Having orphan object or dependency file implies source file was removed
# When any of them is removed, binaries shall also be removed
# to ensure all object files that correspond to all present source files
# can still be linked successfully 
CLEANUP += remove_binaries
endif
ifneq ($(ORPHAN_OBJ_DIRS), )
CLEANUP += remove_orphan_obj_dirs
endif

# Add stack sizes to linker flags
LDFLAGS += -Xlinker --defsym=__stack_size__=$(STACK_SIZE)
LDFLAGS += -Xlinker -Map=$(OUTPUT_LST_DIR)/$(OUTPUT_MAP)

.PHONY : all
all: $(CLEANUP) create_output_dirs build_deps
	@$(MAKE) --no-print-directory all_with_deps

.PHONY : create_output_dirs
create_output_dirs: $(OUTPUT_DIRS)
 
$(OUTPUT_DIRS):
	$(MKDIR) $@

.PHONY : build_deps
build_deps: $(DEPSC) $(DEPSASM)


$(DEPSC): Makefile
	$(CC) $(INC_OPTS) $(CFLAGS) -MM -MP -MF$@ -MT'$(@:%.d=%.o) $(@)' $(patsubst $(OUTPUT_OBJ_DIR)/%.d, $(SRC_DIR)/%.c, $@)

$(DEPSASM): Makefile
	$(CC) $(INC_OPTS) $(CFLAGS) -MM -MP -MF$@ -MT'$(@:%.d=%.o) $(@)' $(patsubst $(OUTPUT_OBJ_DIR)/%.d, $(SRC_DIR)/%.S, $@)


.PHONY : all_with_deps
all_with_deps: build_all

.PHONY : build_all
build_all: create_binary create_listing print_size

.PHONY : create_binary
create_binary: $(OUTPUT_EXE_DIR)/$(OUTPUT_BIN) $(OUTPUT_EXE_DIR)/$(OUTPUT_HEX)

$(OUTPUT_EXE_DIR)/$(OUTPUT_BIN): $(OUTPUT_EXE_DIR)/$(OUTPUT_ELF)
	$(OBJCOPY) -O binary $< $@

$(OUTPUT_EXE_DIR)/$(OUTPUT_HEX): $(OUTPUT_EXE_DIR)/$(OUTPUT_ELF)
	$(OBJCOPY) -O ihex $< $@

$(OUTPUT_EXE_DIR)/$(OUTPUT_ELF): $(OBJS) $(LDSCRIPT) Makefile
	$(LD) -o $@ $(OBJS) $(ARCH_OPTS) $(LDFLAGS) -T$(LDSCRIPT)

$(OBJSC): Makefile
	$(CC) -c $(ARCH_OPTS) $(CFLAGS) $(INC_OPTS) $(patsubst $(OUTPUT_OBJ_DIR)/%.o, $(SRC_DIR)/%.c, $@) -o $@

$(OBJSASM): Makefile
	$(AS) -c $(ARCH_OPTS) $(CFLAGS) $(INC_OPTS) $(patsubst $(OUTPUT_OBJ_DIR)/%.o, $(SRC_DIR)/%.S, $@) -o $@


.PHONY : create_listing
create_listing: $(OUTPUT_LST_DIR)/$(OUTPUT_LST)

$(OUTPUT_LST_DIR)/$(OUTPUT_LST): $(OUTPUT_EXE_DIR)/$(OUTPUT_ELF)
	$(OBJDUMP) -h -S $< > $@

.PHONY : print_size
print_size: $(OUTPUT_LST_DIR)/$(OUTPUT_SIZ)

$(OUTPUT_LST_DIR)/$(OUTPUT_SIZ): $(OUTPUT_EXE_DIR)/$(OUTPUT_ELF)
	$(SIZE) --format=sysv $< > $@
#	@$(SIZE) -A -d $<       # Duplicate size information to console

.PHONY : clean
clean: remove_binaries
	$(RM) -f $(DEPS)
	$(RM) -f $(OBJS)
	$(RM) -f $(OUTPUT_LST_DIR)/$(OUTPUT_MAP)
	$(RM) -f $(OUTPUT_LST_DIR)/$(OUTPUT_LST)
	$(RM) -f $(OUTPUT_LST_DIR)/$(OUTPUT_SIZ)
	$(RM) -f -r $(filter-out . $(SRC_DIR) $(SRC_SUB_DIRS), $(OUTPUT_DIRS))

.PHONY : remove_binaries
remove_binaries:
	$(RM) -f $(OUTPUT_EXE_DIR)/$(OUTPUT_BIN)
	$(RM) -f $(OUTPUT_EXE_DIR)/$(OUTPUT_HEX)
	$(RM) -f $(OUTPUT_EXE_DIR)/$(OUTPUT_ELF)

.PHONY : remove_orphan_objs
remove_orphan_objs:
	$(RM) $(ORPHAN_OBJS)

.PHONY : remove_orphan_deps
remove_orphan_deps:
	$(RM) $(ORPHAN_DEPS)

.PHONY : remove_orphan_obj_dirs
remove_orphan_obj_dirs:
	$(RM) -f -r $(filter-out . $(SRC_DIR) $(SRC_SUB_DIRS), $(ORPHAN_OBJ_DIRS))
