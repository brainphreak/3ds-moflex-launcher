# Clownsec 3DS Moflex Launcher

A Nintendo 3DS homebrew application that makes it easy to launch moflex video files with the 3D Movie Player application. 
Bypasses the 3DS 127 movie limit and allows you to sort your movies and tv shows in their own directories for better organization. 

## Features

- Browse moflex video collections organized in folders
- Automatically moves selected videos to SD root for Movie Player compatibility
- Automatically restores files back to their original location after viewing
- Lazy loading for fast performance even with large video libraries
- Custom animated banner with audio
- Works on both Citra emulator and real 3DS hardware

## Installation

### Requirements
- A hacked Nintendo 3DS console with custom firmware (CFW)
- 3D Movie Player application installed (any region: USA, EUR, JPN, or CIA version)
- SD card with space for your moflex video files

### Install Instructions

#### Method 1: QR Code Install (Easiest)

![QR Code](binaries/qrcode.png)

1. On your 3DS, open **FBI**
2. Navigate to **Remote Install** → **Scan QR Code**
3. Scan the QR code above
4. FBI will download and install the CIA automatically
5. Launch "Clownsec 3DS" from your home menu

#### Method 2: Manual Install

1. Download `clownsec-movie-player.cia` from the [releases](../../releases) page or from `binaries/` folder
2. Copy the CIA file to your 3DS SD card
3. Install using FBI or any CIA installer
4. Launch "Clownsec 3DS" from your home menu

### Homebrew Launcher (Alternative)

You can also run the `.3dsx` file directly from the Homebrew Launcher without installing:

1. Download `clownsec-movie-player.3dsx` from the [releases](../../releases) page or from `binaries/` folder
2. Copy to `/3ds/` folder on your SD card
3. Launch from Homebrew Launcher

## Usage

### First Time Setup

1. Create a folder on your SD card: `sdmc:/MOFLEX/`
2. Inside `/MOFLEX/`, create subfolders for your video collections
3. Place your `.moflex` video files in these subfolders

Example structure:
```
sdmc:/
├── MOFLEX/
│   ├── Animated Movies/
│   │   ├── movie1.moflex
│   │   └── movie2.moflex
│   ├── Short Films/
│   │   └── film1.moflex
│   └── Music Videos/
│       ├── video1.moflex
│       └── video2.moflex
```

### Playing Videos

1. Launch Clownsec 3DS from your home menu
2. Navigate the directory list using **D-Pad Up/Down**
3. Select a folder and press **A** to see how many moflex files it contains
4. Press **A** again to confirm - files will be moved to SD root
5. 3D Movie Player will launch automatically
6. Watch your videos!
7. When done, exit Movie Player and relaunch Clownsec 3DS
8. Files will automatically be restored to their original folder

### Controls

- **D-Pad Up/Down**: Navigate directory list
- **A Button**: Select folder / Confirm action
- **B Button**: Cancel action
- **START**: Exit application

## Important Notes

### Automatic Cleanup

On first launch, if you have any `.moflex` files already in your SD root (`sdmc:/`), they will be automatically moved to `/MOFLEX/OLDMOFLEX/` to keep your SD card organized.

### File Restoration

The app tracks which folder you selected and automatically moves files back when you relaunch it. **Do not manually move moflex files while the app has files in the SD root**, or they may be moved to the wrong location.

### Limitations

- The app moves ALL `.moflex` files from SD root back to the selected folder
- If you manually place other moflex files in the root between launches, they will also be moved
- Best practice: Keep all `.moflex` files organized in `/MOFLEX/` subfolders

## Technical Details

### Supported Formats

- **Video Format**: Moflex (.moflex)
- **3D Movie Player**: All versions supported (USA, EUR, JPN, CIA installations)

### Title IDs

The app attempts to launch 3D Movie Player using these Title IDs:
- `0x0004000000036A00` - CIA installed version
- `0x0004001000021A00` - USA version
- `0x0004001000021A01` - EUR version
- `0x0004001000021A02` - JPN version

### Memory Usage

- Uses linear memory allocation for 3DS hardware compatibility
- Lazy loading prevents memory issues with large libraries
- Small state file footprint (~512 bytes)

## Building from Source

If you want to build the .3dsx file yourself or modify the source code:

### Prerequisites

You need to have devkitPro and devkitARM installed on your system.

**Installation:**
- Download and install devkitPro from: https://devkitpro.org/wiki/Getting_Started
- Make sure devkitARM and libctru are installed via `dkp-pacman`

**Verify Installation:**
```bash
echo $DEVKITPRO    # Should output /opt/devkitpro (or your install path)
echo $DEVKITARM    # Should output /opt/devkitpro/devkitARM
```

### Build Steps

```bash
# Clone the repository
git clone https://github.com/brainphreak/3ds-moflex-launcher.git
cd 3ds-moflex-launcher

# Set environment variables (if not already set)
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM

# Build the 3dsx file
make

# The output will be: clownsec-movie-player.3dsx
```

### Clean Build

```bash
make clean
make
```

### Project Structure

```
3ds-moflex-launcher/
├── source/
│   └── main.c                      # Main application source code
├── icons/
│   ├── icon.png                    # App icon source (48x48)
│   ├── icon-48x48.png              # Icon used by build
│   └── icon.smdh                   # Pre-built SMDH icon
├── binaries/
│   ├── clownsec-movie-player.3dsx  # Homebrew Launcher file
│   ├── clownsec-movie-player.cia   # Installable CIA file
│   ├── clownsec-movie-player.elf   # Compiled ELF (for CIA building)
│   ├── clownsec-movie-player.smdh  # Generated SMDH
│   └── qrcode.png                  # QR code for FBI remote install
├── cia-build/
│   ├── README.md                   # CIA building instructions
│   ├── working.rsf                 # Resource specification file
│   ├── icon.smdh                   # Pre-built icon for CIA
│   ├── banner.bin                  # Pre-built banner
│   ├── banner.png                  # Banner image source
│   └── audio_short.wav             # Banner audio source
├── Makefile                        # Build configuration
└── README.md                       # This file
```

### Source Code Overview

The main application logic is in `source/main.c`:
- **Directory browser**: Navigate folders in `/MOFLEX/`
- **File operations**: Move .moflex files to/from SD root
- **State management**: Track active collection between launches
- **APT integration**: Launch 3D Movie Player automatically
- **Filesystem sync**: Hardware-optimized file operations

### Building the CIA (Optional)

If you want to build the installable CIA file yourself:

1. Build the source code: `make`
2. See instructions in `cia-build/README.md`
3. You'll need to download `bannertool` and `makerom` separately

The pre-built CIA in releases is ready to use - you only need to build it yourself if you're modifying the code or customizing the banner.

## Troubleshooting

### App crashes on launch
- Ensure you have enough free space on your SD card
- Try reinstalling the CIA
- Check that your CFW is up to date

### "Failed to launch Movie Player"
- Verify that 3D Movie Player is installed
- Check that at least one of the supported Title IDs is present
- Ensure the app has proper system permissions

### Files not restoring
- Make sure you relaunch Clownsec 3DS after exiting Movie Player
- Check that the state file wasn't manually deleted
- Verify files are actually in the SD root (`sdmc:/`)

### Emulator vs Real Hardware
- The app works on both Citra and real 3DS hardware
- Emulator may be faster for testing
- Real hardware includes filesystem sync delays for stability

## Credits

- Built with [devkitPro](https://devkitpro.org/) and libctru
- Uses Nintendo 3DS homebrew libraries
- Moflex format is proprietary to Nintendo

## License

This is homebrew software for educational and personal use. Nintendo 3DS and related trademarks are property of Nintendo Co., Ltd.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Changelog

### v1.0.0 (Initial Release)
- Directory-based moflex file browser
- Automatic file moving and restoration
- Support for all 3D Movie Player regions
- Lazy loading for performance
- Automatic cleanup of existing root moflex files
- Custom animated banner with audio
- Hardware-optimized filesystem operations
  
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/F2F35TO7X)
