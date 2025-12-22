.PHONY: debug clean release

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

SMOVER ?= 100
BUILDVER ?= 101
BUILDVERSTR ?= 1.4.0
IP ?= 10.0.0.221 # ftp server ip (usually is switch's local IP)
DEBUGLOG ?= 0 # defaults to disable debug logger 
SERVERIP ?= 0.0.0.0 # put debug logger server IP here

PROJNAME ?= SMOO-Plus

SWITCHPATH := package/$(PROJNAME)-Switch
EMUPATH := package/$(PROJNAME)-Emulator

SCONTENTPATH := $(SWITCHPATH)/atmosphere/contents/0100000000010000
ECONTENTPATH :=  $(EMUPATH)/$(PROJNAME)


debug: format
	cmake -DCMAKE_BUILD_TYPE=Debug -DDEBUG=TRUE -DPROJNAME=$(PROJNAME) -DSMOVER=$(SMOVER) -DBUILDVER=$(BUILDVER) -DBUILDVERSTR=$(BUILDVERSTR) -DSERVERIP=$(SERVERIP) -DDEBUGLOG=$(DEBUGLOG) -S . -B build && $(MAKE) -C build

release: clean format
	cmake -DCMAKE_BUILD_TYPE=Debug -DDEBUG=FALSE -DPROJNAME=$(PROJNAME) -DSMOVER=$(SMOVER) -DBUILDVER=$(BUILDVER) -DBUILDVERSTR=$(BUILDVERSTR) -DSERVERIP=$(SERVERIP) -DDEBUGLOG=$(DEBUGLOG) -S . -B build && $(MAKE) -C build
	python ./make-Release/release.py

format:
	clear
	find ./src -name "*.*" | xargs clang-format -i

clean:
	rm -r build || true

file_structure:
	@echo ""
	@echo "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(BOLD)$(WHITE)  Creating deployment structures...$(RESET)"
	@echo "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	
	@echo "$(YELLOW)  → Creating Switch (Atmosphere) structure...$(RESET)"
	@mkdir -p $(SCONTENTPATH)/exefs/
	
	@echo "$(YELLOW)  → Creating Emulator (SMOO-Plus) structure...$(RESET)"
	@mkdir -p $(EMUPATH)/$(PROJNAME)/exefs/
	@mkdir -p $(EMUPATH)/$(PROJNAME)/romfs/

	@echo "$(BLUE)  → Copying subsdk binaries...$(RESET)" 
	@cp build/$(PROJNAME).nso $(SCONTENTPATH)/exefs/subsdk4 
	@cp build/$(PROJNAME).nso $(ECONTENTPATH)/exefs/subsdk4 

	@echo "$(BLUE)  → Copying npdm file...$(RESET)"
	@cp build/main.npdm $(SCONTENTPATH)/exefs/main.npdm 
	@cp build/main.npdm $(ECONTENTPATH)/exefs/main.npdm 

	@echo "$(BLUE)  → Moving NSS debug symbols...$(RESET)"
	@mv build/$(PROJNAME).nss package/$(PROJNAME).nss 
	
	@echo "$(BLUE)  → Copying romfs data...$(RESET)"
	@cp -R romfs $(SCONTENTPATH)
	@cp -R romfs $(ECONTENTPATH) 2>/dev/null || true

	@echo ""
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(BOLD)$(GREEN)  ✓ Build complete!$(RESET)"
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo "$(WHITE)  Switch (Atmosphere):  $(CYAN)package/$(PROJNAME)-Switch/atmosphere/$(RESET)"
	@echo "$(WHITE)  Emulator (SMOO-Plus): $(CYAN)package/$(PROJNAME)-Emulator/SMOO-Plus/$(RESET)"
	@echo "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo ""