BUILD_DIR := build

.PHONY: debug release install clean

debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR)
	cp $(BUILD_DIR)/shuff ./shuff

release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR)
	cp $(BUILD_DIR)/shuff ./shuff

install: release
	@if [ "$$(uname -s)" = "Darwin" ]; then \
		sudo cp ./shuff /usr/local/bin/shuff; \
	elif [ "$$(uname -s)" = "Linux" ]; then \
		sudo cp ./shuff /usr/bin/shuff; \
	else \
		echo "Unsupported operating system"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
	rm -f ./shuff