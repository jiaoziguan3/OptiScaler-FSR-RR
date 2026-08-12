# OptiScaler-FSR-RR

An enhanced fork of [OptiScaler](https://github.com/optiscaler/OptiScaler) with a focus on **visual configuration tools** and **bilingual localization support**.

---

## 🎯 Key Features

### 📊 Graphical Settings Editor

This project provides a complete **GUI configuration tool** (`OptiScaler Settings`) that lets you adjust all OptiScaler parameters without manually editing INI files.

**Main Features:**
- ✅ **Bilingual Interface** - Auto-detect or manually switch between English/Chinese
- ✅ **Modular Management** - Organized by function (Upscalers, FrameGen, Quality, Performance, etc.)
- ✅ **Live Preview** - Detailed descriptions for every option
- ✅ **Smart Controls** - Sliders, dropdowns, and toggles auto-adapt to parameter types
- ✅ **Safe Saving** - Automatic backup to `.bak` before changes, restart game to apply
- ✅ **Comment Preservation** - Uses patch-in-place mechanism to keep all INI comments intact

### 🛠️ Using the Settings Editor

1. Run `installer/dist/OptiScalerSettings.exe`
2. First launch will auto-locate `OptiScaler.ini` in the current directory
3. Select a module on the left, adjust parameters on the right
4. Click "Save" to apply changes (original file is auto-backed up)
5. Restart the game to see effects

**Supported Configuration Modules:**
- **Upscalers** - DX11/DX12/Vulkan upscaler selection
- **FrameGen** - Frame generation input/output, interpolation multiplier
- **Quality** - Sharpening (RCAS/MAS), output scaling, HDR adjustments
- **Performance** - Resource states, latency optimization, memory management
- **Advanced** - DLSS presets, FSR version overrides, debug options
- **Spoofing** - GPU masquerading (make AMD/Intel GPUs appear as NVIDIA)
- **Keybinds** - Hotkey customization

---

## 📋 INI Configuration Guide

If you prefer directly editing `OptiScaler.ini`, all parameters have detailed inline comments. Here are the key config sections:

### Upscalers
```ini
[Upscalers]
Dx11Upscaler=auto         # DX11: fsr22/fsr31/xess/ffx_12/dlss
Dx12Upscaler=auto         # DX12: xess/fsr21/fsr22/ffx/dlss (auto picks by GPU)
VulkanUpscaler=auto       # Vulkan: fsr22/ffx/xess/dlss
```

### Frame Generation
```ini
[FrameGen]
Enabled=auto              # Master switch for frame generation
FGInput=auto              # Input source: dlssg/nvngxfg/fsrfg/upscaler
FGOutput=auto             # Output tech: fsrfg/xefg/dlssg
```

### Quality Enhancement
```ini
[Sharpness]
Sharpness=auto            # RCAS sharpening strength (0.0-1.0)
UseNasSharpening=auto     # Use NAS (Neural Art Scaling) sharpening

[OutputScaling]
OutputScalingEnabled=auto # Enable output scaling
OutputScalingUseFsr=auto  # Use FSR for output scaling
```

### GPU Spoofing
```ini
[Spoofing]
DxgiSpoofing=auto         # Spoof as NVIDIA GPU (required for AMD/Intel users)
NvidiaSpoofing=auto       # NVAPI spoofing
VulkanSpoofing=auto       # Vulkan layer spoofing
```

For complete parameter documentation, refer to the comments in the INI file or use the graphical editor to view detailed help for each option.

---

## 🚀 Quick Start

### In-Game Controls

- **`Insert`** - Open OptiScaler overlay menu (customizable)
- **`Page Up`** - Show performance stats overlay
- **`Page Down`** - Cycle through stats modes

> **Tip:** If `Insert` doesn't work, some keyboard layouts may need `Alt + Insert`.

### Installation Steps

1. Download the latest release from [Releases](https://github.com/jiaoziguan3/OptiScaler-FSR-RR/releases)
2. Extract to game root directory (same level as game .exe)
3. Run `OptiScalerSettings.exe` to adjust settings
4. Launch game, press `Insert` to open menu and verify installation

For detailed game compatibility and installation guides, refer to the [OptiScaler Wiki](https://github.com/optiscaler/OptiScaler/wiki).

---

## 🔧 Developer Information

### Building the Settings Editor

```bash
cd installer
python -m PyInstaller OptiScalerSettings.spec
# Generated .exe is in dist/OptiScalerSettings.exe
```

### Building OptiScaler Core

**Requirements:** Visual Studio 2022

```bash
# Clone repository (including all submodules)
git clone --recursive https://github.com/jiaoziguan3/OptiScaler-FSR-RR.git

# Open OptiScaler.sln
# Select Release x64 configuration
# Build solution
```

### Project Structure

```
OptiScaler-0.10-2/
├── OptiScaler/          # C++ core code
│   ├── framegen/        # Frame generation impl (FFX/Nvngx/XeFG)
│   ├── inputs/          # DLSS/FSR input hooks
│   ├── upscalers/       # Upscaler backends
│   └── menu/            # ImGui overlay menu
├── installer/           # Python GUI settings editor
│   ├── optiscaler_settings.py   # Main program
│   ├── settings_spec.py         # Field definitions and i18n
│   ├── ini_handler.py           # INI patch-in-place logic
│   └── dist/OptiScalerSettings.exe
└── OptiScaler.ini       # Main config file
```

---

## ⚠️ Important Notes

- **Do NOT use in online games** - May trigger anti-cheat and cause bans
- **GPU Spoofing** - AMD/Intel users must enable `DxgiSpoofing` to use DLSS features
- **Frame Gen Limitations** - OptiFG currently only supports DX12 games
- **FSR4 Support** - Officially limited to RDNA3/RDNA4 dGPUs

---

## 📚 Resources

- **Upstream Project**: [OptiScaler](https://github.com/optiscaler/OptiScaler)
- **Compatibility List**: [Wiki Compatibility List](https://github.com/optiscaler/OptiScaler/wiki/Compatibility-List)
- **FSR4 Test Status**: [FSR4 Compatibility List](https://github.com/optiscaler/OptiScaler/wiki/FSR4-Compatibility-List)
- **Discord Community**: [OptiScaler Server](https://discord.gg/wEyd9w4hG5)

---

## 🙏 Credits

- **OptiScaler Team** - Core functionality and ongoing maintenance
- **@PotatoOfDoom** - CyberFSR2 project
- **@FakeMichau** - Architecture refactoring and feature development
- **Community Contributors** - Testing, feedback, and compatibility list maintenance

---

## 📄 License

Inherits the license from the upstream OptiScaler project.

**This is an unofficial fork focused on providing a more convenient configuration experience. For core functionality and technical support, please refer to the upstream project.**
