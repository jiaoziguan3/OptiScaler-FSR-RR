# UI、翻译与离线发布规则移植实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task with checkpoints.

**Goal:** 以 `J:\OptiScaler-0.10-2` 为基线，合入源版 UI、翻译系统、设置编辑器和完整的离线构建/发布目录组装能力，使复制整个目录后无需访问源版目录或 GitHub 即可构建 `Release|x64` 产物。

**Architecture:** 保留目标项目现有渲染、Hook、FSR4、XeFG、低延迟和输入控制流，仅在公共菜单层增加翻译显示层。将发布组装从长的 MSBuild 命令迁移为独立 PowerShell 脚本，使用项目内固定依赖和本地回退文件；GitHub 下载改为显式更新模式。主 DLL 继续由 Visual Studio/MSBuild 构建，工程保持 `v143`，通过构建入口自动探测本机工具链。

**Tech Stack:** C++/Dear ImGui, Visual Studio `.sln/.vcxproj`, PowerShell, Python Tkinter/ttk, PyInstaller, Windows SDK.

---

## 文件地图

- Create: `OptiScaler/translation/Translation.h` — 语言枚举、初始化、切换和翻译查询接口。
- Create: `OptiScaler/translation/Translation.cpp` — 英文回退、简体中文映射和翻译状态。
- Create: `tools/build.ps1` — MSBuild/VS/工具集探测、离线检查、编译和发布组装入口。
- Create: `tools/prepare_release.ps1` — 本地复制、目录创建、许可证整理、产物完整性检查。
- Create: `tools/check_offline_dependencies.ps1` — 编译库、运行时、字体、回退文件和设置器回退检查。
- Create: `tools/update_external_artifacts.ps1` — 显式执行时从固定 GitHub URL 更新可选二进制，不参与默认构建。
- Create: `tools/build_settings.ps1` — 使用项目内 spec 构建或验证设置器，并复制到发布目录。
- Create: `installer/optiscaler_settings.py` — 源版设置编辑器，按目标配置适配。
- Create: `installer/settings_spec.py` — 目标项目配置字段、范围、帮助文本和中英文标签。
- Create: `installer/ini_handler.py` — 保留注释和未知键的 INI 读写逻辑。
- Create: `installer/OptiScalerSettings.spec` — 设置器 PyInstaller 单文件配置。
- Copy: `streamline/` — 源版 PostBuild 需要的 16 个运行时 DLL。
- Copy: `plugins/OptiPatcher.asi` — OptiPatcher 离线回退文件。
- Copy: `fakenvapi.dll`, `fakenvapi.ini`, `dlssg_to_fsr3_amd_is_better.dll` — 固定版本发布文件的本地回退。
- Modify: `OptiScaler/Config.h` — 增加 Language 和 FSR4 Provider 路径字段时保留目标默认值。
- Modify: `OptiScaler/Config.cpp` — 读取/保存语言、校验枚举，并修正 XeFG/XeLL 配置目标字段。
- Modify: `OptiScaler/pch.h` — 接入 Translation 声明并保持 PCH 首包含约束。
- Modify: `OptiScaler/menu/menu_common.cpp` — 语义合并 UI、语言选择、字体、启动提示和目标特有文本翻译。
- Modify: `OptiScaler/OptiScaler.vcxproj` — 登记翻译源文件，调用独立构建/发布脚本，保留目标 v143 和全部现有编译项。
- Modify: `OptiScaler/OptiScaler.vcxproj.filters` — 登记 Translation 文件过滤器。
- Modify: `OptiScaler/OptiScaler.ini` — 仅在目标配置缺少语言或新增设置时补齐，不覆盖目标已有值。

### Task 1: 建立源版差异和离线依赖基线

**Files:**
- Create: `tools/check_offline_dependencies.ps1`
- Create: `tools/release-manifest.json`
- Copy: `streamline/`, `plugins/OptiPatcher.asi`, `fakenvapi.dll`, `fakenvapi.ini`, `dlssg_to_fsr3_amd_is_better.dll`

- [ ] 从源版复制目标当前缺失的 `translation`、`streamline`、四个离线回退文件，不复制 `build`、`dist`、日志、缓存和安装器产物。
- [ ] 在 `release-manifest.json` 中记录每个必需文件的来源、目标相对路径、类型和是否允许为空；明确列出源版 PostBuild 的 DLL 与许可证路径。
- [ ] 实现检查脚本，输出缺失项的源说明和目标路径；用 `Test-Path`、文件大小检查和目录枚举验证必需文件。
- [ ] 为所有本地回退文件建立离线校验；检查结果为失败时退出非零，不执行 MSBuild。
- [ ] 运行：`powershell -NoProfile -File tools\check_offline_dependencies.ps1 -Configuration Release -Platform x64`。
- [ ] 预期：当前复制后的目标目录报告所有必需源码/库/运行时/回退文件存在；缺失项逐项列出。

### Task 2: 移植翻译模块并接入工程

**Files:**
- Create: `OptiScaler/translation/Translation.h`
- Create: `OptiScaler/translation/Translation.cpp`
- Modify: `OptiScaler/pch.h`
- Modify: `OptiScaler/OptiScaler.vcxproj`
- Modify: `OptiScaler/OptiScaler.vcxproj.filters`

- [ ] 复制源版 Translation 文件作为基础，保留英文原文回退和简体中文映射。
- [ ] 将无效语言值归一化为 English；空中文映射直接回退原文。
- [ ] 在 PCH 和 `.vcxproj`/`.filters` 中登记文件，确认编译项只出现一次。
- [ ] 编写静态检查命令，确认 `Language`、`Translation::Init`、`Translation::Get` 符号存在且工程包含对应源文件。
- [ ] 运行：`Select-String -Path OptiScaler\OptiScaler.vcxproj -Pattern 'translation\\Translation'`。
- [ ] 预期：同时找到一个 Header 项和一个 Compile 项，filters 中存在对应 Translation filter。

### Task 3: 合并配置语言和 FSR4 Provider 字段

**Files:**
- Modify: `OptiScaler/Config.h`
- Modify: `OptiScaler/Config.cpp`
- Modify: `OptiScaler/OptiScaler.ini`

- [ ] 在 Menu 配置结构增加 `Language`，保留目标主题默认颜色和背景默认值。
- [ ] 增加 `Fsr4ProviderPath` 与源版匹配的配置字段，仅在目标 FSR4 实现确实需要时接入；同步确认 `Fsr4Amdxcffx64Path` 的现有命名和用途。
- [ ] 在配置读取处只接受 English/Chinese 的有效整数，缺失或非法值设置为 English。
- [ ] 在配置保存处写回语言值，不改变已有配置键名；修正 XeFGPath/XeLLPath 写入各自库对象的目标错误。
- [ ] 为 FSR4 Provider 保留目标项目自定义 DLL 优先和 SDK/Driver 约束，不能只增加 UI 而改变加载行为。
- [ ] 检查 INI 往返：读取未知键和注释，修改 Language，保存后验证未知内容仍存在。

### Task 4: 合并游戏内 UI 和运行时翻译

**Files:**
- Modify: `OptiScaler/menu/menu_common.cpp`

- [ ] 在 `MenuCommon::Init` 中初始化翻译并从配置恢复语言，不增加重复 Hook、交换链或后端初始化。
- [ ] 增加唯一语言选择器，放入当前两栏菜单结构，不复制源版重复的 Preferences/底栏控件。
- [ ] 将 splash 文本改为英文 key/原文列表，在绘制阶段调用 `Translation::Get`，避免静态初始化固化语言。
- [ ] 按区域把标题、按钮、复选框、组合框、提示、状态、FPS Overlay、Keybind、FSR4、XeFG 和低延迟文本接入翻译；配置键、日志字段、DLL 名不翻译。
- [ ] 对所有控件使用 `显示文本##稳定ID`，动态格式文本先取得翻译格式串再格式化。
- [ ] 保留目标 FSR4 Preset/Debug/Watermark、XeFG、低延迟、OptiInput、主题和后端控制流，仅替换显示文本并接入 Provider UI 所需字段。
- [ ] 保留目标鼠标、键盘、Raw Input、DirectInput、XInput、GameInput 和窗口子类化逻辑，不引入源版重复低级键盘 Hook。
- [ ] 完成英文/中文静态字符串扫描，生成缺失翻译 key 清单并补入 `Translation.cpp`。

### Task 5: 合并 CJK 字体和布局兼容

**Files:**
- Modify: `OptiScaler/menu/menu_common.cpp`
- Modify: `OptiScaler/menu/font/Hack.h` or font-loading path
- Modify: `OptiScaler/OptiScaler.vcxproj`
- Copy: approved redistributable CJK font asset if source version provides one

- [ ] 先确认目标 Hack-Regular.ttf、源版内嵌字体和可再分发 CJK 字体的许可证；不依赖固定系统字体路径作为唯一方案。
- [ ] 自定义 TTF 与内置 Hack 两条加载路径都合并 CJK glyph range；中文字体缺失时安全回退到英文字体。
- [ ] 重新检查固定宽度控件、表格列、启动提示、FPS Overlay 和菜单缩放，避免中文截断或重叠。
- [ ] 用英文和简体中文分别初始化字体 atlas，确认字体加载失败不会破坏 ImGui 后端。

### Task 6: 实现离线发布组装

**Files:**
- Create: `tools/prepare_release.ps1`
- Create: `tools/update_external_artifacts.ps1`
- Modify: `OptiScaler/OptiScaler.vcxproj`

- [ ] 将 DLL 从 `x64\Release` 移动到 `x64\Release\a`，创建 `OptiScaler`、`Licenses`、`streamline`、`plugins` 和 `D3D12_OptiScaler` 目录。
- [ ] 按源版规则复制 XeSS、FidelityFX v1/v2、Agility SDK、Streamline、配置、安装脚本和许可证；将 FidelityFX loader 重命名为 `amd_fidelityfx_dx12.dll`。
- [ ] 复制 `OptiScalerSettings.exe` 到发布根目录；没有 Python 时使用项目内回退 EXE。
- [ ] 默认只使用项目内文件；显式 `-UpdateExternalArtifacts` 时才访问固定版本 GitHub URL，并在下载失败后回退本地文件。
- [ ] 创建 `!! EXTRACT ALL FILES TO GAME FOLDER !!`，复制后对所有必需文件做存在和非零大小检查。
- [ ] 将复杂 PostBuild 命令替换为调用脚本，避免路径空格、错误码吞并和多行 PowerShell 转义问题。
- [ ] 运行：`powershell -NoProfile -File tools\prepare_release.ps1 -Configuration Release -Platform x64 -Offline`。
- [ ] 预期：`x64\Release\a` 生成完整发布目录；缺少本地必需文件时命令失败并指出具体文件。

### Task 7: 移植设置编辑器

**Files:**
- Create: `installer/optiscaler_settings.py`
- Create: `installer/settings_spec.py`
- Create: `installer/ini_handler.py`
- Create: `installer/OptiScalerSettings.spec`
- Create: `tools/build_settings.ps1`

- [ ] 从源版复制设置器源码和 spec，删除安装器导入、安装路径操作和安装器专属逻辑。
- [ ] 将目标项目现有配置和 FSR4、XeFG、低延迟字段加入 settings spec，严格复用 Config/INI 键名。
- [ ] 保留注释、未知键和格式的读写行为；对测试 INI 做读取、修改、保存和再次读取。
- [ ] 在有 PyInstaller 时构建单文件 EXE；无 Python/PyInstaller 时验证项目内回退 EXE，不阻塞 DLL 构建。
- [ ] 运行：`powershell -NoProfile -File tools\build_settings.ps1 -FallbackOnly`。
- [ ] 预期：能找到并验证 `OptiScalerSettings.exe`；显式构建模式下生成同名 EXE 并复制到发布目录。

### Task 8: 实现 Visual Studio/MSBuild 自动探测

**Files:**
- Create: `tools/build.ps1`
- Create: `tools/find_msbuild.ps1`
- Modify: `OptiScaler/OptiScaler.vcxproj`
- Modify: `build_script.ps1` if retained as compatibility wrapper

- [ ] 按 D 盘 Visual Studio、`vswhere`、常见安装路径顺序查找 MSBuild，记录找到的绝对路径。
- [ ] 检查项目工具集；优先目标已安装的 v143，缺失时通过 `/p:PlatformToolset=<detected>` 临时选择本机兼容工具集，不改写工程文件。
- [ ] 检查 Windows SDK、x64 MSVC、Git/PowerShell；把工具链缺失和项目依赖缺失分开报告。
- [ ] 设置 `SolutionDir` 为当前项目根目录，确保任何复制位置都不引用源版绝对路径。
- [ ] 支持 `-Configuration Release -Platform x64 -Offline`，默认执行离线检查、MSBuild 和发布组装。
- [ ] 运行：`powershell -NoProfile -File tools\build.ps1 -Configuration Release -Platform x64 -Offline`。
- [ ] 预期：MSBuild 使用当前设备可用工具链，产物位于 `x64\Release\a`；日志打印工具链和发布检查结果。

### Task 9: 自动化验证和断网验收

**Files:**
- Create: `tools/verify_port.ps1`
- Create: `installer/test_settings.py`
- Modify: `README.md`

- [ ] 静态验证 Translation 工程登记、缺失翻译回退、稳定 `##ID`、发布清单和绝对源路径扫描。
- [ ] 运行设置器 INI 往返测试，确认未知键和注释保持不变。
- [ ] 构建 `Release|x64`，再执行 `Debug|x64` 编译检查；确认 DLL 导出表和延迟加载列表未丢失。
- [ ] 检查发布目录的所有必需文件、许可证、运行时 DLL、设置器和目标项目扩展文件。
- [ ] 复制整个目标目录到临时全新目录，隐藏/重命名 `J:\OptiScaler-master-0.10-2`，断网执行构建和发布组装。
- [ ] 更新 README，说明离线构建命令、工具链前提、可选 GitHub 更新模式和输出目录 `x64\Release\a`。
- [ ] 预期：断网构建不访问源版目录，不产生缺失发布文件；源版目录扫描无命中。

### Task 10: 最终差异审查

**Files:**
- Review: all modified files

- [ ] 对比目标项目原有 FSR4、XeFG、低延迟、OptiInput、主题、Hook、图形后端和配置键，确认没有被源版覆盖。
- [ ] 检查项目根目录只保留必要源码、依赖和构建工具，不纳入源版安装器、缓存、日志或临时产物。
- [ ] 检查脚本不输出密钥、不包含签名凭据、不写入源版绝对路径。
- [ ] 记录构建使用的 MSBuild、Platform Toolset、Windows SDK 和最终发布目录内容。
- [ ] 运行最终 `git diff --check` 和静态完整性检查，确认无空白错误、未跟踪的意外构建缓存或路径泄漏。

