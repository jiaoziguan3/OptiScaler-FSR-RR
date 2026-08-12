# OptiScaler-FSR-RR

基于 [OptiScaler](https://github.com/optiscaler/OptiScaler) 的增强版本，重点提供**可视化配置工具**和**中文本地化支持**。

---

## 🎯 项目特色

### 📊 图形化设置编辑器

本项目提供了一个完整的 **GUI 配置工具** (`OptiScaler Settings`)，让你无需手动编辑 INI 文件即可调整所有 OptiScaler 参数。

**主要特性：**
- ✅ **中英双语界面** - 自动识别或手动切换
- ✅ **分模块管理** - 按功能分类（上采样器、帧生成、画质调整、性能优化等）
- ✅ **实时预览** - 每个选项都有详细的中文说明
- ✅ **智能控件** - 滑块、下拉框、开关自动适配参数类型
- ✅ **安全保存** - 自动备份原文件为 `.bak`，修改后重启游戏生效
- ✅ **保留注释** - 采用 patch-in-place 机制，保留 INI 文件中的所有注释

### 🛠️ 使用设置编辑器

1. 运行 `installer/dist/OptiScalerSettings.exe`
2. 首次运行会自动寻找当前目录的 `OptiScaler.ini`
3. 选择左侧模块，调整右侧参数
4. 点击"保存"应用更改（原文件会自动备份）
5. 重启游戏查看效果

**支持的配置模块：**
- **Upscalers** - DX11/DX12/Vulkan 上采样器选择
- **FrameGen** - 帧生成输入/输出、插帧倍率
- **Quality** - 锐化（RCAS/MAS）、输出缩放、HDR 调整
- **Performance** - 资源状态、延迟优化、内存管理
- **Advanced** - DLSS 预设、FSR 版本覆盖、调试选项
- **Spoofing** - GPU 伪装（让游戏认为你的 AMD/Intel 显卡是 NVIDIA）
- **Keybinds** - 快捷键自定义

---

## 📋 INI 配置说明

如果你更喜欢直接编辑 `OptiScaler.ini`，所有参数都有详细的行内注释。以下是关键配置项：

### 上采样器 (Upscalers)
```ini
[Upscalers]
Dx11Upscaler=auto         # DX11: fsr22/fsr31/xess/ffx_12/dlss
Dx12Upscaler=auto         # DX12: xess/fsr21/fsr22/ffx/dlss (auto根据GPU自动选择)
VulkanUpscaler=auto       # Vulkan: fsr22/ffx/xess/dlss
```

### 帧生成 (FrameGen)
```ini
[FrameGen]
Enabled=auto              # 帧生成总开关
FGInput=auto              # 输入源: dlssg/nvngxfg/fsrfg/upscaler
FGOutput=auto             # 输出技术: fsrfg/xefg/dlssg
```

### 画质增强 (Quality)
```ini
[Sharpness]
Sharpness=auto            # RCAS 锐化强度 (0.0-1.0)
UseNasSharpening=auto     # 使用 NAS (Neural Art Scaling) 锐化

[OutputScaling]
OutputScalingEnabled=auto # 启用输出缩放
OutputScalingUseFsr=auto  # 使用 FSR 进行输出缩放
```

### GPU 伪装 (Spoofing)
```ini
[Spoofing]
DxgiSpoofing=auto         # 伪装成 NVIDIA GPU (AMD/Intel 用户必备)
NvidiaSpoofing=auto       # NVAPI 伪装
VulkanSpoofing=auto       # Vulkan 层伪装
```

完整的参数说明请参考 INI 文件中的注释，或使用图形化编辑器查看每个选项的详细帮助。

---

## 🚀 快速开始

### 游戏内操作

- **`Insert`** - 打开 OptiScaler 叠加层菜单（可自定义）
- **`Page Up`** - 显示性能统计信息
- **`Page Down`** - 切换统计模式

> **提示：** 如果 `Insert` 无效，某些键盘布局需尝试 `Alt + Insert`。

### 安装步骤

1. 从 [Releases](https://github.com/jiaoziguan3/OptiScaler-FSR-RR/releases) 下载最新版本
2. 解压到游戏根目录（与游戏 .exe 同级）
3. 运行 `OptiScalerSettings.exe` 调整设置
4. 启动游戏，按 `Insert` 打开菜单验证安装

详细的游戏兼容性和安装指南请参考 [OptiScaler Wiki](https://github.com/optiscaler/OptiScaler/wiki)。

---

## 🔧 开发者信息

### 编译设置编辑器

```bash
cd installer
python -m PyInstaller OptiScalerSettings.spec
# 生成的 .exe 位于 dist/OptiScalerSettings.exe
```

### 编译 OptiScaler 本体

**要求：** Visual Studio 2022

```bash
# 克隆仓库（包含所有子模块）
git clone --recursive https://github.com/jiaoziguan3/OptiScaler-FSR-RR.git

# 打开 OptiScaler.sln
# 选择 Release x64 配置
# 构建解决方案
```

### 项目结构

```
OptiScaler-0.10-2/
├── OptiScaler/          # C++ 核心代码
│   ├── framegen/        # 帧生成实现（FFX/Nvngx/XeFG）
│   ├── inputs/          # DLSS/FSR 输入钩子
│   ├── upscalers/       # 上采样器后端
│   └── menu/            # ImGui 叠加层菜单
├── installer/           # Python GUI 设置编辑器
│   ├── optiscaler_settings.py   # 主程序
│   ├── settings_spec.py         # 字段定义和中文翻译
│   ├── ini_handler.py           # INI patch-in-place 逻辑
│   └── dist/OptiScalerSettings.exe
└── OptiScaler.ini       # 主配置文件
```

---

## ⚠️ 重要提示

- **不要在联机游戏中使用** - 可能触发反作弊系统导致封号
- **GPU 伪装** - AMD/Intel 用户必须启用 `DxgiSpoofing` 才能使用 DLSS 功能
- **帧生成限制** - OptiFG 目前仅支持 DX12 游戏
- **FSR4 支持** - 官方仅支持 RDNA3/RDNA4 GPU

---

## 📚 资源链接

- **上游项目**: [OptiScaler](https://github.com/optiscaler/OptiScaler)
- **兼容性列表**: [Wiki Compatibility List](https://github.com/optiscaler/OptiScaler/wiki/Compatibility-List)
- **FSR4 测试状态**: [FSR4 Compatibility List](https://github.com/optiscaler/OptiScaler/wiki/FSR4-Compatibility-List)
- **Discord 社区**: [OptiScaler Server](https://discord.gg/wEyd9w4hG5)

---

## 🙏 致谢

- **OptiScaler 团队** - 提供核心功能和持续维护
- **@PotatoOfDoom** - CyberFSR2 项目
- **@FakeMichau** - 架构重构和功能开发
- **社区贡献者** - 测试、反馈和兼容性列表维护

---

## 📄 许可证

继承 OptiScaler 原项目的许可证。

**本项目为非官方分支，专注于提供更便捷的配置体验。核心功能和技术支持请参考上游项目。**
