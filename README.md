# NootedGreen for Intel Iris Xe on Raptor Lake-U (8086:A7A1)

This HP 250 G10 port adds accelerated internal graphics and direct HDMI support for the Core i5-1334U configuration tested on macOS Tahoe 26.7.2 (25G332). On 22 November 2026 the owner confirmed a correctly colored extended desktop on an ASUS VX228 while the internal panel remained usable.

## Tested hardware

| Component | Reference |
| --- | --- |
| GPU | Intel 8086:A7A1 rev 04, HP 103C:8B34, 80 EUs |
| Internal panel | 1920 × 1080 eDP |
| HDMI | Built-in socket, DDI B, Combo PHY B, GMBUS pin 2 |
| External mode | ASUS VX228, 1920 × 1080 at 60 Hz, RGB 8-bit SDR |

The verified sequence was to boot with HDMI disconnected and plug in the monitor at the desktop. HDMI audio, USB-C video, higher pixel clocks, cold boot with HDMI attached and external-display sleep/wake remain unverified.

## What changed

The HP profile selects the GPU topology and handles the 60 MiB of firmware-reserved graphics memory. The HDMI code reads the monitor EDID over GMBUS, limits the external modeset and PLL routing to the measured HP path, requests the display power domains for pipe B and checks for frame progress. In v31 the external display showed a desktop with rainbow bands and black areas in white windows. The v32 build applies the legacy gamma mode to the admitted external pipe. Higher-resolution modes need separate validation.

The driver implementation is in [NootedGreen](NootedGreen), with HDMI helpers in [HPHDMI](NootedGreen/HPHDMI). The HP modifications build on the [dssoftx X1 Carbon port at 5098a08](https://github.com/dssoftx/Hackintosh-X1Carbon-G11-IrisXe-Tahoe/tree/5098a08c68cd03f279670027e8742488011dd245). The original driver is [sgiammori/NootedGreen](https://github.com/sgiammori/NootedGreen). The HP-specific setup, sanitized OpenCore configuration, tested binary and macOS graphics-stack instructions are in the [HP 250 G10 EFI repository](https://github.com/tarekwasfy01/HP-250-G10-OpenCore-Tahoe/tree/f411faa79122c6656967c615599e835251081ffa).

## Install the macOS graphics components

Download **HP250-Graphics-Installer-Tools-v32.pkg** and the matching kext ZIP from [the v32 release](https://github.com/tarekwasfy01/NootedGreen-for-Intel-Iris-Xe-on-Raptor-Lake-U-8086-A7A1-/releases/tag/hp-a7a1-v32). Verify the Developer ID Installer signature and checksum before installing the tools. The public PKG installs the packager, manifest and licenses. A working Python 3.9+ and a verified source copy of the system graphics components are required to assemble the full installer. See [installation and rollback](installer/README.md) for commands, including signing the locally built package and how to use a separate mounted source volume on a fresh installation.

## Build and test

On macOS with Xcode Command Line Tools, run the following from the repository root. The build script fetches MacKernelSDK and Lilu 1.7.2 DEBUG on first use.

```sh
zsh build-nootedgreen.sh
codesign --verify --strict build/NootedGreen.kext
```

Use the resulting bundle as `EFI/OC/Kexts/NootedGreen.kext` on a separate test EFI with the HP configuration and matching system-side drivers. Keep a working boot path. Use the HDMI boot arguments from the linked HP EFI. The runtime marker for this revision is `hp-hdmi-v32-pipe-b-legacy-gamma`.

```sh
ioreg -lw0 | grep -E 'HPHDMITrialBuild|HPHDMIGammaB'
```

The [host-side tests](tests) cover HP policy and HDMI helpers. Run them from the repository root with a C++17 compiler:

```sh
(
  set -e
  testdir=$(mktemp -d)
  trap 'rm -rf "$testdir"' EXIT
  for source in tests/*.cpp; do
    clang++ -std=c++17 "$source" -o "$testdir/check"
    "$testdir/check"
  done
)
```

The executable distributed in the linked EFI has SHA-256 `02b8678bcf05919e59b9e2a40805044157af9c7a8fc4e40fa59a6dcc9c129f93`. This fingerprint identifies the tested binary. Locally rebuilt binaries can have different UUIDs and hashes.

## Credits and license

See [component notices and licenses](installer/Licenses) and the [NootedGreen license](LICENSE). The HP profile and HDMI v32 changes are local modifications of the dssoftx source. The original project history and component licenses are retained.
