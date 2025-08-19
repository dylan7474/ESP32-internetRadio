# RPi Internet Radio

An ESP32-based internet radio player built with the Arduino IDE. It streams preset stations over Wi-Fi and uses a small display with hardware knobs for channel and volume control.

## Building on Linux

1. Verify dependencies:
   ```bash
   ./configure
   ```
2. Compile the project:
   ```bash
   make
   ```

## Basic Controls

- Rotate the **channel knob** to cycle through stored stations.
- Short press the channel knob to switch between Day, Night and Dark display modes.
- Rotate the **volume knob** to adjust playback level.
- Long press the volume knob to power the radio off.

## Roadmap

- Display detailed status during firmware updates with better error handling.
- Ensure menus redraw correctly when returning from other screens.
- Speed up the over‑the‑air update process.
