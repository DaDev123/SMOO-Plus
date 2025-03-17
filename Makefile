.PHONY: debug clean

debug:
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build && $(MAKE) -C build

release: clean
	cmake -DCMAKE_BUILD_TYPE=Release -S . -B build && ninja -C build

clean:
	rm -r build || true
