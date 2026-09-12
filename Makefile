BUILD_DIR := build
PROGRAM := shuff

.PHONY: debug release install remove clean

# --------------------------------------------------
# Windows / MSVC
# --------------------------------------------------

ifeq ($(OS),Windows_NT)

GENERATOR ?= Ninja

debug:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)"
	cmake --build $(BUILD_DIR) --config Debug
	powershell -NoProfile -Command "Copy-Item '$(BUILD_DIR)/$(PROGRAM).exe' './$(PROGRAM).exe' -Force"

release:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)"
	cmake --build $(BUILD_DIR) --config Release
	powershell -NoProfile -Command "Copy-Item '$(BUILD_DIR)/$(PROGRAM).exe' './$(PROGRAM).exe' -Force"

install: release
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force 'C:\Program Files\$(PROGRAM)' | Out-Null; Copy-Item './$(PROGRAM).exe' 'C:\Program Files\$(PROGRAM)\$(PROGRAM).exe' -Force"
	powershell -NoProfile -Command "$$path = [Environment]::GetEnvironmentVariable('Path', 'Machine'); if ($$path -notlike '*C:\Program Files\$(PROGRAM)*') { [Environment]::SetEnvironmentVariable('Path', $$path + ';C:\Program Files\$(PROGRAM)', 'Machine') }"

remove:
	powershell -NoProfile -Command "Remove-Item -Recurse -Force 'C:\Program Files\$(PROGRAM)' -ErrorAction SilentlyContinue"
	powershell -NoProfile -Command "$$path = [Environment]::GetEnvironmentVariable('Path', 'Machine'); $$entry = 'C:\Program Files\$(PROGRAM)'; $$entries = $$path -split ';' | Where-Object { $$_ -and $$_ -ne $$entry }; [Environment]::SetEnvironmentVariable('Path', ($$entries -join ';'), 'Machine')"

clean:
	powershell -NoProfile -Command "Remove-Item -Recurse -Force '$(BUILD_DIR)' -ErrorAction SilentlyContinue; Remove-Item -Force './$(PROGRAM).exe' -ErrorAction SilentlyContinue"

# --------------------------------------------------
# Linux / macOS
# --------------------------------------------------

else

debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR)
	cp $(BUILD_DIR)/$(PROGRAM) ./$(PROGRAM)

release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR)
	cp $(BUILD_DIR)/$(PROGRAM) ./$(PROGRAM)

install: release
	@if [ "$$(uname -s)" = "Darwin" ]; then \
		sudo cp ./$(PROGRAM) /usr/local/bin/$(PROGRAM); \
	elif [ "$$(uname -s)" = "Linux" ]; then \
		sudo cp ./$(PROGRAM) /usr/bin/$(PROGRAM); \
	else \
		echo "Unsupported operating system"; \
		exit 1; \
	fi

remove:
	@if [ "$$(uname -s)" = "Darwin" ]; then \
		sudo rm -f /usr/local/bin/$(PROGRAM); \
	elif [ "$$(uname -s)" = "Linux" ]; then \
		sudo rm -f /usr/bin/$(PROGRAM); \
	else \
		echo "Unsupported operating system"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
	rm -f ./$(PROGRAM)

endif
