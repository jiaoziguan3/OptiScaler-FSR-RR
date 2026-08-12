#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Field specifications for the OptiScaler settings editor.

Each module maps an INI section to a full list of its fields.
Field tuple: (ini_key, widget, options, {'zh': (label, help), 'en': (label, help)})

widget types
    'combo'  -> options is a list of literal values (first one is usually 'auto')
    'slider' -> options is (min, max, step)  ; rendered as auto-checkbox + slider + entry
    'entry'  -> free text (paths, key codes)
"""

# Reusable option lists
BOOL3 = ['auto', 'true', 'false']
RESOURCE_STATES = ['auto', '0', '1', '2', '4', '8', '16', '32', '64', '128']


MODULES = [
    # ------------------------------------------------------------------ #
    {
        'id': 'upscalers',
        'section': 'Upscalers',
        'name': {'zh': '上采样器 (Upscalers)', 'en': 'Upscalers'},
        'fields': [
            ('Dx11Upscaler', 'combo',
             ['auto', 'fsr22', 'fsr31', 'xess', 'xess_12', 'fsr21_12', 'fsr22_12', 'ffx_12', 'dlss'],
             {'zh': ('DX11 上采样器',
                     'DX11 游戏使用的上采样器。\n\n'
                     'fsr22 / fsr31 / xess：原生 DX11 实现。xess 仅 Intel Arc 可用。\n'
                     '带 _12 后缀的选项走 DX11on12 转译层，兼容性更好但有额外开销。\n'
                     'ffx_12：FSR 2.3 / 3.1 / 4.x。\n'
                     'dlss：需要 N 卡。\n\n'
                     'auto 默认为 fsr22。'),
              'en': ('DX11 Upscaler',
                     'Upscaler used for DX11 games.\n\n'
                     'fsr22 / fsr31 / xess: native DX11. xess is Arc-only.\n'
                     '_12 suffixed options run through the DX11on12 layer - better '
                     'compatibility, some overhead.\n'
                     'ffx_12: FSR 2.3 / 3.1 / 4.x.\n'
                     'dlss: requires an NVIDIA GPU.\n\n'
                     'auto defaults to fsr22.')}),

            ('Dx12Upscaler', 'combo',
             ['auto', 'xess', 'fsr21', 'fsr22', 'ffx', 'dlss'],
             {'zh': ('DX12 上采样器',
                     'DX12 游戏使用的上采样器。\n\n'
                     'ffx 覆盖 FSR 2.3 / 3.1 / 4.x。\n\n'
                     'auto 会按显卡能力挑选：支持 DLSS 就用 DLSS，支持 FSR4 就用 FSR4，'
                     '否则退回 XeSS。'),
              'en': ('DX12 Upscaler',
                     'Upscaler used for DX12 games.\n\n'
                     'ffx covers FSR 2.3 / 3.1 / 4.x.\n\n'
                     'auto picks by GPU capability: DLSS if capable, FSR4 if capable, '
                     'otherwise XeSS.')}),

            ('VulkanUpscaler', 'combo',
             ['auto', 'fsr21', 'fsr22', 'ffx', 'xess', 'fsr21_12', 'ffx_12', 'dlss'],
             {'zh': ('Vulkan 上采样器',
                     'Vulkan 游戏使用的上采样器。\n\n'
                     'fsr21 / fsr22 / ffx / xess：原生 Vulkan 实现。\n'
                     '带 _12 后缀的走 VKon12 转译层。\n\n'
                     'auto 默认为 fsr22。'),
              'en': ('Vulkan Upscaler',
                     'Upscaler used for Vulkan games.\n\n'
                     'fsr21 / fsr22 / ffx / xess: native Vulkan.\n'
                     '_12 suffixed options run through the VKon12 layer.\n\n'
                     'auto defaults to fsr22.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'framegen',
        'section': 'FrameGen',
        'name': {'zh': '帧生成 (FrameGen)', 'en': 'Frame Generation'},
        'fields': [
            ('Enabled', 'combo', BOOL3,
             {'zh': ('启用帧生成',
                     '帧生成总开关。\n\n关闭时下面所有帧生成相关选项都不起作用。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Frame Generation',
                     'Master switch for frame generation.\n\nWhen off, none of the options '
                     'below take effect.\n\nauto defaults to false.')}),

            ('FGInput', 'combo',
             ['auto', 'nofg', 'dlssg', 'nvngxfg', 'fsrfg', 'upscaler', 'fsrfg30'],
             {'zh': ('帧生成输入源',
                     '帧生成的数据来源。\n\n'
                     'dlssg：可搭配任意输出，自带 Hudless 支持，但只限使用 Streamline + DLSSG 的游戏。\n'
                     'nvngxfg：只能配 FSR 3 FG，需要游戏本身有 DLSSG，用 Streamline 交换链做帧序控制。\n'
                     'fsrfg：可搭配任意输出，自带 Hudless 支持。\n'
                     'upscaler：需先启用上采样器，兼容性最广但部分游戏效果不完美，'
                     '需要 Hudfix 才能避免 UI 撕裂。\n'
                     'fsrfg30：可搭配任意输出，自带 Hudless 支持。\n\n'
                     'auto 默认为 nofg（不启用）。'),
              'en': ('FG Input / Source',
                     'Where frame generation gets its data.\n\n'
                     'dlssg: works with any output, Hudless out of the box, limited to games '
                     'using Streamline and DLSSG.\n'
                     'nvngxfg: FSR 3 FG only, requires DLSSG in the game, uses the Streamline '
                     'swapchain for pacing.\n'
                     'fsrfg: works with any output, Hudless out of the box.\n'
                     'upscaler: requires an upscaler; broadest compatibility but imperfect in '
                     'some games. Hudfix is needed to avoid UI glitching.\n'
                     'fsrfg30: works with any output, Hudless out of the box.\n\n'
                     'auto defaults to nofg.')}),

            ('FGOutput', 'combo',
             ['auto', 'nofg', 'fsrfg', 'xefg', 'nvngxfg', 'dlssg', 'dlssgwithnvngx'],
             {'zh': ('帧生成输出',
                     '实际生成插帧的后端，各自有 DLL 依赖。\n\n'
                     'fsrfg：需要 amd_fidelityfx_dx12.dll，或 amd_fidelityfx_loader_dx12.dll '
                     '+ amd_fidelityfx_framegeneration_dx12.dll。\n'
                     'xefg：需要 libxess_fg.dll 和 libxell.dll。\n'
                     'nvngxfg：需要 dlssg_to_fsr3_amd_is_better.dll 或 dlss-enabler-headless.dll。\n'
                     'dlssg：需要 OptiScaler/streamline 文件夹里的 streamline DLL + nvngx_dlssg.dll。\n'
                     'dlssgwithnvngx：上面两者的 DLL 都要。\n\n'
                     '缺少对应 DLL 时该输出会失效。auto 默认为 nofg。'),
              'en': ('FG Output',
                     'The backend that actually interpolates frames. Each has DLL '
                     'requirements.\n\n'
                     'fsrfg: amd_fidelityfx_dx12.dll, or amd_fidelityfx_loader_dx12.dll + '
                     'amd_fidelityfx_framegeneration_dx12.dll.\n'
                     'xefg: libxess_fg.dll and libxell.dll.\n'
                     'nvngxfg: dlssg_to_fsr3_amd_is_better.dll or dlss-enabler-headless.dll.\n'
                     'dlssg: streamline DLLs in OptiScaler/streamline plus nvngx_dlssg.dll.\n'
                     'dlssgwithnvngx: DLLs from both of the above.\n\n'
                     'The output silently fails if its DLLs are missing. auto defaults to '
                     'nofg.')}),

            ('FTInput', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('帧时间来源',
                     '影响帧序节奏（frame pacing）。\n\n'
                     '0：取自输入端（DLSSG / FSRFG / OptiFG）\n'
                     '1：取两次 Present 调用之间的时间\n'
                     '2：恒为零，仅适用于 XeFG\n\n'
                     '帧生成时间不均匀、画面有节奏性卡顿时可以换一个试试。auto 默认为 0。'),
              'en': ('Frametime Source',
                     'Affects frame pacing.\n\n'
                     '0: from the FG input (DLSSG / FSRFG / OptiFG)\n'
                     '1: time between Present calls\n'
                     '2: zero, XeFG only\n\n'
                     'Worth switching if pacing feels uneven. auto defaults to 0.')}),

            ('DebugView', 'combo', BOOL3,
             {'zh': ('帧生成调试视图',
                     '开启后画面上叠加帧生成的调试可视化，能看到运动矢量、插帧区域等。\n\n'
                     '排查画面异常时很有用，正常游玩请关闭。auto 默认为 false。'),
              'en': ('FG Debug View',
                     'Overlays frame generation debug visualisation - motion vectors, '
                     'interpolated regions, etc.\n\n'
                     'Useful for diagnosing artifacts, turn off for normal play. auto '
                     'defaults to false.')}),

            ('DrawUIOverFG', 'combo', BOOL3,
             {'zh': ('在插帧上重绘 UI',
                     '把 UI 画在生成帧之上，需要 Hudless 和一张 UI 贴图，主要给 FSR FG 用。\n\n'
                     '能解决插帧导致的 UI 拖影/重影，但要求游戏能正确提供 UI 贴图，'
                     '否则可能 UI 消失。auto 默认为 false。'),
              'en': ('Draw UI Over FG',
                     'Draws the UI on top of generated frames. Requires Hudless and a UI '
                     'texture; mostly for FSR FG.\n\n'
                     'Fixes UI ghosting from interpolation, but needs the game to hand over a '
                     'valid UI texture or the UI may disappear. auto defaults to false.')}),

            ('UIPremultipliedAlpha', 'combo', BOOL3,
             {'zh': ('UI 贴图预乘 Alpha',
                     '声明提供的 UI 贴图是否为预乘 Alpha 格式。\n\n'
                     '配合「在插帧上重绘 UI」+ FSR FG 使用时会改变 UI 的观感。'
                     '设错会让 UI 边缘发黑或发白。auto 默认为 true。'),
              'en': ('UI Premultiplied Alpha',
                     'Declares whether the supplied UI texture uses premultiplied alpha.\n\n'
                     'With DrawUIOverFG + FSR FG this changes how the UI looks. Getting it '
                     'wrong gives dark or bright fringes. auto defaults to true.')}),

            ('DisableHudless', 'combo', BOOL3,
             {'zh': ('禁用 Hudless',
                     '即使帧生成输入端已提供 Hudless（无 HUD 画面），也强制不使用。\n\n'
                     '一般不需要动，除非 Hudless 通道本身导致画面出错。auto 默认为 false。'),
              'en': ('Disable Hudless',
                     'Forces Hudless off even when the FG input provides one.\n\n'
                     'Rarely needed, unless the Hudless path itself causes artifacts. auto '
                     'defaults to false.')}),

            ('DisableUI', 'combo', BOOL3,
             {'zh': ('禁用 UI 贴图',
                     '即使帧生成输入端已提供 UI 贴图，也强制不使用。\n\n'
                     'auto 默认为 true。'),
              'en': ('Disable UI Texture',
                     'Forces the UI texture off even when the FG input provides one.\n\n'
                     'auto defaults to true.')}),

            ('SkipReset', 'combo', BOOL3,
             {'zh': ('忽略重置信号',
                     '忽略帧生成输入端发来的 Reset 信号。\n\n'
                     'Reset 通常在场景切换时触发以清空历史。忽略它可以减少切场景时的闪烁，'
                     '但也可能让残影跨场景残留。auto 默认为 false。'),
              'en': ('Skip Reset',
                     'Ignores Reset signals coming from FG inputs.\n\n'
                     'Reset normally fires on scene changes to clear history. Ignoring it can '
                     'reduce flicker at cuts but may carry ghosting across scenes. auto '
                     'defaults to false.')}),

            ('RectLeft', 'slider', (0, 3840, 1),
             {'zh': ('帧生成区域 左',
                     '限定帧生成作用的矩形区域，左边界像素坐标。\n\n'
                     '一般保持 auto（整屏）。只在需要把插帧限制在画面局部时才设。'),
              'en': ('FG Rect Left',
                     'Left edge, in pixels, of the rectangle frame generation applies to.\n\n'
                     'Normally leave on auto (whole screen).')}),

            ('RectTop', 'slider', (0, 2160, 1),
             {'zh': ('帧生成区域 上',
                     '帧生成矩形区域的上边界像素坐标。\n\n一般保持 auto（整屏）。'),
              'en': ('FG Rect Top',
                     'Top edge, in pixels, of the frame generation rectangle.\n\n'
                     'Normally leave on auto (whole screen).')}),

            ('RectWidth', 'slider', (0, 3840, 1),
             {'zh': ('帧生成区域 宽',
                     '帧生成矩形区域的宽度（像素）。\n\n一般保持 auto（整屏）。'),
              'en': ('FG Rect Width',
                     'Width, in pixels, of the frame generation rectangle.\n\n'
                     'Normally leave on auto (whole screen).')}),

            ('RectHeight', 'slider', (0, 2160, 1),
             {'zh': ('帧生成区域 高',
                     '帧生成矩形区域的高度（像素）。\n\n一般保持 auto（整屏）。'),
              'en': ('FG Rect Height',
                     'Height, in pixels, of the frame generation rectangle.\n\n'
                     'Normally leave on auto (whole screen).')}),

            ('AllowedFrameAhead', 'slider', (1, 3, 1),
             {'zh': ('允许超前帧数',
                     '帧生成允许领先游戏逻辑的帧数。\n\n'
                     '调高能减少帧生成反复开关的抖动，但也可能引入新问题和额外延迟。\n\n'
                     '范围 1-3，auto 默认为 1。'),
              'en': ('Allowed Frames Ahead',
                     'How many frames FG may run ahead of the game.\n\n'
                     'Raising it can stop FG from toggling on and off, but may introduce '
                     'other issues and latency.\n\n'
                     'Range 1-3, auto defaults to 1.')}),

            ('DepthValidNow', 'combo', BOOL3,
             {'zh': ('深度始终标记为有效',
                     '总是把深度缓冲标记为 ValidNow。\n\n'
                     '会多占用显存，但 Uniscaler 需要它。auto 默认为 false。'),
              'en': ('Depth Always ValidNow',
                     'Always tags the depth buffer as ValidNow.\n\n'
                     'Uses more VRAM but is required by Uniscaler. auto defaults to false.')}),

            ('VelocityValidNow', 'combo', BOOL3,
             {'zh': ('运动矢量始终标记为有效',
                     '总是把速度/运动矢量缓冲标记为 ValidNow。\n\n'
                     '会多占用显存，但 Uniscaler 需要它。auto 默认为 false。'),
              'en': ('Velocity Always ValidNow',
                     'Always tags the velocity buffer as ValidNow.\n\n'
                     'Uses more VRAM but is required by Uniscaler. auto defaults to false.')}),

            ('HudlessValidNow', 'combo', BOOL3,
             {'zh': ('Hudless 始终标记为有效',
                     '总是把 Hudless 缓冲标记为 ValidNow。\n\n'
                     '会多占用显存，但 Uniscaler 需要它。auto 默认为 false。'),
              'en': ('Hudless Always ValidNow',
                     'Always tags the Hudless buffer as ValidNow.\n\n'
                     'Uses more VRAM but is required by Uniscaler. auto defaults to false.')}),

            ('OnlyAcceptFirstHudless', 'combo', BOOL3,
             {'zh': ('只接受第一个 Hudless',
                     '当输入端提交了多个 Hudless 贴图时，只用第一个。\n\n'
                     '某些游戏会提交多张导致 UI 处理出错，此时可以开启。auto 默认为 false。'),
              'en': ('Only Accept First Hudless',
                     'If the source tags more than one hudless, use only the first.\n\n'
                     'Helps in games that submit several and confuse UI handling. auto '
                     'defaults to false.')}),

            ('PreserveSwapChain', 'combo', BOOL3,
             {'zh': ('保留交换链',
                     '阻止释放帧生成使用的交换链。\n\n'
                     '能避免释放交换链时崩溃（切分辨率、切窗口模式时常见）。'
                     'auto 默认为 true，遇到崩溃再考虑关。'),
              'en': ('Preserve SwapChain',
                     'Prevents the FG swapchain from being released.\n\n'
                     'Avoids crashes when the swapchain is released - common when changing '
                     'resolution or window mode. auto defaults to true.')}),

            ('SkipResizeBuffers', 'combo', BOOL3,
             {'zh': ('跳过 ResizeBuffers',
                     '当新旧描述完全一致时，跳过对交换链的 ResizeBuffers 调用。\n\n'
                     '能避免 ResizeBuffers 之后崩溃。auto 默认为 true。'),
              'en': ('Skip ResizeBuffers',
                     'Skips the swapchain ResizeBuffers call when old and new descriptions '
                     'match.\n\nPrevents crashes after ResizeBuffers. auto defaults to true.')}),

            ('ModifyBufferState', 'combo', BOOL3,
             {'zh': ('修改缓冲区状态',
                     '跳过 ResizeBuffers 时，尝试手动把交换链缓冲设为正确状态'
                     '（正常情况下 ResizeBuffers 后应为 COMMON）。\n\n'
                     '可能修好崩溃，也可能引入崩溃，属于试错型选项。auto 默认为 false。'),
              'en': ('Modify Buffer State',
                     'When skipping ResizeBuffers, try to set swapchain buffer states manually '
                     '(normally they should be COMMON afterwards).\n\n'
                     'May fix crashes or cause them - trial and error. auto defaults to '
                     'false.')}),

            ('ModifySCIndex', 'combo', BOOL3,
             {'zh': ('修改交换链索引',
                     '跳过 ResizeBuffers 时，反复调用 Present 直到交换链当前索引归零'
                     '（正常情况下 ResizeBuffers 后应为 0）。\n\n'
                     '可能修好崩溃，也可能引入崩溃。auto 默认为 false。'),
              'en': ('Modify SwapChain Index',
                     'When skipping ResizeBuffers, call Present repeatedly until the swapchain '
                     'index reaches 0 (normally it should already be 0).\n\n'
                     'May fix crashes or cause them. auto defaults to false.')}),

            ('HudCutoff', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('HUD 透明度裁切',
                     '裁掉 UI 的低透明度部分，帮助插帧算法处理 UI。\n\n'
                     '只对 FSR FG 输出和 NvngxFG 生效。\n'
                     '范围 0.0-1.0，auto 默认为 0.0（除非某游戏已知需要）。'),
              'en': ('HUD Cutoff',
                     'Cuts off low-alpha parts of the UI to help interpolation.\n\n'
                     'Only affects the FSR FG output and NvngxFG.\n'
                     'Range 0.0-1.0, auto defaults to 0.0 unless a game is known to need '
                     'it.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'menu',
        'section': 'Menu',
        'name': {'zh': '菜单与叠加层 (Menu)', 'en': 'Menu & Overlay'},
        'fields': [
            ('OverlayMenu', 'combo', BOOL3,
             {'zh': ('启用叠加菜单',
                     '启用新版 ImGui 叠加菜单。\n\n'
                     '注意：关闭此项会让 OptiScaler 禁用所有帧生成功能。\n\n'
                     'auto 的行为取决于代理 DLL 名：作为 nvngx.dll 时为 false，其他情况为 true。'),
              'en': ('Enable Overlay Menu',
                     'Enables the new ImGui overlay menus.\n\n'
                     'Note: turning this off makes OptiScaler disable all FG features.\n\n'
                     'auto is false when OptiScaler is nvngx.dll, true otherwise.')}),

            ('Scale', 'slider', (0.5, 2.0, 0.05),
             {'zh': ('菜单缩放',
                     '游戏内 ImGui 菜单的整体缩放。\n\n'
                     '范围 0.5-2.0，auto 默认为 1.0，低于 900p 时会自动调小。'),
              'en': ('Menu Scale',
                     'Overall scale of the in-game ImGui menu.\n\n'
                     'Range 0.5-2.0, auto defaults to 1.0 and gets smaller below 900p.')}),

            ('ShortcutKey', 'entry', None,
             {'zh': ('菜单快捷键',
                     '打开菜单的按键，填 Windows 虚拟键码（16 进制）。\n\n'
                     '例：0x2D = Insert（当前默认），0x24 = Home（旧默认），0x08 = Backspace。\n'
                     '填 -1 表示不设快捷键。\n\n'
                     '完整键码表见微软文档 Virtual-Key Codes。'),
              'en': ('Menu Shortcut Key',
                     'Key that opens the menu, as a Windows virtual key code in hex.\n\n'
                     'e.g. 0x2D = Insert (current default), 0x24 = Home (old default), '
                     '0x08 = Backspace.\n'
                     '-1 disables the shortcut.\n\n'
                     'See the Microsoft Virtual-Key Codes reference for the full list.')}),

            ('ExtendedLimits', 'combo', BOOL3,
             {'zh': ('扩展缩放比例上限',
                     '把缩放比例的可调范围放宽到 0.1-6.0。\n\n'
                     '用于极端降采样或超采样实验，正常使用不需要。auto 默认为 false。'),
              'en': ('Extended Limits',
                     'Extends the scaling ratio range to 0.1-6.0.\n\n'
                     'For extreme down/supersampling experiments. auto defaults to false.')}),

            ('UseHQFont', 'combo', BOOL3,
             {'zh': ('使用高质量字体',
                     '菜单使用高质量字体渲染。\n\n'
                     '会多占一些显存。如果在 Vulkan 下看到黑色叠加层或菜单相关崩溃，'
                     '把这项关掉试试。auto 默认为 true。'),
              'en': ('Use HQ Font',
                     'Renders the menu with a high quality font.\n\n'
                     'Uses a bit more VRAM. If you see a black overlay or menu crashes under '
                     'Vulkan, try disabling it. auto defaults to true.')}),

            ('FontSize', 'slider', (8.0, 32.0, 0.5),
             {'zh': ('字体大小',
                     '只改字号，不影响其他界面元素尺寸。\n\n'
                     '搭配 TTFFontPath 用自定义字体时比较有用。auto 默认为 14.0。'),
              'en': ('Font Size',
                     'Changes just the font size without touching other style elements.\n\n'
                     'Mostly useful with a custom TTFFontPath. auto defaults to 14.0.')}),

            ('TTFFontPath', 'entry', None,
             {'zh': ('自定义字体路径',
                     '高质量字体所用的 TTF 文件路径，需要先开启「使用高质量字体」。\n\n'
                     '建议用等宽字体。例：C:\\Windows\\Fonts\\comic.ttf\n\n'
                     'auto 使用 OptiScaler 自带的 Hack-Regular。'),
              'en': ('Custom Font Path',
                     'Path to a TTF used as the HQ font. Requires UseHQFont=true.\n\n'
                     'Monospace fonts work best. Example: C:\\Windows\\Fonts\\comic.ttf\n\n'
                     'auto uses the bundled Hack-Regular.')}),

            ('DisableSplash', 'combo', BOOL3,
             {'zh': ('禁用启动提示',
                     '关闭启动时的 Splash 提示信息。\n\nauto 默认为 false。'),
              'en': ('Disable Splash',
                     'Disables the startup splash message.\n\nauto defaults to false.')}),

            ('ShowFps', 'combo', BOOL3,
             {'zh': ('显示 FPS 叠加层',
                     '在画面上显示帧率叠加层。\n\nauto 默认为 false。'),
              'en': ('Show FPS Overlay',
                     'Shows the FPS overlay.\n\nauto defaults to false.')}),

            ('FpsScale', 'slider', (0.5, 2.0, 0.05),
             {'zh': ('FPS 叠加层缩放',
                     'FPS 叠加层的缩放比例。\n\n'
                     '范围 0.5-2.0，auto 表示跟随菜单缩放。'),
              'en': ('FPS Overlay Scale',
                     'Scale of the FPS overlay.\n\n'
                     'Range 0.5-2.0, auto follows the menu scale.')}),

            ('FpsOverlayPos', 'combo', ['auto', '0', '1', '2', '3'],
             {'zh': ('FPS 叠加层位置',
                     '0 = 左上　1 = 右上　2 = 左下　3 = 右下\n\nauto 默认为 0。'),
              'en': ('FPS Overlay Position',
                     '0 = Top Left, 1 = Top Right, 2 = Bottom Left, 3 = Bottom Right\n\n'
                     'auto defaults to 0.')}),

            ('FpsOverlayType', 'combo', ['auto', '0', '1', '2', '3', '4', '5', '6'],
             {'zh': ('FPS 叠加层类型',
                     '0 = 仅帧率\n1 = 简略\n2 = 详细\n3 = 详细 + 曲线图\n'
                     '4 = 完整\n5 = 完整 + 曲线图\n6 = Reflex 延迟时序\n\nauto 默认为 0。'),
              'en': ('FPS Overlay Type',
                     '0 = Just FPS\n1 = Simple\n2 = Detailed\n3 = Detailed + Graph\n'
                     '4 = Full\n5 = Full + Graph\n6 = Reflex timings\n\nauto defaults to 0.')}),

            ('FpsShortcutKey', 'entry', None,
             {'zh': ('FPS 叠加层快捷键',
                     '切换 FPS 叠加层的按键，虚拟键码（16 进制）。\n\n'
                     'auto 默认为 0x21 = Page Up。填 -1 表示不设快捷键。'),
              'en': ('FPS Overlay Shortcut',
                     'Key that toggles the FPS overlay, as a hex virtual key code.\n\n'
                     'auto defaults to 0x21 = Page Up. -1 disables it.')}),

            ('FpsCycleShortcutKey', 'entry', None,
             {'zh': ('FPS 类型切换快捷键',
                     '循环切换 FPS 叠加层类型的按键，虚拟键码（16 进制）。\n\n'
                     'auto 默认为 0x22 = Page Down。填 -1 表示不设快捷键。'),
              'en': ('FPS Type Cycle Shortcut',
                     'Key that cycles the FPS overlay type, as a hex virtual key code.\n\n'
                     'auto defaults to 0x22 = Page Down. -1 disables it.')}),

            ('FpsOverlayHorizontal', 'combo', BOOL3,
             {'zh': ('FPS 叠加层横向排列',
                     '把 FPS 叠加层的内容改为横向布局。\n\nauto 默认为 false。'),
              'en': ('Horizontal FPS Layout',
                     'Lays the FPS overlay out horizontally.\n\nauto defaults to false.')}),

            ('FpsOverlayAlpha', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('FPS 叠加层背景不透明度',
                     '0.0 全透明，1.0 完全不透明。\n\nauto 默认为 0.4。'),
              'en': ('FPS Overlay Background Alpha',
                     '0.0 fully transparent, 1.0 fully opaque.\n\nauto defaults to 0.4.')}),

            ('FGShortcutKey', 'entry', None,
             {'zh': ('帧生成开关快捷键',
                     '快速开关帧生成的按键，虚拟键码（16 进制）。\n\n'
                     'auto 默认为 0x23 = End。填 -1 表示不设快捷键。\n\n'
                     '对比帧生成开/关效果时很方便。'),
              'en': ('FG Toggle Shortcut',
                     'Key that toggles frame generation, as a hex virtual key code.\n\n'
                     'auto defaults to 0x23 = End. -1 disables it.\n\n'
                     'Handy for A/B comparing FG on and off.')}),

            ('OverlaysUseTheme', 'combo', BOOL3,
             {'zh': ('叠加层套用主题色',
                     '让 FPS 等叠加层也使用菜单的主题配色。\n\nauto 默认为 false。'),
              'en': ('Overlays Use Theme',
                     'Makes overlays follow the menu theme colours.\n\n'
                     'auto defaults to false.')}),

            ('LightTheme', 'combo', BOOL3,
             {'zh': ('浅色主题',
                     '菜单使用浅色主题。\n\nauto 默认为 false（深色）。'),
              'en': ('Light Theme',
                     'Uses the light theme for the menu.\n\nauto defaults to false (dark).')}),

            ('AccentColorR', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('强调色 R',
                     '菜单强调色的红色分量，0.0-1.0。\n\nauto 默认为 0.0（配合默认 G/B 为蓝色）。'),
              'en': ('Accent Colour R',
                     'Red component of the menu accent colour, 0.0-1.0.\n\n'
                     'auto defaults to 0.0 (blue with the default G/B).')}),

            ('AccentColorG', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('强调色 G',
                     '菜单强调色的绿色分量，0.0-1.0。\n\nauto 默认为 0.4。'),
              'en': ('Accent Colour G',
                     'Green component of the menu accent colour, 0.0-1.0.\n\n'
                     'auto defaults to 0.4.')}),

            ('AccentColorB', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('强调色 B',
                     '菜单强调色的蓝色分量，0.0-1.0。\n\nauto 默认为 0.77。'),
              'en': ('Accent Colour B',
                     'Blue component of the menu accent colour, 0.0-1.0.\n\n'
                     'auto defaults to 0.77.')}),

            ('BGColorR', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('背景色 R',
                     '菜单背景色的红色分量，0.0-1.0。\n\nauto 默认为 0.0（黑色）。'),
              'en': ('Background Colour R',
                     'Red component of the menu background, 0.0-1.0.\n\n'
                     'auto defaults to 0.0 (black).')}),

            ('BGColorG', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('背景色 G',
                     '菜单背景色的绿色分量，0.0-1.0。\n\nauto 默认为 0.0。'),
              'en': ('Background Colour G',
                     'Green component of the menu background, 0.0-1.0.\n\n'
                     'auto defaults to 0.0.')}),

            ('BGColorB', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('背景色 B',
                     '菜单背景色的蓝色分量，0.0-1.0。\n\nauto 默认为 0.0。'),
              'en': ('Background Colour B',
                     'Blue component of the menu background, 0.0-1.0.\n\n'
                     'auto defaults to 0.0.')}),

            ('BGColorA', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('背景不透明度',
                     '0.0 全透明，1.0 完全不透明。\n\nauto 默认为 0.99。'),
              'en': ('Background Alpha',
                     '0.0 fully transparent, 1.0 fully opaque.\n\nauto defaults to 0.99.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'fsrfg',
        'section': 'FSRFG',
        'name': {'zh': 'FSR 帧生成 (FSRFG)', 'en': 'FSR Frame Generation'},
        'fields': [
            ('DebugTearLines', 'combo', BOOL3,
             {'zh': ('调试撕裂线',
                     '显示 FSR3.1 帧生成的撕裂线。\n\n仅用于排查画面撕裂问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('Debug Tear Lines',
                     'Draws FSR3.1 frame generation tear lines.\n\n'
                     'Diagnostic only.\n\nauto defaults to false.')}),

            ('DebugResetLines', 'combo', BOOL3,
             {'zh': ('调试重置线',
                     '显示帧生成插值被跳过的位置。\n\n'
                     '插值跳过通常发生在场景切换或运动矢量失效时。\n\n'
                     'auto 默认为 false。'),
              'en': ('Debug Reset Lines',
                     'Shows where frame generation interpolation was skipped.\n\n'
                     'Skips usually happen on scene cuts or invalid motion vectors.\n\n'
                     'auto defaults to false.')}),

            ('DebugPacingLines', 'combo', BOOL3,
             {'zh': ('调试节奏线',
                     '显示 FSR3.1 帧生成的帧节奏线。\n\n'
                     '用于观察生成帧的间隔是否均匀。\n\nauto 默认为 false。'),
              'en': ('Debug Pacing Lines',
                     'Draws FSR3.1 frame generation pacing lines.\n\n'
                     'Useful for checking whether generated frames are evenly spaced.\n\n'
                     'auto defaults to false.')}),

            ('AllowAsync', 'combo', BOOL3,
             {'zh': ('允许异步计算',
                     '启用异步 FSR3.1 帧生成。\n\n'
                     '可能提升性能，但在部分游戏上会造成不稳定。\n\n'
                     'auto 默认为 false。'),
              'en': ('Allow Async',
                     'Enables async FSR3.1 frame generation.\n\n'
                     'May improve performance but can be unstable in some games.\n\n'
                     'auto defaults to false.')}),

            ('UseMutexForSwapchain', 'combo', BOOL3,
             {'zh': ('交换链使用互斥锁',
                     '帧生成交换链的 Present 调用是否加互斥锁。\n\n'
                     '关闭可能提升性能，但会牺牲稳定性。\n\nauto 默认为 true。'),
              'en': ('Use Mutex For Swapchain',
                     'Whether FG swapchain Present calls use a mutex.\n\n'
                     'Disabling may improve performance at the cost of stability.\n\n'
                     'auto defaults to true.')}),

            ('FramePacingTuning', 'combo', BOOL3,
             {'zh': ('启用帧节奏调优',
                     '启用下面那组自定义帧节奏参数。\n\n'
                     '关闭时使用 FSR SDK 的内置默认值，下面几项会被忽略。\n\n'
                     'auto 默认为 true。'),
              'en': ('Frame Pacing Tuning',
                     'Enables the custom frame pacing parameters below.\n\n'
                     'When off, the FSR SDK defaults are used and the fields '
                     'below are ignored.\n\nauto defaults to true.')}),

            ('FPTSafetyMarginInMs', 'slider', (0.0, 2.0, 0.01),
             {'zh': ('安全余量 (毫秒)',
                     '帧节奏的安全余量，单位毫秒。\n\n'
                     '官方三组预设：\n'
                     '  默认调优：0.1 配 方差 0.1\n'
                     '  调优 A：0.75 配 方差 0.1\n'
                     '  调优 B：0.01 配 方差 0.3\n\n'
                     'OptiScaler 默认走调优 B。\n\nauto 默认为 0.01。'),
              'en': ('Safety Margin (ms)',
                     'Frame pacing safety margin in milliseconds.\n\n'
                     'The three official presets:\n'
                     '  Default:  0.1 with variance 0.1\n'
                     '  Tuning A: 0.75 with variance 0.1\n'
                     '  Tuning B: 0.01 with variance 0.3\n\n'
                     'OptiScaler uses Tuning B by default.\n\n'
                     'auto defaults to 0.01.')}),

            ('FPTVarianceFactor', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('方差因子',
                     '帧节奏的方差因子，0.0-1.0。\n\n'
                     '如果开帧生成后，从复杂场景过渡到简单场景时帧率意外偏低，\n'
                     '可以试试调优 B（安全余量 0.01 + 方差 0.3），\n'
                     '用略高的方差换回丢掉的帧率。\n\nauto 默认为 0.3。'),
              'en': ('Variance Factor',
                     'Frame pacing variance factor, 0.0-1.0.\n\n'
                     'If your frame rate stays unexpectedly low after moving from a '
                     'complex scene to a simple one, try Tuning B '
                     '(margin 0.01 + variance 0.3) to recover the lost FPS at the '
                     'cost of slightly higher variance.\n\nauto defaults to 0.3.')}),

            ('FPTHybridSpin', 'combo', BOOL3,
             {'zh': ('混合自旋',
                     '允许节奏自旋锁进入睡眠，降低 CPU 占用。\n\n'
                     '副作用是帧率爬升变慢。\n\nauto 默认为 false。'),
              'en': ('Hybrid Spin',
                     'Lets the pacing spinlock sleep, which reduces CPU usage.\n\n'
                     'Side effect: slower FPS ramp-up.\n\nauto defaults to false.')}),

            ('FPTHybridSpinTime', 'slider', (0, 64, 1),
             {'zh': ('混合自旋时长',
                     '开启混合自旋后的自旋时长，单位为计时器分辨率。\n\n'
                     '不建议低于 2，否则会频繁过冲。\n\nauto 默认为 2。'),
              'en': ('Hybrid Spin Time',
                     'How long to spin when Hybrid Spin is on, in timer '
                     'resolution units.\n\n'
                     'Going below 2 is not recommended: it causes frequent '
                     'overshoots.\n\nauto defaults to 2.')}),

            ('FPTWaitForSingleObjectOnFence', 'combo', BOOL3,
             {'zh': ('围栏等待用 WaitForSingleObject',
                     '等待围栏值时改用 WaitForSingleObject，而不是自旋。\n\n'
                     'auto 默认为 false。'),
              'en': ('Wait For Single Object On Fence',
                     'Uses WaitForSingleObject instead of spinning while waiting '
                     'for a fence value.\n\nauto defaults to false.')}),

            ('EnableWatermark', 'combo', BOOL3,
             {'zh': ('显示 Redstone 水印',
                     '显示 FSR-FG Redstone 水印。\n\nauto 默认为 false。'),
              'en': ('Enable Watermark',
                     'Shows the FSR-FG Redstone watermark.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'xefg',
        'section': 'XeFG',
        'name': {'zh': 'XeSS 帧生成 (XeFG)', 'en': 'XeSS Frame Generation'},
        'fields': [
            ('IgnoreInitChecks', 'combo', BOOL3,
             {'zh': ('跳过初始化检查',
                     '跳过 XeFG 的 ini 前置检查。\n\n'
                     '可能导致画质异常或崩溃，除非明确知道自己在做什么，'
                     '否则不要开。\n\nauto 默认为 false。'),
              'en': ('Ignore Init Checks',
                     'Skips XeFG pre-ini checks.\n\n'
                     'May cause bad image quality or crashes. Leave off unless you '
                     'know what you are doing.\n\nauto defaults to false.')}),

            ('InterpolationCount', 'combo', ['auto', '1', '2', '3'],
             {'zh': ('插帧倍率',
                     '生成帧的数量（多帧生成）。\n\n'
                     '1 = 2倍 | 2 = 3倍 | 3 = 4倍\n\nauto 默认为 1。'),
              'en': ('Interpolation Count',
                     'Number of interpolated frames (MFG).\n\n'
                     '1 = 2X | 2 = 3X | 3 = 4X\n\nauto defaults to 1.')}),

            ('DepthInverted', 'combo', BOOL3,
             {'zh': ('深度反转',
                     'XeFG 的 DepthInverted 标志。\n\n'
                     '大部分现代游戏用反转深度缓冲，改错会让插帧质量明显变差。\n\n'
                     'auto 默认为 true。'),
              'en': ('Depth Inverted',
                     'The XeFG DepthInverted flag.\n\n'
                     'Most modern games use an inverted depth buffer; getting this '
                     'wrong noticeably degrades interpolation.\n\n'
                     'auto defaults to true.')}),

            ('UIComposition', 'combo', BOOL3,
             {'zh': ('UI 合成',
                     '关闭 UI 插值。\n\n仅在游戏提供 hudless 资源时生效。\n\n'
                     'auto 默认为 false。'),
              'en': ('UI Composition',
                     'Disables UI interpolation.\n\n'
                     'Only works when a hudless resource is provided.\n\n'
                     'auto defaults to false.')}),

            ('JitteredMV', 'combo', BOOL3,
             {'zh': ('抖动运动矢量',
                     'XeFG 的 JitteredMV 标志。\n\n'
                     '告诉 XeFG 运动矢量里带了抖动偏移。\n\nauto 默认为 false。'),
              'en': ('Jittered MV',
                     'The XeFG JitteredMV flag.\n\n'
                     'Tells XeFG the motion vectors include jitter offsets.\n\n'
                     'auto defaults to false.')}),

            ('HighResMV', 'combo', BOOL3,
             {'zh': ('高分辨率运动矢量',
                     'XeFG 的 HighResMV 标志。\n\n'
                     '运动矢量为显示分辨率而非渲染分辨率时开启。\n\n'
                     'auto 默认为 false。'),
              'en': ('High Res MV',
                     'The XeFG HighResMV flag.\n\n'
                     'Enable when motion vectors are at display resolution rather '
                     'than render resolution.\n\nauto defaults to false.')}),

            ('DebugView', 'combo', BOOL3,
             {'zh': ('调试视图',
                     '显示 XeFG 的调试方块。\n\nauto 默认为 false。'),
              'en': ('Debug View',
                     'Shows XeFG debug squares.\n\nauto defaults to false.')}),

            ('ForceBorderless', 'combo', BOOL3,
             {'zh': ('强制无边框',
                     '用无边框窗口取代独占全屏。\n\n'
                     '切换显示模式时可能不稳定；另外当独占全屏分辨率小于'
                     '显示器分辨率时，不开这项可能出现画质问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('Force Borderless',
                     'Uses borderless mode instead of exclusive fullscreen.\n\n'
                     'May cause instability on mode changes. Conversely, image '
                     'quality issues can appear when exclusive fullscreen is '
                     'smaller than the display resolution.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'dlssg',
        'section': 'DLSSG',
        'name': {'zh': 'DLSS 帧生成 (DLSSG)', 'en': 'DLSS Frame Generation'},
        'fields': [
            ('InterpolationCount', 'combo', ['auto', '1', '2', '3', '4', '5'],
             {'zh': ('插帧倍率',
                     '生成帧的数量（多帧生成）。\n\n'
                     '1 = 2倍 | 2 = 3倍 | 3 = 4倍 | 4 = 5倍 | 5 = 6倍\n\n'
                     'auto 默认为 1。'),
              'en': ('Interpolation Count',
                     'Number of interpolated frames (MFG).\n\n'
                     '1 = 2X | 2 = 3X | 3 = 4X | 4 = 5X | 5 = 6X\n\n'
                     'auto defaults to 1.')}),

            ('UseGamesReflexMarkers', 'combo', BOOL3,
             {'zh': ('使用游戏的 Reflex 标记',
                     '沿用游戏自身的 Reflex 标记，而不是 OptiScaler 注入的。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use Game Reflex Markers',
                     "Uses the game's own Reflex markers instead of the ones "
                     'OptiScaler injects.\n\nauto defaults to true.')}),

            ('OverrideInterpolationCount', 'combo', BOOL3,
             {'zh': ('覆盖插帧倍率',
                     '覆盖游戏发给 Streamline 的插帧倍率值。\n\n'
                     '可能是 Nvngx FG，也可能是 noFG（当用户使用真实 DLSSG 时）。\n\n'
                     'auto 默认为不覆盖。'),
              'en': ('Override Interpolation Count',
                     "Overrides the value the game sends to Streamline.\n\n"
                     'Could be Nvngx FG, or noFG when someone uses real DLSSG.\n\n'
                     'auto means no override.')}),

            ('OverrideForceDMFG', 'combo', BOOL3,
             {'zh': ('强制动态多帧生成',
                     '尝试强制开启动态多帧生成 (DMFG)。\n\n'
                     '需要 NVIDIA Blackwell，或任意 AMD / Intel 显卡。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override Force DMFG',
                     'Tries to force dynamic multi frame generation.\n\n'
                     'Requires NVIDIA Blackwell, or any AMD / Intel GPU.\n\n'
                     'auto defaults to false.')}),

            ('FramerateTargetDMFG', 'slider', (0.0, 500.0, 1.0),
             {'zh': ('DMFG 目标帧率',
                     '动态多帧生成的目标帧率。\n\n'
                     '非零时无法再手动调整帧生成倍率。\n'
                     '需要先启用 DMFG。\n\nauto 默认为 0.0，即跟随显示器刷新率。'),
              'en': ('DMFG Framerate Target',
                     'FPS target for dynamic multi frame generation.\n\n'
                     'When non-zero you cannot adjust the FG multiplier manually.\n'
                     'Requires DMFG to be enabled.\n\n'
                     "auto defaults to 0.0 (the monitor's refresh rate).")}),

        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'optifg',
        'section': 'OptiFG',
        'name': {'zh': 'OptiScaler 帧生成 (OptiFG)', 'en': 'OptiFG'},
        'fields': [
            ('DisableHUDFix', 'combo', BOOL3,
             {'zh': ('关闭 HUDFix 资源追踪',
                     '完全停止 HUDFix 的资源追踪。\n\n'
                     '配合自己就能处理 HUD 元素的机器学习帧生成器使用时，'
                     '关掉追踪可以降低 CPU 开销。\n\nauto 默认为 false。'),
              'en': ('Disable HUDFix Tracking',
                     'Disables HUDFix resource tracking entirely.\n\n'
                     'Reduces CPU overhead when used with ML frame generators '
                     'that already handle HUD elements themselves.\n\n'
                     'auto defaults to false.')}),

            ('HUDFix', 'combo', BOOL3,
             {'zh': ('启用 HUDFix 无 HUD 画面追踪',
                     '为帧生成追踪不含 HUD 的画面（hudless），避免界面元素被插值糊掉。\n\n'
                     '可能导致崩溃，开启异步时尤其明显。\n\nauto 默认为 false。'),
              'en': ('HUDFix Hudless Tracking',
                     'Tracks the hudless image for frame generation so UI elements '
                     'do not get smeared by interpolation.\n\n'
                     'May cause crashes, especially with Async enabled.\n\n'
                     'auto defaults to false.')}),

            ('HUDLimit', 'slider', (1, 20, 1),
             {'zh': ('无 HUD 画面捕获延迟',
                     '延迟几帧再抓取 hudless 画面。\n\n'
                     '设得太高很可能崩溃。取值需大于 0。\n\nauto 默认为 1。'),
              'en': ('Hudless Capture Delay',
                     'Delays the hudless image capture by this many frames.\n\n'
                     'May cause crashes if set too high. Must be above 0.\n\n'
                     'auto defaults to 1.')}),

            ('HUDFixExtended', 'combo', BOOL3,
             {'zh': ('扩展 HUDFix 格式检查',
                     '对更多图像格式做 hudless 检查。\n\n'
                     '可能引起崩溃和卡顿。\n\nauto 默认为 false。'),
              'en': ('Extended HUDFix Checks',
                     'Runs hudless checks against more image formats.\n\n'
                     'Might cause crashes and slowdowns.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableRTV', 'combo', BOOL3,
             {'zh': ('不用 RTV 追踪资源',
                     '资源追踪时不使用 CreateRenderTargetView。\n\n'
                     '这一组 Hudfix Disable 开关用来逐个排除追踪入口，'
                     '排查某个游戏的 hudless 捕获问题时逐项试。\n\nauto 默认为 false。'),
              'en': ('Skip RTV for Tracking',
                     'Does not use CreateRenderTargetView for resource tracking.\n\n'
                     'This group of Hudfix Disable switches lets you rule out '
                     'tracking entry points one at a time when debugging hudless '
                     'capture in a specific game.\n\nauto defaults to false.')}),

            ('HudfixDisableSRV', 'combo', BOOL3,
             {'zh': ('不用 SRV 追踪资源',
                     '资源追踪时不使用 CreateShaderResourceView。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip SRV for Tracking',
                     'Does not use CreateShaderResourceView for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableUAV', 'combo', BOOL3,
             {'zh': ('不用 UAV 追踪资源',
                     '资源追踪时不使用 CreateUnorderedAccessView。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip UAV for Tracking',
                     'Does not use CreateUnorderedAccessView for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableOM', 'combo', BOOL3,
             {'zh': ('不用 OMSetRenderTargets 追踪资源',
                     '资源追踪时不使用 OMSetRenderTargets。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip OMSetRenderTargets',
                     'Does not use OMSetRenderTargets for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableSCR', 'combo', BOOL3,
             {'zh': ('不用计算根描述符表追踪资源',
                     '资源追踪时不使用 SetComputeRootDescriptorTable。\n\n'
                     'auto 默认为 true（即默认就不用）。'),
              'en': ('Skip SetComputeRootDescriptorTable',
                     'Does not use SetComputeRootDescriptorTable for resource '
                     'tracking.\n\nauto defaults to true (skipped by default).')}),

            ('HudfixDisableSGR', 'combo', BOOL3,
             {'zh': ('不用图形根描述符表追踪资源',
                     '资源追踪时不使用 SetGraphicsRootDescriptorTable。\n\n'
                     'auto 默认为 true（即默认就不用）。'),
              'en': ('Skip SetGraphicsRootDescriptorTable',
                     'Does not use SetGraphicsRootDescriptorTable for resource '
                     'tracking.\n\nauto defaults to true (skipped by default).')}),

            ('HudfixDisableDI', 'combo', BOOL3,
             {'zh': ('不用 DrawInstanced 追踪资源',
                     '资源追踪时不使用 DrawInstanced。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip DrawInstanced',
                     'Does not use DrawInstanced for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableDII', 'combo', BOOL3,
             {'zh': ('不用 DrawIndexedInstanced 追踪资源',
                     '资源追踪时不使用 DrawIndexedInstanced。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip DrawIndexedInstanced',
                     'Does not use DrawIndexedInstanced for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HudfixDisableDispatch', 'combo', BOOL3,
             {'zh': ('不用 Dispatch 追踪资源',
                     '资源追踪时不使用 Dispatch。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip Dispatch',
                     'Does not use Dispatch for resource tracking.\n\n'
                     'auto defaults to false.')}),

            ('HUDFixDontUseSwapchainBuffers', 'combo', BOOL3,
             {'zh': ('禁止把交换链缓冲当作 hudless',
                     '不允许交换链缓冲被当成 hudless 画面使用。\n\n'
                     '有助于修复叠加层（如 Steam 覆盖层）异常，但可能降低兼容性。\n\n'
                     'auto 默认为 false。'),
              'en': ('Block Swapchain Buffers as Hudless',
                     'Prevents swapchain buffers from being used as the hudless '
                     'image.\n\nMight fix overlay issues but can also reduce '
                     'compatibility.\n\nauto defaults to false.')}),

            ('HUDFixRelaxedResolutionCheck', 'combo', BOOL3,
             {'zh': ('放宽 hudless 分辨率检查',
                     '把 hudless 的分辨率匹配容差放宽 32 像素。\n\n'
                     '对某些分辨率和画面比例下加黑边的游戏有用（例如巫师 3）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Relaxed Hudless Resolution Check',
                     'Relaxes the hudless resolution match by 32 pixels.\n\n'
                     'Helps games that add black borders at certain resolutions '
                     'and aspect ratios (e.g. Witcher 3).\n\n'
                     'auto defaults to false.')}),

            ('HUDFixImmediate', 'combo', BOOL3,
             {'zh': ('着色器执行前捕获',
                     '在着色器执行之前就抓取资源。\n\n'
                     '提高 hudless 捕获成功率，但可能抓到不需要的资源。\n\n'
                     'auto 默认为 false。'),
              'en': ('Capture Before Shader Execution',
                     'Captures resources before shader execution.\n\n'
                     'Increases hudless capture chances but might capture '
                     'unnecessary resources.\n\nauto defaults to false.')}),

            ('AlwaysTrackHeaps', 'combo', BOOL3,
             {'zh': ('始终追踪描述符堆',
                     '无视 Hudfix 的设置，始终启用资源追踪。\n\n'
                     '可能带来性能损失，但关掉又可能影响稳定性。\n\n'
                     'auto 默认为 false。'),
              'en': ('Always Track Heaps',
                     'Keeps resource tracking on regardless of the Hudfix setting.\n\n'
                     'Might cost performance, but disabling it can hurt stability.\n\n'
                     'auto defaults to false.')}),

            ('UseShards', 'combo', BOOL3,
             {'zh': ('分片锁追踪资源',
                     '用分片（shard）替代单个互斥锁来追踪资源。\n\n'
                     '在多线程压力大的游戏里可能提升性能。\n\nauto 默认为 false。'),
              'en': ('Use Sharded Locks',
                     'Uses shards instead of a single mutex for resource tracking.\n\n'
                     'Might improve performance in heavily multithreaded games.\n\n'
                     'auto defaults to false.')}),

            ('ResourceBlocking', 'combo', BOOL3,
             {'zh': ('屏蔽低频资源',
                     '禁止很少被用到的资源充当 hudless，减少闪烁之类的问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('Resource Blocking',
                     'Blocks rarely used resources from acting as hudless, '
                     'reducing flicker and similar issues.\n\n'
                     'auto defaults to false.')}),

            ('MakeDepthCopy', 'combo', BOOL3,
             {'zh': ('复制深度缓冲',
                     '给 Hudfix 的帧生成调用单独复制一份深度缓冲。\n\n'
                     '关掉基本上会偶发画面错乱。\n\nauto 默认为 true。'),
              'en': ('Copy Depth Buffer',
                     'Makes a copy of depth for the Hudfix frame generation call.\n\n'
                     'Setting it false will most likely cause occasional garbling.\n\n'
                     'auto defaults to true.')}),

            ('EnableDepthScale', 'combo', BOOL3,
             {'zh': ('启用深度缩放',
                     '按下面的深度缩放上限对深度缓冲做缩放。\n\n'
                     '用于修复虚幻引擎搭配 DLSS-D 时深度信息异常的问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Depth Scale',
                     'Scales the depth buffer according to the Depth Scale Max value.\n\n'
                     'Fixes broken depth buffer info for DLSS-D with Unreal Engine.\n\n'
                     'auto defaults to false.')}),

            ('DepthScaleMax', 'slider', (1.0, 100000.0, 100.0),
             {'zh': ('深度缩放上限',
                     '深度缩放启用后，深度值会被这个数除。\n\n'
                     'auto 默认为 10000.0。'),
              'en': ('Depth Scale Max',
                     'Depth values get divided by this number when Depth Scale '
                     'is enabled.\n\nauto defaults to 10000.0.')}),

            ('MakeMVCopy', 'combo', BOOL3,
             {'zh': ('复制运动矢量',
                     '给 Hudfix 的帧生成调用单独复制一份运动矢量。\n\n'
                     '关掉基本上会偶发画面错乱。\n\nauto 默认为 true。'),
              'en': ('Copy Motion Vectors',
                     'Makes a copy of motion vectors for the Hudfix frame '
                     'generation call.\n\nSetting it false will most likely cause '
                     'occasional garbling.\n\nauto defaults to true.')}),

            ('ResourceFlip', 'combo', BOOL3,
             {'zh': ('翻转深度与速度贴图',
                     '上下翻转深度和速度（运动矢量）贴图。\n\n'
                     '通常用于修复 Unity 游戏的 OptiFG 问题。\n\nauto 默认为 false。'),
              'en': ('Flip Depth & Velocity',
                     'Flips the depth and velocity (motion vector) textures.\n\n'
                     'Should fix OptiFG issues with Unity games.\n\n'
                     'auto defaults to false.')}),

            ('ResourceFlipOffset', 'combo', BOOL3,
             {'zh': ('按插值区域翻转',
                     '按照插值区域来翻转深度和速度贴图。\n\n'
                     '同样用于修复 Unity 游戏的 OptiFG 问题。\n\nauto 默认为 false。'),
              'en': ('Flip by Interpolation Area',
                     'Flips the depth and velocity textures according to the '
                     'interpolation area.\n\nAlso targets OptiFG issues with Unity '
                     'games.\n\nauto defaults to false.')}),

            ('AlwaysCaptureFSRFGSwapchain', 'combo', BOOL3,
             {'zh': ('始终接管 FSR-FG 交换链',
                     '总是捕获 FSR-FG 3.1 的交换链，并替换为所选输出。\n\n'
                     '用于修复"游戏创建了 FSR-FG 交换链但没实际使用"'
                     '导致的重复呈现问题（例如《寂静岭 f》）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Always Capture FSR-FG Swapchain',
                     'Always captures the FSR-FG 3.1 swapchain and swaps it with '
                     'the selected output.\n\nFixes the double present call issue '
                     'in games that create but never use the FSR-FG swapchain '
                     '(e.g. Silent Hill f).\n\nauto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'inputs',
        'section': 'Inputs',
        'name': {'zh': '输入接管 (Inputs)', 'en': 'Inputs'},
        'fields': [
            ('EnableDlssInputs', 'combo', BOOL3,
             {'zh': ('挂钩 DLSS 输入',
                     '挂钩 nvngx.dll 并接收 DLSS 的输入数据。\n\n'
                     '本模块决定 OptiScaler 从哪些上采样接口拿数据，'
                     '通常保持 auto 即可，排查兼容问题时才逐项关闭。\n\n'
                     'auto 默认为 true。'),
              'en': ('Hook DLSS Inputs',
                     'Hooks nvngx.dll and consumes DLSS inputs.\n\n'
                     'This module decides which upscaling interfaces OptiScaler '
                     'takes data from. Leave on auto unless you are chasing a '
                     'compatibility problem.\n\nauto defaults to true.')}),

            ('EnableXeSSInputs', 'combo', BOOL3,
             {'zh': ('挂钩 XeSS 输入',
                     '挂钩 libxess.dll 并接收 XeSS 的输入数据。\n\n'
                     'auto 默认为 true。'),
              'en': ('Hook XeSS Inputs',
                     'Hooks libxess.dll and consumes XeSS inputs.\n\n'
                     'auto defaults to true.')}),

            ('EnableFsr2Inputs', 'combo', BOOL3,
             {'zh': ('挂钩 FSR2 输入',
                     '挂钩 FSR2 的输入接口。\n\nauto 默认为 true。'),
              'en': ('Hook FSR2 Inputs',
                     'Hooks the FSR2 input interface.\n\nauto defaults to true.')}),

            ('UseFsr2Dx11Inputs', 'combo', BOOL3,
             {'zh': ('用 FSR2 的 DX11 输入',
                     '挂钩 FSR2 的 DX11 接口，而不是 DX12 那个。\n\n'
                     'auto 默认为 false。'),
              'en': ('Use FSR2 DX11 Inputs',
                     'Hooks the FSR2 DX11 interface instead of the DX12 one.\n\n'
                     'auto defaults to false.')}),

            ('UseFsr2VulkanInputs', 'combo', BOOL3,
             {'zh': ('用 FSR2 的 Vulkan 输入',
                     '挂钩 FSR2 的 Vulkan 接口，而不是 DX11 / DX12 那两个。\n\n'
                     'auto 默认为 false。'),
              'en': ('Use FSR2 Vulkan Inputs',
                     'Hooks the FSR2 Vulkan interface instead of the DX11 and '
                     'DX12 ones.\n\nauto defaults to false.')}),

            ('UseFsr2Inputs', 'combo', BOOL3,
             {'zh': ('启用 FSR2 输入',
                     '实际使用已挂钩到的 FSR2 输入数据。\n\n'
                     '与"挂钩 FSR2 输入"的区别：挂钩只是接上，这一项才决定是否真的用。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use FSR2 Inputs',
                     'Actually consumes the hooked FSR2 inputs.\n\n'
                     'Hooking only attaches the interface; this switch decides '
                     'whether the data gets used.\n\nauto defaults to true.')}),

            ('Fsr2Pattern', 'combo', BOOL3,
             {'zh': ('特征码搜索 FSR2 函数',
                     '用特征码匹配的方式查找 FSR2 的函数。\n\n'
                     '会拖慢游戏启动速度。\n\nauto 默认为 false。'),
              'en': ('FSR2 Pattern Matching',
                     'Finds FSR2 methods through pattern matching.\n\n'
                     'Slows down game loading.\n\nauto defaults to false.')}),

            ('EnableFsr3Inputs', 'combo', BOOL3,
             {'zh': ('挂钩 FSR3 输入',
                     '挂钩 FSR3 的输入接口。\n\nauto 默认为 true。'),
              'en': ('Hook FSR3 Inputs',
                     'Hooks the FSR3 input interface.\n\nauto defaults to true.')}),

            ('UseFsr3Inputs', 'combo', BOOL3,
             {'zh': ('启用 FSR3 输入',
                     '实际使用已挂钩到的 FSR3 输入数据。\n\nauto 默认为 true。'),
              'en': ('Use FSR3 Inputs',
                     'Actually consumes the hooked FSR3 inputs.\n\n'
                     'auto defaults to true.')}),

            ('Fsr3Pattern', 'combo', BOOL3,
             {'zh': ('特征码搜索 FSR3 函数',
                     '用特征码匹配的方式查找 FSR3 的函数。\n\n'
                     '会拖慢游戏启动速度。\n\nauto 默认为 false。'),
              'en': ('FSR3 Pattern Matching',
                     'Finds FSR3 methods through pattern matching.\n\n'
                     'Slows down game loading.\n\nauto defaults to false.')}),

            ('EnableFfxInputs', 'combo', BOOL3,
             {'zh': ('挂钩 FidelityFX 输入',
                     '挂钩 FidelityFX API（amd_fidelityfx_dx12.dll）的输入。\n\n'
                     'auto 默认为 true。'),
              'en': ('Hook FidelityFX Inputs',
                     'Hooks the FidelityFX API (amd_fidelityfx_dx12.dll) inputs.\n\n'
                     'auto defaults to true.')}),

            ('UseFfxInputs', 'combo', BOOL3,
             {'zh': ('启用 FidelityFX 输入',
                     '实际使用已挂钩到的 FidelityFX API 输入数据。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use FidelityFX Inputs',
                     'Actually consumes the hooked FidelityFX API inputs.\n\n'
                     'auto defaults to true.')}),

            ('EnableHotSwapping', 'combo', BOOL3,
             {'zh': ('允许热切换上采样器',
                     '允许在游戏自带的 FSR3.1 上采样器与 OptiScaler 所选上采样器之间热切换。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Hot Swapping',
                     'Allows hot swapping between the game\'s own FSR3.1 upscaler '
                     'and the upscaler selected in OptiScaler.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'fsrfginputs',
        'section': 'FSRFGInputs',
        'name': {'zh': 'FSR-FG 输入 (FSRFGInputs)', 'en': 'FSR-FG Inputs'},
        'fields': [
            ('SkipConfigForHudless', 'combo', BOOL3,
             {'zh': ('忽略 ffxConfig 的 hudless',
                     '不使用 ffxConfig 里设定的 hudless 资源。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip ffxConfig Hudless',
                     'Ignores the hudless resource set through ffxConfig.\n\n'
                     'auto defaults to false.')}),

            ('SkipDispatchForHudless', 'combo', BOOL3,
             {'zh': ('忽略 ffxDispatch 的 hudless',
                     '不使用 ffxDispatch 里设定的 hudless 资源。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip ffxDispatch Hudless',
                     'Ignores the hudless resource set through ffxDispatch.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'framerate',
        'section': 'Framerate',
        'name': {'zh': '帧率限制 (Framerate)', 'en': 'Framerate'},
        'fields': [
            ('FramerateLimit', 'slider', (0.0, 500.0, 1.0),
             {'zh': ('帧率上限',
                     '限制最高帧率，优先使用 Reflex 实现（如果有的话）。\n\n'
                     '包括所有能替代 Reflex 的技术，比如 XeLL 或 AntiLag 2。\n\n'
                     '设为 0.0 表示不限制。\n\nauto 默认为 0.0（关闭限制）。'),
              'en': ('Framerate Limit',
                     'Caps the maximum framerate using Reflex when available.\n\n'
                     'Includes any tech that replaces Reflex, such as XeLL or '
                     'AntiLag 2.\n\nSet to 0.0 to disable.\n\n'
                     'auto defaults to 0.0 (disabled).')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'xess',
        'section': 'XeSS',
        'name': {'zh': 'XeSS 设置', 'en': 'XeSS'},
        'fields': [
            ('BuildPipelines', 'combo', BOOL3,
             {'zh': ('预先构建管线',
                     '在 XeSS 初始化之前就构建好管线对象。\n\n'
                     'auto 默认为 true。'),
              'en': ('Build Pipelines Early',
                     'Builds the XeSS pipeline objects before initialization.\n\n'
                     'auto defaults to true.')}),

            ('CreateHeaps', 'combo', BOOL3,
             {'zh': ('预先创建堆',
                     '在 XeSS 初始化之前就创建好堆对象。\n\n'
                     'auto 默认为 false。'),
              'en': ('Create Heaps Early',
                     'Creates the XeSS heap objects before initialization.\n\n'
                     'auto defaults to false.')}),

            ('NetworkModel', 'combo', ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('神经网络模型',
                     '选择 XeSS 使用的神经网络模型。\n\n'
                     '0 = KPSS | 1 = Splat | 2-5 = Model 3-6\n\n'
                     '目前这个选项似乎不起作用。\n\nauto 默认为 0。'),
              'en': ('Network Model',
                     'Selects the XeSS neural network model.\n\n'
                     '0 = KPSS | 1 = Splat | 2-5 = Model 3-6\n\n'
                     'Currently does not seem to do anything.\n\n'
                     'auto defaults to 0.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'fsr',
        'section': 'FSR',
        'name': {'zh': 'FSR 设置', 'en': 'FSR'},
        'fields': [
            ('VerticalFov', 'slider', (0.0, 180.0, 1.0),
             {'zh': ('垂直视场角',
                     '摄像机垂直 FOV，供 FSR 和 FSR 帧生成使用。\n\n'
                     '取值范围 0.0 到 180.0 度。\n\nauto 默认为 60.0。'),
              'en': ('Vertical FOV',
                     'Camera vertical field of view for FSR and FSR frame generation.\n\n'
                     'Range 0.0 to 180.0 degrees.\n\nauto defaults to 60.0.')}),

            ('HorizontalFov', 'slider', (0.0, 180.0, 1.0),
             {'zh': ('水平视场角',
                     '当垂直 FOV 未定义时，用这个来反算垂直 FOV。\n\n'
                     '取值范围 0.0 到 180.0 度。\n\nauto 默认为关闭。'),
              'en': ('Horizontal FOV',
                     'Used to calculate vertical FOV when it is not defined.\n\n'
                     'Range 0.0 to 180.0 degrees.\n\nauto defaults to off.')}),

            ('CameraNear', 'slider', (0.0, 1000.0, 0.01),
             {'zh': ('摄像机近平面',
                     '摄像机近裁剪面距离，供 FSR 和 FSR 帧生成使用。\n\n'
                     'auto 默认为 0.1。'),
              'en': ('Camera Near Plane',
                     'Camera near clipping plane distance for FSR and FSR frame '
                     'generation.\n\nauto defaults to 0.1.')}),

            ('CameraFar', 'slider', (0.0, 100000.0, 100.0),
             {'zh': ('摄像机远平面',
                     '摄像机远裁剪面距离，供 FSR 和 FSR 帧生成使用。\n\n'
                     'auto 默认为 10000.0。'),
              'en': ('Camera Far Plane',
                     'Camera far clipping plane distance for FSR and FSR frame '
                     'generation.\n\nauto defaults to 10000.0.')}),

            ('UseFsrInputValues', 'combo', BOOL3,
             {'zh': ('使用 FSR 输入的摄像机值',
                     '启用从 FSR2/3 或 FFX 输入接口接收到的摄像机参数（近/远/FOV）。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use FSR Input Camera Values',
                     'Uses camera values (near, far, FOV) received from the '
                     'FSR2/3 or FFX input interfaces.\n\nauto defaults to true.')}),

            ('DebugView', 'combo', BOOL3,
             {'zh': ('调试视图',
                     '启用 FSR3.X 上采样器的调试视图。\n\nauto 默认为 false。'),
              'en': ('Debug View',
                     'Enables debug view for the FSR3.X upscaler.\n\n'
                     'auto defaults to false.')}),

            ('UpscalerIndex', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('上采样器后端',
                     '选择 FSR3.X/4 使用的上采样器后端版本。\n\n'
                     '0 = FSR 4.0.2 | 1 = FSR 3.1.5 | 2 = FSR 2.3.4\n\n'
                     'auto 会根据 GPU 自动选择：RDNA4 用 FSR4，其他用 FSR3。'),
              'en': ('Upscaler Backend',
                     'Selects the upscaler backend for FSR3.X/4.\n\n'
                     '0 = FSR 4.0.2 | 1 = FSR 3.1.5 | 2 = FSR 2.3.4\n\n'
                     'auto picks FSR4 for RDNA4, FSR3 for everything else.')}),

            ('FGIndex', 'combo', ['auto', '0', '1'],
             {'zh': ('帧生成后端',
                     '选择 FFX 使用的帧生成后端版本。\n\n'
                     '0 = FSR 4.0.0 | 1 = FSR 3.1.6\n\n'
                     'auto 会根据 GPU 自动选择：RDNA4 用 FSR4，其他用 FSR3。'),
              'en': ('Frame Generation Backend',
                     'Selects the frame generation backend for FFX.\n\n'
                     '0 = FSR 4.0.0 | 1 = FSR 3.1.6\n\n'
                     'auto picks FSR4 for RDNA4, FSR3 for everything else.')}),

            ('VelocityFactor', 'slider', (0.0, 1.0, 0.01),
             {'zh': ('速度因子',
                     'FSR3.1.1 及更高版本的速度因子。\n\n'
                     '设为 0.0 可以提升明亮像素的时域稳定性。\n\nauto 默认为 1.0。'),
              'en': ('Velocity Factor',
                     'Velocity factor for FSR3.1.1 and above.\n\n'
                     'A value of 0.0 can improve temporal stability of bright pixels.\n\n'
                     'auto defaults to 1.0.')}),

            ('ReactiveScale', 'slider', (0.0, 10.0, 0.1),
             {'zh': ('响应性缩放',
                     'FSR3.1.4 及更高版本的响应性缩放。\n\n'
                     '开发用途，测试向响应性遮罩写入更大值能否减少重影。\n\n'
                     'auto 默认为 1.0。'),
              'en': ('Reactive Scale',
                     'Reactive scale for FSR3.1.4 and above.\n\n'
                     'For development: tests if writing a larger value to the '
                     'reactive mask reduces ghosting.\n\nauto defaults to 1.0.')}),

            ('ShadingScale', 'slider', (0.0, 10.0, 0.1),
             {'zh': ('明暗变化缩放',
                     'FSR3.1.4 及更高版本的明暗变化缩放。\n\n'
                     '提高这个值会放大 FSR3.1 计算的明暗变化值，增强响应性。\n\n'
                     'auto 默认为 1.0。'),
              'en': ('Shading Scale',
                     'Shading scale for FSR3.1.4 and above.\n\n'
                     'Increasing this scales the FSR3.1 computed shading change '
                     'value to have higher reactiveness.\n\nauto defaults to 1.0.')}),

            ('AccAddPerFrame', 'slider', (0.0, 1.0, 0.001),
             {'zh': ('每帧累积增量',
                     'FSR3.1.4 及更高版本每帧添加的累积量。\n\n'
                     '对应遮挡解除或响应性遮罩 > 0.0 时每帧添加的累积量。\n'
                     '降低此值并把重影物体（无运动矢量）以接近 1.0 的值写入响应性遮罩，'
                     '可以减少时域重影，但可能导致细节像素闪烁。\n\n'
                     'auto 默认为 0.333。'),
              'en': ('Accumulation Per Frame',
                     'Accumulation added per frame for FSR3.1.4 and above.\n\n'
                     'Corresponds to the amount added per frame at pixel coordinates '
                     'where disocclusion occurred or when reactive mask value > 0.0.\n'
                     'Decreasing this and drawing the ghosting object (no MV) to the '
                     'reactive mask with value close to 1.0 can decrease temporal '
                     'ghosting, but may cause thin feature pixels to flicker.\n\n'
                     'auto defaults to 0.333.')}),

            ('MinDisOccAcc', 'slider', (-1.0, 1.0, 0.001),
             {'zh': ('最小遮挡解除累积',
                     'FSR3.1.4 及更高版本的最小遮挡解除累积量。\n\n'
                     '提高此值可以减少摇摆细物体周围的白像素时域闪烁。\n'
                     '太高会加重重影。足够负的值意味着在第 N 帧遮挡解除的像素，'
                     '从第 N+2 帧开始添加累积增量。\n\n'
                     'auto 默认为 -0.333。'),
              'en': ('Min Disocclusion Accumulation',
                     'Minimum disocclusion accumulation for FSR3.1.4 and above.\n\n'
                     'Increasing this may reduce white pixel temporal flickering '
                     'around swaying thin objects that are disoccluding one another '
                     'often. Too high increases ghosting. A sufficiently negative '
                     'value means for a pixel disoccluded at frame N, add the per-frame '
                     'accumulation starting at frame N+2.\n\nauto defaults to -0.333.')}),

            ('UseReactiveMaskForTransparency', 'combo', BOOL3,
             {'zh': ('响应性遮罩用作透明遮罩',
                     '把原始 DLSS 响应性遮罩当作透明遮罩使用。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use Reactive Mask as Transparency',
                     'Uses the raw DLSS reactive mask as a transparency mask.\n\n'
                     'auto defaults to true.')}),

            ('DlssReactiveMaskBias', 'slider', (0.0, 0.9, 0.01),
             {'zh': ('DLSS 响应性遮罩偏移',
                     '与 FSR 配合使用 DLSS 响应性遮罩时的偏移量。\n\n'
                     '值越高，新帧的权重越大。\n\nauto 默认为 0.45。'),
              'en': ('DLSS Reactive Mask Bias',
                     'Bias to apply to the DLSS reactive mask when using it with FSR.\n\n'
                     'Higher values bias toward the new frame.\n\n'
                     'auto defaults to 0.45.')}),

            ('Fsr4ForceModel', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('强制 FSR4 模型',
                     '强制 Opti 使用特定的 FSR 4 模型。\n\n'
                     '0 = 不覆盖 | 1 = FP8 | 2 = INT8\n\n'
                     '在不支持的 GPU 上强制 FP8 没用，驱动会回退到 FSR 3。\n\n'
                     'auto 默认为 0。'),
              'en': ('Force FSR4 Model',
                     'Forces Opti to use a specific FSR 4 model.\n\n'
                     '0 = No override | 1 = FP8 | 2 = INT8\n\n'
                     'Forcing FP8 on unsupported GPUs does nothing; the driver '
                     'falls back to FSR 3.\n\nauto defaults to 0.')}),

            ('Fsr4Preset', 'combo', ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('FSR4 内部预设',
                     '选择 FSR4 内部使用的质量预设。\n\n'
                     '0 = Native AA | 1 = Ultra Quality/Quality | 2 = Balanced\n'
                     '3 = Performance | 4 = DRS | 5 = Ultra Performance\n\n'
                     'auto 默认为游戏自己的设定。'),
              'en': ('FSR4 Internal Preset',
                     'Selects the internal FSR4 quality preset.\n\n'
                     '0 = Native AA | 1 = Ultra Quality/Quality | 2 = Balanced\n'
                     '3 = Performance | 4 = DRS | 5 = Ultra Performance\n\n'
                     'auto defaults to the game\'s default.')}),

            ('Fsr4EnableWatermark', 'combo', BOOL3,
             {'zh': ('启用 FSR4 水印',
                     '启用 FSR4 的水印显示。\n\nauto 默认为 false。'),
              'en': ('Enable FSR4 Watermark',
                     'Enables the FSR4 watermark.\n\nauto defaults to false.')}),

            ('Fsr4DoNotLoadAmdxc64', 'combo', BOOL3,
             {'zh': ('不加载 amdxc64.dll',
                     '阻止 OptiScaler 加载 amdxc64.dll。\n\nauto 默认为 false。'),
              'en': ('Do Not Load amdxc64.dll',
                     'Prevents OptiScaler from loading amdxc64.dll.\n\n'
                     'auto defaults to false.')}),

            ('Fsr4Provider', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('FSR4 提供者',
                     '0 = 自动\n1 = SDK\n2 = 驱动\n\nauto 默认为 0。'),
              'en': ('FSR4 Provider',
                     '0 = Auto\n1 = SDK\n2 = Driver\n\nauto defaults to 0.')}),

            ('Fsr4Amdxcffx64Path', 'entry', None,
             {'zh': ('amdxcffx64.dll 自定义路径',
                     '留空时使用游戏目录和驱动存储中的默认搜索路径。'),
              'en': ('Custom amdxcffx64.dll Path',
                     'Leave empty to use the default game-directory and driver-store search paths.')}),

            ('FsrNonLinearColorSpace', 'combo', BOOL3,
             {'zh': ('输入色彩空间非线性',
                     '指示输入颜色资源使用非线性色彩空间。\n\n'
                     '可能提升 FSR4 画质。\n\nauto 默认为 false。'),
              'en': ('Non-Linear Color Space',
                     'Indicates the input color resource uses a non-linear color '
                     'space.\n\nMight improve FSR4 image quality.\n\n'
                     'auto defaults to false.')}),

            ('FsrNonLinearSRGB', 'combo', BOOL3,
             {'zh': ('输入为感知 sRGB',
                     '指示输入颜色资源包含感知 sRGB 颜色。\n\n'
                     '可能提升 FSR4 画质。\n\nauto 默认为 false。'),
              'en': ('Non-Linear sRGB',
                     'Indicates the input color resource contains perceptual sRGB '
                     'colors.\n\nMight improve FSR4 image quality.\n\n'
                     'auto defaults to false.')}),

            ('FsrNonLinearPQ', 'combo', BOOL3,
             {'zh': ('输入为感知 PQ',
                     '指示输入颜色资源包含感知 PQ 颜色。\n\n'
                     '可能提升 FSR4 画质。\n\nauto 默认为 false。'),
              'en': ('Non-Linear PQ',
                     'Indicates the input color resource contains perceptual PQ '
                     'colors.\n\nMight improve FSR4 image quality.\n\n'
                     'auto defaults to false.')}),

            ('FsrAgilitySDKUpgrade', 'combo', BOOL3,
             {'zh': ('升级 DirectX 12 Agility SDK',
                     '更新 DirectX 12 Agility SDK，在 Windows 10 老游戏里启用 FSR4。\n\n'
                     '例如《赛博朋克 2077》。\n\n'
                     '你必须把 D3D12_OptiScaler 文件夹复制到游戏 exe 旁边！\n\n'
                     'auto 默认为 false。'),
              'en': ('Upgrade DX12 Agility SDK',
                     'Updates the DirectX 12 Agility SDK, enabling FSR4 on '
                     'Windows 10 in older titles.\n\nFor example, Cyberpunk 2077.\n\n'
                     'You MUST copy the D3D12_OptiScaler folder next to the '
                     'game\'s exe!\n\nauto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'dlss',
        'section': 'DLSS',
        'name': {'zh': 'DLSS 设置', 'en': 'DLSS'},
        'fields': [
            ('Enabled', 'combo', BOOL3,
             {'zh': ('启用原始 NVNGX',
                     '启用对原始 NVNGX（NVIDIA NGX）的调用。\n\n'
                     'auto 默认为 true。'),
              'en': ('Enable Original NVNGX',
                     'Enables calls to the original NVNGX (NVIDIA NGX).\n\n'
                     'auto defaults to true.')}),

            ('RenderPresetOverride', 'combo', BOOL3,
             {'zh': ('覆盖渲染预设',
                     '启用自定义渲染预设覆盖功能。\n\n'
                     '启用后下面的 RenderPreset* 设置才会生效。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override Render Presets',
                     'Enables custom render preset overrides.\n\n'
                     'When enabled, the RenderPreset* settings below will take effect.\n\n'
                     'auto defaults to false.')}),

            ('RenderPresetForAll', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('统一渲染预设',
                     '对所有质量档位应用同一个渲染预设。\n\n'
                     '0 = Default | 1 = A | 2 = B | ... | 15 = O\n\n'
                     '具体预设名称取决于 DLSS 版本。\n\nauto 默认为 0。'),
              'en': ('Render Preset for All',
                     'Applies the same render preset to all quality levels.\n\n'
                     '0 = Default | 1 = A | 2 = B | ... | 15 = O\n\n'
                     'Preset names depend on DLSS version.\n\nauto defaults to 0.')}),

            ('RenderPresetDLAA', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('DLAA 渲染预设',
                     'DLAA（原生分辨率抗锯齿）的渲染预设。\n\nauto 默认为 0。'),
              'en': ('DLAA Render Preset',
                     'Render preset for DLAA (native resolution anti-aliasing).\n\n'
                     'auto defaults to 0.')}),

            ('RenderPresetUltraQuality', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('超高质量渲染预设',
                     'Ultra Quality 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Ultra Quality Preset',
                     'Render preset for Ultra Quality mode.\n\nauto defaults to 0.')}),

            ('RenderPresetQuality', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('质量渲染预设',
                     'Quality 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Quality Preset',
                     'Render preset for Quality mode.\n\nauto defaults to 0.')}),

            ('RenderPresetBalanced', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('平衡渲染预设',
                     'Balanced 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Balanced Preset',
                     'Render preset for Balanced mode.\n\nauto defaults to 0.')}),

            ('RenderPresetPerformance', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('性能渲染预设',
                     'Performance 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Performance Preset',
                     'Render preset for Performance mode.\n\nauto defaults to 0.')}),

            ('RenderPresetUltraPerformance', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', '10', '11', '12', '13', '14', '15'],
             {'zh': ('超高性能渲染预设',
                     'Ultra Performance 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Ultra Performance Preset',
                     'Render preset for Ultra Performance mode.\n\nauto defaults to 0.')}),

            ('UseGenericAppIdWithDlss', 'combo', BOOL3,
             {'zh': ('使用通用 App ID',
                     '在 NGX 调用时使用通用的 App ID。\n\n'
                     '修复某些游戏中 OptiScaler 预设覆盖功能不生效的问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('Use Generic App ID',
                     'Uses a generic App ID with NGX.\n\n'
                     'Fixes OptiScaler preset override not working in certain games.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'dlssd',
        'section': 'DLSSD',
        'name': {'zh': 'DLSS-D 设置', 'en': 'DLSS-D'},
        'fields': [
            ('RenderPresetOverride', 'combo', BOOL3,
             {'zh': ('覆盖渲染预设',
                     '启用 DLSS-D 的自定义渲染预设覆盖功能。\n\n'
                     '启用后下面的 RenderPreset* 设置才会生效。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override Render Presets',
                     'Enables custom render preset overrides for DLSS-D.\n\n'
                     'When enabled, the RenderPreset* settings below will take effect.\n\n'
                     'auto defaults to false.')}),

            ('RenderPresetForAll', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('统一渲染预设',
                     '对 DLSS-D 所有质量档位应用同一个渲染预设。\n\n'
                     '0 = Default | 1 = A | 2 = B | 3 = C | 4 = D | 5 = E\n\n'
                     '具体预设名称取决于 DLSSD 版本。\n\nauto 默认为 0。'),
              'en': ('Render Preset for All',
                     'Applies the same render preset to all DLSS-D quality levels.\n\n'
                     '0 = Default | 1 = A | 2 = B | 3 = C | 4 = D | 5 = E\n\n'
                     'Preset names depend on DLSSD version.\n\nauto defaults to 0.')}),

            ('RenderPresetDLAA', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('DLAA 渲染预设',
                     'DLSS-D DLAA 的渲染预设。\n\nauto 默认为 0。'),
              'en': ('DLAA Render Preset',
                     'Render preset for DLSS-D DLAA.\n\nauto defaults to 0.')}),

            ('RenderPresetUltraQuality', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('超高质量渲染预设',
                     'DLSS-D Ultra Quality 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Ultra Quality Preset',
                     'Render preset for DLSS-D Ultra Quality mode.\n\n'
                     'auto defaults to 0.')}),

            ('RenderPresetQuality', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('质量渲染预设',
                     'DLSS-D Quality 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Quality Preset',
                     'Render preset for DLSS-D Quality mode.\n\nauto defaults to 0.')}),

            ('RenderPresetBalanced', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('平衡渲染预设',
                     'DLSS-D Balanced 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Balanced Preset',
                     'Render preset for DLSS-D Balanced mode.\n\nauto defaults to 0.')}),

            ('RenderPresetPerformance', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('性能渲染预设',
                     'DLSS-D Performance 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Performance Preset',
                     'Render preset for DLSS-D Performance mode.\n\n'
                     'auto defaults to 0.')}),

            ('RenderPresetUltraPerformance', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5'],
             {'zh': ('超高性能渲染预设',
                     'DLSS-D Ultra Performance 档位的渲染预设。\n\nauto 默认为 0。'),
              'en': ('Ultra Performance Preset',
                     'Render preset for DLSS-D Ultra Performance mode.\n\n'
                     'auto defaults to 0.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'nvngxfg',
        'section': 'NvngxFG',
        'name': {'zh': 'NVNGX 帧生成', 'en': 'NVNGX Frame Generation'},
        'fields': [
            ('MakeDepthCopy', 'combo', BOOL3,
             {'zh': ('复制深度缓冲',
                     '修复某些游戏（主要是非 UE 引擎）在 AMD GPU + Windows 下的画面错误。\n\n'
                     '可能导致卡顿，因此只在必要时启用（确认有画面问题时）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Make Depth Copy',
                     'Fixes broken visuals in some games (mostly non-UE) on '
                     'AMD GPUs under Windows.\n\n'
                     'Can cause stutters, so use only when necessary and mentioned.\n\n'
                     'auto defaults to false.')}),

            ('DispatchFlags', 'entry', None,
             {'zh': ('派发标志',
                     'NVNGX 帧生成派发标志，可填写十六进制组合值。\n\n'
                     'auto 默认为 0。'),
              'en': ('Dispatch Flags',
                     'NVNGX frame generation dispatch flags. Hexadecimal flag '
                     'combinations are accepted.\n\nauto defaults to 0.')}),

            ('ShowDebug', 'slider', (0, 16, 1),
             {'zh': ('调试显示',
                     '非零时显示 NVNGX 帧生成内部调试叠加层。\n\n'
                     'auto 默认为 0。'),
              'en': ('Show Debug',
                     'Shows the NVNGX frame generation debug overlay when non-zero.\n\n'
                     'auto defaults to 0.')}),

            ('DisableHudless', 'combo', BOOL3,
             {'zh': ('禁用 hudless',
                     '不把 hudless 资源传给 NVNGX 帧生成。\n\n'
                     'auto 默认为 false。'),
              'en': ('Disable Hudless',
                     'Stops the hudless resource from being sent to NVNGX frame '
                     'generation.\n\nauto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'libraries',
        'section': 'Libraries',
        'name': {'zh': '库文件路径 (Libraries)', 'en': 'Libraries'},
        'fields': [
            ('OptiDllPath', 'entry', None,
             {'zh': ('主 DLL 目录',
                     'OptiScaler 查找下面各个 dll 的主目录。\n\n'
                     '下面每一项都可以单独指定路径来覆盖这个默认目录，'
                     '留 auto 就用本项的目录。\n\n'
                     'auto 默认为 .\\OptiScaler'),
              'en': ('Main DLL Folder',
                     'Main folder OptiScaler searches for the dll files listed below.\n\n'
                     'Each entry below can override this folder individually; '
                     'leave them on auto to use this folder.\n\n'
                     'auto defaults to .\\OptiScaler')}),
            ('NvngxPath', 'entry', None,
             {'zh': ('nvngx.dll 路径',
                     '覆盖 nvngx.dll 或 _nvngx.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('nvngx.dll Path',
                     'Overrides the path for nvngx.dll or _nvngx.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('NvngxDlssPath', 'entry', None,
             {'zh': ('nvngx_dlss.dll 路径',
                     '覆盖 DLSS 超分主库 nvngx_dlss.dll 的查找路径。\n\n'
                     '想用某个特定 DLSS 版本时可以指到那个文件所在目录。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('nvngx_dlss.dll Path',
                     'Overrides the path for nvngx_dlss.dll (the DLSS upscaler library).\n\n'
                     'Point this at a folder if you want a specific DLSS version.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('NvapiPath', 'entry', None,
             {'zh': ('nvapi64.dll 路径',
                     '覆盖 nvapi64.dll 或 fakenvapi.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('nvapi64.dll Path',
                     'Overrides the path for nvapi64.dll or fakenvapi.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxDx12Path', 'entry', None,
             {'zh': ('FidelityFX DX12 路径',
                     '覆盖 amd_fidelityfx_dx12.dll 或 '
                     'amd_fidelityfx_loader_dx12.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX DX12 Path',
                     'Overrides the path for amd_fidelityfx_dx12.dll or '
                     'amd_fidelityfx_loader_dx12.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxDx12SRPath', 'entry', None,
             {'zh': ('FidelityFX 超分路径',
                     '覆盖 amd_fidelityfx_upscaler_dx12.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX Upscaler Path',
                     'Overrides the path for amd_fidelityfx_upscaler_dx12.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxDx12FGPath', 'entry', None,
             {'zh': ('FidelityFX 帧生成路径',
                     '覆盖 amd_fidelityfx_framegeneration_dx12.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX Frame Gen Path',
                     'Overrides the path for amd_fidelityfx_framegeneration_dx12.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxDx12RRPath', 'entry', None,
             {'zh': ('FidelityFX 降噪器路径',
                     '覆盖 amd_fidelityfx_denoiser_dx12.dll 的查找路径。\n\n'
                     '这是 FSR 光线重生成（Ray Regeneration）用的降噪库。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX Denoiser Path',
                     'Overrides the path for amd_fidelityfx_denoiser_dx12.dll.\n\n'
                     'This is the denoiser used by FSR Ray Regeneration.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxDx12RCPath', 'entry', None,
             {'zh': ('FidelityFX 辐射缓存路径',
                     '覆盖 amd_fidelityfx_radiancecache_dx12.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX Radiance Cache Path',
                     'Overrides the path for amd_fidelityfx_radiancecache_dx12.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('FfxVkPath', 'entry', None,
             {'zh': ('FidelityFX Vulkan 路径',
                     '覆盖 amd_fidelityfx_vk.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('FidelityFX Vulkan Path',
                     'Overrides the path for amd_fidelityfx_vk.dll.\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('XeSSPath', 'entry', None,
             {'zh': ('libxess.dll 路径',
                     '覆盖 XeSS 主库 libxess.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('libxess.dll Path',
                     'Overrides the path for libxess.dll (the XeSS library).\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('XeFGPath', 'entry', None,
             {'zh': ('libxess_fg.dll 路径',
                     '覆盖 XeSS 帧生成库 libxess_fg.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('libxess_fg.dll Path',
                     'Overrides the path for libxess_fg.dll (XeSS Frame Generation).\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('XeLLPath', 'entry', None,
             {'zh': ('libxell.dll 路径',
                     '覆盖 XeLL 低延迟库 libxell.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('libxell.dll Path',
                     'Overrides the path for libxell.dll (XeLL low latency).\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('XeSSDx11Path', 'entry', None,
             {'zh': ('libxess_dx11.dll 路径',
                     '覆盖 DX11 版 XeSS 库 libxess_dx11.dll 的查找路径。\n\n'
                     'auto 表示在主 DLL 目录里找。'),
              'en': ('libxess_dx11.dll Path',
                     'Overrides the path for libxess_dx11.dll (DX11 XeSS).\n\n'
                     'auto looks inside the main DLL folder.')}),
            ('NvngxFeaturePath', 'entry', None,
             {'zh': ('Nvngx 特性搜索路径',
                     '这个路径会在 Nvngx 初始化时追加到它的搜索路径里。\n\n'
                     '用于让 Nvngx 找到额外的特性 dll。\n\n'
                     'auto 表示不追加。'),
              'en': ('Nvngx Feature Path',
                     'This path is appended to Nvngx search paths during init.\n\n'
                     'Use it to let Nvngx find extra feature dlls.\n\n'
                     'auto adds nothing.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'spoofing',
        'section': 'Spoofing',
        'name': {'zh': '显卡伪装 (Spoofing)', 'en': 'Spoofing'},
        'fields': [
            ('SpoofedVendorId', 'entry', None,
             {'zh': ('伪装厂商 ID',
                     '向游戏报告的显卡厂商 ID（十六进制）。\n\n'
                     '0x10de = NVIDIA｜0x8086 = Intel｜0x1002 = AMD\n\n'
                     'auto 默认为 0x10de（NVIDIA）。'),
              'en': ('Spoofed Vendor ID',
                     'GPU vendor ID reported to the game (hex).\n\n'
                     '0x10de = NVIDIA | 0x8086 = Intel | 0x1002 = AMD\n\n'
                     'auto defaults to 0x10de (NVIDIA).')}),
            ('SpoofedDeviceId', 'entry', None,
             {'zh': ('伪装设备 ID',
                     '向游戏报告的显卡型号 ID（十六进制）。\n\n'
                     '0x2684 = RTX 4090｜0xE20B = Arc B580｜0x7550 = RX 9070 XT\n\n'
                     'auto 默认为 0x2684（RTX 4090）。'),
              'en': ('Spoofed Device ID',
                     'GPU device ID reported to the game (hex).\n\n'
                     '0x2684 = RTX 4090 | 0xE20B = Arc B580 | 0x7550 = RX 9070 XT\n\n'
                     'auto defaults to 0x2684 (RTX 4090).')}),
            ('TargetVendorId', 'entry', None,
             {'zh': ('目标厂商 ID',
                     '只对这个厂商的显卡启用伪装。\n\n'
                     '多显卡系统里用来限定伪装范围。\n\n'
                     'auto 表示对所有显卡生效。'),
              'en': ('Target Vendor ID',
                     'Only spoof GPUs matching this vendor ID.\n\n'
                     'Useful on multi-GPU systems to limit the scope.\n\n'
                     'auto applies to all.')}),
            ('TargetDeviceId', 'entry', None,
             {'zh': ('目标设备 ID',
                     '只对这个型号的显卡启用伪装。\n\n'
                     'auto 表示对所有显卡生效。'),
              'en': ('Target Device ID',
                     'Only spoof GPUs matching this device ID.\n\n'
                     'auto applies to all.')}),
            ('SpoofedGPUName', 'entry', None,
             {'zh': ('伪装显卡名称',
                     '向游戏报告的显卡名称字符串。\n\n'
                     '例：NVIDIA GeForce RTX 4090｜Intel(R) Arc(TM) B580 Graphics｜'
                     'AMD Radeon RX 9070 XT\n\n'
                     'auto 默认为 NVIDIA GeForce RTX 4090。'),
              'en': ('Spoofed GPU Name',
                     'GPU name string reported to the game.\n\n'
                     'e.g. NVIDIA GeForce RTX 4090 | Intel(R) Arc(TM) B580 Graphics | '
                     'AMD Radeon RX 9070 XT\n\n'
                     'auto defaults to NVIDIA GeForce RTX 4090.')}),
            ('StreamlineSpoofing', 'combo', BOOL3,
             {'zh': ('Streamline 伪装',
                     '只对 Streamline 启用显卡伪装，游戏其余部分不受影响。\n\n'
                     '这样通常能用上 fakenvapi（有时还有 DLSS/DLSSG），'
                     '又不用把整个游戏都骗过去，副作用更小。\n\n'
                     'auto 默认为 true。'),
              'en': ('Streamline Spoofing',
                     'Enables GPU spoofing for Streamline only, even when DXGI '
                     'spoofing is off for the rest of the game.\n\n'
                     'Usually enough to get fakenvapi (and sometimes DLSS/DLSSG) '
                     'working without spoofing the whole game.\n\n'
                     'auto defaults to true.')}),
            ('Dxgi', 'combo', BOOL3,
             {'zh': ('DXGI 伪装',
                     '在 DXGI 层把显卡伪装成 NVIDIA（厂商可在上面改）。\n\n'
                     '这是最全面的伪装方式，也最容易和别的 mod 打架。\n\n'
                     'auto 在 AMD/Intel 上默认为 true，NVIDIA 上为 false。'),
              'en': ('DXGI Spoofing',
                     'Spoofs the GPU as NVIDIA at the DXGI level (vendor '
                     'configurable above).\n\n'
                     'This is the broadest form of spoofing and the most likely '
                     'to conflict with other mods.\n\n'
                     'auto defaults to true on AMD/Intel, false on NVIDIA.')}),
            ('DxgiFactoryWrapping', 'combo', BOOL3,
             {'zh': ('包装 DxgiFactory',
                     '用包装 DxgiFactory 的方式代替挂钩。\n\n'
                     '在 NVIDIA 显卡上配合其他 mod 时可能有帮助，'
                     '但也可能引入新问题。\n\n'
                     'auto 默认为 false。'),
              'en': ('DxgiFactory Wrapping',
                     'Wraps DxgiFactory instead of hooking it.\n\n'
                     'May help NVIDIA cards coexist with other mods, but can '
                     'also cause issues.\n\n'
                     'auto defaults to false.')}),
            ('DxgiBlacklist', 'entry', None,
             {'zh': ('DXGI 伪装黑名单',
                     '当调用方在名单里时跳过 DXGI 伪装，'
                     '用竖线 "|" 分隔方法名。\n\n'
                     '例：slInit|slGetPluginFunction|nvapi_QueryInterface\n\n'
                     '注意：这会让其余调用的伪装也一并失效，且在 Linux 上无效。\n\n'
                     'auto 表示不启用。'),
              'en': ('DXGI Blacklist',
                     'Skips DXGI spoofing when the caller is in this list. '
                     'Separate method names with a pipe "|".\n\n'
                     'e.g. slInit|slGetPluginFunction|nvapi_QueryInterface\n\n'
                     'Careful: this disables spoofing for the remaining calls too, '
                     'and does not work on Linux.\n\n'
                     'auto is disabled.')}),
            ('DxgiVRAM', 'entry', None,
             {'zh': ('DXGI 伪装显存',
                     '向 DXGI 报告的显存容量，单位 GB。\n\n'
                     '某些游戏会根据显存决定画质选项能否开启。\n\n'
                     'auto 表示不伪装。'),
              'en': ('DXGI VRAM',
                     'VRAM amount reported through DXGI, in GB.\n\n'
                     'Some games gate quality options on available VRAM.\n\n'
                     'auto is disabled.')}),
            ('Vulkan', 'combo', BOOL3,
             {'zh': ('Vulkan 伪装',
                     '在 Vulkan 层把显卡伪装成 NVIDIA（厂商可在上面改）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Vulkan Spoofing',
                     'Spoofs the GPU as NVIDIA for Vulkan (vendor configurable '
                     'above).\n\nauto defaults to false.')}),
            ('VulkanExtensionSpoofing', 'combo', BOOL3,
             {'zh': ('Vulkan 扩展伪装',
                     '伪装 NVIDIA 专有的 Vulkan 扩展。\n\n'
                     '某些只在 NVIDIA 上启用的功能需要它。\n\n'
                     'auto 默认为 false。'),
              'en': ('Vulkan Extension Spoofing',
                     'Spoofs NVIDIA-specific Vulkan extensions.\n\n'
                     'Needed by features that only light up on NVIDIA.\n\n'
                     'auto defaults to false.')}),
            ('VulkanVRAM', 'entry', None,
             {'zh': ('Vulkan 伪装显存',
                     '向 Vulkan 报告的显存容量，单位 GB。\n\n'
                     'auto 表示不伪装。'),
              'en': ('Vulkan VRAM',
                     'VRAM amount reported through Vulkan, in GB.\n\n'
                     'auto is disabled.')}),
            ('SpoofHAGS', 'combo', BOOL3,
             {'zh': ('伪装硬件加速 GPU 计划',
                     '假装系统已开启硬件加速 GPU 计划（HAGS）。\n\n'
                     'Nukem 的帧生成 mod 需要它；RTX 40 系用户也可以靠它'
                     '在不开 HAGS 的情况下用 DLSSG。\n\n'
                     'auto 默认为 false，但启用 DLSSG mod 时会自动打开。'),
              'en': ('Spoof HAGS',
                     'Pretends Hardware Accelerated GPU Scheduling is enabled.\n\n'
                     "Required by Nukem's mod, and lets RTX 40 series users run "
                     'DLSSG without HAGS turned on.\n\n'
                     'auto defaults to false, unless the DLSSG mod is enabled.')}),
            ('D3DFeatureLevel', 'combo', BOOL3,
             {'zh': ('覆盖 D3D 特性级别',
                     '允许覆盖游戏请求的 D3D 特性级别。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override D3D Feature Level',
                     'Allows overriding the D3D feature level the game requests.\n\n'
                     'auto defaults to false.')}),
            ('UEIntelAtomics', 'combo', BOOL3,
             {'zh': ('UE Intel 原子操作修正',
                     '避免 Intel 显卡开启伪装后，'
                     'UE 引擎游戏报「不支持 DirectX 12」。\n\n'
                     'auto 默认为 false。'),
              'en': ('UE Intel Atomics',
                     'Prevents the "DirectX12 not supported" error in UE games '
                     'on Intel cards when spoofing is enabled.\n\n'
                     'auto defaults to false.')}),
            ('Registry', 'combo', BOOL3,
             {'zh': ('注册表级伪装',
                     '在注册表层面伪装厂商 ID、设备 ID 和驱动版本。\n\n'
                     '一些会直接读注册表判断显卡的游戏需要它。\n\n'
                     'auto 默认为 false。'),
              'en': ('Registry Spoofing',
                     'Spoofs vendor ID, device ID and driver version at the '
                     'registry level.\n\n'
                     'Needed by games that read the registry directly.\n\n'
                     'auto defaults to false.')}),
            ('RegistryDriver', 'entry', None,
             {'zh': ('伪装驱动版本',
                     '在注册表里报告的驱动版本号。\n\n'
                     'auto 默认为 32.0.15.9155（NVIDIA 驱动的版本格式）。'),
              'en': ('Spoofed Driver Version',
                     'Driver version reported at the registry level.\n\n'
                     'auto defaults to 32.0.15.9155 (an NVIDIA driver version).')}),
            ('User32', 'combo', BOOL3,
             {'zh': ('User32 级伪装',
                     '在 User32 层面伪装厂商 ID 和设备 ID。\n\n'
                     'auto 默认为 false。'),
              'en': ('User32 Spoofing',
                     'Spoofs vendor ID and device ID at the User32 level.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'plugins',
        'section': 'Plugins',
        'name': {'zh': '插件 (Plugins)', 'en': 'Plugins'},
        'fields': [
            ('Path', 'entry', None,
             {'zh': ('插件目录',
                     'OptiScaler 在这个目录里查找同名插件'
                     '（dxgi.dll、winmm.dll 等）。\n\n'
                     'auto 默认为主 DLL 目录下的 plugins 子目录，'
                     '即 .\\OptiScaler\\plugins\\'),
              'en': ('Plugins Folder',
                     'Folder searched for same-filename plugins '
                     '(dxgi.dll, winmm.dll, etc.).\n\n'
                     'auto defaults to the plugins folder under OptiDllPath, '
                     'i.e. .\\OptiScaler\\plugins\\')}),
            ('LoadAsiPlugins', 'combo', BOOL3,
             {'zh': ('加载 ASI 插件',
                     '允许 OptiScaler 加载插件目录里的 *.asi 文件。\n\n'
                     'auto 默认为 false。'),
              'en': ('Load ASI Plugins',
                     'Lets OptiScaler load *.asi files from the plugins folder.\n\n'
                     'auto defaults to false.')}),
            ('LateAsiPluginsDelay', 'slider', (0, 120, 1),
             {'zh': ('延迟加载 ASI 的等待秒数',
                     '延迟注入的 ASI 插件要等多少秒才加载。\n\n'
                     '想让某个 ASI 延迟加载，在扩展名前加 -loadlate，'
                     '例如 myplugin-loadlate.asi\n\n'
                     '单位为秒，auto 默认为 30。'),
              'en': ('Late ASI Plugin Delay',
                     'Delay before injecting late-loaded *.asi plugins.\n\n'
                     'To load an ASI plugin late, add -loadlate before the '
                     'extension, e.g. myplugin-loadlate.asi\n\n'
                     'In seconds, auto defaults to 30.')}),
            ('LoadSpecialK', 'combo', BOOL3,
             {'zh': ('加载 Special K',
                     '从游戏 exe 所在目录加载 SpecialK64.dll。\n\n'
                     '需要在 SpecialK64.dll 旁边建一个空的 SpecialK.dxgi 文件'
                     '来指定它的工作模式，否则不会被激活。\n\n'
                     '注意：因为存在稳定性问题，启用 OptiFG、'
                     '经 Streamline 的 DLSSG 或 FSR-FG 输入时都不会加载。\n\n'
                     'auto 默认为 false。'),
              'en': ('Load Special K',
                     "Loads SpecialK64.dll from the game's exe folder.\n\n"
                     'Create an empty SpecialK.dxgi file next to SpecialK64.dll '
                     "to set Special K's working mode, otherwise it stays inactive.\n\n"
                     'Note: because of stability issues it will not be loaded when '
                     'OptiFG, DLSSG via Streamline, or FSR-FG inputs are enabled.\n\n'
                     'auto defaults to false.')}),
            ('LoadReshade', 'combo', BOOL3,
             {'zh': ('加载 ReShade',
                     "从游戏 exe 所在目录加载 ReShade64.dll。\n\n"
                     '把 ReShade 的 dll 改名为 ReShade64.dll，'
                     '放到 OptiScaler 旁边，然后设为 true。\n\n'
                     'auto 默认为 false。'),
              'en': ('Load ReShade',
                     "Loads ReShade64.dll from the game's exe folder.\n\n"
                     'Rename the ReShade dll to ReShade64.dll, put it next to '
                     'OptiScaler, and set this to true.\n\n'
                     'auto defaults to false.')}),
            ('LoadCustomAmdxc64OnRdna2', 'combo', BOOL3,
             {'zh': ('RDNA 2 加载自定义 amdxc64',
                     '为所有检测到的 RDNA 2 显卡加载自定义的 amdxc64.dll。\n\n'
                     'OptiScaler 从主 DLL 目录加载它，'
                     '例如 .\\OptiScaler\\amdxc64.dll\n\n'
                     'auto 默认为 false。'),
              'en': ('Custom amdxc64 on RDNA 2',
                     'Loads a custom amdxc64.dll for all detected RDNA 2 GPUs.\n\n'
                     'OptiScaler loads it from OptiDllPath, '
                     'e.g. .\\OptiScaler\\amdxc64.dll\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'fakenvapi',
        'section': 'fakenvapi',
        'name': {'zh': 'fakenvapi 延迟优化', 'en': 'fakenvapi'},
        'fields': [
            ('UseFakenvapi', 'combo', BOOL3,
             {'zh': ('启用 fakenvapi',
                     '当系统里没有 nvapi64.dll 时改用 fakenvapi。\n\n'
                     'fakenvapi 会替换 NVIDIA 的 nvapi 调用，让 AMD 的 AntiLag 2 '
                     '或 Intel 的 XeLL 顶替 Reflex 来降低延迟。\n\n'
                     '有些游戏还必须靠它才能让 DLSS / DLSSG 的选项显示出来。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use fakenvapi',
                     'Uses fakenvapi when nvapi64.dll is missing.\n\n'
                     'fakenvapi replaces nvapi calls so features like AntiLag 2 '
                     'or XeLL can stand in for Reflex.\n\n'
                     'Sometimes also required for DLSS/DLSSG options to show up.\n\n'
                     'auto defaults to true.')}),

            ('ForceXeLL', 'combo', BOOL3,
             {'zh': ('强制启用 XeLL',
                     '让 XeLL 在非 Intel 显卡上不开帧生成也能工作。\n\n'
                     '需要较新的 libxell 与 libxess_fg。\n\n'
                     '注意：开启后会禁用帧生成相关选项。\n\n'
                     'auto 默认为 false。'),
              'en': ('Force XeLL',
                     'Lets XeLL work without FG on non-Intel cards.\n\n'
                     'Requires updated libxell + libxess_fg.\n\n'
                     'Disables FG options.\n\nauto defaults to false.')}),

            ('ForceLatencyFlex', 'combo', BOOL3,
             {'zh': ('强制使用 LatencyFlex',
                     '即使有更好的降延迟方案（AntiLag 2 / XeLL）也强制走 LatencyFlex。\n\n'
                     '使用 XeFG 时该选项不生效。\n\n'
                     'auto 默认为 false。'),
              'en': ('Force LatencyFlex',
                     'Uses LatencyFlex even when better options are available.\n\n'
                     'Has no effect when using XeFG.\n\n'
                     'auto defaults to false.')}),

            ('LatencyFlexMode', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('LatencyFlex 模式',
                     '控制 LatencyFlex 的工作力度。\n\n'
                     '0 = 保守（默认）\n'
                     '1 = 激进：延迟更低，但某些情况下掉帧比预期更多\n'
                     '2 = 使用 Reflex 帧 ID：部分游戏不兼容（例如赛博朋克 2077），'
                     '不兼容时会自动退回激进模式\n\n'
                     'auto 默认为 0。'),
              'en': ('LatencyFlex Mode',
                     'Controls how LatencyFlex works.\n\n'
                     '0 = conservative (default)\n'
                     '1 = aggressive: better latency, but may cost more fps '
                     'than expected in some cases\n'
                     '2 = use Reflex frame ids: some games are incompatible '
                     '(e.g. Cyberpunk) and fall back to aggressive\n\n'
                     'auto defaults to 0.')}),

            ('ForceReflex', 'combo', ['auto', '0', '1', '2'],
             {'zh': ('强制 Reflex 状态',
                     '强行指定 Reflex 的开关状态。\n\n'
                     '0 = 跟随游戏内设置（默认）\n'
                     '1 = 强制关闭\n'
                     '2 = 强制开启\n\n'
                     '适用于那些"只有开 DLSSG 才会启用 Reflex、又没给单独开关"的游戏。\n\n'
                     'auto 默认为 0。'),
              'en': ('Force Reflex',
                     'Forces the Reflex state.\n\n'
                     '0 = follow in-game setting (default)\n'
                     '1 = force disable\n'
                     '2 = force enable\n\n'
                     'Useful for games that only enable Reflex when using DLSSG '
                     'without a separate Reflex toggle.\n\n'
                     'auto defaults to 0.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'nvapi',
        'section': 'NvApi',
        'name': {'zh': 'NvApi', 'en': 'NvApi'},
        'fields': [
            ('DisableFlipMetering', 'combo', BOOL3,
             {'zh': ('禁用 Flip Metering',
                     '关闭 NVIDIA 的 Flip Metering。\n\n'
                     '用 NukemFG 时帧生成曲线会变得又粗又抖，'
                     '关掉这个可以让帧时间图恢复平滑。\n\n'
                     '需要 fakenvapi 一起工作才有效。\n\n'
                     'auto 在 NVIDIA 卡上默认为 false，其他厂商默认为 true。'),
              'en': ('Disable Flip Metering',
                     'Disables Nvidia Flip Metering.\n\n'
                     'Fixes the thick frametime graph caused by Flip Metering '
                     'when using NukemFG.\n\n'
                     'Needs fakenvapi to work.\n\n'
                     'auto defaults to false for Nvidia, true for others.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'dx11withdx12',
        'section': 'Dx11withDx12',
        'name': {'zh': 'DX11 转 DX12 兼容层', 'en': 'Dx11 with Dx12'},
        'fields': [
            ('UseDelayedInit', 'combo', BOOL3,
             {'zh': ('延迟初始化',
                     '创建 DX11-on-DX12 功能时把部分操作推迟执行，以提高兼容性。\n\n'
                     '如果 DX11 游戏用上采样时闪退或黑屏，可以试着开启。\n\n'
                     'auto 默认为 false。'),
              'en': ('Use Delayed Init',
                     'Delays some operations during creation of D11wDx12 '
                     'features to increase compatibility.\n\n'
                     'auto defaults to false.')}),

            ('DontUseNTShared', 'combo', BOOL3,
             {'zh': ('不使用 NT 共享句柄',
                     '优先使用更底层的 D3D11_RESOURCE_MISC_SHARED。\n\n'
                     '性能略好，但兼容性可能差一些。遇到 DX11 游戏画面异常时可关闭。\n\n'
                     'auto 默认为 true。'),
              'en': ('Don\'t Use NT Shared',
                     'Prefers D3D11_RESOURCE_MISC_SHARED, which is lower level, '
                     'a bit more performant and possibly less compatible.\n\n'
                     'auto defaults to true.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'hooks',
        'section': 'Hooks',
        'name': {'zh': '钩子与挂载 (Hooks)', 'en': 'Hooks'},
        'fields': [
            ('EarlyHooking', 'combo', BOOL3,
             {'zh': ('提前挂载内核方法',
                     '在早期阶段就挂载 kernel 方法。\n\n'
                     '可能导致兼容性问题！只在排查特定问题时使用。\n\n'
                     'auto 默认为 false。'),
              'en': ('Early Hooking',
                     'Hooks kernel methods at an early stage.\n\n'
                     'May cause compatibility issues!\n\n'
                     'auto defaults to false.')}),

            ('HookOriginalNvngxOnly', 'combo', BOOL3,
             {'zh': ('只挂载注册表中的 nvngx',
                     '跳过挂载本地 nvngx 文件，只挂载注册表里的 nvngx。\n\n'
                     'Uniscaler + 帧生成组合时需要此选项。\n\n'
                     'auto 默认为 false。'),
              'en': ('Hook Original Nvngx Only',
                     'Skips hooking local nvngx files; only hooks registry nvngx.\n\n'
                     'Needed for Uniscaler + FG.\n\n'
                     'auto defaults to false.')}),

            ('UseNtdllHooks', 'combo', BOOL3,
             {'zh': ('仅挂载 ntdll.dll',
                     '只挂载 ntdll.dll 方法，跳过 kernel32.dll 和 kernelbase.dll。\n\n'
                     '默认启用，某些情况下可提高稳定性。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use Ntdll Hooks',
                     'Only hooks ntdll.dll methods; skips kernel32 and kernelbase.\n\n'
                     'auto defaults to true.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'sharpness',
        'section': 'Sharpness',
        'name': {'zh': '锐化 (Sharpness)', 'en': 'Sharpness'},
        'fields': [
            ('Shader', 'combo', ['auto', 'rcas', 'da', 'lcda'],
             {'zh': ('锐化着色器',
                     '选择锐化算法。\n\n'
                     'rcas = AMD 的鲁棒对比度自适应锐化（默认）\n'
                     'da = 基于深度的边缘检测锐化，减少物体边界的光晕，'
                     '离得越远锐化越强（最高 1.0），适合复杂几何场景\n'
                     'lcda = da 的局部对比度版本\n\n'
                     'auto 默认为 rcas。'),
              'en': ('Sharpness Shader',
                     'Selects the sharpening algorithm.\n\n'
                     'rcas = AMD Robust Contrast-Adaptive Sharpening (default)\n'
                     'da = Depth-based edge detection; reduces halos across boundaries; '
                     'more sharpening for distant objects (capped at 1.0)\n'
                     'lcda = local contrast version of da\n\n'
                     'auto defaults to rcas.')}),

            ('OverrideSharpness', 'combo', BOOL3,
             {'zh': ('覆盖 DLSS 锐化参数',
                     '用固定值覆盖 DLSS 自己的锐化参数。\n\n'
                     '启用后 Sharpness 字段生效，否则跟随游戏内设置。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override Sharpness',
                     'Overrides DLSS sharpness with a fixed value.\n\n'
                     'When enabled, the Sharpness field is used; '
                     'otherwise follows in-game setting.\n\n'
                     'auto defaults to false.')}),

            ('Sharpness', 'slider', (0.0, 1.3, 0.05),
             {'zh': ('锐化强度',
                     '锐化强度，范围 0.0 到 1.0（RCAS 模式可到 1.3）。\n\n'
                     '需要先启用 OverrideSharpness 才生效。\n\n'
                     'auto 默认为 0.3。'),
              'en': ('Sharpness',
                     'Sharpening strength, 0.0 to 1.0 (RCAS allows up to 1.3).\n\n'
                     'Requires OverrideSharpness to be enabled.\n\n'
                     'auto defaults to 0.3.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'outputscaling',
        'section': 'OutputScaling',
        'name': {'zh': '输出缩放 (Output Scaling)', 'en': 'Output Scaling'},
        'fields': [
            ('Enabled', 'combo', BOOL3,
             {'zh': ('启用输出缩放',
                     '为 DX12 和 DX11-on-DX12 后端启用输出缩放。\n\n'
                     '先上采样到原生分辨率，再进一步放大到更高分辨率输出。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Output Scaling',
                     'Enables output scaling for DX12 and DX11-on-DX12.\n\n'
                     'Upscales to native, then scales further to output.\n\n'
                     'auto defaults to false.')}),

            ('Multiplier', 'slider', (0.5, 3.0, 0.1),
             {'zh': ('输出缩放倍率',
                     '相对于原生分辨率的缩放倍率，0.5 到 3.0。\n\n'
                     '例如 1.5 表示输出分辨率是原生的 1.5 倍。\n\n'
                     'auto 默认为 1.5。'),
              'en': ('Output Multiplier',
                     'Output scaling ratio relative to native, 0.5 to 3.0.\n\n'
                     'E.g., 1.5 means output is 1.5× native resolution.\n\n'
                     'auto defaults to 1.5.')}),

            ('Downscaler', 'combo',
             ['auto', '0', '1', '2', '3', '4', '5', '6', '7'],
             {'zh': ('降采样器',
                     '输出缩放时使用的降采样算法。\n\n'
                     '0 = FSR1（默认）\n'
                     '1 = Bicubic（双三次）\n'
                     '2 = Catmull-Rom\n'
                     '3 = Lanczos2\n'
                     '4 = Lanczos3\n'
                     '5 = Kaiser2\n'
                     '6 = Kaiser3\n'
                     '7 = MAGIC\n\n'
                     'auto 默认为 0（FSR1）。'),
              'en': ('Downscaler',
                     'Downsampling algorithm for output scaling.\n\n'
                     '0 = FSR1 (default)\n'
                     '1 = Bicubic\n'
                     '2 = Catmull-Rom\n'
                     '3 = Lanczos2\n'
                     '4 = Lanczos3\n'
                     '5 = Kaiser2\n'
                     '6 = Kaiser3\n'
                     '7 = MAGIC\n\n'
                     'auto defaults to 0 (FSR1).')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'cas',
        'section': 'CAS',
        'name': {'zh': '对比度自适应锐化 (CAS)', 'en': 'CAS'},
        'fields': [
            ('Enabled', 'combo', BOOL3,
             {'zh': ('启用 RCAS 锐化',
                     '启用 AMD 的鲁棒对比度自适应锐化（RCAS）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable RCAS Sharpening',
                     'Enables AMD Robust Contrast-Adaptive Sharpening.\n\n'
                     'auto defaults to false.')}),

            ('MotionSharpnessEnabled', 'combo', BOOL3,
             {'zh': ('启用运动锐化',
                     '根据运动长度动态调整锐化强度。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Motion Sharpness',
                     'Adjusts sharpening dynamically based on motion length.\n\n'
                     'auto defaults to false.')}),

            ('MotionSharpness', 'slider', (-1.3, 1.3, 0.05),
             {'zh': ('运动锐化强度',
                     '根据运动长度增加或减少锐化，范围 -1.3 到 1.3。\n\n'
                     '正值增加运动区域锐化，负值减少。\n\n'
                     'auto 默认为 0.4。'),
              'en': ('Motion Sharpness',
                     'Sharpening added or removed based on motion, -1.3 to 1.3.\n\n'
                     'auto defaults to 0.4.')}),

            ('MotionThreshold', 'slider', (0.0, 100.0, 1.0),
             {'zh': ('运动阈值',
                     '像素移动多少距离后开始应用运动锐化，0.0 到 100.0。\n\n'
                     'auto 默认为 0.0。'),
              'en': ('Motion Threshold',
                     'How much a pixel must move before motion sharpness applies, 0.0 to 100.0.\n\n'
                     'auto defaults to 0.0.')}),

            ('MotionScaleLimit', 'slider', (0.0, 100.0, 1.0),
             {'zh': ('运动缩放上限',
                     '像素移动多少距离达到最大运动锐化值，0.0 到 100.0。\n\n'
                     '介于 MotionThreshold 和此值之间的运动会按比例缩放锐化强度。\n\n'
                     'auto 默认为 10.0。'),
              'en': ('Motion Scale Limit',
                     'Pixel movement to reach max MotionSharpness, 0.0 to 100.0.\n\n'
                     'Motion between threshold and this value scales sharpness.\n\n'
                     'auto defaults to 10.0.')}),

            ('ContrastEnabled', 'combo', BOOL3,
             {'zh': ('启用对比度锐化',
                     '在高对比度区域增强锐化。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Contrast Sharpening',
                     'Increases sharpness at high-contrast areas.\n\n'
                     'auto defaults to false.')}),

            ('Contrast', 'slider', (0.0, 2.0, 0.05),
             {'zh': ('对比度锐化强度',
                     '高对比度区域的锐化强度，0.0 到 2.0。\n\n'
                     '高值配合高锐化可能导致图形故障！\n\n'
                     'auto 默认为 0.0。'),
              'en': ('Contrast',
                     'Contrast-based sharpening strength, 0.0 to 2.0.\n\n'
                     'High values with high sharpness may cause GLITCHES!\n\n'
                     'auto defaults to 0.0.')}),

            ('SharpenerDebug', 'combo', BOOL3,
             {'zh': ('锐化调试高亮',
                     '启用运动/深度感知锐化的调试高亮。\n\n'
                     '红色色调 = 增加锐化，绿色 = 减少锐化，蓝色 = DA 模式检测到的边缘。\n\n'
                     'auto 默认为 false。'),
              'en': ('Sharpener Debug',
                     'Enables debug highlighting for motion/DA sharpening.\n\n'
                     'Reddish = added sharpness, greenish = reduced, blue = detected edges in DA.\n\n'
                     'auto defaults to false.')}),

            ('DADepthScale', 'slider', (2.0, 600.0, 1.0),
             {'zh': ('深度感知边缘缩放',
                     '控制跨深度边缘减少锐化的强度，2.0 到 600.0。\n\n'
                     '高值更积极地阻止物体边界锐化（减少光晕），低值允许更多边缘锐化（更锐但有风险）。\n\n'
                     'auto 默认为 4.0。'),
              'en': ('DA Depth Scale',
                     'Controls sharpening reduction across depth edges, 2.0 to 600.0.\n\n'
                     'Higher = more aggressive blocking at boundaries (reduces halos), '
                     'lower = sharper but riskier.\n\n'
                     'auto defaults to 4.0.')}),

            ('DADepthBias', 'slider', (0.005, 0.03, 0.001),
             {'zh': ('深度感知偏移',
                     '忽略小的深度差异，避免边缘检测噪声，0.005 到 0.03。\n\n'
                     '高值减少深度变化引起的闪烁，低值保留细节但可能对深度噪声敏感。\n\n'
                     'auto 默认为 0.01。'),
              'en': ('DA Depth Bias',
                     'Ignores small depth differences before edge detection, 0.005 to 0.03.\n\n'
                     'Higher = less flicker from minor depth changes, lower = more detail but noise-sensitive.\n\n'
                     'auto defaults to 0.01.')}),

            ('DAClampOutput', 'combo', BOOL3,
             {'zh': ('深度感知输出钳位',
                     '将最终图像钳位到 [0, 1] 范围，防止过冲伪影（如亮光晕或负色）。\n\n'
                     'LDR 管线推荐启用；HDR 管线视色调映射而定。\n\n'
                     'auto 默认为 false。'),
              'en': ('DA Clamp Output',
                     'Clamps final image to [0, 1] range; prevents overshoot artifacts.\n\n'
                     'Recommended for LDR; optional for HDR depending on tone-mapping.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'magnifier',
        'section': 'Magnifier',
        'name': {'zh': '放大镜', 'en': 'Magnifier'},
        'fields': [
            ('Enabled', 'combo', BOOL3,
             {'zh': ('启用放大镜',
                     '启用菜单中的画面放大镜。\n\nauto 默认为 true。'),
              'en': ('Enable Magnifier',
                     'Enables the in-menu image magnifier.\n\nauto defaults to true.')}),
            ('Size', 'slider', (1.0, 50.0, 0.1),
             {'zh': ('放大镜尺寸',
                     '放大镜半径，占屏幕高度的百分比。\n\nauto 默认为 15.0。'),
              'en': ('Magnifier Size',
                     'Magnifier radius as a percentage of screen height.\n\n'
                     'auto defaults to 15.0.')}),
            ('ZoomFactor', 'slider', (2, 20, 1),
             {'zh': ('缩放倍数',
                     '放大镜的缩放倍数。\n\nauto 默认为 4。'),
              'en': ('Zoom Factor',
                     'Magnifier zoom factor.\n\nauto defaults to 4.')}),
            ('BorderSize', 'slider', (0.0, 2.0, 0.1),
             {'zh': ('边框尺寸',
                     '黑色边框宽度，占屏幕高度的百分比。\n\nauto 默认为 0.3。'),
              'en': ('Border Size',
                     'Black border width as a percentage of screen height.\n\n'
                     'auto defaults to 0.3.')}),
            ('CursorOffsetX', 'slider', (-1000, 1000, 1),
             {'zh': ('鼠标横向偏移',
                     '放大镜相对鼠标位置的横向像素偏移。\n\nauto 默认为 0。'),
              'en': ('Cursor X Offset',
                     'Horizontal pixel offset from the cursor.\n\nauto defaults to 0.')}),
            ('CursorOffsetY', 'slider', (-1000, 1000, 1),
             {'zh': ('鼠标纵向偏移',
                     '放大镜相对鼠标位置的纵向像素偏移。\n\nauto 默认为 0。'),
              'en': ('Cursor Y Offset',
                     'Vertical pixel offset from the cursor.\n\nauto defaults to 0.')}),
            ('StaticPosX', 'slider', (0, 100, 1),
             {'zh': ('固定横向位置',
                     '固定位置占屏幕宽度的百分比；横纵坐标同时设置时启用固定模式。'),
              'en': ('Static X Position',
                     'Static position as a percentage of screen width. Static mode '
                     'is enabled when both coordinates are set.')}),
            ('StaticPosY', 'slider', (0, 100, 1),
             {'zh': ('固定纵向位置',
                     '固定位置占屏幕高度的百分比；横纵坐标同时设置时启用固定模式。'),
              'en': ('Static Y Position',
                     'Static position as a percentage of screen height. Static mode '
                     'is enabled when both coordinates are set.')}),
        ],
    },

    {
        'id': 'log',
        'section': 'Log',
        'name': {'zh': '日志 (Log)', 'en': 'Log'},
        'fields': [
            ('LogToFile', 'combo', BOOL3,
             {'zh': ('写入日志文件',
                     '是否创建日志文件。\n\n'
                     'auto 默认为 false。'),
              'en': ('Log To File',
                     'Whether to create a log file.\n\n'
                     'auto defaults to false.')}),

            ('LogLevel', 'combo',
             ['auto', '0', '1', '2', '3', '4'],
             {'zh': ('日志详细等级',
                     '文件日志的详细程度。\n\n'
                     '0 = Trace（最详细）\n'
                     '1 = Debug\n'
                     '2 = Info\n'
                     '3 = Warning\n'
                     '4 = Error（仅错误）\n\n'
                     'auto 默认为 0（Trace）。'),
              'en': ('Log Level',
                     'Verbosity level of file logs.\n\n'
                     '0 = Trace (most verbose)\n'
                     '1 = Debug\n'
                     '2 = Info\n'
                     '3 = Warning\n'
                     '4 = Error (errors only)\n\n'
                     'auto defaults to 0 (Trace).')}),

            ('LogToConsole', 'combo', BOOL3,
             {'zh': ('输出到控制台',
                     '将日志输出到控制台（等级固定为 2 Info，性能考虑）。\n\n'
                     'auto 默认为 false。'),
              'en': ('Log To Console',
                     'Logs to console (level always 2 Info for performance).\n\n'
                     'auto defaults to false.')}),

            ('LogToNGX', 'combo', BOOL3,
             {'zh': ('输出到 NVNGX API',
                     '将日志输出到 NVNGX API。\n\n'
                     'auto 默认为 false。'),
              'en': ('Log To NGX',
                     'Logs to NVNGX API.\n\n'
                     'auto defaults to false.')}),

            ('LogToDebug', 'combo', BOOL3,
             {'zh': ('输出到调试输出',
                     '将日志输出到 Debug output。\n\n'
                     'auto 默认为 false。'),
              'en': ('Log To Debug',
                     'Logs to Debug output.\n\n'
                     'auto defaults to false.')}),

            ('OpenConsole', 'combo', BOOL3,
             {'zh': ('打开控制台窗口',
                     '为日志打开一个控制台窗口。\n\n'
                     'auto 默认为 false。'),
              'en': ('Open Console',
                     'Opens a console window for logs.\n\n'
                     'auto defaults to false.')}),

            ('SingleFile', 'combo', BOOL3,
             {'zh': ('单文件模式',
                     '设为 false 时每次 OptiScaler 会话创建新日志文件。\n\n'
                     'auto 默认为 true。'),
              'en': ('Single File',
                     'When false, creates a new log file per OptiScaler session.\n\n'
                     'auto defaults to true.')}),

            ('LogFileName', 'entry', None,
             {'zh': ('自定义日志文件名',
                     '可指定自定义日志文件名或路径。\n\n'
                     'auto 默认为当前目录下的 OptiScaler.log。'),
              'en': ('Log Filename',
                     'Custom log filename or path.\n\n'
                     'auto defaults to OptiScaler.log in current folder.')}),

            ('LogAsync', 'combo', BOOL3,
             {'zh': ('异步日志',
                     '启用异步日志记录。\n\n'
                     'auto 默认为 true。'),
              'en': ('Async Logging',
                     'Enables asynchronous logging.\n\n'
                     'auto defaults to true.')}),

            ('LogAsyncThreads', 'slider', (1, 8, 1),
             {'zh': ('异步日志线程数',
                     '异步日志使用的线程数，1 到 8。\n\n'
                     'auto 默认为 1。'),
              'en': ('Async Logging Threads',
                     'Number of threads for async logging, 1 to 8.\n\n'
                     'auto defaults to 1.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'initflags',
        'section': 'InitFlags',
        'name': {'zh': '初始化标志 (Init Flags)', 'en': 'Init Flags'},
        'fields': [
            ('AutoExposure', 'combo', BOOL3,
             {'zh': ('强制自动曝光',
                     '强制添加 ENABLE_AUTOEXPOSURE 到初始化标志。\n\n'
                     '某些虚幻引擎游戏需要此项，修复暗部颜色。\n\n'
                     'auto 默认跟随 DLSS 值。'),
              'en': ('Force Auto Exposure',
                     'Forces ENABLE_AUTOEXPOSURE init flag.\n\n'
                     'Some UE games need this; fixes colors in dark areas.\n\n'
                     'auto defaults to DLSS value.')}),

            ('HDR', 'combo', BOOL3,
             {'zh': ('强制 HDR 输入色彩',
                     '强制添加 HDR_INPUT_COLOR 到初始化标志。\n\n'
                     'auto 默认跟随 DLSS 值。'),
              'en': ('Force HDR Input Color',
                     'Forces HDR_INPUT_COLOR init flag.\n\n'
                     'auto defaults to DLSS value.')}),

            ('DepthInverted', 'combo', BOOL3,
             {'zh': ('强制反转深度',
                     '强制添加 INVERTED_DEPTH 到初始化标志。\n\n'
                     'auto 默认跟随 DLSS 值。'),
              'en': ('Force Inverted Depth',
                     'Forces INVERTED_DEPTH init flag.\n\n'
                     'auto defaults to DLSS value.')}),

            ('JitterCancellation', 'combo', BOOL3,
             {'zh': ('强制抖动取消',
                     '强制添加 JITTERED_MV 标志到初始化标志。\n\n'
                     'auto 默认跟随 DLSS 值。'),
              'en': ('Force Jitter Cancellation',
                     'Forces JITTERED_MV init flag.\n\n'
                     'auto defaults to DLSS value.')}),

            ('DisplayResolution', 'combo', BOOL3,
             {'zh': ('强制高分辨率运动矢量',
                     '强制添加 HIGH_RES_MV 标志到初始化标志。\n\n'
                     'auto 默认跟随 DLSS 值。'),
              'en': ('Force Display Resolution MV',
                     'Forces HIGH_RES_MV init flag.\n\n'
                     'auto defaults to DLSS value.')}),

            ('DisableReactiveMask', 'combo', BOOL3,
             {'zh': ('禁用响应像素掩码',
                     '强制移除 RESPONSIVE_PIXEL_MASK 初始化标志。\n\n'
                     'auto 默认为 true。'),
              'en': ('Disable Reactive Mask',
                     'Forces removal of RESPONSIVE_PIXEL_MASK init flag.\n\n'
                     'auto defaults to true.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'upscaleratio',
        'section': 'UpscaleRatio',
        'name': {'zh': '上采样比例 (Upscale Ratio)', 'en': 'Upscale Ratio'},
        'fields': [
            ('UpscaleRatioOverrideEnabled', 'combo', BOOL3,
             {'zh': ('启用比例覆盖',
                     '启用内部分辨率比例覆盖。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Ratio Override',
                     'Enables internal resolution ratio override.\n\n'
                     'auto defaults to false.')}),

            ('UpscaleRatioOverrideValue', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('强制上采样比例',
                     '强制的上采样比例值，1.0 到 3.0。\n\n'
                     'auto 默认为 1.3。'),
              'en': ('Forced Upscale Ratio',
                     'Forced upscale ratio value, 1.0 to 3.0.\n\n'
                     'auto defaults to 1.3.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'qualityoverrides',
        'section': 'QualityOverrides',
        'name': {'zh': '质量档位覆盖 (Quality Overrides)', 'en': 'Quality Overrides'},
        'fields': [
            ('QualityRatioOverrideEnabled', 'combo', BOOL3,
             {'zh': ('启用质量档位覆盖',
                     '启用自定义质量模式比例覆盖。\n\n'
                     'auto 默认为 false。'),
              'en': ('Enable Quality Overrides',
                     'Enables custom quality mode ratio overrides.\n\n'
                     'auto defaults to false.')}),

            ('QualityRatioDLAA', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('DLAA 比例',
                     '自定义 DLAA 模式的上采样比例。\n\n'
                     'auto 默认为 1.0。'),
              'en': ('DLAA Ratio',
                     'Custom upscaling ratio for DLAA mode.\n\n'
                     'auto defaults to 1.0.')}),

            ('QualityRatioUltraQuality', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('超质量比例',
                     '自定义超质量模式的上采样比例。\n\n'
                     'auto 默认为 1.3。'),
              'en': ('Ultra Quality Ratio',
                     'Custom upscaling ratio for Ultra Quality mode.\n\n'
                     'auto defaults to 1.3.')}),

            ('QualityRatioQuality', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('质量比例',
                     '自定义质量模式的上采样比例。\n\n'
                     'auto 默认为 1.5。'),
              'en': ('Quality Ratio',
                     'Custom upscaling ratio for Quality mode.\n\n'
                     'auto defaults to 1.5.')}),

            ('QualityRatioBalanced', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('平衡比例',
                     '自定义平衡模式的上采样比例。\n\n'
                     'auto 默认为 1.7。'),
              'en': ('Balanced Ratio',
                     'Custom upscaling ratio for Balanced mode.\n\n'
                     'auto defaults to 1.7.')}),

            ('QualityRatioPerformance', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('性能比例',
                     '自定义性能模式的上采样比例。\n\n'
                     'auto 默认为 2.0。'),
              'en': ('Performance Ratio',
                     'Custom upscaling ratio for Performance mode.\n\n'
                     'auto defaults to 2.0.')}),

            ('QualityRatioUltraPerformance', 'slider', (1.0, 3.0, 0.05),
             {'zh': ('超性能比例',
                     '自定义超性能模式的上采样比例。\n\n'
                     'auto 默认为 3.0。'),
              'en': ('Ultra Performance Ratio',
                     'Custom upscaling ratio for Ultra Performance mode.\n\n'
                     'auto defaults to 3.0.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'drs',
        'section': 'DRS',
        'name': {'zh': '动态分辨率缩放 (DRS)', 'en': 'DRS'},
        'fields': [
            ('DrsMinOverrideEnabled', 'combo', BOOL3,
             {'zh': ('限制 DRS 最小分辨率',
                     '限制 DRS 最小分辨率为渲染分辨率。\n\n'
                     'auto 默认为 false。'),
              'en': ('Limit DRS Min Resolution',
                     'Limits DRS min resolution to rendering resolution.\n\n'
                     'auto defaults to false.')}),

            ('DrsMaxOverrideEnabled', 'combo', BOOL3,
             {'zh': ('限制 DRS 最大分辨率',
                     '限制 DRS 最大分辨率为渲染分辨率。\n\n'
                     'auto 默认为 false。'),
              'en': ('Limit DRS Max Resolution',
                     'Limits DRS max resolution to rendering resolution.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'hdr',
        'section': 'HDR',
        'name': {'zh': 'HDR', 'en': 'HDR'},
        'fields': [
            ('ForceHDR', 'combo', BOOL3,
             {'zh': ('强制 HDR 色彩空间',
                     '强制使用 HDR 色彩空间。\n\n'
                     '用于游戏本身没正确设置 HDR、但显示器已开 HDR 的情况。\n\n'
                     'auto 默认为 false。'),
              'en': ('Force HDR',
                     'Forces the HDR color space.\n\n'
                     'auto defaults to false.')}),

            ('UseHDR10', 'combo', BOOL3,
             {'zh': ('使用 HDR10 格式',
                     '用 R10G10B10A2_UNORM 代替 R16G16B16A16_FLOAT 作为输出格式。\n\n'
                     '10 位整数格式显存占用更小，但精度低于 16 位浮点。\n\n'
                     'auto 默认为 false。'),
              'en': ('Use HDR10',
                     'Uses DXGI_FORMAT_R10G10B10A2_UNORM instead of '
                     'DXGI_FORMAT_R16G16B16A16_FLOAT.\n\n'
                     'auto defaults to false.')}),

            ('SkipColorSpace', 'combo', BOOL3,
             {'zh': ('跳过设置色彩空间',
                     '不去主动设置 HDR 色彩空间。\n\n'
                     '当游戏或其他 mod 已经处理好色彩空间时可以开启，避免冲突。\n\n'
                     'auto 默认为 false。'),
              'en': ('Skip Color Space',
                     'Skips setting the HDR colorspace.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'vsync',
        'section': 'V-Sync',
        'name': {'zh': '垂直同步 (V-Sync)', 'en': 'V-Sync'},
        'fields': [
            ('OverrideVsync', 'combo', BOOL3,
             {'zh': ('启用垂直同步覆盖',
                     '打开本项后，下面两个 V-Sync 选项才会生效。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override V-Sync',
                     'Enables the V-Sync controls below.\n\n'
                     'auto defaults to false.')}),

            ('ForceVsync', 'combo', BOOL3,
             {'zh': ('强制垂直同步开关',
                     '强制指定 V-Sync 的开关状态，覆盖游戏内设置。\n\n'
                     '需要先启用上面的「启用垂直同步覆盖」。\n\n'
                     'auto 表示不指定。'),
              'en': ('Force V-Sync',
                     'Forces the V-Sync value, overriding the in-game setting.\n\n'
                     'Requires Override V-Sync above.\n\n'
                     'auto means no selection.')}),

            ('SyncInterval', 'combo', ['auto', '0', '1', '2', '3'],
             {'zh': ('同步间隔',
                     'Present 时使用的同步间隔。\n\n'
                     '0 = 不等待垂直消隐（撕裂）\n'
                     '1 = 每次垂直消隐同步一次（标准 V-Sync）\n'
                     '2 = 每两次同步一次（半帧率）\n'
                     '3 = 每三次同步一次（三分之一帧率）\n\n'
                     'auto 表示不指定。'),
              'en': ('Sync Interval',
                     'Sync interval value used on Present.\n\n'
                     '0 = no wait for vblank (tearing)\n'
                     '1 = sync every vblank (standard V-Sync)\n'
                     '2 = sync every second vblank (half rate)\n'
                     '3 = sync every third vblank\n\n'
                     'auto means no selection.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'anisotropy',
        'section': 'Anisotropy',
        'name': {'zh': '各向异性过滤 (Anisotropy)', 'en': 'Anisotropy'},
        'fields': [
            ('AnisotropyOverride', 'combo', ['auto', '2', '4', '8', '16'],
             {'zh': ('各向异性过滤级别',
                     '强制覆盖纹理的最大各向异性过滤级别。\n\n'
                     '级别越高，斜视角下的地面/墙面纹理越清晰，性能代价很小。\n'
                     '16x 是常见的画质首选。\n\n'
                     'auto 表示不覆盖（沿用游戏设置）。'),
              'en': ('Anisotropy Override',
                     'Overrides max anisotropic filtering for textures.\n\n'
                     'Higher values keep textures sharp at oblique angles at very '
                     'little performance cost. 16x is the usual choice.\n\n'
                     'auto means disabled (game setting is kept).')}),

            ('ModifyComparison', 'combo', BOOL3,
             {'zh': ('修改比较型采样器',
                     '把比较型（comparison）过滤器也改成各向异性版本。\n\n'
                     '比较型采样器主要用于阴影贴图，改动后阴影边缘可能变化。\n\n'
                     'auto 默认为 true。'),
              'en': ('Modify Comparison Filters',
                     'Changes comparison filters to anisotropic ones.\n\n'
                     'These are mostly used for shadow maps.\n\n'
                     'auto defaults to true.')}),

            ('ModifyMinMax', 'combo', BOOL3,
             {'zh': ('修改 min/max 采样器',
                     '把 min/max 过滤器也改成各向异性版本。\n\n'
                     'auto 默认为 true。'),
              'en': ('Modify Min/Max Filters',
                     'Changes min/max filters to anisotropic ones.\n\n'
                     'auto defaults to true.')}),

            ('SkipPointFilter', 'combo', BOOL3,
             {'zh': ('跳过点采样过滤器',
                     '不改动点采样（point/nearest）过滤器。\n\n'
                     '点采样常用于像素风纹理、查找表和后处理，'
                     '改成各向异性会破坏画面，建议保持开启。\n\n'
                     'auto 默认为 true。'),
              'en': ('Skip Point Filter',
                     'Skips overriding point filters.\n\n'
                     'Point sampling is used for pixel-art textures, lookup tables '
                     'and post-processing; converting it would break visuals. '
                     'Best left on.\n\nauto defaults to true.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'mipmap',
        'section': 'Mipmap',
        'name': {'zh': 'Mipmap LOD 偏置', 'en': 'Mipmap'},
        'fields': [
            ('MipmapBiasOverride', 'slider', (-15.0, 15.0, 0.1),
             {'zh': ('LOD 偏置覆盖值',
                     '覆盖纹理的 Mipmap LOD 偏置。\n\n'
                     '上采样时游戏按低分辨率选 mip 级别，纹理会偏模糊。'
                     '设成负值（例如 -1.0）让它选更高清的 mip，画面更锐利。\n'
                     '过负会引入纹理闪烁（aliasing）。\n\n'
                     'auto 表示不覆盖。'),
              'en': ('Mipmap Bias Override',
                     'Override value for mipmap LOD bias.\n\n'
                     'When upscaling, the game picks mip levels for the lower render '
                     'resolution, so textures look soft. A negative value (e.g. -1.0) '
                     'pulls in sharper mips. Too negative causes texture shimmering.\n\n'
                     'auto means disabled.')}),

            ('MipmapBiasFixedOverride', 'combo', BOOL3,
             {'zh': ('使用固定 LOD 偏置',
                     '直接把上面的值当作固定偏置使用，而不是按分辨率比例计算。\n\n'
                     'auto 默认为 false。'),
              'en': ('Fixed LOD Bias',
                     'Uses the value above as a fixed LOD bias.\n\n'
                     'auto defaults to false.')}),

            ('MipmapBiasScaleOverride', 'combo', BOOL3,
             {'zh': ('偏置值作为倍率',
                     '把上面的值当作缩放倍率，而不是绝对偏置量。\n\n'
                     'auto 默认为 false。'),
              'en': ('Bias as Scale Multiplier',
                     'Uses the override value as a scale multiplier.\n\n'
                     'auto defaults to false.')}),

            ('MipmapBiasOverrideAll', 'combo', BOOL3,
             {'zh': ('覆盖所有纹理',
                     '覆盖全部纹理的 LOD 偏置。\n\n'
                     '默认只覆盖偏置本来是负值的纹理；开启后连偏置为 0 的也一起改。\n\n'
                     'auto 默认为 false。'),
              'en': ('Override All Textures',
                     'Overrides LOD bias for all textures.\n\n'
                     'By default only textures with a negative bias are touched.\n\n'
                     'auto defaults to false.')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'processfilter',
        'section': 'ProcessFilter',
        'name': {'zh': '进程过滤 (Process Filter)', 'en': 'Process Filter'},
        'fields': [
            ('TargetProcessName', 'entry', None,
             {'zh': ('目标进程名',
                     '只注入到这个指定的进程（不区分大小写）。\n\n'
                     '例如：endfield.exe\n\n'
                     '适用于启动器和游戏本体同目录、需要精确指定的情况。\n\n'
                     'auto 表示注入所有进程。'),
              'en': ('Target Process Name',
                     'Only inject into this specific process (case-insensitive).\n\n'
                     'Example: endfield.exe\n\n'
                     'auto means disabled (injects into all processes).')}),

            ('ProcessExclusionList', 'entry', None,
             {'zh': ('进程排除列表',
                     '不注入到这些进程（不区分大小写），多个名称用竖线 | 分隔。\n\n'
                     '例如：crashpad_handler.exe|launcher.exe\n\n'
                     'auto 表示使用内置默认列表'
                     '（含 crashpad_handler.exe 等，见 Config.h 的 ProcessFilter）。'),
              'en': ('Process Exclusion List',
                     'Do not inject into these processes (case-insensitive), '
                     'separated with a pipe "|".\n\n'
                     'Example: crashpad_handler.exe|launcher.exe\n\n'
                     'auto uses the built-in default list (crashpad_handler.exe and '
                     'others, listed in Config.h under ProcessFilter).')}),
        ],
    },

    # ------------------------------------------------------------------ #
    {
        'id': 'hotfix',
        'section': 'Hotfix',
        'name': {'zh': '兼容性修复 (Hotfix)', 'en': 'Hotfix'},
        'fields': [
            ('CheckForUpdate', 'combo', BOOL3,
             {'zh': ('检查更新',
                     '启动时从 GitHub 检查是否有新版本。\n\n'
                     'auto 默认为 true。'),
              'en': ('Check For Update',
                     'Checks GitHub for the latest version.\n\n'
                     'auto defaults to true.')}),

            ('DisableOverlays', 'combo', BOOL3,
             {'zh': ('屏蔽 Steam / Epic 覆盖层',
                     '阻止 Steam 和 Epic 的游戏内覆盖层加载，同时也会屏蔽 Steam Input。\n\n'
                     '覆盖层会挂钩 Present，与帧生成冲突。\n\n'
                     'auto 默认为 false，但启用 OptiFG 时默认为 true。'),
              'en': ('Disable Overlays',
                     'Blocks the Steam and Epic overlays. Also blocks Steam Input.\n\n'
                     'Overlays hook Present and conflict with frame generation.\n\n'
                     'auto defaults to false, except for OptiFG where it is true.')}),

            ('ManualInputPolling', 'combo', BOOL3,
             {'zh': ('手动轮询输入',
                     '用手动轮询代替挂钩 WndProc 来读取输入。\n\n'
                     '对无法正常捕获输入的游戏有帮助。'
                     '代价是菜单打开时无法屏蔽输入，'
                     '也就是操作会同时传给游戏。\n\n'
                     'auto 默认为 false。'),
              'en': ('Manual Input Polling',
                     'Uses manual input polling instead of hooking WndProc.\n\n'
                     'Helps games that do not capture inputs properly. Downside: '
                     'inputs can no longer be blocked while the menu is open.\n\n'
                     'auto defaults to false.')}),

            ('SimulateWaitableObject', 'combo', BOOL3,
             {'zh': ('模拟可等待对象信号',
                     '为帧生成交换链模拟 waitable object 的事件信号。\n\n'
                     '用于游戏依赖该信号做帧节奏控制、但 FG 交换链没提供的情况。\n\n'
                     'auto 默认为 false。'),
              'en': ('Simulate Waitable Object',
                     'Simulates waitable object event signals for the FG swapchain.\n\n'
                     'auto defaults to false.')}),

            ('CreateD3D12DeviceForLuma', 'combo', BOOL3,
             {'zh': ('为 Luma 预建 D3D12 设备',
                     '延后加载 ReShade，先创建 D3D12 设备。\n\n'
                     '可避免 Luma 及部分其他 ReShade mod 的警告或崩溃。'
                     '使用 Luma mod 时应开启。\n\n'
                     'auto 默认为 false。'),
              'en': ('Create D3D12 Device For Luma',
                     'Delays loading ReShade and creates the D3D12 device first.\n\n'
                     'Prevents warnings and crashes with Luma and some other ReShade '
                     'mods. Enable this when using the Luma mod.\n\n'
                     'auto defaults to false.')}),

            ('PreferDedicatedGpu', 'combo', BOOL3,
             {'zh': ('优先使用独立显卡',
                     '尝试强制使用高性能 GPU。\n\n'
                     '用于双显卡笔记本误用集显的情况。\n\n'
                     'auto 默认为 false。'),
              'en': ('Prefer Dedicated GPU',
                     'Tries to force the High Performance GPU.\n\n'
                     'auto defaults to false.')}),

            ('PreferFirstDedicatedGpu', 'combo', BOOL3,
             {'zh': ('只上报第一块独立显卡',
                     '只向游戏上报第一块高性能 GPU。\n\n'
                     '多独显机器上，游戏枚举到多张卡可能选错，本项可强制其只看到一张。\n\n'
                     'auto 默认为 false。'),
              'en': ('Prefer First Dedicated GPU',
                     'Reports only the first High Performance GPU.\n\n'
                     'Useful on multi-dGPU machines where the game picks the wrong one.\n\n'
                     'auto defaults to false.')}),

            ('RoundInternalResolution', 'combo', ['auto', '2', '4', '8', '16', '32'],
             {'zh': ('内部分辨率取整',
                     '把内部渲染分辨率的宽高取整为该值的倍数。\n\n'
                     '某些游戏或上采样器要求分辨率对齐，否则出现画面错位或崩溃。\n\n'
                     'auto 表示不取整。'),
              'en': ('Round Internal Resolution',
                     'Rounds internal resolution width and height to a multiple of '
                     'this value.\n\n'
                     'Some games or upscalers require aligned resolutions.\n\n'
                     'auto means disabled.')}),

            ('SkipFirstFrames', 'slider', (0, 120, 1),
             {'zh': ('跳过前 N 帧上采样',
                     '开头的 N 帧不做上采样。\n\n'
                     '用于绕过某些游戏在初始化阶段资源还没就绪导致的崩溃或花屏。\n\n'
                     'auto 表示不跳过。'),
              'en': ('Skip First Frames',
                     'Skips upscaling for n frames.\n\n'
                     'Works around crashes or corruption caused by resources not '
                     'being ready during startup.\n\n'
                     'auto means disabled.')}),

            ('RestoreComputeSignature', 'combo', BOOL3,
             {'zh': ('还原计算根签名',
                     '上采样结束后恢复游戏上一次使用的 compute 根签名。\n\n'
                     '若上采样后画面或特效异常，可尝试开启。\n\n'
                     'auto 默认为 false。'),
              'en': ('Restore Compute Signature',
                     'Restores the last used compute signature after upscaling.\n\n'
                     'Try this if visuals break right after upscaling.\n\n'
                     'auto defaults to false.')}),

            ('RestoreGraphicSignature', 'combo', BOOL3,
             {'zh': ('还原图形根签名',
                     '上采样结束后恢复游戏上一次使用的 graphics 根签名。\n\n'
                     'auto 默认为 false。'),
              'en': ('Restore Graphics Signature',
                     'Restores the last used graphics signature after upscaling.\n\n'
                     'auto defaults to false.')}),

            ('ExtendedStateRestore', 'combo', BOOL3,
             {'zh': ('扩展状态还原',
                     '追踪根签名的变化以便完整还原。\n\n'
                     '对使用 bindless 资源绑定的游戏有用。开销略高。\n\n'
                     'auto 默认为 false。'),
              'en': ('Extended State Restore',
                     'Tracks signature changes to fully restore them.\n\n'
                     'Useful for games using bindless. Slightly more overhead.\n\n'
                     'auto defaults to false.')}),

            ('UsePrecompiledShaders', 'combo', BOOL3,
             {'zh': ('使用预编译着色器',
                     '为 RCAS、输出缩放和 Mask Bias 使用预编译着色器。\n\n'
                     '关闭会改为运行时编译，启动更慢，仅在预编译版本有问题时才关。\n\n'
                     'auto 默认为 true。'),
              'en': ('Use Precompiled Shaders',
                     'Uses precompiled shaders for RCAS, Output Scaling and Mask Bias.\n\n'
                     'Turning it off falls back to runtime compilation (slower startup).\n\n'
                     'auto defaults to true.')}),

            ('ColorResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('颜色资源状态',
                     '为颜色贴图指定初始资源状态并插入资源屏障。\n\n'
                     '修复 AMD 显卡上的彩虹色/花屏（多见于 UE 引擎游戏）。\n'
                     'UE 游戏在 AMD 上建议设为 4（RENDER_TARGET）。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Color Resource Barrier',
                     'Sets the initial resource state for the color texture and adds '
                     'a resource barrier.\n\n'
                     'Fixes rainbow colors on AMD cards (mostly UE games). '
                     'For UE games on AMD set this to 4 (RENDER_TARGET).\n\n'
                     'auto means state correction disabled.')}),

            ('MotionVectorResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('运动矢量资源状态',
                     '为运动矢量贴图指定初始资源状态并插入资源屏障。\n\n'
                     'UE 游戏在 AMD 上建议设为 8（UNORDERED_ACCESS）。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Motion Vector Resource Barrier',
                     'Sets the initial resource state for the motion vector texture.\n\n'
                     'For UE games on AMD set this to 8 (UNORDERED_ACCESS).\n\n'
                     'auto means state correction disabled.')}),

            ('DepthResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('深度资源状态',
                     '为深度缓冲指定初始资源状态并插入资源屏障。\n\n'
                     '常用值：16（DEPTH_WRITE）、32（DEPTH_READ）。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Depth Resource Barrier',
                     'Sets the initial resource state for the depth buffer.\n\n'
                     'Common values: 16 (DEPTH_WRITE), 32 (DEPTH_READ).\n\n'
                     'auto means state correction disabled.')}),

            ('ColorMaskResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('颜色遮罩资源状态',
                     '为颜色遮罩贴图指定初始资源状态并插入资源屏障。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Color Mask Resource Barrier',
                     'Sets the initial resource state for the color mask texture.\n\n'
                     'auto means state correction disabled.')}),

            ('ExposureResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('曝光资源状态',
                     '为曝光贴图指定初始资源状态并插入资源屏障。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Exposure Resource Barrier',
                     'Sets the initial resource state for the exposure texture.\n\n'
                     'auto means state correction disabled.')}),

            ('OutputResourceBarrier', 'combo', RESOURCE_STATES,
             {'zh': ('输出资源状态',
                     '为输出贴图指定初始资源状态并插入资源屏障。\n\n'
                     'auto 表示不做状态纠正。'),
              'en': ('Output Resource Barrier',
                     'Sets the initial resource state for the output texture.\n\n'
                     'auto means state correction disabled.')}),
        ],
    },
]


def module_by_id(mid):
    for m in MODULES:
        if m['id'] == mid:
            return m
    return None
