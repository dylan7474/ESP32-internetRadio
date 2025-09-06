.DEFAULT_GOAL := build
FQBN=esp32:esp32:esp32
SKETCH=iradio_v6_17.ino
SKETCH_COPY=ESP32-internetRadio.ino
BUILD_DIR=build
BIN=$(BUILD_DIR)/$(SKETCH_COPY).bin

.PHONY: build clean

build:
	cp $(SKETCH) $(SKETCH_COPY)
	arduino-cli compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) .
	@echo "Firmware binary: $(BIN)"
	rm $(SKETCH_COPY)

clean:
	rm -rf $(BUILD_DIR)
