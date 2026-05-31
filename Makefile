.PHONY: debug clean release file_structure release_build

# Color definitions
RESET := \e[0m
BOLD := \e[1m
RED := \e[31m
GREEN := \e[32m
YELLOW := \e[33m
BLUE := \e[34m
MAGENTA := \e[35m
CYAN := \e[36m
WHITE := \e[37m

SMOVER ?= 100
BUILDVER ?= 101
BUILDVERSTR ?= 0.8.0-SR-pre
DEBUGLOG ?= 1 # defaults to disable debug logger 
SERVERIP ?= 192.168.178.41 # put debug logger server IP here

PROJNAME ?= SMOO-Plus-Speedrun

SWITCHPATH := package/$(PROJNAME)-Switch
EMUPATH := package/$(PROJNAME)-Emulator

SCONTENTPATH := $(SWITCHPATH)/atmosphere/contents/0100000000010000
ECONTENTPATH :=  $(EMUPATH)/$(PROJNAME)


debug: format
	cmake -DCMAKE_BUILD_TYPE=Debug -DDEBUG=TRUE -DPROJNAME=$(PROJNAME) -DSMOVER=$(SMOVER) -DBUILDVER=$(BUILDVER) -DBUILDVERSTR=$(BUILDVERSTR) -DSERVERIP=$(SERVERIP) -DDEBUGLOG=$(DEBUGLOG) -S . -B build && $(MAKE) -C build

release_build: clean format
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDEBUG=FALSE -DPROJNAME=$(PROJNAME) -DSMOVER=$(SMOVER) -DBUILDVER=$(BUILDVER) -DBUILDVERSTR=$(BUILDVERSTR) -DSERVERIP=$(SERVERIP) -DDEBUGLOG=0 -S . -B build && $(MAKE) -C build

release:
	$(MAKE) release_build
	$(MAKE) file_structure

setup:
	python sys/tools/setup_libcxx_prepackaged.py
	python sys/tools/setup_sail.py

format:
	clear || true
	find ./src -name "*.*" | xargs clang-format -i || true
	find ./include -name "*.*" | xargs clang-format -i || true
	find ./lib/custom -name "*.*" | xargs clang-format -i || true

clean:
	yes | rm -r build package || true

file_structure:
	@echo -e ""
	@echo -e "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo -e "$(BOLD)$(WHITE)  Creating deployment structures...$(RESET)"
	@echo -e "$(CYAN)════════════════════════════════════════════════════════════════$(RESET)"
	
	@echo -e "$(YELLOW)  → Creating Switch (Atmosphere) structure...$(RESET)"
	@mkdir -p $(SCONTENTPATH)/exefs/
	
	@echo -e "$(YELLOW)  → Creating Emulator (SMOO-Plus) structure...$(RESET)"
	@mkdir -p $(EMUPATH)/$(PROJNAME)/exefs/
	@mkdir -p $(EMUPATH)/$(PROJNAME)/romfs/

	@echo -e "$(BLUE)  → Copying subsdk binaries...$(RESET)" 
	@cp build/$(PROJNAME).nso $(SCONTENTPATH)/exefs/subsdk4 
	@cp build/$(PROJNAME).nso $(ECONTENTPATH)/exefs/subsdk4 

	@echo -e "$(BLUE)  → Copying npdm file...$(RESET)"
	@cp build/main.npdm $(SCONTENTPATH)/exefs/main.npdm 
	@cp build/main.npdm $(ECONTENTPATH)/exefs/main.npdm 

	@echo -e "$(BLUE)  → Moving NSS debug symbols...$(RESET)"
	@mv build/$(PROJNAME).nss package/$(PROJNAME).nss || true
	
	@echo -e "$(BLUE)  → Copying romfs data...$(RESET)"
	@cp -R romfs/ $(SCONTENTPATH)
	@cp -R romfs/ $(ECONTENTPATH) 2>/dev/null || true

	@echo -e ""
	@echo -e "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo -e "$(BOLD)$(GREEN)  ✓ Build complete!$(RESET)"
	@echo -e "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo -e "$(WHITE)  Switch (Atmosphere):  $(CYAN)package/$(PROJNAME)-Switch/atmosphere/$(RESET)"
	@echo -e "$(WHITE)  Emulator (SMOO-Plus): $(CYAN)package/$(PROJNAME)-Emulator/SMOO-Plus/$(RESET)"
	@echo -e "$(GREEN)════════════════════════════════════════════════════════════════$(RESET)"
	@echo -e ""
