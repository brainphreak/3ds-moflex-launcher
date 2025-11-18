# Building the CIA File

This folder contains everything needed to build the installable CIA file yourself.

## Prerequisites

You'll need to download these tools separately:

1. **bannertool** - For creating/modifying banner.bin
   - Download from: https://gbatemp.net/attachments/bannertool-master-zip.18136/
   - Extract and use the version for your OS (Windows: `bannertool.exe`, Mac/Linux: `bannertool`)
   - Note: The original GitHub repo is no longer maintained

2. **makerom** - For creating the CIA file
   - Download from: https://github.com/3DSGuy/Project_CTR/releases
   - Look for "makerom" in the releases (or search for "makerom" builds online)
   - Mac users: You may need to build from source or find an ARM64/x64 build
   - Windows: Use `makerom.exe`
   - Linux: Use the Linux build

## Files in this folder

- `working.rsf` - Resource specification file (app metadata)
- `icon.smdh` - Pre-built app icon (48x48)
- `banner.bin` - Pre-built animated banner with audio
- `banner.png` - Banner image source (256x128)
- `audio_short.wav` - Banner audio source (3 seconds, 16-bit, 44100Hz)

## Building the CIA

### Step 1: Build the ELF file

First, you need to compile the source code from the parent directory:

```bash
cd ..
make
```

This creates `clownsec-movie-player.elf` in the `binaries/` directory.

### Step 2: Create the CIA

```bash
# From the main project directory
./makerom -f cia -o binaries/clownsec-movie-player.cia \
  -elf binaries/clownsec-movie-player.elf \
  -rsf cia-build/working.rsf \
  -icon cia-build/icon.smdh \
  -banner cia-build/banner.bin \
  -exefslogo \
  -target t
```

The output will be `clownsec-movie-player.cia` in the `binaries/` directory.

## Customizing the Banner

If you want to create your own banner with custom audio:

### Option 1: Rebuild banner.bin from sources

```bash
# Make sure your audio is 3 seconds, 16-bit, 44100Hz WAV
bannertool makebanner \
  -i cia-build/banner.png \
  -a cia-build/audio_short.wav \
  -o cia-build/banner.bin
```

### Option 2: Extract different audio from source

If you have a longer audio file and want to extract a different 3-second clip:

```bash
# Extract 3 seconds starting at a specific time (e.g., 1:30)
ffmpeg -i your-audio.wav \
  -ss 00:01:30 \
  -t 00:00:03 \
  -af "volume=3dB" \
  -acodec pcm_s16le \
  -ar 44100 \
  cia-build/audio_short.wav

# Then rebuild the banner
bannertool makebanner \
  -i cia-build/banner.png \
  -a cia-build/audio_short.wav \
  -o cia-build/banner.bin
```

### Audio Requirements

- **Duration**: Maximum 3 seconds
- **Format**: WAV (PCM)
- **Sample Rate**: 44100 Hz (or 48000 Hz)
- **Bit Depth**: 16-bit
- **Channels**: Mono or Stereo

## Understanding working.rsf

The RSF file contains metadata about your app:

- **Title**: "Clownsec 3DS"
- **Product Code**: CTR-N-CLWN
- **Unique ID**: 0xCE001
- **Permissions**: File system access, service access, etc.
- **Stack Size**: 256KB

You can modify this file to change app metadata, but be careful with permissions and system settings.

## Troubleshooting

### "NCCH AES Key could not be loaded" warning
This is normal and expected. The CIA will work fine without encryption for homebrew purposes.

### "CIA WARNING: Common Key could not be loaded"
This is also normal. Your CIA will install and run correctly.

### Banner is too large
- Ensure audio is exactly 3 seconds or less
- Check that your audio file is not using a higher bit rate than necessary
- Banner.bin should be around 500-600KB

### CIA won't install
- Make sure you built the ELF file first with `make`
- Verify all paths in the makerom command are correct
- Check that icon.smdh and banner.bin exist in cia-build/

## Notes

- The pre-built `clownsec-movie-player.cia` in releases is already compiled and ready to use
- You only need to rebuild if you're modifying the source code or customizing the banner
- Keep bannertool and makerom executables out of git (download them separately)
