			
.DEFAULT_GOAL := build
FQBN=esp32:esp32:esp32
SKETCH=ESP32-internetRadio.ino
BUILD_DIR=build
BIN=$(BUILD_DIR)/$(SKETCH).bin

.PHONY: build clean check

build: check
	mkdir -p $(BUILD_DIR)
	arduino-cli compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) $(SKETCH)
	@echo "Firmware binary: $(BIN)"

check:
	./configure

clean:
	rm -rf $(BUILD_DIR)
