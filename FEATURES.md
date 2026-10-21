# Functionality assessment

These categories are **menu links to working native Flipper firmware apps**, not rewritten Bruce tooling. Actual native performance and any optional firmware app availability depend on the upstream build and hardware.

- Native: Sub-GHz, NFC, LF RFID, IR, iButton, Bad USB, U2F, Bluetooth settings, GPIO, Archive, apps menu, clock, battery, status LED, vibration, backlight, lock, power.
- ESP32-specific: WiFi scan, WiFi packet monitoring, BLE functions beyond Flipper's Bluetooth radio, network services, USB-Ethernet. Not included. External WiFi boards require firmware and a compatible interface FAP.
- Third-party external FAP availability: not bundled; apps menu only opens what is installed.
- Display fidelity: Bruce-style border, 12 category menu carousel, monochrome vector icons, shark splash, battery top-bar. TFT colors, rotary control and graphics cannot match LILYGO hardware on 128x64 screen.
- Firmware completeness: source code modifications supplied, **build not performed in this environment**, binary not verified, hardware not tested.
