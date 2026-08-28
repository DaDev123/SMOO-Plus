.PHONY: debug clean release package format

PROJNAME ?= SMOO-Plus-Speedrun
BUILDVER ?= SR-1.0.1-pre

LOGGER ?= 0
LOGGERIP ?= 192.168.178.41


SCONTENTPATH := package/$(PROJNAME)-Switch/atmosphere/contents/0100000000010000
ECONTENTPATH :=  package/$(PROJNAME)-Emulator/$(PROJNAME)

CMAKE_FLAGS = -G "Ninja"\
			  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
			  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
			  -DBUILDVER=$(BUILDVER) -DPROJNAME=$(PROJNAME)

#I hate Nixos (sometimes)
export SOURCE_DATE_EPOCH = $(shell date +%s)

debug: format
	cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -DLOGGER=$(LOGGER) -DLOGIP=$(LOGGERIP) -S . -B build/debug
	ln -sf build/debug/compile_commands.json .
	cmake --build build/debug

release: format
	cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B build/release
	cmake --build build/release
	$(MAKE) package

format:
	-clear
	python3 check_includes.py
	find src include lib/custom  -type f -regex ".*\.[ch]p*" -exec clang-format -i {} '+'

clean:
	rm -rf build package compile_commands.json

package:
	
# 	Create Switch (Atmosphere) structure
	@mkdir -p $(SCONTENTPATH)/exefs/
	
# 	Create Emulator (SMOO-Plus) structure
	@mkdir -p $(ECONTENTPATH)/exefs/
	@mkdir -p $(ECONTENTPATH)/romfs/

# 	Copy subsdk binaries
	@cp build/release/$(PROJNAME).nso $(SCONTENTPATH)/exefs/subsdk4 
	@cp build/release/$(PROJNAME).nso $(ECONTENTPATH)/exefs/subsdk4 

# 	Copy npdm file
	@cp build/release/main.npdm $(SCONTENTPATH)/exefs/main.npdm 
	@cp build/release/main.npdm $(ECONTENTPATH)/exefs/main.npdm 

# 	Copy NSS debug symbols
	@cp build/release/$(PROJNAME).nss package/$(PROJNAME).nss
	
# 	Copying romfs data
	@cp -R romfs/ $(SCONTENTPATH)
	@cp -R romfs/ $(ECONTENTPATH)

	@echo -e "\n\e[32m════════════════════════════════════"
	@echo -e "\e[1m         ✓ Build complete!\e[0m"
	@echo -e "\e[32m════════════════════════════════════\e[0m"