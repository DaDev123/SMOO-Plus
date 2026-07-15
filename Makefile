.PHONY: debug clean release file_structure release_build0

BUILDVER ?= SR-1.0.0-pre
DEBUGLOG ?= 1 # defaults to enable debug logger 
SERVERIP ?= 192.168.178.41 # put debug logger server IP here

PROJNAME ?= SMOO-Plus-Speedrun

SCONTENTPATH := package/$(PROJNAME)-Switch/atmosphere/contents/0100000000010000
ECONTENTPATH :=  package/$(PROJNAME)-Emulator/$(PROJNAME)

#I hate Nixos (sometimes)
export SOURCE_DATE_EPOCH = $(shell date +%s)


debug: format
	cmake -DCMAKE_BUILD_TYPE=Debug -DPROJNAME=$(PROJNAME) -DBUILDVER=$(BUILDVER) -DSERVERIP=$(SERVERIP) -DDEBUGLOG=$(DEBUGLOG) -S . -B build && $(MAKE) -C build

release_build: clean format
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPROJNAME=$(PROJNAME) -DBUILDVER=$(BUILDVER) -S . -B build && $(MAKE) -C build

release:
	$(MAKE) release_build
	$(MAKE) file_structure

setup:
	python sys/tools/setup_libcxx_prepackaged.py
	python sys/tools/setup_sail.py

format:
	-clear
	find src include lib/custom -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format -i

clean:
	rm -rf build package

file_structure:
	
# 	Create Switch (Atmosphere) structure
	@mkdir -p $(SCONTENTPATH)/exefs/
	
# 	Create Emulator (SMOO-Plus) structure
	@mkdir -p $(ECONTENTPATH)/exefs/
	@mkdir -p $(ECONTENTPATH)/romfs/

# 	Copy subsdk binaries
	@cp build/$(PROJNAME).nso $(SCONTENTPATH)/exefs/subsdk4 
	@cp build/$(PROJNAME).nso $(ECONTENTPATH)/exefs/subsdk4 

# 	Copy npdm file
	@cp build/main.npdm $(SCONTENTPATH)/exefs/main.npdm 
	@cp build/main.npdm $(ECONTENTPATH)/exefs/main.npdm 

# 	Copy NSS debug symbols
	@cp build/$(PROJNAME).nss package/$(PROJNAME).nss
	
# 	Copying romfs data
	@cp -R romfs/ $(SCONTENTPATH)
	@cp -R romfs/ $(ECONTENTPATH)

	@echo -e ""
	@echo -e "\e[32m════════════════════════════════════"
	@echo -e "\e[1m         ✓ Build complete!\e[0m"
	@echo -e "\e[32m════════════════════════════════════\e[0m"
	@echo -e ""
