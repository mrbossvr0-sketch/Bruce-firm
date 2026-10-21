# Bruce Shark — **native Flipper Zero firmware desktop** (source overlay)

This project alters the **Flipper Zero's desktop service inside a firmware build**. It does **not** create a `.fap` app. On boot, the normal desktop is replaced with a monochrome Bruce-themed shark splash screen and the Bruce-style, large-icon carousel.

## Status — read before flashing

- **Source code ready**: C home UI, desktop events, native app launch integration, Bruce-like shark splash, GitHub Actions updater builder.
- **Firmware binary not yet built or hardware-tested**. This package by itself **is not flashable**; the GitHub Actions workflow or a local Flipper firmware toolchain must compile it to a `.tgz` firmware update. Never rename this ZIP to `.tgz` or `.dfu`.
- Based on **Flipper official firmware**, pinned to upstream `d8dfb3147c31c25fe2b69c4a4eed1c04451fa30e`; **not Momentum**. Once flashed successfully, it **replaces Momentum**. The installed app files and backup should be reviewed before flashing.
- **Not genuine ESP32 Bruce firmware**. Flipper Zero uses STM32WB55, a 128x64 monochrome screen, and no built-in Wi-Fi. Bruce's ESP32-only functions cannot run unchanged. Wi-Fi entries honestly explain external hardware requirements.
- Built-in Flipper apps are opened through the firmware's loader; they remain maintained by the underlying Flipper firmware. Supported radio behavior remains subject to hardware and region restrictions.

## Built-in UI navigation

1. On startup: shark splash. Press OK (or an arrow) to enter the Bruce category carousel.
2. Left/right: switch between 12 categories; OK/down: open category.
3. Up/down: choose a command; OK: run it; Back: return.
4. Back from carousel: Flipper applications list.
5. Long Up: lock the Flipper (stock security still applies). Hold Back 5 seconds: initiate device power off.

### Wired-up functionality

| Bruce heading | Native functionality |
| --- | --- |
| RF | Flipper Sub-GHz tools; saved signals |
| NFC | Native NFC reader/writer/emulation UI; saved NFC files |
| RFID | Native 125 kHz RFID UI; saved keys |
| IR | Native infrared transmit/receive UI; saved remotes |
| iButton | Native iButton tools and keys |
| USB | Native Bad USB and U2F security key apps |
| BLE | Built-in Bluetooth settings; installed apps list |
| GPIO | Native GPIO tools |
| Files | Native Archive, installed apps |
| Others | Clock, battery readout, LED blink, vibration, backlight |
| Config | Bluetooth, applications list, lock, power, about |
| WiFi | Hardware requirement explainer; optional installed apps |

**Not included**: ESP32 Wi-Fi scanning, packet injection, hardware-only CC1101 features absent from a stock Flipper, color TFT display, or ESP32 Bruce's complete native toolchain. No false controls for those capabilities.

## Build a flashable firmware (recommended: GitHub Actions)

1. Extract this ZIP, then upload the contents to a **new GitHub repository** with GitHub Actions enabled (folder `.github/workflows` must stay intact).
2. Open the repository's **Actions** tab → **Build Bruce Shark Flipper firmware** → **Run workflow**.
3. When the job finishes successfully, open the workflow run → **Artifacts** → download **Bruce-Shark-Flipper-Zero-Firmware**.
4. Extract that CI artifact and find the **`.tgz` updater package**. **Only flash it if the build succeeded, it targets Flipper Zero f7, and you understand it replaces Momentum.**
5. Make a backup first. Use the official Flipper firmware update instructions / qFlipper and an SD card for recovery. Build/run results must be checked before flashing.

### Local computer build (requires network, disk space, build tools)

```bash
git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git upstream
cd upstream && git checkout d8dfb3147c31c25fe2b69c4a4eed1c04451fa30e && git submodule update --init --recursive && cd ..
python3 apply_overlay.py upstream
cd upstream
./fbt COMPACT=1 DEBUG=0 updater_package
# Look for .tgz in dist/
```

The overlay deliberately checks upstream source text anchors. If they change, it **fails** rather than writing incompatible firmware blindly.

## Safety / limitations

- Back up `ext` / SD card files and settings, and make sure you can recover the firmware before trying experimental images.
- This custom firmware source is **not endorsed by Bruce or Flipper Devices**.
- Switching away from Momentum can change app compatibility. Third-party `.fap` apps may need rebuilding.
- The pixel-art shark is an original monochrome reinterpretation, **not Bruce's exact copyrighted color logo**. The Flipper cannot physically display Bruce's purple/magenta TFT colors.
- The clock redraws upon entering, and readouts redraw when buttons are pressed (live background ticking is not implemented). A first boot/update build and hardware validation are still required.

Useful references: https://github.com/flipperdevices/flipperzero-firmware and https://github.com/BruceDevices/firmware and https://docs.flipper.net/zero/firmware-update
