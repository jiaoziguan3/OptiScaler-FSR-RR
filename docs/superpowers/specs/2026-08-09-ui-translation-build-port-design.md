# UI、翻译与发布规则移植设计

## 目标

将 `J:\OptiScaler-master-0.10-2` 的游戏内 UI、翻译系统、设置编辑器和 Release 发布目录组装规则移植到 `J:\OptiScaler-0.10-2`，同时完整保留目标项目已有的 FSR4、XeFG、低延迟、Hook、图形后端和其他定制功能。

## 范围

移植内容包括：

- 游戏内 Dear ImGui 菜单结构、显示逻辑和中文字体支持。
- `Translation.h` 与 `Translation.cpp` 提供的英文、简体中文翻译系统。
- `optiscaler_settings.py`、`settings_spec.py`、`ini_handler.py` 和 `OptiScalerSettings.spec` 构成的设置编辑器。
- Visual Studio 工程中新文件的编译规则。
- `Release|x64` 构建前事件、构建后事件和 `x64\Release\a` 发布目录结构。
- GitHub 下载、本地回退、项目文件复制、许可证复制和发布完整性检查。
- 自动探测 D 盘 Visual Studio、MSBuild 和本机可用工具集的构建入口。
- 将源版中当前项目缺失的必需编译依赖、运行时文件、许可证、设置器构建所需文件和本地回退文件复制到目标目录并纳入长期维护。
- 默认构建不依赖网络，GitHub 下载仅作为可选更新或人工恢复手段。

不移植内容包括：

- Python GUI 安装器及其 PyInstaller 配置。
- 与本次功能无关的源码覆盖或重构。
- 根级 CMake 构建系统。
- 签名凭据和本地代码签名流程。

## 合并策略

采用三方语义合并，以目标项目作为功能基线。按 UI、翻译、配置、设置编辑器、Visual Studio 工程和发布规则分别比较两版实现，吸收源版能力并适配目标项目特有功能。

不直接覆盖 `menu_common.cpp`、`Config` 或 `.vcxproj`。出现冲突时保留目标项目运行行为，再将源版显示层、翻译入口和发布规则接入。DLL 导出名称、配置键名、调用约定和图形后端生命周期保持不变。

## 可移植离线源码包

目标目录必须成为自包含的 Windows 构建包。将从源版复制所有参与主 DLL 编译、链接、资源生成、设置器打包和 Release 发布组装的缺失文件，而不是只复制源代码文件。复制后不依赖源目录、当前机器的临时目录或之前生成的发布目录。

自包含范围包括：

- 主工程引用的第三方头文件、源码、`.lib`、`.dll`、字体数据、预编译 shader 和资源文件。
- FSR、FidelityFX、XeSS、Vulkan、FreeType、Streamline、NVAPI、DLSS、Detours、SimpleIni、spdlog、magic_enum、unordered_dense、nlohmann 等目标工程实际引用的依赖。
- 源版发布规则需要的 XeSS/FidelityFX/Agility SDK 运行时、许可证、`fakenvapi`、`dlssg_to_fsr3`、`OptiPatcher` 和 Streamline 文件，并保留本地回退副本。
- 设置编辑器源码、规格文件、INI 处理器、PyInstaller spec，以及已有可直接使用的 `OptiScalerSettings.exe` 回退副本。
- 构建脚本、工具探测脚本、发布清单和离线完整性检查脚本。

不复制中间文件、编译日志、缓存、临时压缩包、源版安装器及安装器产物。对于无法以源码形式重建的闭源或预编译依赖，必须在目标目录保留与 x64 配置匹配的本地二进制和许可证，并由检查脚本验证架构、文件存在性和非零大小。

“任何设备”指将完整目标文件夹复制到另一台 Windows 设备后，在该设备已安装兼容 Visual Studio C++ 工具链和 Windows SDK 的前提下，可以不联网完成构建；不能绕过操作系统、MSVC、Windows SDK 或 Python/PyInstaller 本身的安装要求。设置编辑器发布优先使用已纳入目录的 `OptiScalerSettings.exe`，因此没有 Python 时也不影响主发布包组装。

## UI 设计

- 保留 `MenuCommon` 作为 DX11、DX12 和 Vulkan 共用的 UI 业务层。
- 合入源版菜单布局、语言选择和 CJK 字体字形范围。
- 保留目标项目全部特有面板，并为其接入翻译。
- ImGui 控件使用稳定内部 ID，显示文本变化不能改变控件标识。
- 保留 Hack 字体和当前缩放机制，在字体 atlas 中增加中文覆盖。
- 启动提示在显示时查询翻译，避免静态初始化固化语言。
- 语言切换不得重建 Hook、交换链或图形后端，只刷新显示文本和必要字体状态。

## 翻译设计

- 引入 `Language::English` 和 `Language::Chinese`。
- 英文直接使用原始文本，中文从内置映射读取。
- 缺少、为空或无效的中文条目回退英文。
- 语言写入现有配置系统，缺失或无效值默认英文。
- 源版已有中文翻译直接合入，目标项目新增功能补充中英文条目。
- 配置键、日志机器字段和 DLL 导出名不参与翻译。
- 翻译检查禁止空标签和重复控件 ID，允许明确的英文回退。

## 设置编辑器设计

- 只移植设置编辑器，不移植安装器。
- 使用 Python Tkinter/ttk，不增加运行时第三方 GUI 依赖。
- 保留中英文界面、INI 注释保留、未知键保留、类型和范围校验。
- `settings_spec.py` 补齐目标项目特有 FSR4、XeFG 和低延迟配置，字段名与 `OptiScaler.ini` 和 `Config` 严格一致。
- 使用 `OptiScalerSettings.spec` 生成单文件 `OptiScalerSettings.exe`。
- 设置编辑器构建失败不阻止主 DLL 编译，但发布完整性检查会明确报告缺失；已有可用 EXE 可以作为本地回退。
- 不复制源目录中的 `build`、`dist`、缓存、日志、备份和安装器产物。

## 发布目录规则

源版 `Release|x64` 的最终可发布根目录是 `x64\Release\a`，而不是直接使用 `x64\Release`。目标项目复刻以下目录树：

```text
x64\Release\a\
├── OptiScaler.dll
├── OptiScaler.ini
├── setup_windows.bat
├── setup_linux.sh
├── OptiScalerSettings.exe
├── !! EXTRACT ALL FILES TO GAME FOLDER !!
├── Licenses\
│   ├── XeSS_LICENSE.txt
│   ├── FidelityFX_v1_LICENSE.md
│   ├── FidelityFX_v2_LICENSE.md
│   └── DirectX_LICENSE.txt
└── OptiScaler\
    ├── XeSS 运行时 DLL
    ├── amd_fidelityfx_vk.dll
    ├── amd_fidelityfx_dx12.dll
    ├── amd_fidelityfx_upscaler_dx12.dll
    ├── amd_fidelityfx_framegeneration_dx12.dll
    ├── fakenvapi.dll
    ├── fakenvapi.ini
    ├── dlssg_to_fsr3_amd_is_better.dll
    ├── streamline\
    ├── plugins\OptiPatcher.asi
    └── D3D12_OptiScaler\DirectX Agility SDK DLL
```

如果目标项目当前发布规则包含源版清单之外且仍被现有功能使用的文件，这些文件继续保留并加入完整性检查。

## 文件来源

### 编译输出

- 将 `x64\Release\OptiScaler.dll` 移动到 `x64\Release\a\OptiScaler.dll`。
- 设置编辑器生成的 `OptiScalerSettings.exe` 复制到发布根目录。

### 从项目目录复制

- `external\xess\LICENSE.txt` 复制为 `Licenses\XeSS_LICENSE.txt`。
- `external\xess\bin\*.dll` 复制到 `OptiScaler\`。
- FidelityFX v1 许可证和 `amd_fidelityfx_vk.dll` 复制到对应目录。
- FidelityFX v2 loader、upscaler 和 frame generation DLL 复制到 `OptiScaler\`，loader 重命名为 `amd_fidelityfx_dx12.dll`。
- `streamline\*` 递归复制到 `OptiScaler\streamline\`。
- FidelityFX v2、DirectX 许可证复制到 `Licenses\`。
- `OptiScaler.ini`、`setup_windows.bat`、`setup_linux.sh` 复制到发布根目录。
- DirectX Agility SDK DLL 复制到 `OptiScaler\D3D12_OptiScaler\`。
- 创建解压提示空文件。

### 从 GitHub 下载并本地回退

- FakeNVAPI 从 `optiscaler/fakenvapi` 的 `v1.4.1` 发布包下载，解压 `fakenvapi.dll` 和 `fakenvapi.ini`；下载失败时从项目根目录复制同名文件。
- `dlssg_to_fsr3_amd_is_better.dll` 从 `optiscaler/dlssg-to-fsr3` 的 `0.130` 发布包下载；下载失败时从项目根目录复制。
- `OptiPatcher.asi` 从 `optiscaler/OptiPatcher` 的 `rolling` 发布下载；下载失败时从 `plugins\OptiPatcher.asi` 复制。
- 下载缓存放入 `x64\Release\_tmp`，构建前清理。
- 下载只在目标文件缺失时执行，避免无意义重复下载。
- 下载后删除第三方压缩包附带但不属于统一发布清单的顶层 `LICENSE*` 文件，许可证统一放入 `Licenses\`。
- 默认不执行网络下载；只有显式启用更新模式时才访问 GitHub。
- 离线模式缺少任何必需本地回退文件时，在编译前完整性检查阶段失败，并输出具体文件、目标路径和来源说明。

## 构建前规则

- 清理旧的 `x64\Release\a\OptiScaler` 和 `x64\Release\_tmp`，防止陈旧运行时混入发布包。
- 生成 `resource_build_date.h`。
- 使用 Git 短提交号生成 `resource_build_commit.h`；非 Git 工作区使用 `unknown`。
- 不删除目标项目源码、第三方依赖或用户配置源文件。

## 编译工具探测

- 新构建入口优先检查 D 盘 Visual Studio 安装。
- 未找到时调用 `vswhere`，随后检查常见 Visual Studio 安装目录。
- 优先使用源版要求的工具集；不可用时使用目标项目的 `v143`；仍不可用时选择本机实际安装的兼容工具集。
- 工具集回退通过 MSBuild 参数或临时生成的构建设置完成，不永久覆盖工程要求。
- 构建报告记录实际使用的 MSBuild、Platform Toolset、Windows SDK、配置和平台。
- 主验收目标为 `Release|x64`，辅助检查 `Debug|x64`。

## 错误处理

- 编译、链接、设置编辑器打包、网络下载、本地复制和发布完整性检查分别报告结果。
- 可选设置编辑器失败不掩盖 DLL 编译结果，但最终发布包标记为不完整。
- 所有可选下载使用固定版本 URL，避免未审查的 latest 版本改变二进制行为。
- 本地复制前检查源文件，复制后检查目标文件存在且非空。
- 发布组装不静默忽略必需文件缺失。
- 复制到新设备后先执行离线依赖检查，再执行 MSBuild；结果明确区分缺少源码/库、缺少工具链和缺少可选设置器构建环境。

## 验证

- 编译 `Release|x64` 并检查链接成功、导出表未丢失。
- 编译或至少完成 `Debug|x64` 编译检查。
- 检查翻译源码和字体已纳入工程。
- 检查英文、中文、运行时切换、缺失翻译回退和稳定 ImGui ID。
- 检查设置编辑器可启动，能读取、修改和保存测试 INI，未知键与注释保持不变。
- 检查 `x64\Release\a` 的完整目录树、必需文件和非空文件。
- 核对 GitHub 下载文件与本地回退路径。
- 核对目标项目已有特有运行时文件未因源版规则而丢失。
- 使用发布目录而不是中间输出目录作为最终验收对象。
- 在断网条件下从全新复制的目标目录完成 Release x64 构建和发布目录组装。
- 删除或隐藏源版目录后，构建仍不读取源版路径中的任何文件。

## 验收标准

- 当前项目特有功能和配置均保留。
- 游戏内菜单可使用英文和简体中文，中文字符正常显示。
- DX11、DX12、Vulkan UI 后端保持可编译。
- 设置编辑器存在且支持当前项目配置，不包含安装器。
- 本机可自动找到 D 盘或其他位置的可用 Visual Studio 编译工具。
- `Release|x64` 构建后自动生成与源版规则一致并包含目标项目扩展的 `x64\Release\a` 发布目录。
- GitHub 下载失败时正确使用本地回退；两者均缺失时构建明确失败。
- 不自动提交代码。
