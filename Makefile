# TODO (Khangaroo): Make this process a lot less hacky (no, export did not work)
# See MakefileNSO

.PHONY: all clean starlight send

# Color definitions
RESET := \033[0m
BOLD := \033[1m
RED := \033[31m
GREEN := \033[32m
YELLOW := \033[33m
BLUE := \033[34m
MAGENTA := \033[35m
CYAN := \033[36m
WHITE := \033[37m

GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GIT_COMMIT := $(shell git rev-parse --short HEAD)
GIT_REV    := ${GIT_BRANCH}-${GIT_COMMIT}
ifneq (,$(strip $(shell git status --porcelain 2>/dev/null)))
GIT_REV := $(GIT_REV)-dirty
endif

SMOVER ?= 100
BUILDVER ?= 101
BUILDVERSTR ?= 1.4.0
IP ?= 10.0.0.221 # ftp server ip (usually is switch's local IP)
DEBUGLOG ?= 0 # defaults to disable debug logger 
SERVERIP ?= 0.0.0.0 # put debug logger server IP here

PROJNAME ?= SuperMarioOdysseyOnline

# Single build directory
BUILD_DIR := build$(SMOVER)

all: starlight

starlight:
	$(MAKE) all -f MakefileNSO SMOVER=$(SMOVER) BUILD=$(BUILD_DIR) BUILDVERSTR=$(BUILDVERSTR) BUILDVER=$(BUILDVER) DEBUGLOG=$(DEBUGLOG) SERVERIP=${SERVERIP}
	$(MAKE) starlight_patch_$(SMOVER)/*.ips
	
	@echo ""
	@echo "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(BOLD)$(WHITE)  Creating deployment structures...$(RESET)"
	@echo "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	
	@echo "$(YELLOW)  → Creating Switch (Atmosphere) structure...$(RESET)"
	@mkdir -p starlight_patch_$(SMOVER)/atmosphere/exefs_patches/$(PROJNAME)/
	@mkdir -p starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000/exefs/
	
	@echo "$(YELLOW)  → Creating Emulator (SMOO-Plus) structure...$(RESET)"
	@mkdir -p starlight_patch_$(SMOVER)/SMOO-Plus/exefs/
	@mkdir -p starlight_patch_$(SMOVER)/SMOO-Plus/romfs/
	
	@echo "$(BLUE)  → Copying IPS patches...$(RESET)"
	@cp starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips starlight_patch_$(SMOVER)/atmosphere/exefs_patches/$(PROJNAME)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	@cp starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips starlight_patch_$(SMOVER)/SMOO-Plus/exefs/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	
	@echo "$(BLUE)  → Copying subsdk1 binaries...$(RESET)"
	@cp $(shell basename $(CURDIR))$(SMOVER).nso starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000/exefs/subsdk1
	@cp $(shell basename $(CURDIR))$(SMOVER).nso starlight_patch_$(SMOVER)/SMOO-Plus/exefs/subsdk1
	
	@echo "$(BLUE)  → Moving ELF debug symbols...$(RESET)"
	@mv $(shell basename $(CURDIR))$(SMOVER).elf starlight_patch_$(SMOVER)/subsdk1.elf
	
	@echo "$(BLUE)  → Copying romfs data...$(RESET)"
	@cp -R romfs starlight_patch_$(SMOVER)/atmosphere/contents/0100000000010000
	@cp -R romfs/* starlight_patch_$(SMOVER)/SMOO-Plus/romfs/ 2>/dev/null || true
	
	@echo "$(MAGENTA)  → Cleaning up temporary files...$(RESET)"
	@rm -f starlight_patch_$(SMOVER)/3CA12DFAAF9C82DA064D1698DF79CDA1.ips
	@rm -f $(shell basename $(CURDIR))$(SMOVER).nso

	@echo ""
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(BOLD)$(GREEN)  ✓ Build complete!$(RESET)"
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(WHITE)  Switch (Atmosphere):  $(CYAN)starlight_patch_$(SMOVER)/atmosphere/$(RESET)"
	@echo "$(WHITE)  Emulator (SMOO-Plus): $(CYAN)starlight_patch_$(SMOVER)/SMOO-Plus/$(RESET)"
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo ""

starlight_patch_$(SMOVER)/*.ips: patches/*.slpatch patches/configs/$(SMOVER).config patches/maps/$(SMOVER)/*.map \
								$(BUILD_DIR)/$(shell basename $(CURDIR))$(SMOVER).map scripts/genPatch.py
	@rm -f starlight_patch_$(SMOVER)/*.ips
	python3 scripts/genPatch.py $(SMOVER) $(BUILD_DIR)

# builds and sends project to FTP server hosted on provided IP
send: all
	python3 scripts/sendPatch.py $(IP) $(PROJNAME)

log: all
	python3 scripts/tcpServer.py $(SERVERIP)

sendlog: all
	python3 scripts/sendPatch.py $(IP) $(PROJNAME) $(USER) $(PASS)
	python3 scripts/tcpServer.py $(SERVERIP)

clean:
	@echo "$(RED)Cleaning build files...$(RESET)"
	$(MAKE) clean -f MakefileNSO
	@rm -fr starlight_patch_*
	@echo "$(GREEN)Clean complete.$(RESET)"