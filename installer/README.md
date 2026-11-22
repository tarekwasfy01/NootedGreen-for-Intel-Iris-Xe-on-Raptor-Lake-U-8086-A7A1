# Install the HP 250 G10 graphics stack on macOS

**Target:** HP 250 G10, Core i5-1334U, Intel 8086:A7A1 rev 04, Intel macOS Tahoe build 25G332. Run `sw_vers -buildVersion` to check. The tested setup uses OpenCore 1.0.8 and the [matching HP EFI](https://github.com/tarekwasfy01/HP-250-G10-OpenCore-Tahoe/tree/f411faa79122c6656967c615599e835251081ffa). Keep a recovery boot device and a backup of the working macOS installation. Graphics files installed under macOS affect every EFI that boots it.

If graphics already work on this system, the HDMI v32 update requires the matching EFI driver and boot arguments. Keep the working system stack.

## 1. Install the public tools package

Download the tools PKG from the release into Downloads. Inspect its signature, confirm **Developer ID Installer: Tarek Wasfy (9NND3237CS)**, then install:

```sh
cd "$HOME/Downloads"
pkgutil --check-signature HP250-Graphics-Installer-Tools-v32.pkg &&
sudo installer -pkg HP250-Graphics-Installer-Tools-v32.pkg -target /
```

This installs a builder, file manifest and licenses in `/Library/Application Support/HP250Graphics/Tools`. The full graphics payload contains proprietary system components and is assembled privately. A clean macOS installation needs a legitimate matching source copy of the eleven component directories, laid out relative to a single source root.

## 2. Build the private system package

Use Xcode Command Line Tools and Python 3.9 or later. On the already working HP, use the startup volume as the source:

```sh
python3 "/Library/Application Support/HP250Graphics/Tools/build_local.py" \
  --source-root / \
  --output "$HOME/Desktop/HP250-NootedGreen-SystemStack-25G332.pkg"
```

For a fresh installation on build 25G332, set `--source-root` to the mounted backup directory containing the verified `Library` and `System` trees, for example `"/Volumes/HP-Graphics-Backup"`. The builder checks each payload hash and requires a new output filename. The public tools do not download or reconstruct missing binaries. Locally generated packages are unsigned unless separately signed with an Installer identity.

## 3. Install, restart and verify

Copy the locally generated full PKG to the target HP if needed. On the target startup volume:

```sh
sudo installer -pkg "$HOME/Desktop/HP250-NootedGreen-SystemStack-25G332.pkg" -target /
```

The installer verifies the eleven bundles and saves the existing system files and kernel collections. It installs the TGL and HookCase kexts, Metal and media bundles, and shader compiler components. It then verifies the expected Auxiliary Kernel Collection module UUIDs. Allow the installer to finish successfully before restarting. The PKG does not install an EFI.

Start through the matching test EFI with HDMI disconnected. Check the internal display and Metal support first, then plug the monitor directly into the laptop:

```sh
system_profiler SPDisplaysDataType
kmutil showloaded | grep -E 'NootedGreen|TGL|HookCase'
ioreg -lw0 | grep -E 'HPHDMITrialBuild|HPHDMIGammaB'
```

## Rollback

The install log prints the private backup directory under `/Library/Application Support/HP250Graphics/Backups`. For restoration on the same running build, substitute that actual directory:

```sh
backup="/Library/Application Support/HP250Graphics/Backups/TIMESTAMP"
sudo python3 "$backup/RESTORE.py" restore "$backup"
```

Restart after a successful restore. For an unbootable installation use your separate system backup or recovery route. License texts and component-specific terms are in [Licenses](Licenses/COMPONENTS.md).
