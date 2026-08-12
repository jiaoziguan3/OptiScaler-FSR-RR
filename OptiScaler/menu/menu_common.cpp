#include "pch.h"
#include "menu_common.h"

#include "input/input_system.h"

#include "font/Hack_Compressed.h"

#include <proxies/XeSS_Proxy.h>
#include <proxies/XeFG_Proxy.h>
#include <proxies/FfxApi_Proxy.h>
#include <proxies/Streamline_Proxy.h>

#include <framegen/nvngx/Nvngx_FG.h>

#include <nvapi/fakenvapi.h>
#include <hooks/Reflex_Hooks.h>

#include <version_check.h>
#include <upscaler_time/UpscalerTime_Vk.h>
#include <upscaler_time/UpscalerTime_Dx11.h>
#include <upscaler_time/UpscalerTime_Dx12.h>

#include <imgui/imgui_internal.h>
#include <imgui/ImGuiNotify.hpp>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_uwp.h>

#include <mutex>
#include <cstdarg>

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <type_traits>
#include <misc/IdentifyGpu.h>
#include <hooks/Xell_Hooks.h>
#include <low_latency/input/input_common.h>

#define MARK_ALL_BACKENDS_CHANGED()                                                                                    \
    for (auto& singleChangeBackend : State::Instance().changeBackend)                                                  \
        singleChangeBackend.second = true;

static float fontSize = 14.0f; // just changing this doesn't make other elements scale ideally
static ImVec2 overlaySize(0.0f, 0.0f);
static ImVec2 overlayPosition(-1000.0f, -1000.0f);
static int _activeSection = 0;
static bool _hdrTonemapApplied = false;
static ImVec4 SdrColors[ImGuiCol_COUNT];

// Low-level keyboard hook — catches shortcut keys even when games steal input
static HHOOK _llKeyboardHook = nullptr;
static std::atomic<USHORT> _llKeyStates[256] {};

static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && wParam == WM_KEYDOWN && lParam != 0)
    {
        auto* ks = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (ks->vkCode < 256)
            _llKeyStates[ks->vkCode].store(0x8000, std::memory_order_relaxed);
    }
    else if (code == HC_ACTION && wParam == WM_KEYUP && lParam != 0)
    {
        auto* ks = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (ks->vkCode < 256)
            _llKeyStates[ks->vkCode].store(0, std::memory_order_relaxed);
    }
    return CallNextHookEx(_llKeyboardHook, code, wParam, lParam);
}

// CJK Unified Ideographs range for Chinese text rendering
static const ImWchar* GetCJKGlyphRanges()
{
    static const ImWchar ranges[] =
    {
        0x4E00, 0x9FFF, // CJK Unified Ideographs
        0x3000, 0x303F, // CJK Symbols and Punctuation
        0x2E80, 0x2EFF, // CJK Radicals Supplement
        0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
        0,
    };
    return ranges;
}

static bool inputMenu = false;
static bool inputFG = false;
static bool inputFps = false;
static bool inputFpsCycle = false;
static uint64_t lastInputTick = 0;
constexpr uint64_t debounceThreshold = 1000;

static bool hasGamepad = false;
static bool ffxInitTried = false;
static bool xefgInitTried = false;
static std::string windowTitle;
static std::string selectedUpscalerName = "";
static Upscaler currentBackend = Upscaler::Reset;
static std::string currentBackendName = "";
static int refreshRate = 0;
static ImVec2 lastPosition(-1000.0f, -1000.0f);

static ImVec2 splashPosition(-1000.0f, -1000.0f);
static ImVec2 splashSize(0.0f, 0.0f);
static double splashStart = 0.0;
static double splashLimit = 0.0;
static std::vector<std::string> splashText = { Translation::Get("Cope smarter, not harder"),
                                               Translation::Get("Coping is strong with this one..."),
                                               Translation::Get("This is where the fun begins..."),
                                               Translation::Get("Got any more of them scalers?..."),
                                               Translation::Get("Fake pixels and even faker frames..."),
                                               Translation::Get("Fake frames, get your fake frames..."),
                                               Translation::Get("I'm here to kick pixels and chew frames..."),
                                               Translation::Get("I find your lack of supersampling disturbing..."),
                                               Translation::Get("Frame by frame, I scale-up!"),
                                               Translation::Get("Resistance is futile. Your pixels will be upscaled."),
                                               Translation::Get("I've got 99 problems, but low-res ain't one."),
                                               Translation::Get("It's over, DLSS, I have the higher ground!"),
                                               Translation::Get("This isn't the resolution you're looking for."),
                                               Translation::Get("To infinity and beyond... with ray tracing off."),
                                               Translation::Get("I have a bad feeling about this frame pacing."),
                                               Translation::Get("It's Dangerous to Go Alone-Take This Upscaler"),
                                               Translation::Get("Upscaled beyond recognition."),
                                               Translation::Get("Trust the process. Ignore the shimmer."),
                                               Translation::Get("Real fake frames. Certified."),
                                               Translation::Get("The illusion of performance, perfected."),
                                               Translation::Get("This upscaler belongs in a museum!"),
                                               Translation::Get("Because native rendering is overrated."),
                                               Translation::Get("The more you upscaler, the more you save"),
                                               Translation::Get("It's never too late to buy a better GPU"),
                                               Translation::Get("We don't need real pixels where we're going"),
                                               Translation::Get("Did you know that Intel released XeFG for everyone?"),
                                               Translation::Get("MFG totally works with Nukem's 100%% no scam"),
                                               Translation::Get("Some of those pixels might even be real!"),
                                               Translation::Get("Just don't look too closely at the image"),
                                               Translation::Get("Even supports \"software\" XeSS!"),
                                               Translation::Get("It's too blurry to go alone, take RCAS with you"),
                                               Translation::Get("Thanks nitec, back to you nitec"),
                                               Translation::Get("Tested and approved by By-U"),
                                               Translation::Get("0.8 was an inside job"),
                                               Translation::Get("FSR4 DP4a wenETA, AMD plz"),
                                               Translation::Get("OptiCopers, assemble!"),
                                               Translation::Get("The Way It's Meant To Be Upscaled"),
                                               Translation::Get("Your game may not even crash today"),
                                               Translation::Get("Expanded and Enhanced"),
                                               Translation::Get("It's only my 5th crash today"),
                                               Translation::Get("Latency with FG? But I have good internet"),
                                               Translation::Get("Console peasants can't do that"),
                                               Translation::Get("Hope you don't have a good eyesight"),
                                               Translation::Get("Such an aggressive upscaling? A bold move"),
                                               Translation::Get("I almost don't feel the input lag"),
                                               Translation::Get("And that's how you get to 60 FPS"),
                                               Translation::Get("Together We Upscale"),
                                               Translation::Get("For upscalers, by upscalers"),
                                               Translation::Get("Opti Sports, it's in the sampling"),
                                               Translation::Get("Render in your world. Upscale in ours"),
                                               Translation::Get("All your pixels are belong to us"),
                                               Translation::Get("Upscaling for the masses, not the classes"),
                                               Translation::Get("Generating discord since 2023"),
                                               Translation::Get("Enabling DLSS since 2023"),
                                               Translation::Get("[REDACTED] never looked better"),
                                               Translation::Get("Free and always free"),
                                               Translation::Get("Getting unshackled from green chains in progress..."),
                                               Translation::Get("Who's Nukem anyway?"),
                                               Translation::Get("Compiling shaders... ETA: 05h:49m"),
                                               Translation::Get("Did you really just pay 70 EUR for this game?!"),
                                               Translation::Get("Guess who forgot about a nullptr check again"),
                                               Translation::Get("AI can't outslop this"),
                                               Translation::Get("Guess we're pre-alpha build demos now"),
                                               Translation::Get("New app on the block - TH"),
                                               Translation::Get("One more stutter and I might lose it"),
                                               Translation::Get("Mostly stable, unlike the driver"),
                                               Translation::Get("Vul... what? ~AMD"),
                                               Translation::Get("My 8 points are floating"),
                                               Translation::Get("No floating here - I'm strictly between -128 and 127"),
                                               Translation::Get("Fake it til you bake it"),
                                               Translation::Get("Worst case just turn it off and on"),
                                               Translation::Get("*On a generative damage control mode at geometry level*"),
                                               Translation::Get("Deep Learning Slop Sampling 5"),
                                               Translation::Get("2D AI filters, now powered by just 2x 5090s"),
                                               Translation::Get("Neural Slop Sampling with DLSS5"),
                                               Translation::Get("DLSS 5 - the way it's meant to be slopped"),
                                               Translation::Get("Just when I think I'm out, they scale me back in"),
                                               Translation::Get("Like going in the first gear on the highway"),
                                               Translation::Get("Nitec's Bizarre Upscaling"),
                                               Translation::Get("\"Framegen really attracts some strange clientelle\""),
                                               Translation::Get("How to remove those corny messages?!"),
                                               Translation::Get("<Your funny text goes here>") };

static std::string updateNoticeTag;
static std::string updateNoticeUrl;
static float lastMenuScale = 0.0f;
static CustomOptional<uint32_t> comboPreset { 0 };
static int lastKey = 0;
static bool capturingKey = false;

template <typename T, size_t N> struct RingBuffer
{
    std::array<T, N> data {};
    size_t head { 0 };
    size_t count { N };
    double sum { 0.0 };

    RingBuffer() { data.fill(static_cast<T>(0)); }

    void Push(T v)
    {
        if (count == N)
        {
            sum -= data[head];
        }
        else
        {
            ++count;
        }
        data[head] = v;
        sum += v;
        head = (head + 1) % N;
    }

    size_t Size() const { return N; }

    T At(size_t i) const
    {
        size_t start = head;
        return data[(start + i) % N];
    }

    float Average() const { return static_cast<float>(sum / static_cast<double>(N)); }
};

const int plotWidth = 360;
static RingBuffer<float, plotWidth> gFrameTimes;
static RingBuffer<float, plotWidth> gUpscalerTimes;

struct FsExistsCache
{
    std::wstring lastPath;
    bool cached { false };
    std::chrono::steady_clock::time_point nextRefresh { std::chrono::steady_clock::time_point::min() };
    std::chrono::milliseconds interval { 2000 };

    bool Get(const std::filesystem::path& path)
    {
        auto now = std::chrono::steady_clock::now();
        if (path != lastPath || now >= nextRefresh)
        {
            lastPath = path;
            cached = std::filesystem::exists(path);
            nextRefresh = now + interval;
        }
        return cached;
    }
};

static FsExistsCache nukemsExists;
static FsExistsCache enablerExists;

struct FlagDefinition
{
    std::string name;
    uint32_t mask;
    std::string description;
};

inline std::string StrFmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = std::vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    std::string out(len, '\0');
    va_start(args, fmt);
    std::vsnprintf(out.data(), len + 1, fmt, args);
    va_end(args);
    return out;
}

void MenuCommon::UpdateManualInput(HWND targetHwnd)
{
    OptiInput::BeginFrame(targetHwnd);

    const auto config = Config::Instance();

    auto CheckShortcut = [&](int vk, bool& inputFlag, const char* logMessage)
    {
        if (inputFlag)
            return;

        if (vk <= 0 || vk >= 256)
            return;

        if (OptiInput::IsKeyReleased(vk))
        {
            lastKey = vk;
            // receivingWmInputs = false;
            inputFlag = true;
            LOG_DEBUG("{}", logMessage);
        }
    };

    const auto currentTick = GetTickCount64();
    const bool canAcceptInputs = lastInputTick + debounceThreshold < currentTick;

    if (!capturingKey && canAcceptInputs)
    {
        CheckShortcut(config->ShortcutKey.value_or_default(), inputMenu, "Menu key pressed, will be switching menu");
        CheckShortcut(config->FpsShortcutKey.value_or_default(), inputFps, "Menu key pressed, will be switching FPS");
        CheckShortcut(config->FGShortcutKey.value_or_default(), inputFG, "Menu key pressed, will be switching FG mode");
        CheckShortcut(config->FpsCycleShortcutKey.value_or_default(), inputFpsCycle,
                      "Menu key pressed, will be switching FPS mode");
    }
    else if (capturingKey)
    {
        lastInputTick = currentTick;
    }

    lastKey = OptiInput::GetLastPressedKey();
}

void MenuCommon::ShowTooltip(const char* tip)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::BeginTooltip();
        ImGui::Text(tip);
        ImGui::EndTooltip();
    }
}

void MenuCommon::ShowHelpMarker(const char* tip)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    ShowTooltip(tip);
}

void MenuCommon::ShowResetButton(CustomOptional<bool, NoDefault>* initFlag, std::string buttonName)
{
    ImGui::SameLine();

    ImGui::BeginDisabled(!initFlag->has_value());

    if (ImGui::Button(buttonName.c_str()))
    {
        initFlag->reset();
        ReInitUpscaler();
    }

    ImGui::EndDisabled();
}

inline void MenuCommon::ReInitUpscaler()
{
    if (!State::Instance().currentFeature)
        return;

    if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSSD)
        State::Instance().newBackend = Upscaler::DLSSD;
    else
        State::Instance().newBackend = currentBackend;

    MARK_ALL_BACKENDS_CHANGED();
}

void MenuCommon::SeparatorWithHelpMarker(const char* label, const char* tip)
{
    auto marker = "(?) ";
    ImGui::SeparatorTextEx(0, label, ImGui::FindRenderedTextEnd(label),
                           ImGui::CalcTextSize(marker, ImGui::FindRenderedTextEnd(marker)).x);
    ShowHelpMarker(tip);
}

class Keybind
{
    std::string name;
    int id;
    bool waitingForKey = false;

  public:
    Keybind(std::string name, int id) : name(name), id(id) {}

    static std::string KeyNameFromVirtualKeyCode(USHORT virtualKey)
    {
        if (virtualKey == (USHORT) UnboundKey)
            return "Unbound";

        UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);

        // Keys like Home would display as Num 0 without this fix
        switch (virtualKey)
        {
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_NUMLOCK:
        case VK_DIVIDE:
        case VK_RCONTROL:
        case VK_RMENU:
            scanCode |= 0xE000;
            break;
        }

        LONG lParam = (scanCode & 0xFF) << 16;
        if (scanCode & 0xE000)
            lParam |= 1 << 24;

        wchar_t buf[64] = {};
        if (GetKeyNameTextW(lParam, buf, static_cast<int>(std::size(buf))) != 0)
            return wstring_to_string(buf);

        return "Unknown";
    }

    void Render(CustomOptional<int>& configKey)
    {
        ImGui::PushID(id);
        if (ImGui::Button(name.c_str()))
        {
            waitingForKey = true;
            capturingKey = true;
            lastKey = 0;
        }
        ImGui::PopID();

        if (waitingForKey)
        {
            ImGui::SameLine();
            ImGui::Text(Translation::Get("Press any key..."));

            if (lastKey == 0 || lastKey == VK_LBUTTON || lastKey == VK_RBUTTON || lastKey == VK_MBUTTON)
                return;

            if (lastKey == VK_ESCAPE)
            {
                waitingForKey = false;
                capturingKey = false;
                return;
            }

            if (lastKey == VK_BACK)
                lastKey = UnboundKey;

            configKey = lastKey;
            waitingForKey = false;
            capturingKey = false;
            return;
        }

        ImGui::SameLine();
        ImGui::Text(KeyNameFromVirtualKeyCode(configKey.value_or_default()).c_str());

        ImGui::SameLine();
        ImGui::PushID(id);
        if (ImGui::Button(Translation::Get("R")))
        {
            configKey.reset();
        }
        ImGui::PopID();
    }
};

Upscaler MenuCommon::GetBackendCode(const API api)
{
    if (auto feature = State::Instance().currentFeature)
        return feature->GetUpscalerType();

    Upscaler upscaler;

    if (api == DX11)
        upscaler = Config::Instance()->Dx11Upscaler.value_or_default();
    else if (api == DX12)
        upscaler = Config::Instance()->Dx12Upscaler.value_or_default();
    else
        upscaler = Config::Instance()->VulkanUpscaler.value_or_default();

    return upscaler;
}

void MenuCommon::GetCurrentBackendInfo(const API api, Upscaler& upscaler, std::string* name)
{
    upscaler = GetBackendCode(api);
    *name = UpscalerDisplayName(upscaler, api);
}

void MenuCommon::RenderUpscalerCombo(const API api, Upscaler currentUpscaler, const std::vector<Upscaler>& options)
{
    auto primaryGpu = IdentifyGpu::getPrimaryGpu();

    // Determine display name
    Upscaler targetBackend = State::Instance().newBackend;
    if (targetBackend == Upscaler::Reset)
        targetBackend = currentUpscaler;

    std::string selectedName = UpscalerDisplayName(targetBackend, api);

    if (ImGui::BeginCombo("##UpscalerCombo", selectedName.c_str()))
    {
        for (auto opt : options)
        {
            // Check if GPU is capable of a given backend
            if (opt == Upscaler::DLSS && !primaryGpu.dlssCapable)
                continue;

            // Not all Intel GPUs support native DX11 XeSS but don't think we have a good way to check exactly
            if (opt == Upscaler::XeSS && api == API::DX11 && primaryGpu.vendorId != VendorId::Intel)
                continue;

            bool isSelected = (currentUpscaler == opt);
            if (ImGui::Selectable(UpscalerDisplayName(opt, api).c_str(), isSelected))
            {
                State::Instance().newBackend = opt;
            }
        }
        ImGui::EndCombo();
    }
}

void MenuCommon::AddDx11Backends(Upscaler upscaler)
{
    RenderUpscalerCombo(API::DX11, upscaler,
                        { Upscaler::XeSS, Upscaler::FSR22, Upscaler::FSR31, Upscaler::XeSS_on12, Upscaler::FSR21_on12,
                          Upscaler::FSR22_on12, Upscaler::FFX_on12, Upscaler::DLSS });
}

void MenuCommon::AddDx12Backends(Upscaler upscaler)
{
    RenderUpscalerCombo(API::DX12, upscaler,
                        { Upscaler::XeSS, Upscaler::FSR21, Upscaler::FSR22, Upscaler::FFX, Upscaler::DLSS });
}

void MenuCommon::AddVulkanBackends(Upscaler upscaler)
{
    RenderUpscalerCombo(API::Vulkan, upscaler,
                        { Upscaler::XeSS, Upscaler::FSR21, Upscaler::FSR22, Upscaler::FFX, Upscaler::FSR21_on12,
                          Upscaler::FFX_on12, Upscaler::DLSS });
}

template <HasDefaultValue B> void MenuCommon::AddResourceBarrier(std::string name, CustomOptional<int32_t, B>* value)
{
    const char* states[] = { "AUTO",
                             "COMMON",
                             "VERTEX_AND_CONSTANT_BUFFER",
                             "INDEX_BUFFER",
                             "RENDER_TARGET",
                             "UNORDERED_ACCESS",
                             "DEPTH_WRITE",
                             "DEPTH_READ",
                             "NON_PIXEL_SHADER_RESOURCE",
                             "PIXEL_SHADER_RESOURCE",
                             "STREAM_OUT",
                             "INDIRECT_ARGUMENT",
                             "COPY_DEST",
                             "COPY_SOURCE",
                             "RESOLVE_DEST",
                             "RESOLVE_SOURCE",
                             "RAYTRACING_ACCELERATION_STRUCTURE",
                             "SHADING_RATE_SOURCE",
                             "GENERIC_READ",
                             "ALL_SHADER_RESOURCE",
                             "PRESENT",
                             "PREDICATION",
                             "VIDEO_DECODE_READ",
                             "VIDEO_DECODE_WRITE",
                             "VIDEO_PROCESS_READ",
                             "VIDEO_PROCESS_WRITE",
                             "VIDEO_ENCODE_READ",
                             "VIDEO_ENCODE_WRITE" };
    const int values[] = { -1,  0,   1,     2,      4,      8,      16,      32,       64,   128,
                           256, 512, 1024,  2048,   4096,   8192,   4194304, 16777216, 2755, 192,
                           0,   310, 65536, 131072, 262144, 524288, 2097152, 8388608 };

    int selected = value->value_or(-1);

    const char* selectedName = "";

    for (int n = 0; n < 28; n++)
    {
        if (values[n] == selected)
        {
            selectedName = states[n];
            break;
        }
    }

    if (ImGui::BeginCombo(name.c_str(), selectedName))
    {
        if (ImGui::Selectable(states[0], !value->has_value()))
            value->reset();

        for (int n = 1; n < 28; n++)
        {
            if (ImGui::Selectable(states[n], selected == values[n]))
                *value = values[n];
        }

        ImGui::EndCombo();
    }
}

static uint32_t GetPresetIndex(IFeature* feature, bool dlssd = false)
{
    auto ratio = (float) feature->TargetWidth() / (float) feature->RenderWidth();

    if (!dlssd)
    {
        if (State::Instance().dlssPresetsOverridenByOpti)
        {
            LOG_DEBUG("DLSS Presets overridden by Opti, using Opti preset indices with ratio: {}", ratio);

            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetUltraPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetBalanced.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetQuality.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetUltraQuality.value_or_default());
            }
            else
            {
                return Config::Instance()->RenderPresetForAll.value_or(
                    Config::Instance()->RenderPresetDLAA.value_or_default());
            }
        }
        else if (State::Instance().dlssPresetsOverriddenExternally)
        {
            LOG_DEBUG("DLSS Presets overridden externally, using external preset index: {}",
                      State::Instance().dlssRenderPresetExternal);

            return State::Instance().dlssRenderPresetExternal;
        }
        else
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetUltraPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetBalanced;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetQuality;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssRenderPresetUltraQuality;
            }
            else
            {
                return State::Instance().dlssRenderPresetDLAA;
            }
        }
    }
    else
    {
        if (State::Instance().dlssdPresetsOverridenByOpti)
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetUltraPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetPerformance.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetBalanced.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetQuality.value_or_default());
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetUltraQuality.value_or_default());
            }
            else
            {
                return Config::Instance()->DLSSDRenderPresetForAll.value_or(
                    Config::Instance()->DLSSDRenderPresetDLAA.value_or_default());
            }
        }
        else if (State::Instance().dlssdPresetsOverriddenExternally)
        {
            return State::Instance().dlssdRenderPresetExternal;
        }
        else
        {
            if (ratio <= (Config::Instance()->QualityRatio_UltraPerformance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetUltraPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Performance.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetPerformance;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Balanced.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetBalanced;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_Quality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetQuality;
            }
            else if (ratio <= (Config::Instance()->QualityRatio_UltraQuality.value_or_default() + 0.01f))
            {
                return State::Instance().dlssdRenderPresetUltraQuality;
            }
            else
            {
                return State::Instance().dlssdRenderPresetDLAA;
            }
        }
    }

    return 0;
}

// TODO: disable presets based on the detected DLSS version
template <HasDefaultValue B> void MenuCommon::AddDLSSRenderPreset(std::string name, CustomOptional<uint32_t, B>* value)
{
    // clang-format off
    static const std::vector<MenuOption<uint32_t>> presets = {
        { NVSDK_NGX_DLSS_Hint_Render_Preset_Default, Translation::Get("DEFAULT"), 
            Translation::Get("Whatever the game uses") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_A, Translation::Get("PRESET A"),
            Translation::Get("Intended for Performance/Balanced/Quality modes.\nAn older variant best suited to combat ghosting...\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_B, Translation::Get("PRESET B"),
            Translation::Get("Intended for Ultra Performance mode.\nSimilar to Preset A...\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_C, Translation::Get("PRESET C"),
            Translation::Get("Intended for Performance/Balanced/Quality modes.\nGenerally favors current frame information...\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_D, Translation::Get("PRESET D"),
            Translation::Get("Default preset for Performance/Balanced/Quality modes;\ngenerally favors image stability.\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_E, Translation::Get("PRESET E"),
            Translation::Get("DLSS 3.7+, a better D preset\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_F, Translation::Get("PRESET F"),
            Translation::Get("Default preset for Ultra Performance and DLAA modes\nRemoved on recent versions!") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_G, Translation::Get("PRESET G"),
            Translation::Get("Unused") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_H_Reserved, Translation::Get("PRESET H"),
            Translation::Get("Unused") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_I_Reserved, Translation::Get("PRESET I"),
            Translation::Get("Unused") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_J, Translation::Get("PRESET J"),
            Translation::Get("Similar to preset K. Preset J might exhibit slightly\nless ghosting...\n1st Gen Transformer") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_K, Translation::Get("PRESET K"),
            Translation::Get("Default preset for DLAA/Balanced/Quality modes...\n1st Gen Transformer") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_L, Translation::Get("PRESET L"),
            Translation::Get("Default for Ultra Perf mode\n2nd Gen Transformers") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_M, Translation::Get("PRESET M"),
            Translation::Get("Default for Perf mode\n2nd Gen Transformer") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_N, Translation::Get("PRESET N"),
            Translation::Get("Unused") },
        { NVSDK_NGX_DLSS_Hint_Render_Preset_O, Translation::Get("PRESET O"),
            Translation::Get("Unused") },
        { NV_PRESET_LATEST, Translation::Get("Latest"),
            Translation::Get("Latest supported by the dll") }
    };
    // clang-format on

    PopulateCombo(name, *value, presets);
}

template <HasDefaultValue B> void MenuCommon::AddDLSSDRenderPreset(std::string name, CustomOptional<uint32_t, B>* value)
{
    // We don't have DLSSD definitions so using raw values
    static const std::vector<MenuOption<uint32_t>> presets = {
        { 0, Translation::Get("DEFAULT"), Translation::Get("Whatever the game uses") },
        { 1, Translation::Get("PRESET A"), Translation::Get("Preset A\nRemoved on recent versions!") },
        { 2, Translation::Get("PRESET B"), Translation::Get("Preset B\nRemoved on recent versions!") },
        { 3, Translation::Get("PRESET C"), Translation::Get("Preset C\nRemoved on recent versions!") },
        { 4, Translation::Get("PRESET D"), Translation::Get("Default model, Transformer") },
        { 5, Translation::Get("PRESET E"), Translation::Get("Latest Transformer model\nMust use if DoF guide is needed") },
        { NV_PRESET_LATEST, Translation::Get("Latest"), Translation::Get("Latest supported by the dll") }
    };

    PopulateCombo(name, *value, presets);
}

template <typename TStorage, typename T>
void MenuCommon::PopulateCombo(const std::string& name, TStorage& currentValue,
                               const std::vector<MenuOption<T>>& options)
{
    if (options.empty())
        return;

    // Assumes that different types mean that TStorage is std::optional
    T currentVal;
    if constexpr (std::is_same_v<TStorage, T>)
        currentVal = currentValue;
    else
        currentVal = currentValue.value_or(options[0].value);

    // Find the label for the currently selected item (translate at render time)
    const char* preview = "Unknown";
    for (const auto& opt : options)
    {
        if (opt.value == currentVal)
        {
            preview = Translation::Get(opt.label.c_str());
            break;
        }
    }

    if (ImGui::BeginCombo(name.c_str(), preview))
    {
        for (const auto& opt : options)
        {
            if (opt.disabled)
                ImGui::BeginDisabled();

            bool isSelected = (currentVal == opt.value);
            if (ImGui::Selectable(Translation::Get(opt.label.c_str()), isSelected))
                currentValue = opt.value;

            // Show tooltip for the individual item if it exists
            if (!opt.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", Translation::Get(opt.tooltip.c_str()));

            if (opt.disabled)
                ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
}

static ImVec4 toneMapColor(const ImVec4& color)
{
    if (State::Instance().isHdrActive ||
        (!Config::Instance()->OverlayMenu.value_or_default() && State::Instance().currentFeature != nullptr &&
         State::Instance().currentFeature->IsHdr()))
    {
        // Controls how strongly HDR/UI colors are pushed into the tone mapper before compression.
        // Higher values make colors brighter before mapping; lower values make the result dimmer.
        constexpr float exposure = 1.0f;

        // Blends between original color and fully tone-mapped color.
        // 0.0 = no tone mapping, 1.0 = full Reinhard compression.
        constexpr float strength = 1.0f;

        float peak = std::max(color.x, std::max(color.y, color.z));

        if (peak <= 0.0f)
            return color;

        float exposedPeak = peak * exposure;
        float mappedPeak = exposedPeak / (1.0f + exposedPeak);

        float reinhardScale = mappedPeak / peak;
        float scale = 1.0f + (reinhardScale - 1.0f) * strength;

        return ImVec4(color.x * scale, color.y * scale, color.z * scale, color.w);
    }

    return color;
}

static void MenuHdrCheck(ImGuiIO io)
{
    // If game is using HDR, apply tone mapping to the ImGui style
    if (State::Instance().isHdrActive ||
        (!Config::Instance()->OverlayMenu.value_or_default() && State::Instance().currentFeature != nullptr &&
         State::Instance().currentFeature->IsHdr()))
    {
        if (!_hdrTonemapApplied)
        {
            ImGuiStyle& style = ImGui::GetStyle();

            CopyMemory(SdrColors, style.Colors, sizeof(style.Colors));

            // Apply tone mapping to the ImGui style
            for (int i = 0; i < ImGuiCol_COUNT; ++i)
            {
                ImVec4 color = style.Colors[i];
                style.Colors[i] = toneMapColor(color);
            }

            _hdrTonemapApplied = true;
        }
    }
    else
    {
        if (_hdrTonemapApplied)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            CopyMemory(style.Colors, SdrColors, sizeof(style.Colors));
            _hdrTonemapApplied = false;
        }
    }
}

static float MenuResolutionScale(ImGuiIO io)
{
    if (Config::Instance()->MenuScale.has_value())
        return Config::Instance()->MenuScale.value();

    // Calculate menu scale according to display resolution
    float y = State::Instance().screenHeight;

    if (io.DisplaySize.y != 0)
        y = (float) io.DisplaySize.y;

    // 1000p is minimum for 1.0 menu ratio
    float result = (float) ((int) (y / 108.0f)) / 10.0f;

    result = std::round(result * 10.0f) / 10.0f;

    if (result < 0.5f)
        result = 0.5f;

    if (result > 2.0f)
        result = 2.0f;

    return result;
}

inline static std::string GetSourceString(UINT source)
{
    switch (source)
    {
    case 1:
        return "RTV";
    case 2:
        return "SRV";
    case 4:
        return "UAV";
    case 8:
        return "OM";
    case 16:
        return "Ups";
    case 32:
        return "SCR";
    case 64:
        return "SGR";
    default:
        return std::format("{}", source);
    }
}

inline static std::string GetDispatchString(UINT source)
{
    switch (source)
    {
    case 512:
        return "DI";
    case 1024:
        return "DII";
    case 256:
        return "Disp";
    default:
        return std::format("{}", source);
    }
}

static void ApplyThemeStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    auto conf = Config::Instance();
    bool lightTheme = conf->LightTheme.value_or_default();

    // Reset to unscaled defaults first: ScaleAllSizes() below multiplies in place,
    // so without this every call would compound the scale factor.
    style = ImGuiStyle();

    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 6.0f;

    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;

    style.FrameBorderSize = lightTheme ? 1.0f : 0.0f;
    style.TabBorderSize = lightTheme ? 1.0f : 0.0f;

    style.ScrollbarSize = 11.0f;
    style.GrabMinSize = 11.0f;

    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 5.0f);
    style.ItemSpacing = ImVec2(9.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(7.0f, 5.0f);
    style.IndentSpacing = 14.0f;
    style.SeparatorTextPadding = ImVec2(12.0f, 5.0f);
    style.SeparatorTextBorderSize = 1.0f;

    // Center window titles and keep control labels aligned with their widgets.
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    auto Clamp01 = [](float v) { return std::max(0.0f, std::min(v, 1.0f)); };

    auto Mix = [](const ImVec4& a, const ImVec4& b, float t, float alpha = 1.0f)
    { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, alpha); };

    auto Luminance = [](const ImVec4& c) { return c.x * 0.2126f + c.y * 0.7152f + c.z * 0.0722f; };

    auto Saturate = [&](const ImVec4& color, float amount)
    {
        float lum = Luminance(color);

        return ImVec4(Clamp01(lum + (color.x - lum) * amount), Clamp01(lum + (color.y - lum) * amount),
                      Clamp01(lum + (color.z - lum) * amount), color.w);
    };

    ImVec4 accent = ImVec4(conf->MenuAccentColorR.value_or_default(), conf->MenuAccentColorG.value_or_default(),
                           conf->MenuAccentColorB.value_or_default(), 1.0f);

    ImVec4 bgAccent = ImVec4(conf->MenuBGColorR.value_or_default(), conf->MenuBGColorG.value_or_default(),
                             conf->MenuBGColorB.value_or_default(), 1.0f);

    float luminance = Luminance(accent);

    // Widen the gap between surface levels so nested panels read as distinct
    // layers instead of one flat sheet.
    const ImVec4 bgDark = lightTheme ? ImVec4(0.82f, 0.84f, 0.88f, 1.00f) : ImVec4(0.07f, 0.08f, 0.09f, 1.00f);
    const ImVec4 bgMid = lightTheme ? ImVec4(0.91f, 0.93f, 0.96f, 1.00f) : ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    const ImVec4 bgLight = lightTheme ? ImVec4(0.98f, 0.98f, 1.00f, 1.00f) : ImVec4(0.17f, 0.18f, 0.21f, 1.00f);

    const ImVec4 textPrimary = lightTheme ? ImVec4(0.05f, 0.06f, 0.08f, 1.00f) : ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    const ImVec4 textDim = lightTheme ? ImVec4(0.32f, 0.35f, 0.41f, 1.00f) : ImVec4(0.64f, 0.68f, 0.74f, 1.00f);

    const ImVec4 borderCol = lightTheme ? ImVec4(0.40f, 0.45f, 0.55f, 1.00f) : ImVec4(0.28f, 0.30f, 0.34f, 1.00f);
    const ImVec4 dimBg = lightTheme ? ImVec4(0.30f, 0.33f, 0.38f, 0.20f) : ImVec4(0.09f, 0.10f, 0.13f, 0.20f);
    const ImVec4 modalDimBg = lightTheme ? ImVec4(0.22f, 0.24f, 0.28f, 0.55f) : ImVec4(0.04f, 0.04f, 0.07f, 0.55f);

    // MenuBGColor: only background/surface tint.
    auto BgTint = [&](const ImVec4& base, float strength = 1.0f, float alpha = 1.0f)
    {
        float t = lightTheme ? (0.180f * strength) : (0.120f * strength);
        return Mix(base, bgAccent, t, alpha);
    };

    // MenuAccentColor: all visible interactive accent colors.
    auto AccentSoft = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.14f, alpha) : Mix(bgDark, accent, 0.32f, alpha); };

    auto AccentMed = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.42f, alpha) : Mix(bgDark, accent, 0.55f, alpha); };

    auto AccentStrong = [&](float alpha = 1.0f) { return ImVec4(accent.x, accent.y, accent.z, alpha); };

    const ImVec4 bgTitle = AccentSoft();

    auto SurfaceHover = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.12f, alpha) : Mix(bgLight, accent, 0.18f, alpha); };

    auto SurfaceActive = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgLight, accent, 0.20f, alpha) : Mix(bgLight, accent, 0.28f, alpha); };

    auto TitleActive = [&](float alpha = 1.0f)
    { return lightTheme ? Mix(bgTitle, accent, 0.18f, alpha) : Mix(bgTitle, accent, 0.16f, alpha); };

    auto PlotAccent = [&](float alpha = 1.0f)
    {
        if (lightTheme)
        {
            // Darken slightly for contrast on light bg — no channel floors
            return Mix(accent, ImVec4(0.00f, 0.00f, 0.00f, 1.00f), 0.20f, alpha);
        }

        // Brighten slightly for visibility on dark bg — no channel floors
        return Mix(accent, ImVec4(1.00f, 1.00f, 1.00f, 1.00f), 0.35f, alpha);
    };

    auto PlotAccentHovered = [&](float alpha = 1.0f)
    {
        if (lightTheme)
        {
            return Mix(PlotAccent(alpha), ImVec4(0.00f, 0.00f, 0.00f, 1.00f), 0.15f, alpha);
        }

        return Mix(PlotAccent(alpha), ImVec4(1.00f, 1.00f, 1.00f, 1.00f), 0.25f, alpha);
    };

    auto AccentReadable = [&](float alpha = 1.0f)
    {
        // Apply saturation boost and luminance correction only here,
        // so AccentStrong / AccentMed / AccentSoft stay true to the user's pick.
        ImVec4 a = Saturate(accent, lightTheme ? 1.35f : 1.25f);
        float lum = Luminance(a);

        if (lightTheme && lum > 0.72f)
            a = Mix(a, ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 0.35f, 1.0f);

        if (!lightTheme && lum < 0.25f)
            a = Mix(a, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 0.30f, 1.0f);

        return ImVec4(a.x, a.y, a.z, alpha);
    };

    ImVec4* c = ImGui::GetStyle().Colors;

    float minAlpha = Config::Instance()->MenuBGColorA.value_or_default() >= 0.5f
                         ? Config::Instance()->MenuBGColorA.value_or_default()
                         : 0.5f;

    c[ImGuiCol_Text] = textPrimary;
    c[ImGuiCol_TextDisabled] = textDim;
    c[ImGuiCol_TextLink] = AccentReadable();

    // MenuBGColor only.
    c[ImGuiCol_WindowBg] = BgTint(bgDark, 1.00f, Config::Instance()->MenuBGColorA.value_or_default());
    c[ImGuiCol_ChildBg] = BgTint(bgMid, 1.10f, minAlpha + 0.1f);
    c[ImGuiCol_PopupBg] =
        lightTheme ? BgTint(bgLight, 0.90f) : BgTint(ImVec4(0.09f, 0.10f, 0.13f, 0.97f), 0.90f, 0.97f);
    c[ImGuiCol_MenuBarBg] = BgTint(bgDark, 0.85f);
    c[ImGuiCol_DockingEmptyBg] = BgTint(bgDark, 0.75f);

    c[ImGuiCol_Border] = borderCol;
    c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Neutral background, not MenuBGColor.
    c[ImGuiCol_FrameBg] = BgTint(bgLight, 0.50f, minAlpha + 0.15f);
    c[ImGuiCol_FrameBgHovered] = SurfaceHover();
    c[ImGuiCol_FrameBgActive] = SurfaceActive();

    c[ImGuiCol_TitleBg] = BgTint(bgTitle, 0.40f);
    c[ImGuiCol_TitleBgActive] = TitleActive();
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(bgTitle.x, bgTitle.y, bgTitle.z, 0.75f);

    c[ImGuiCol_ScrollbarBg] = BgTint(bgDark, 0.60f, minAlpha + 0.2f);
    c[ImGuiCol_ScrollbarGrab] = AccentSoft();
    c[ImGuiCol_ScrollbarGrabHovered] = AccentMed();
    c[ImGuiCol_ScrollbarGrabActive] = AccentStrong();

    c[ImGuiCol_CheckMark] = AccentReadable();
    c[ImGuiCol_SliderGrab] = AccentMed();
    c[ImGuiCol_SliderGrabActive] = AccentReadable();
    c[ImGuiCol_InputTextCursor] = AccentReadable();

    c[ImGuiCol_Button] = AccentSoft();
    c[ImGuiCol_ButtonHovered] = AccentMed();
    c[ImGuiCol_ButtonActive] = AccentStrong();

    c[ImGuiCol_Header] = AccentSoft(0.90f);
    c[ImGuiCol_HeaderHovered] = AccentMed(0.95f);
    c[ImGuiCol_HeaderActive] = AccentStrong();

    c[ImGuiCol_Separator] = borderCol;
    c[ImGuiCol_SeparatorHovered] = AccentMed(0.85f);
    c[ImGuiCol_SeparatorActive] = AccentStrong();

    c[ImGuiCol_ResizeGrip] = AccentSoft(0.30f);
    c[ImGuiCol_ResizeGripHovered] = AccentStrong(0.70f);
    c[ImGuiCol_ResizeGripActive] = AccentStrong(0.95f);

    c[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TabHovered] = lightTheme ? ImVec4(0.00f, 0.00f, 0.00f, 0.08f) : ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    c[ImGuiCol_TabSelected] = lightTheme ? ImVec4(0.00f, 0.00f, 0.00f, 0.10f) : ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
    c[ImGuiCol_TabSelectedOverline] = AccentStrong();
    c[ImGuiCol_TabDimmed] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TabDimmedSelected] = AccentSoft(0.75f);
    c[ImGuiCol_TabDimmedSelectedOverline] = borderCol;

    c[ImGuiCol_DockingPreview] = AccentStrong(0.70f);

    c[ImGuiCol_PlotLines] = PlotAccent();
    c[ImGuiCol_PlotLinesHovered] = PlotAccentHovered();
    c[ImGuiCol_PlotHistogram] = PlotAccent(0.85f);
    c[ImGuiCol_PlotHistogramHovered] = PlotAccentHovered();

    c[ImGuiCol_TableHeaderBg] = BgTint(bgMid, 0.80f, minAlpha + 0.25f);
    c[ImGuiCol_TableBorderStrong] = borderCol;
    c[ImGuiCol_TableBorderLight] = lightTheme ? ImVec4(0.68f, 0.72f, 0.80f, 1.00f) : AccentSoft();
    c[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = lightTheme ? ImVec4(0.00f, 0.00f, 0.00f, 0.045f) : ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    c[ImGuiCol_TreeLines] = borderCol;
    c[ImGuiCol_TextSelectedBg] = AccentMed(0.38f);
    c[ImGuiCol_DragDropTarget] = AccentStrong(0.90f);
    c[ImGuiCol_NavCursor] = AccentReadable();
    c[ImGuiCol_NavWindowingHighlight] = AccentStrong(0.70f);
    c[ImGuiCol_NavWindowingDimBg] = dimBg;
    c[ImGuiCol_ModalWindowDimBg] = modalDimBg;

    // Scale sizes last, after every size field above has been set. Every caller
    // gets a correctly scaled style, so scaling never depends on call order.
    float uiScale = MenuResolutionScale(ImGui::GetIO());
    style.ScaleAllSizes(uiScale);
    style.MouseCursorScale = 1.0f;

    _hdrTonemapApplied = false;
    MenuHdrCheck(ImGui::GetIO());
}

static double lastTime = 0.0;
static double lastFrameTime = 0.0;
static UINT64 uwpTargetFrame = 0;

void MenuCommon::Present()
{
    _frameCount++;

    auto now = Util::MillisecondsNow();

    if (lastTime > 0.0)
        lastFrameTime = now - lastTime;

    lastTime = now;

    // Only feed manual input in overlay mode — injected mode uses ImGui_ImplWin32_NewFrame
    if (Config::Instance()->OverlayMenu.value_or_default() && _handle != nullptr)
        UpdateManualInput(_handle);
}

struct VersionCheckStatus
{
    bool completed = false;
    bool updateAvailable = false;
    std::string latestTag;
    std::string latestUrl;
    std::string error;
};

struct MenuCommon::RenderMenuContext
{
    State& state;
    decltype(Config::Instance()) config;
    ImGuiIO& io;
    IFeature* currentFeature = nullptr;

    double now = 0.0;
    double frameTime = 0.0;
    double frameRate = 0.0;
    float menuResScale = 1.0f;
    float fpsScale = 1.0f;
    float averageFrameTime = 0.0f;
    float averageUpscalerFT = 0.0f;

    bool frameTimesCalculated = false;
    bool newFrame = false;

    VersionCheckStatus versionStatus;
    std::string currentVersionText;

    // Cached when the menu is visible and shared by RenderMainMenuWindow section helpers.
    std::unique_ptr<std::decay_t<decltype(IdentifyGpu::getPrimaryGpu())>> primaryGpu;
};

static std::string splashMessage;

void MenuCommon::UpdateRenderTiming(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& now = ctx.now;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;

    if (config->OverlayMenu.value_or_default())
    {
        _frameCount++;

        // FPS & frame time calculation
        if (lastTime > 0.0)
        {
            frameTime = now - lastTime;
            frameRate = 1000.0 / frameTime;
        }

        lastTime = now;

        if (_handle != nullptr)
            UpdateManualInput(_handle);
    }
    else
    {
        if (state.activeFgInput == FGInput::NoFG || state.activeFgOutput == FGOutput::NoFG)
            MenuCommon::Present();

        frameTime = lastFrameTime;
        frameRate = 1000.0 / frameTime;
    }

    state.frameTimes.pop_front();
    state.frameTimes.push_back(frameTime);
}

void MenuCommon::UpdateMenuInputMode(RenderMenuContext& ctx)
{
    auto& io = ctx.io;

    // Moved here to prevent gamepad key replay
    if (_isVisible)
    {
        if (hasGamepad)
            io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    }
    else
    {
        capturingKey = false;
        hasGamepad = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;
    }
}

void MenuCommon::HandleMenuShortcuts(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;

    // Poll shortcuts via low-level keyboard hook — bypasses game input interception
    {
        auto PollKey = [&](int vk, bool& flag)
        {
            if (vk <= 0 || vk >= 256)
                return;

            static bool prevDown[256] {};
            bool down = (_llKeyStates[vk].load(std::memory_order_relaxed) & 0x8000) != 0;
            if (prevDown[vk] && !down)
                flag = true;
            prevDown[vk] = down;
        };

        PollKey(config->ShortcutKey.value_or_default(), inputMenu);
        PollKey(config->FpsShortcutKey.value_or_default(), inputFps);
        PollKey(config->FGShortcutKey.value_or_default(), inputFG);
        PollKey(config->FpsCycleShortcutKey.value_or_default(), inputFpsCycle);
    }

    // Handle Inputs
    {
        if (inputFG)
        {
            inputFG = false;

            if (state.activeFgInput != FGInput::NoFG && state.activeFgOutput != FGOutput::NoFG &&
                (state.currentFGSwapchain != nullptr || state.activeFgInput == FGInput::NvngxFG))
            {
                config->FGEnabled = !config->FGEnabled.value_or_default();
                LOG_DEBUG("FG toggle key pressed, setting FGEnabled to {}", config->FGEnabled.value_or_default());

                if (config->FGEnabled.value_or_default())
                    state.fgChanged = true;
            }
        }

        if (inputFps)
        {
            inputFps = false;
            config->ShowFps = !config->ShowFps.value_or_default();
        }

        if (inputFpsCycle && config->ShowFps.value_or_default())
            config->FpsOverlayType = (FpsOverlay) ((config->FpsOverlayType.value_or_default() + 1) % FpsOverlay_COUNT);

        if (inputMenu)
        {
            inputMenu = false;
            _isVisible = !_isVisible;

            LOG_DEBUG("Menu key pressed, {0}", _isVisible ? "opening ImGui" : "closing ImGui");

            if (_isVisible)
            {
                io.ClearEventsQueue();
                io.ClearInputKeys();
                io.ClearInputMouse();

                OptiInput::ResetMenuInputTransientState();

                ApplyThemeStyle();

                refreshRate = Util::GetActiveRefreshRate(_handle);

                auto optiPath = std::filesystem::path(Config::Instance()->MainDllPath.value());
                auto dllPath = optiPath / L"dlss-enabler-headless.dll";
                state.nvngxFgFilesAvailable = enablerExists.Get(dllPath);

                if (!state.nvngxFgFilesAvailable)
                {
                    dllPath = optiPath / L"dlssg_to_fsr3_amd_is_better.dll";
                    state.nvngxFgFilesAvailable = nukemsExists.Get(dllPath);
                }

                if (State::Instance().currentFeature != nullptr)
                {
                    if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSSD)
                        comboPreset = config->DLSSDRenderPresetForAll.value_or_default();
                    else if (State::Instance().currentFeature->GetUpscalerType() == Upscaler::DLSS)
                        comboPreset = config->RenderPresetForAll.value_or_default();
                }
            }
            else
            {
                ImGui::CloseCurrentPopup();

                _showMipmapCalcWindow = false;
                _showHudlessWindow = false;
            }

            io.MouseDrawCursor = false;
            io.WantCaptureKeyboard = _isVisible;
            io.WantCaptureMouse = _isVisible;
        }

        inputFpsCycle = false;
    }
}

void MenuCommon::UpdateVersionAndStartupNotifications(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& now = ctx.now;
    auto& versionStatus = ctx.versionStatus;

    constexpr double splashTime = 7000.0;
    constexpr int updateNoticeTime = 10000;

    // Version check state is copied while locked, then consumed by the UI render pass.
    {
        std::scoped_lock lock(state.versionCheckMutex);
        versionStatus.completed = state.versionCheckCompleted;
        versionStatus.updateAvailable = state.updateAvailable;
        versionStatus.latestTag = state.latestVersionTag;
        versionStatus.latestUrl = state.latestVersionUrl;
        versionStatus.error = state.versionCheckError;
    }

    ctx.currentVersionText = VersionCheck::CurrentVersionString();

    if (versionStatus.completed && versionStatus.updateAvailable && !versionStatus.latestTag.empty())
    {
        if (updateNoticeTag != versionStatus.latestTag)
        {
            updateNoticeTag = versionStatus.latestTag;
            updateNoticeUrl = versionStatus.latestUrl;
            const auto notice = [&]()
            {
                ImGuiToast updateNotification { ImGuiToastType::Error, updateNoticeTime };
                updateNotification.setTitle(Translation::Get("OptiScaler Update available"));
                updateNotification.setContent(
                    Translation::Get("Press %s for more info"),
                    Keybind::KeyNameFromVirtualKeyCode(config->ShortcutKey.value_or_default()).c_str());
                ImGui::InsertNotification(updateNotification);
                return true;
            };
            static auto res = notice();
        }
    }

    // One-shot startup warning notifications.
    if (!state.postDone)
    {
        if (state.postCodes & PostCode::SlPluginsAlreadyInMemory)
        {
            auto filename = Util::DllPath().filename().string();
            to_lower_in_place(filename);

            ImGuiToast notification { ImGuiToastType::Warning, 10000 };
            notification.setTitle(Translation::Get("Late Streamline hook detected"));
            notification.setContent(
                Translation::Get("Consider renaming OptiScaler from %s to other supported name.\nYou may experience issues otherwise."),
                filename.c_str());
            ImGui::InsertNotification(notification);
        }

        if (state.postCodes & PostCode::TryingFsr4Fp8OnUnsupported)
        {
            ImGuiToast notification { ImGuiToastType::Warning, 10000 };
            notification.setTitle("Silly goose detected");
            notification.setContent("FSR 4 FP8 only works on AMD");
            ImGui::InsertNotification(notification);
        }

        state.postDone = true;
    }

    // Initialize splash timing and select the splash text once per process.
    if (splashLimit < 1.0f)
    {
        splashStart = now + 100.0;
        splashLimit = splashStart + splashTime;

        std::srand(static_cast<unsigned>(std::time(nullptr)));
        splashMessage = splashText[std::rand() % splashText.size()];
    }
}

void MenuCommon::BeginMenuFrameIfNeeded(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& now = ctx.now;
    auto& newFrame = ctx.newFrame;

    // New frame check
    if ((!config->DisableSplash.value_or_default() && now > splashStart && now < splashLimit) ||
        config->ShowFps.value_or_default() || _isVisible || ImGui::notifications.size() > 0)
    {
        if (!_isUWP)
        {
            ImGui_ImplWin32_NewFrame();
        }
        else
        {
            ImVec2 displaySize { state.screenWidth, state.screenHeight };
            ImGui_ImplUwp_NewFrame(displaySize);
        }

        OptiInput::FeedImGui(_isVisible);

        MenuHdrCheck(io);
        ImGui::NewFrame();

        newFrame = true;
    }
}

void MenuCommon::RenderSplashWindow(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& now = ctx.now;

    constexpr double fadeTime = 1000.0;

    // Splash screen
    if (!config->DisableSplash.value_or_default())
    {
        if (now > splashStart && now < splashLimit)
        {

            ImGui::SetNextWindowSize({ 0.0f, 0.0f });
            ImGui::SetNextWindowBgAlpha(config->FpsOverlayAlpha.value_or_default());
            ImGui::SetNextWindowPos(splashPosition, ImGuiCond_Always);

            float windowAlpha = 1.0f;
            if (auto diff = now - splashStart; diff < fadeTime)
                windowAlpha = static_cast<float>(diff / fadeTime);
            else if (auto diff = splashLimit - now; diff < fadeTime)
                windowAlpha = static_cast<float>(diff / fadeTime);

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));

            if (!config->OverlaysUseTheme.value_or_default())
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
            }

        if (ImGui::Begin(Translation::Get("Splash"), nullptr,
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav))
            {
                float splashScale = 1.0f;
                float baseScaleHeight = 720.0f;

                if (io.DisplaySize.y > baseScaleHeight)
                    splashScale = io.DisplaySize.y / baseScaleHeight;

                if (config->UseHQFont.value_or_default())
                    ImGui::PushFontSize(std::round(splashScale * fontSize));
                else
                    ImGui::SetWindowFontScale(splashScale);

                ImGui::Text(Translation::Get("OptiScaler - %s for menu"),
                            Keybind::KeyNameFromVirtualKeyCode(config->ShortcutKey.value_or_default()).c_str());
                ImGui::TextColored(toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 0.7f)), splashMessage.c_str());

                splashSize = ImGui::GetWindowSize();

                if (config->UseHQFont.value_or_default())
                    ImGui::PopFontSize();

                ImGui::End();

                splashPosition.x = 0.0f; // io.DisplaySize.x - splashWinSize.x;
                splashPosition.y = io.DisplaySize.y - splashSize.y;
            }

            if (!config->OverlaysUseTheme.value_or_default())
                ImGui::PopStyleColor(4);
            else
                ImGui::PopStyleColor(2);

            ImGui::PopStyleVar(2);
        }
    }
}

void MenuCommon::RenderNotifications(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& io = ctx.io;

    // Notifications
    bool tonemapRequired = State::Instance().isHdrActive ||
                           (!Config::Instance()->OverlayMenu.value_or_default() &&
                            State::Instance().currentFeature != nullptr && State::Instance().currentFeature->IsHdr());

    float screenHeight = State::Instance().screenHeight;
    if (io.DisplaySize.y != 0)
        screenHeight = io.DisplaySize.y;

    // Map resolution height to scale, 0.5 for 480p, 2.0 for 1440p
    constexpr float slope = (2.0f - 0.5f) / (1440.f - 480.f);
    float notificationScale = 0.5f + slope * (screenHeight - 480.f);
    notificationScale = std::clamp(notificationScale, 0.5f, 2.0f);

    if (config->UseHQFont.value_or_default())
        ImGui::PushFontSize(std::round(notificationScale * fontSize));

    // No fallback font, SetWindowFontScale needs to be called after Begin()

    ImGui::RenderNotifications(ImGuiToastPos::TopCenter, notificationScale, tonemapRequired);

    if (config->UseHQFont.value_or_default())
        ImGui::PopFontSize();
}

void MenuCommon::UpdateFrameTimeAverages(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& frameTimesCalculated = ctx.frameTimesCalculated;
    auto& menuResScale = ctx.menuResScale;
    auto& fpsScale = ctx.fpsScale;
    auto& averageFrameTime = ctx.averageFrameTime;
    auto& averageUpscalerFT = ctx.averageUpscalerFT;

    // FPS Overlay font
    fpsScale = config->FpsScale.value_or(menuResScale);

    // Update frame time & upscaler time averages
    averageFrameTime = 0.0f;
    averageUpscalerFT = 0.0f;

    if (config->ShowFps.value_or_default() || _isVisible)
    {
        float frameCnt = 0;
        frameTime = 0;
        for (size_t i = 299; i > 199; i--)
        {
            if (state.frameTimes[i] > 0.0)
            {
                frameTime += state.frameTimes[i];
                frameCnt++;
            }
        }

        frameTime /= frameCnt;
        frameRate = 1000.0 / frameTime;
        frameTimesCalculated = true;

        float lastFT = static_cast<float>(state.frameTimes.empty() ? 0.0f : state.frameTimes.back());
        float lastUT = static_cast<float>(state.upscaleTimes.empty() ? 0.0f : state.upscaleTimes.back());
        gFrameTimes.Push(lastFT);
        gUpscalerTimes.Push(lastUT);

        averageFrameTime = gFrameTimes.Average();
        averageUpscalerFT = gUpscalerTimes.Average();
    }
}

void MenuCommon::RenderPerformanceOverlay(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& currentFeature = ctx.currentFeature;
    auto& now = ctx.now;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& menuResScale = ctx.menuResScale;
    auto& fpsScale = ctx.fpsScale;
    auto& averageFrameTime = ctx.averageFrameTime;
    auto& averageUpscalerFT = ctx.averageUpscalerFT;

    // If Fps overlay is visible
    if (config->ShowFps.value_or_default())
    {
        bool stylePushed = false;

        const static auto defaultStyle = ImGuiStyle();

        // Rescale the fps overlay every frame because it shares style with the main menu
        if (config->FpsScale.has_value() && config->FpsScale.value() != menuResScale)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, defaultStyle.WindowPadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, defaultStyle.FramePadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, defaultStyle.CellPadding * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, defaultStyle.SeparatorTextPadding * fpsScale);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultStyle.ItemSpacing * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, defaultStyle.ItemInnerSpacing * fpsScale);
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, defaultStyle.IndentSpacing * fpsScale);

            stylePushed = true;
        }

        // Set overlay position
        ImGui::SetNextWindowPos(overlayPosition, ImGuiCond_Always);

        // Set overlay window properties
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));  // Transparent border
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0)); // Transparent frame background

        if (!config->OverlaysUseTheme.value_or_default())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, toneMapColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        ImGui::SetNextWindowBgAlpha(config->FpsOverlayAlpha.value_or_default()); // Transparent background

        if (!config->OverlaysUseTheme.value_or_default())
        {
            ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, toneMapColor(green));
        }

        if (ImGui::Begin(Translation::Get("Performance Overlay"), nullptr,
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav))
        {
            std::string api;
            if (IdentifyGpu::getPrimaryGpu().usesDxvk && state.api == DX11)
            {
                api = "DXVK";
            }
            else if (IdentifyGpu::getPrimaryGpu().usesVkd3dProton && state.api == DX12)
            {
                api = "VKD3D";
            }
            else
            {
                switch (state.swapchainApi)
                {
                case Vulkan:
                    api = "VLK";
                    break;

                case DX11:
                    api = "D3D11";
                    break;

                case DX12:
                    api = "D3D12";
                    break;

                default:
                    switch (state.api)
                    {
                    case Vulkan:
                        api = "VLK";
                        break;

                    case DX11:
                        api = "D3D11";
                        break;

                    case DX12:
                        api = "D3D12";
                        break;

                    default:
                        api = "???";
                        break;
                    }

                    break;
                }
            }

            if (config->UseHQFont.value_or_default())
                ImGui::PushFontSize(std::round(fpsScale * fontSize));
            else
                ImGui::SetWindowFontScale(fpsScale);

            std::string firstLine = "";
            std::string secondLine = "";
            std::string thirdLine = "";

            auto fg = state.currentFG;
            auto fgText = (fg != nullptr && fg->IsActive() && !fg->IsPaused()) ? (" (" + std::string(fg->Name()) + ")")
                                                                               : std::string();

            if (state.activeFgOutput == FGOutput::NvngxFG || state.activeFgOutput == FGOutput::DLSSGWithNvngx)
            {
                if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
                {
                    if (state.dlssgDetectedInterpolationCount == 0)
                        fgText = " (Enabler off)";
                    else
                        fgText = std::format(" (Enabler x{})", state.dlssgDetectedInterpolationCount + 1);
                }
                else
                {
                    if (state.dlssgDetectedInterpolationCount == 0)
                        fgText = " (Nukems off)";
                    else if (state.dlssgDetectedInterpolationCount == 1)
                        fgText = std::format(" (Nukems x2)");
                    else
                        fgText = std::format(" (Nukems Doesn't support more than 2x)");
                }
            }
            else if (state.activeFgOutput == FGOutput::DLSSG)
            {
                if (state.dlssgDetectedInterpolationCount == 0)
                    fgText = " (DLSSG off)";
                else
                    fgText = std::format(" (DLSSG x{})", state.dlssgDetectedInterpolationCount + 1);
            }

            const auto overlayType = config->FpsOverlayType.value_or_default();
            const bool hasFeature = currentFeature && !currentFeature->IsFrozen();

            // Prepare Line 1
            std::string featurePart;
            std::string fpsPart;

            if (hasFeature)
            {
                const bool usesDx12CompatLayer = currentFeature->IsWithDx12();

                featurePart = StrFmt(" | %s -> %s %u.%u.%u%s", ApiUpscalerInputName(state.currentInputApiName).c_str(),
                                     currentFeature->ShortName().c_str(), currentFeature->Version().major,
                                     currentFeature->Version().minor, currentFeature->Version().patch,
                                     usesDx12CompatLayer ? " w/Dx12" : "");
            }

            switch (overlayType)
            {
            case FpsOverlay_JustFPS:
                fpsPart = StrFmt("FPS: %6.1f", frameRate);
                break;

            case FpsOverlay_Simple:
                fpsPart = StrFmt("FPS: %6.1f, %7.2f ms", frameRate, frameTime);
                break;

            default:
                fpsPart = StrFmt("FPS: %6.1f, Avg: %6.1f", frameRate, 1000.0f / averageFrameTime);
                break;
            }

            if (overlayType == FpsOverlay_JustFPS)
                firstLine = StrFmt("%s", fpsPart.c_str());
            else
                firstLine = StrFmt("%s | %s%s%s", api.c_str(), fpsPart.c_str(), fgText.c_str(), featurePart.c_str());

            // Prepare Line 2
            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Detailed)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                secondLine = StrFmt("Frame Time: %7.2f ms, Avg: %7.2f ms", state.frameTimes.back(), averageFrameTime);
            }

            // Prepare Line 3
            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Full)
            {
                thirdLine =
                    StrFmt("Upscaler Time: %7.2f ms, Avg: %7.2f ms", state.upscaleTimes.back(), averageUpscalerFT);
            }

            ImVec2 plotSize;
            if (config->FpsOverlayHorizontal.value_or_default())
            {
                plotSize = { fpsScale * 150, fpsScale * 16 };
            }
            else
            {
                // Find the widest text width
                auto firstSize = ImGui::CalcTextSize(firstLine.c_str());
                auto secondSize = ImGui::CalcTextSize(secondLine.c_str());
                auto thirdSize = ImGui::CalcTextSize(thirdLine.c_str());
                auto textWidth = 0.0f;

                if (firstSize.x > secondSize.x)
                    textWidth = firstSize.x > thirdSize.x ? firstSize.x : thirdSize.x;
                else
                    textWidth = secondSize.x > thirdSize.x ? secondSize.x : thirdSize.x;

                auto minWidth = fpsScale * 300.0f;
                auto plotWidth = textWidth < minWidth ? minWidth : textWidth;

                plotSize = { plotWidth, fpsScale * 30 };
            }

            // Draw the overlay
            ImGui::Text(firstLine.c_str());

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Detailed)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                ImGui::Text(secondLine.c_str());
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_DetailedGraph)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                    ImGui::SameLine(0.0f, 0.0f);

                // Graph of frame times
                ImGui::PlotLines(
                    "##FrameTimeGraph",
                    [](void* rb, int idx) -> float { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gFrameTimes, plotWidth, 0, nullptr, 0.0f, 66.6f, plotSize);
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_Full)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                {
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text(" | ");
                    ImGui::SameLine(0.0f, 0.0f);
                }
                else
                {
                    ImGui::Spacing();
                }

                ImGui::Text(thirdLine.c_str());
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_FullGraph)
            {
                if (config->FpsOverlayHorizontal.value_or_default())
                    ImGui::SameLine(0.0f, 0.0f);

                // Graph of upscaler times
                ImGui::PlotLines(
                    "##UpscalerFrameTimeGraph",
                    [](void* rb, int idx) -> float { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gUpscalerTimes, plotWidth, 0, nullptr, 0.0f, 20.0f, plotSize);
            }

            if (config->FpsOverlayType.value_or_default() >= FpsOverlay_ReflexTimings)
            {
                constexpr auto delayBetweenPollsMs = 500;
                static auto previousPoll = 0.0;
                static bool gotData = false;

#ifdef LOW_LATENCY_INPUTS
                static TimingData timingData {};

                if (previousPoll <= 0.001 || previousPoll + delayBetweenPollsMs < now)
                {
                    gotData = InputCommon::get_timing_data(timingData);
                    previousPoll = now;
                }

                if (gotData && timingData.timeRange.has_value())
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    constexpr float offsetForText = 155;

                    const auto& rangeInNs = timingData.timeRange.value().length;

                    UINT64 localFrameCount = 0;

                    if (fg != nullptr)
                        localFrameCount = fg->FrameCount();

                    ImGui::Text(Translation::Get("FGId: %llu, RfxId: %llu"), localFrameCount, state.reflexFrameId);
                    ImGui::Text(Translation::Get("Low latency timings, whole frame: %.1fms"), rangeInNs / 1000.0);

                    const auto maxWidth =
                        config->FpsOverlayHorizontal.value_or_default() ? ImGui::GetWindowWidth() : plotSize.x;

                    const auto drawTiming = [&](const auto& timingOpt, const char* desc, ImVec4 color)
                    {
                        if (!timingOpt.has_value())
                            return;

                        auto toneMappedColor = State::Instance().isHdrActive ? toneMapColor(color) : color;

                        const auto& timing = timingOpt.value();
                        float duration = static_cast<float>(timing.length * rangeInNs / 1000.0);

                        ImGui::TextColored(toneMappedColor, "%-12s %4.1fms", desc, duration);

                        auto leftLimit = ImGui::GetItemRectMin().x + offsetForText * fpsScale;

                        auto start = static_cast<float>(leftLimit + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                        timing.position);

                        auto end = static_cast<float>(start + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                  timing.length);

                        auto pos = ImVec2(start, ImGui::GetItemRectMin().y);
                        auto size = ImVec2(end, ImGui::GetItemRectMax().y);

                        drawList->AddRectFilled(pos, size, ImGui::ColorConvertFloat4ToU32(toneMappedColor));
                    };

                    drawTiming(timingData.simulation, "Simulation", ImVec4(0.768f, 0.169f, 0.169f, 1.0f));
                    drawTiming(timingData.renderSubmit, "RenderSubmit", ImVec4(0.235f, 0.705f, 0.294f, 1.0f));
                    drawTiming(timingData.present, "Present", ImVec4(1.0f, 0.88f, 0.098f, 1.0f));
                    drawTiming(timingData.driver, "Driver", ImVec4(0.263f, 0.388f, 0.847f, 1.0f));
                    drawTiming(timingData.osRenderQueue, "RenderQueue", ImVec4(0.76f, 0.51f, 0.188f, 1.0f));
                    drawTiming(timingData.gpuRender, "GpuRender", ImVec4(0.569f, 0.117f, 0.705f, 1.0f));
                }
#else
                if (previousPoll <= 0.001 || previousPoll + delayBetweenPollsMs < now)
                {
                    gotData = ReflexHooks::updateTimingData();
                    previousPoll = now;
                }

                auto& timingData = ReflexHooks::timingData;

                if (gotData && timingData[TimingType::TimeRange].has_value())
                {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    constexpr float offsetForText = 155;

                    const auto& rangeInNs = timingData[TimingType::TimeRange].value().length;

                    UINT64 localFrameCount = 0;

                    if (fg != nullptr)
                        localFrameCount = fg->FrameCount();

                    ImGui::Text(Translation::Get("FGId: %llu, RfxId: %llu"), localFrameCount, state.reflexFrameId);
                    ImGui::Text(Translation::Get("Reflex timings, whole frame: %.1fms"), rangeInNs / 1000.0);

                    const auto maxWidth =
                        config->FpsOverlayHorizontal.value_or_default() ? ImGui::GetWindowWidth() : plotSize.x;

                    const auto drawTiming = [&](TimingType type, const char* desc, ImVec4 color)
                    {
                        if (!timingData[type].has_value())
                            return;

                        auto toneMappedColor = toneMapColor(color);

                        auto& timing = timingData[type].value();
                        float duration = static_cast<float>(timing.length * rangeInNs / 1000.0);
                        ImGui::TextColored(toneMappedColor, "%-12s %4.1fms", desc, duration);
                        auto leftLimit = ImGui::GetItemRectMin().x + offsetForText * fpsScale;
                        auto start = static_cast<float>(leftLimit + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                        timing.position);
                        auto end = static_cast<float>(start + (ImGui::GetItemRectMin().x + maxWidth - leftLimit) *
                                                                  timing.length);
                        auto pos = ImVec2(start, ImGui::GetItemRectMin().y);
                        auto size = ImVec2(end, ImGui::GetItemRectMax().y);
                        drawList->AddRectFilled(pos, size, ImGui::ColorConvertFloat4ToU32(toneMappedColor));
                    };

                    drawTiming(TimingType::Simulation, "Simulation", ImVec4(0.768f, 0.169f, 0.169f, 1.0f));
                    drawTiming(TimingType::RenderSubmit, "RenderSubmit", ImVec4(0.235f, 0.705f, 0.294f, 1.0f));
                    drawTiming(TimingType::Present, "Present", ImVec4(1.0f, 0.88f, 0.098f, 1.0f));
                    drawTiming(TimingType::Driver, "Driver", ImVec4(0.263f, 0.388f, 0.847f, 1.0f));
                    drawTiming(TimingType::OsRenderQueue, "RenderQueue", ImVec4(0.76f, 0.51f, 0.188f, 1.0f));
                    drawTiming(TimingType::GpuRender, "GpuRender", ImVec4(0.569f, 0.117f, 0.705f, 1.0f));
                }
#endif
            }
        }

        // Restore the style
        if (!config->OverlaysUseTheme.value_or_default())
            ImGui::PopStyleColor(5);
        else
            ImGui::PopStyleColor(2);

        // Get size for postioning
        overlaySize = ImGui::GetWindowSize();

        if (config->UseHQFont.value_or_default())
            ImGui::PopFontSize();

        ImGui::End();

        if (stylePushed)
            ImGui::PopStyleVar(7);

        // Left / Right
        if (config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopLeft ||
            config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_BottomLeft)
        {
            overlayPosition.x = 0;
        }
        else
        {
            overlayPosition.x = io.DisplaySize.x - overlaySize.x;
        }

        // Top / Bottom
        if (config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopLeft ||
            config->FpsOverlayPosition.value_or_default() == FpsOverlayPos_TopRight)
        {
            overlayPosition.y = 0;
        }
        else
        {
            // Prevent overlapping with splash message
            if (!config->DisableSplash.value_or_default() && now > splashStart && now < splashLimit)
                overlayPosition.y = io.DisplaySize.y - overlaySize.y - splashSize.y;
            else
                overlayPosition.y = io.DisplaySize.y - overlaySize.y;
        }
    }
}

void MenuCommon::RenderCursor(RenderMenuContext& ctx)
{
    if (!_isVisible)
        return;

    // Let ImGui draw the mouse cursor directly into the rendered output.
    // This avoids the system cursor being clipped by the game/overlay viewport.
    ctx.io.MouseDrawCursor = true;
}

void MenuCommon::RenderMainMenuHeaderMessages(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& versionStatus = ctx.versionStatus;
    auto& currentVersionText = ctx.currentVersionText;
    auto& primaryGpu = *ctx.primaryGpu;

    if (!_showMipmapCalcWindow && !_showHudlessWindow && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
        ImGui::SetWindowFocus();

    if (config->MenuScale.has_value())
    {
        _selectedScale = ((int) (menuResScale * 10.0f)) - 4;
    }
    else
    {
        _selectedScale = 0;
    }

    if (versionStatus.completed)
    {
        if (versionStatus.updateAvailable && !versionStatus.latestTag.empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), Translation::Get("Update available: %s (current %s)"),
                               versionStatus.latestTag.c_str(), currentVersionText.c_str());

            if (!versionStatus.latestUrl.empty())
            {
                ImGui::SameLine();
                ImGui::TextLinkOpenURL(Translation::Get("Open release page"), versionStatus.latestUrl.c_str());
            }

            ImGui::Spacing();
        }
        else if (!versionStatus.error.empty())
        {
            LOG_ERROR("Version check failed: {0}", versionStatus.error);
            versionStatus.error.clear();
        }
        // Disabled error message
        // else if (!versionStatus.error.empty())
        //{
        //    ImGui::Spacing();
        //    ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.4f, 0.f, 1.f)), "%s", versionStatus.error.c_str());
        //    ImGui::Spacing();
        //}
    }

    // No active upscaler message
    if (currentFeature == nullptr || !currentFeature->IsInited())
    {
        ImGui::Spacing();

        if (config->UseHQFont.value_or_default())
            ImGui::PushFontSize(std::round(fontSize * menuResScale * 2.5f));
        else
            ImGui::SetWindowFontScale(menuResScale * 2.5f);

        if (state.nvngxExists || state.nvngxReplacement.has_value() ||
            (state.libxessExists || XeSSProxy::Module() != nullptr))
        {
            ImGui::Spacing();

            std::vector<std::string> upscalers;

            if (state.fsrHooks)
                upscalers.push_back("FSR");

            if (state.nvngxExists || state.nvngxReplacement.has_value() || primaryGpu.dlssCapable)
                upscalers.push_back(Translation::Get("DLSS"));

            if (state.libxessExists || XeSSProxy::Module() != nullptr)
                upscalers.push_back(Translation::Get("XeSS"));

            auto joined = upscalers | std::views::join_with(std::string { " or " });

            std::string joinedUpscalers(joined.begin(), joined.end());

            ImGui::Text(Translation::Get("Please select %s as upscaler from game\noptions and load a save game "
                        "to enable Opti settings.\nUpscalers don't always work in menus."),
                        joinedUpscalers.c_str());

            if (config->UseHQFont.value_or_default())
                ImGui::PopFontSize();
            else
                ImGui::SetWindowFontScale(menuResScale);

            ImGui::Spacing();

            if (primaryGpu.dlssCapable)
            {
            ImGui::Text(Translation::Get("nvngx_dlss : %s"), state.NVNGX_DLSS_Path.has_value() ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
                ImGui::SameLine(0.0f, 16.0f);
            ImGui::Text(Translation::Get("nvngx_dlssd : %s"), state.NVNGX_DLSSD_Path.has_value() ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
            }
            else
            {
            ImGui::Text(Translation::Get("nvngx.dll: %s"), state.nvngxExists ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
                ImGui::SameLine(0.0f, 16.0f);
            ImGui::Text(Translation::Get("nvngx replacement: %s"), state.nvngxReplacement.has_value() ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
            }

            ImGui::Text(Translation::Get("libxess: %s"),
                        (state.libxessExists || XeSSProxy::Module() != nullptr) ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));

        ImGui::Text(Translation::Get("FSR Hooks: %s"), state.fsrHooks ? Translation::Get("Exist") : Translation::Get("Don't Exist"));
            ImGui::SameLine(0.0f, 16.0f);
        ImGui::Text(Translation::Get("FSR 3.1: %s"), FfxApiProxy::Dx12Module() != nullptr ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
        ImGui::SameLine(0.0f, 16.0f);
        ImGui::Text(Translation::Get("FSR 3.1 SR: %s"), FfxApiProxy::Dx12Module_SR() != nullptr ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));
        ImGui::SameLine(0.0f, 16.0f);
        ImGui::Text(Translation::Get("FSR 3.1 FG: %s"), FfxApiProxy::Dx12Module_FG() != nullptr ? Translation::Get("Exists") : Translation::Get("Doesn't Exist"));

            ImGui::Spacing();
        }
        else
        {
            ImGui::Spacing();
            ImGui::Text(Translation::Get("Can't find nvngx.dll and libxess.dll and FSR inputs\nUpscaling support will NOT work."));
            ImGui::Spacing();

            if (config->UseHQFont.value_or_default())
                ImGui::PopFont();
            else
                ImGui::SetWindowFontScale(menuResScale);
        }
    }
    else if (currentFeature->IsFrozen())
    {
        ImGui::Spacing();

        if (config->UseHQFont.value_or_default())
            ImGui::PushFontSize(std::round(fontSize * menuResScale * 3.0f));
        else
            ImGui::SetWindowFontScale(menuResScale * 3.0f);

        ImGui::Text(Translation::Get("%s is active, but not currently used by the game\nPlease enter the game"),
                    currentFeature->Name().c_str());

        if (config->UseHQFont.value_or_default())
            ImGui::PopFont();
        else
            ImGui::SetWindowFontScale(menuResScale);
    }
}

void MenuCommon::RenderActiveUpscalerSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // UPSCALERS -----------------------------
        ImGui::SeparatorText(Translation::Get("Upscalers"));
        ShowTooltip(Translation::Get("Which copium do you choose?"));

        GetCurrentBackendInfo(state.api, currentBackend, &currentBackendName);

        std::string spoofingText;

        ImGui::PushItemWidth(180.0f * menuResScale);

        const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;
        const bool usesDx12CompatLayer = currentFeature->IsWithDx12();

        switch (state.api)
        {
        case DX11:
            ImGui::Text(primaryGpu.name.c_str());

            ImGui::Text(Translation::Get("D3D11 %s| %s %d.%d.%d%s"), primaryGpu.usesDxvk ? "(DXVK) " : "",
                        currentFeature->ShortName().c_str(), currentFeature->Version().major,
                        currentFeature->Version().minor, currentFeature->Version().patch,
                        usesDx12CompatLayer ? " w/Dx12" : "");
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text(Translation::Get("| Input: %s"), ApiUpscalerInputName(state.currentInputApiName).c_str());

            ImGui::SameLine(0.0f, 6.0f);
            spoofingText = config->DxgiSpoofing.value_or_default() ? Translation::Get("On") : Translation::Get("Off");
            ImGui::Text(Translation::Get("| Spoof: %s"), spoofingText.c_str());

            if (!usesDlssd)
                AddDx11Backends(currentBackend);

            break;

        case DX12:
            ImGui::Text(primaryGpu.name.c_str());

            ImGui::Text(Translation::Get("D3D12 %s| %s %d.%d.%d"), primaryGpu.usesDxvk ? "(DXVK) " : "",
                        currentFeature->ShortName().c_str(), currentFeature->Version().major,
                        currentFeature->Version().minor, currentFeature->Version().patch);
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text(Translation::Get("| Input: %s"), ApiUpscalerInputName(state.currentInputApiName).c_str());

            ImGui::SameLine(0.0f, 6.0f);
            spoofingText = config->DxgiSpoofing.value_or_default() ? Translation::Get("On") : Translation::Get("Off");
            ImGui::Text(Translation::Get("| Spoof: %s"), spoofingText.c_str());

            if (!usesDlssd)
                AddDx12Backends(currentBackend);

            break;

        default:
            ImGui::Text(primaryGpu.name.c_str());

            ImGui::Text(Translation::Get("Vulkan %s| %s %d.%d.%d%s"), primaryGpu.usesDxvk ? "(DXVK) " : "",
                        currentFeature->ShortName().c_str(), currentFeature->Version().major,
                        currentFeature->Version().minor, currentFeature->Version().patch,
                        usesDx12CompatLayer ? " w/Dx12" : "");
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text(Translation::Get("| Input: %s"), ApiUpscalerInputName(state.currentInputApiName).c_str());

            auto vlkSpoof = config->VulkanSpoofing.value_or_default();
            auto vlkExtSpoof = config->VulkanExtensionSpoofing.value_or_default();

            if (vlkSpoof && vlkExtSpoof)
                spoofingText = Translation::Get("On + Ext");
            else if (vlkSpoof)
                spoofingText = Translation::Get("On");
            else if (vlkExtSpoof)
                spoofingText = Translation::Get("Just Ext");
            else
                spoofingText = Translation::Get("Off");

            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text(Translation::Get("| Spoof: %s"), spoofingText.c_str());

            if (!usesDlssd)
                AddVulkanBackends(currentBackend);
        }

        ImGui::PopItemWidth();

        if (!usesDlssd)
        {
            ImGui::SameLine(0.0f, 6.0f);

            if (ImGui::Button(Translation::Get("Change Upscaler##2")) && state.newBackend != Upscaler::Reset &&
                state.newBackend != currentBackend)
            {
                if (state.newBackend == Upscaler::XeSS)
                {
                    // Reseting them for xess
                    config->DisableReactiveMask.reset();
                    config->DlssReactiveMaskBias.reset();
                }

                MARK_ALL_BACKENDS_CHANGED();
            }
        }

        if (currentFeature->AccessToReactiveMask())
        {
            ImGui::BeginDisabled(config->DisableReactiveMask.value_or(false));

            auto useAsTransparency = config->FsrUseMaskForTransparency.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Use Reactive Mask as Transparency Mask"), &useAsTransparency))
                config->FsrUseMaskForTransparency = useAsTransparency;

            ImGui::EndDisabled();
        }

        if (primaryGpu.dlssCapable && !state.NVNGX_DLSS_Path.has_value())
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), Translation::Get("nvngx_dlss.dll not found, DLSS disabled!"));
        }
    }

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;

        // Dx11 with Dx12
        if (state.api == DX11 && currentFeature->IsWithDx12())
        {
            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Dx11 with Dx12 Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                if (bool dontUseNTShared = config->DontUseNTShared.value_or_default();
                    ImGui::Checkbox(Translation::Get("Don't Use NTShared"), &dontUseNTShared))
                    config->DontUseNTShared = dontUseNTShared;

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }

        if (state.api == Vulkan && currentFeature->IsWithDx12())
        {
            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Vulkan with Dx12 Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                if (bool inputsUseCopy = config->VulkanUseCopyForInputs.value_or_default();
                    ImGui::Checkbox(Translation::Get("Use CopyResource for Inputs"), &inputsUseCopy))
                    config->VulkanUseCopyForInputs = inputsUseCopy;

                if (bool outputUseCopy = config->VulkanUseCopyForOutput.value_or_default();
                    ImGui::Checkbox(Translation::Get("Use CopyResource for Output"), &outputUseCopy))
                    config->VulkanUseCopyForOutput = outputUseCopy;

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }

        // UPSCALER SPECIFIC -----------------------------

        // XeSS -----------------------------
        if (currentBackend == Upscaler::XeSS && !usesDlssd)
        {
            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("XeSS Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                const char* models[] = { "KPSS", "SPLAT", "MODEL_3", "MODEL_4", "MODEL_5", "MODEL_6" };
                auto configModes = config->NetworkModel.value_or_default();

                if (configModes < 0 || configModes > 5)
                    configModes = 0;

                const char* selectedModel = models[configModes];

                if (ImGui::BeginCombo(Translation::Get("Network Models"), selectedModel))
                {
                    for (int n = 0; n < 6; n++)
                    {
                        if (ImGui::Selectable(models[n], (config->NetworkModel.value_or_default() == n)))
                        {
                            config->NetworkModel = n;
                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        }
                    }

                    ImGui::EndCombo();
                }
                ShowHelpMarker(Translation::Get("Likely doesn't do much"));

                if (bool dbg = state.xessDebug; ImGui::Checkbox(Translation::Get("Dump (Shift+Del)"), &dbg))
                    state.xessDebug = dbg;

                ImGui::SameLine(0.0f, 6.0f);
                int dbgCount = state.xessDebugFrames;

                ImGui::PushItemWidth(95.0f * menuResScale);
                if (ImGui::InputInt(Translation::Get("frames"), &dbgCount))
                {
                    if (dbgCount < 4)
                        dbgCount = 4;
                    else if (dbgCount > 999)
                        dbgCount = 999;

                    state.xessDebugFrames = dbgCount;
                }

                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }

        // FFX -----------------
        if (!usesDlssd && (currentBackend == Upscaler::FFX || currentBackend == Upscaler::FFX_on12))
        {
            ImGui::SeparatorText(Translation::Get("FFX Settings"));

            if (_ffxUpscalerIndex < 0)
                _ffxUpscalerIndex = config->FfxUpscalerIndex.value_or_default();

            if (currentBackend == Upscaler::FFX ||
                currentBackend == Upscaler::FFX_on12 && state.ffxUpscalerVersionNames.size() > 0)
            {
                ImGui::PushItemWidth(135.0f * menuResScale);

                auto currentName = StrFmt("FSR %s", state.ffxUpscalerVersionNames[_ffxUpscalerIndex]);
                if (ImGui::BeginCombo(Translation::Get("FFX Upscaler"), currentName.c_str()))
                {
                    for (int n = 0; n < state.ffxUpscalerVersionIds.size(); n++)
                    {
                        auto name = StrFmt("FSR %s", state.ffxUpscalerVersionNames[n]);
                        if (ImGui::Selectable(name.c_str(), config->FfxUpscalerIndex.value_or_default() == n))
                            _ffxUpscalerIndex = n;
                    }

                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                ShowHelpMarker(Translation::Get("List of upscalers reported by FFX SDK"));

                ImGui::SameLine(0.0f, 6.0f);

                if (ImGui::Button(Translation::Get("Change Upscaler")) &&
                    _ffxUpscalerIndex != config->FfxUpscalerIndex.value_or_default())
                {
                    config->FfxUpscalerIndex = _ffxUpscalerIndex;
                    state.newBackend = currentBackend;
                    MARK_ALL_BACKENDS_CHANGED();
                }

                auto majorFsrVersion = currentFeature->Version().major;

                if (majorFsrVersion >= 4)
                {
                    ImGui::Spacing();

                    ImGui::BeginDisabled(config->FsrNonLinearSRGB.value_or_default() ||
                                         config->FsrNonLinearPQ.value_or_default());

                    if (bool nlCS = config->FsrNonLinearColorSpace.value_or_default();
                        ImGui::Checkbox(Translation::Get("Non-Linear Color Space"), &nlCS))
                    {
                        config->FsrNonLinearColorSpace = nlCS;
                        state.newBackend = currentBackend;
                        MARK_ALL_BACKENDS_CHANGED();
                    }

                    ImGui::EndDisabled();

                    ShowHelpMarker(Translation::Get("Indicates input color resource uses Non-Linear color space\n"
                                   "Might improve upscaling quality of FSR4\n"
                                   "Might increase ghosting"));

                    if (ImGui::BeginTable("nonLinear", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        bool nlSRGB = config->FsrNonLinearSRGB.value_or_default();
                        bool nlPQ = config->FsrNonLinearPQ.value_or_default();

                        // Helper to keep code DRY when updating the states
                        auto ApplyColorSpaceState = [&](bool srgb, bool pq)
                        {
                            config->FsrNonLinearSRGB = srgb;
                            config->FsrNonLinearPQ = pq;

                            if (srgb || pq)
                            {
                                config->FsrNonLinearColorSpace.set_volatile_value(true);
                            }
                            else
                            {
                                // Revert to config value if available, otherwise reset
                                if (config->FsrNonLinearColorSpace.value_for_config().has_value())
                                    config->FsrNonLinearColorSpace = config->FsrNonLinearColorSpace.value_for_config();
                                else
                                    config->FsrNonLinearColorSpace.reset();
                            }

                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        };

                        ImGui::TableNextColumn();

                        // Using boolean overload: if clicked, we toggle its state and force the other off
                        if (ImGui::RadioButton(Translation::Get("Non-Linear sRGB Input"), nlSRGB))
                        {
                            ApplyColorSpaceState(!nlSRGB, false);
                        }
                        ShowHelpMarker(Translation::Get("Indicates input color resource contains perceptual sRGB colors\n"
                                       "Might improve upscaling quality of FSR4\n"
                                       "Might increase ghosting"));

                        ImGui::TableNextColumn();

                        if (ImGui::RadioButton(Translation::Get("Non-Linear PQ Input"), nlPQ))
                        {
                            ApplyColorSpaceState(false, !nlPQ);
                        }
                        ShowHelpMarker(Translation::Get("Indicates input color resource contains perceptual PQ colors\n"
                                       "Might improve upscaling quality of FSR4\n"
                                       "Rarest, might increase ghosting and break lights"));

                        ImGui::EndTable();
                    }

                    std::array<const char*, 7> models = { Translation::Get("Default"), Translation::Get("Model 0"), Translation::Get("Model 1"), Translation::Get("Model 2"),
                                                      Translation::Get("Model 3"), Translation::Get("Model 4"), Translation::Get("Model 5") };

                    // Conversion from 0 -> 6 into nullopt + 0 -> 5 is required
                    uint32_t configModes = 0;

                    if (config->Fsr4Preset.has_value())
                        configModes = config->Fsr4Preset.value_or(0) + 1;

                    if (configModes < 0 || configModes >= models.size())
                        configModes = 0;

                    const char* selectedModel = models[configModes];

                    if (ImGui::BeginTable("fsr4Presets", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableNextColumn();

                        if (ImGui::BeginCombo(Translation::Get("Presets"), selectedModel))
                        {
                            for (int n = 0; n < models.size(); n++)
                            {
                                uint32_t selection = 0;

                                if (config->Fsr4Preset.has_value())
                                    selection = config->Fsr4Preset.value_or(0) + 1;

                                if (ImGui::Selectable(models[n], selection == n))
                                {
                                    if (n < 1)
                                        config->Fsr4Preset.reset();
                                    else
                                        config->Fsr4Preset = n - 1;

                                    state.newBackend = currentBackend;
                                    MARK_ALL_BACKENDS_CHANGED();
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ShowHelpMarker(Translation::Get("Each FSR4 preset uses its own model.\n"
                                       "Selecting a model won't change the upscaler preset!\n\n"
                                       "Model 0 is meant for FSR Native AA\n"
                                       "Model 1 is meant for Quality/Ultra Quality\n"
                                       "Model 2 is meant for Balanced\n"
                                       "Model 3 is meant for Performance\n"
                                       "Model 5 is meant for Ultra Performance"));

                        // ImGui::PopItemWidth();

                        // ImGui::SameLine(0.0f, 6.0f);

                        ImGui::TableNextColumn();

                        if (state.currentFsr4Preset.has_value())
                            ImGui::Text(Translation::Get("Current preset: %d"), state.currentFsr4Preset.value());
                        else
                            ImGui::Text(Translation::Get("Failed to hook"));

                        ImGui::EndTable();
                    }
                }

                if (majorFsrVersion >= 3)
                {
                    if (bool dView = config->FsrDebugView.value_or_default();
                        ImGui::Checkbox(Translation::Get("Upscaler Debug View"), &dView))
                    {
                        config->FsrDebugView = dView;

                        // FSR 4's debug view requires backend reinit
                        if (majorFsrVersion > 3)
                        {
                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        }
                    }

                    if (majorFsrVersion > 3)
                    {
                        ShowHelpMarker(Translation::Get("Top left: Dilated Motion Vectors\n"
                                       "Top right: Predicted Blend Factor"));
                    }
                    else
                    {
                        ShowHelpMarker(Translation::Get("Top left: Dilated Motion Vectors\n"
                                       "Top middle: Protected Areas\n"
                                       "Top right: Dilated Depth\n"
                                       "Middle: Upscaled frame\n"
                                       "Bottom left: Disocclusion mask\n"
                                       "Bottom middle: Reactiveness\n"
                                       "Bottom right: Detail Protection Takedown"));
                    }

                    if (majorFsrVersion > 3)
                    {
                        ImGui::SameLine(0.0f, 6.0f);

                        if (bool fsr4wm = config->Fsr4EnableWatermark.value_or_default();
                            ImGui::Checkbox(Translation::Get("Upscaler Watermark"), &fsr4wm))
                        {
                            LOG_DEBUG("FSR4 Watermark set to {}", fsr4wm);
                            config->Fsr4EnableWatermark = fsr4wm;
                        }

                        ShowHelpMarker(Translation::Get("After changing this option, please Save Settings\n"
                                       "It will be applied on next launch."));

                        ImGui::Spacing();

                        // FSR4 Provider selection
                        const char* providerItems[] = {
                            Translation::Get("Auto"),
                            Translation::Get("SDK (amd_fidelityfx_upscaler_dx12.dll)"),
                            Translation::Get("Driver (amdxcffx64.dll)")
                        };
                        int selectedProvider = static_cast<int>(config->Fsr4ProviderPath.value_or_default());
                        if (ImGui::Combo(Translation::Get("FSR4 Provider"), &selectedProvider, providerItems,
                                         IM_ARRAYSIZE(providerItems)))
                        {
                            config->Fsr4ProviderPath = static_cast<Fsr4Provider>(selectedProvider);
                            state.newBackend = currentBackend;
                            MARK_ALL_BACKENDS_CHANGED();
                        }
                        ShowHelpMarker(Translation::Get("Select how FSR4.1 is loaded:\n"
                                       "Auto - Use both paths (default)\n"
                                       "SDK - Use bundled amd_fidelityfx_upscaler_dx12.dll\n"
                                       "Driver - Use amdxcffx64.dll from AMD driver"));
                    }
                }

                if (currentFeature->Version() >= feature_version { 3, 1, 1 } &&
                    currentFeature->Version() < feature_version { 4, 0, 0 })
                {
                    if (auto ch = ScopedCollapsingHeader(Translation::Get("FSR 3 Upscaler Fine Tuning")); ch.IsHeaderOpen())
                    {
                        ScopedIndent indent {};
                        ImGui::Spacing();
                        ImGui::Spacing();

                        ImGui::PushItemWidth(220.0f * menuResScale);

                        float velocity = config->FsrVelocity.value_or_default();
                        if (ImGui::SliderFloat(Translation::Get("Velocity Factor"), &velocity, 0.00f, 1.0f, "%.2f"))
                            config->FsrVelocity = velocity;

                        ShowHelpMarker(Translation::Get("Value of 0.0f can improve temporal stability of bright pixels\n"
                                       "Lower values are more stable with ghosting\n"
                                       "Higher values are more pixelly but less ghosting."));

                        if (currentFeature->Version() >= feature_version { 3, 1, 4 })
                        {
                            // Reactive Scale
                            float reactiveScale = config->FsrReactiveScale.value_or_default();
                            if (ImGui::SliderFloat(Translation::Get("Reactive Scale"), &reactiveScale, 0.0f, 100.0f, "%.1f"))
                                config->FsrReactiveScale = reactiveScale;

                            ShowHelpMarker(Translation::Get("Meant for development purpose to test if\n"
                                           "writing a larger value to reactive mask, reduces ghosting."));

                            // Shading Scale
                            float shadingScale = config->FsrShadingScale.value_or_default();
                            if (ImGui::SliderFloat(Translation::Get("Shading Scale"), &shadingScale, 0.0f, 100.0f, "%.1f"))
                                config->FsrShadingScale = shadingScale;

                            ShowHelpMarker(Translation::Get("Increasing this scales fsr3.1 computed shading\n"
                                           "change value at read to have higher reactiveness."));

                            // Accumulation Added Per Frame
                            float accAddPerFrame = config->FsrAccAddPerFrame.value_or_default();
                            if (ImGui::SliderFloat(Translation::Get("Acc. Added Per Frame"), &accAddPerFrame, 0.00f, 1.0f, "%.2f"))
                                config->FsrAccAddPerFrame = accAddPerFrame;

                            ShowHelpMarker(Translation::Get("Corresponds to amount of accumulation added per frame\n"
                                           "at pixel coordinate where disocclusion occured or when\n"
                                           "reactive mask value is > 0.0f. Decreasing this and \n"
                                           "drawing the ghosting object (IE no mv) to reactive mask \n"
                                           "with value close to 1.0f can decrease temporal ghosting.\n"
                                           "Decreasing this could result in more thin feature pixels flickering."));

                            // Min Disocclusion Accumulation
                            float minDisOccAcc = config->FsrMinDisOccAcc.value_or_default();
                            if (ImGui::SliderFloat(Translation::Get("Min. Disocclusion Acc."), &minDisOccAcc, -1.0f, 1.0f, "%.2f"))
                                config->FsrMinDisOccAcc = minDisOccAcc;

                            ShowHelpMarker(Translation::Get("Increasing this value may reduce white pixel temporal\n"
                                           "flickering around swaying thin objects that are disoccluding \n"
                                           "one another often. Too high value may increase ghosting."));
                        }

                        ImGui::PopItemWidth();

                        ImGui::Spacing();
                        ImGui::Spacing();
                    }
                }
            }
        }

        // DLSS -----------------
        if ((config->DLSSEnabled.value_or_default() && currentBackend == Upscaler::DLSS &&
             currentFeature->Version().major > 2) ||
            usesDlssd)
        {

            if (usesDlssd)
                ImGui::SeparatorText(Translation::Get("DLSSD Settings"));
            else
                ImGui::SeparatorText(Translation::Get("DLSS Settings"));

            auto overridden =
                usesDlssd ? state.dlssdPresetsOverriddenExternally : state.dlssPresetsOverriddenExternally;

            if (overridden)
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)), Translation::Get("Presets are overridden externally"));
                ShowHelpMarker(Translation::Get("This usually happens due to using tools\n"
                               "such as Nvidia App or Nvidia Inspector"));
                // ImGui::Text("Selecting setting below will disable that external override\n"
                //             "but you need to Save Settings and restart the game");

                ImGui::Spacing();
            }

            if (usesDlssd)
            {
                if (bool pOverride = config->DLSSDRenderPresetOverride.value_or_default();
                    ImGui::Checkbox(Translation::Get("Render Presets Override"), &pOverride))
                    config->DLSSDRenderPresetOverride = pOverride;

                ShowHelpMarker(Translation::Get("Each render preset has it strengths and weaknesses\n"
                               "Override to potentially improve image quality\n"
                               "Press apply after enable/disable"));

                /*
                auto currentPresetIndex = GetPresetIndex(currentFeature, true);

                if (currentPresetIndex == 0)
                    ImGui::Text("Current Preset: Default");
                else
                    ImGui::Text("Current Preset: %c", 64 + currentPresetIndex);
                */

                ImGui::BeginDisabled(!config->DLSSDRenderPresetOverride.value_or_default() /*|| overridden*/);
                ImGui::PushItemWidth(135.0f * menuResScale);

                AddDLSSDRenderPreset(Translation::Get("Override Preset"), &comboPreset);

                ImGui::PopItemWidth();
                ImGui::EndDisabled();
            }
            else
            {
                if (bool pOverride = config->RenderPresetOverride.value_or_default();
                    ImGui::Checkbox(Translation::Get("Render Presets Override"), &pOverride))
                    config->RenderPresetOverride = pOverride;

                ShowHelpMarker(Translation::Get("Each render preset has it strengths and weaknesses\n"
                               "Override to potentially improve image quality\n"
                               "Press apply after enable/disable"));

                /*
                auto currentPresetIndex = GetPresetIndex(currentFeature, false);

                if (currentPresetIndex == 0)
                    ImGui::Text("Current Preset: Default");
                else
                    ImGui::Text("Current Preset: %c", 64 + currentPresetIndex);
                */

                ImGui::BeginDisabled(!config->RenderPresetOverride.value_or_default() /*|| overridden*/);

                ImGui::PushItemWidth(135.0f * menuResScale);

                AddDLSSRenderPreset(Translation::Get("Override Preset"), &comboPreset);

                ImGui::PopItemWidth();
                ImGui::EndDisabled();
            }

            ImGui::SameLine(0.0f, 6.0f);

            if (ImGui::Button(Translation::Get("Apply Changes")))
            {
                LOG_DEBUG("Applying DLSS/DLSSD preset override changes, preset index: {}",
                          comboPreset.value_or_default());

                if (usesDlssd)
                {
                    config->DLSSDRenderPresetForAll = comboPreset.value_or_default();
                    state.newBackend = Upscaler::DLSSD;
                }
                else
                {
                    config->RenderPresetForAll = comboPreset.value_or_default();
                    state.newBackend = currentBackend;
                }

                MARK_ALL_BACKENDS_CHANGED();
            }

            ImGui::Spacing();

            if (auto ch = ScopedCollapsingHeader(Translation::Get(usesDlssd ? "Advanced DLSSD Settings" : "Advanced DLSS Settings"));
                ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                bool appIdOverride = config->UseGenericAppIdWithDlss.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Use Generic App Id with DLSS"), &appIdOverride))
                    config->UseGenericAppIdWithDlss = appIdOverride;

                ShowHelpMarker(Translation::Get("Use generic appid with NGX\n"
                               "Fixes OptiScaler preset override not working with certain games\n"
                               "Requires a game restart."));

                ImGui::BeginDisabled(!config->RenderPresetOverride.value_or_default() || overridden);
                ImGui::Spacing();
                ImGui::PushItemWidth(135.0f * menuResScale);

                if (usesDlssd)
                {
                    AddDLSSDRenderPreset(Translation::Get("DLAA Preset"), &config->DLSSDRenderPresetDLAA);
                    AddDLSSDRenderPreset(Translation::Get("UltraQ Preset"), &config->DLSSDRenderPresetUltraQuality);
                    AddDLSSDRenderPreset(Translation::Get("Quality Preset"), &config->DLSSDRenderPresetQuality);
                    AddDLSSDRenderPreset(Translation::Get("Balanced Preset"), &config->DLSSDRenderPresetBalanced);
                    AddDLSSDRenderPreset(Translation::Get("Perf Preset"), &config->DLSSDRenderPresetPerformance);
                    AddDLSSDRenderPreset(Translation::Get("UltraP Preset"), &config->DLSSDRenderPresetUltraPerformance);
                }
                else
                {
                    AddDLSSRenderPreset(Translation::Get("DLAA Preset"), &config->RenderPresetDLAA);
                    AddDLSSRenderPreset(Translation::Get("UltraQ Preset"), &config->RenderPresetUltraQuality);
                    AddDLSSRenderPreset(Translation::Get("Quality Preset"), &config->RenderPresetQuality);
                    AddDLSSRenderPreset(Translation::Get("Balanced Preset"), &config->RenderPresetBalanced);
                    AddDLSSRenderPreset(Translation::Get("Perf Preset"), &config->RenderPresetPerformance);
                    AddDLSSRenderPreset(Translation::Get("UltraP Preset"), &config->RenderPresetUltraPerformance);
                }
                ImGui::PopItemWidth();
                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }
    }
}

void MenuCommon::RenderFrameGenerationSelection(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;

    /// FG INPUTS

    static std::vector<MenuOption<FGInput>> inputOptions;
    inputOptions.clear();

    // clang-format off

    inputOptions = {
        { FGInput::NoFG, Translation::Get("None") },
        { FGInput::NvngxFG, Translation::Get("Nukem's/Artur's DLSSG"),
            Translation::Get("Limited to FSR3-FG\n\nSupports Hudless out of the box\n\nUses Streamline swapchain for pacing") },
        { FGInput::FSRFG, Translation::Get("FSR 3.1 FG"),
            Translation::Get("Can be used with any FG Output\n\nSupports Hudless out of the box") },
        { FGInput::DLSSG, Translation::Get("DLSSG via Streamline"),
            Translation::Get("Can be used with any FG Output\n\nSupports Hudless out of the box") },
        { FGInput::XeFG, Translation::Get("XeFG") },
        { FGInput::Upscaler, Translation::Get("OptiFG (Upscaler)"),
            Translation::Get("Upscaler must be enabled\n\nCan be used with any FG Output, but might be imperfect with some\n\nTo prevent UI glitching, HUDfix required") },
        { FGInput::FSRFG30, Translation::Get("FSR 3.0 FG"),
            Translation::Get("Can be used with any FG Output\n\nSupports Hudless out of the box") }
    };

    // clang-format on

    auto constexpr nvngxInputIndex = (uint32_t) FGInput::NvngxFG;
    if (state.activeFgInput == FGInput::NvngxFG)
    {
        if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
            inputOptions[nvngxInputIndex].label = Translation::Get("Artur's DLSSG");
        else
            inputOptions[nvngxInputIndex].label = Translation::Get("Nukem's DLSSG");
    }

    // XeFG input requirements
    auto constexpr xefgInputIndex = (uint32_t) FGInput::XeFG;
    inputOptions[xefgInputIndex].set_disabled(true, Translation::Get("Support not implemented"));

    // OptiFG requirements
    auto constexpr optiFgIndex = (uint32_t) FGInput::Upscaler;
    inputOptions[optiFgIndex].set_disabled(state.swapchainApi == API::DX11 || state.swapchainApi == API::Vulkan,
                                           Translation::Get("Unsupported API"));

    if (!inputOptions[optiFgIndex].disabled && state.activeFgOutput == FGOutput::FSRFG && !FfxApiProxy::IsFGReady() &&
        !ffxInitTried)
    {
        ffxInitTried = true;
        FfxApiProxy::InitFfxDx12();
        inputOptions[optiFgIndex].set_disabled(!FfxApiProxy::IsFGReady(), Translation::Get("amd_fidelityfx_dx12.dll is missing"));
    }
    else if (!inputOptions[optiFgIndex].disabled && state.activeFgOutput == FGOutput::XeFG && !xefgInitTried &&
             XeFGProxy::Module() == nullptr)
    {
        xefgInitTried = true;
        XeFGProxy::InitXeFG();
        inputOptions[optiFgIndex].set_disabled(XeFGProxy::Module() == nullptr, Translation::Get("libxess_fg.dll is missing"));
    }

    // DLSSG inputs requirements
    auto constexpr dlssgInputIndex = (uint32_t) FGInput::DLSSG;
    inputOptions[dlssgInputIndex].set_disabled(state.swapchainApi == API::DX11, Translation::Get("Unsupported API"));

    // FSRFG inputs requirements
    auto constexpr fsrfgInputIndex = (uint32_t) FGInput::FSRFG;
    inputOptions[fsrfgInputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));

    // FSRFG30 inputs requirements
    auto constexpr fsrfg30InputIndex = (uint32_t) FGInput::FSRFG30;
    inputOptions[fsrfg30InputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));

    if (!config->FGInput.has_value())
        config->FGInput = config->FGInput.value_or_default(); // need to have a value before combo

    /// FG OUTPUTS

    static std::vector<MenuOption<FGOutput>> outputOptions;
    outputOptions.clear();

    // clang-format off

    outputOptions = {
        { FGOutput::NoFG, Translation::Get("None") },
        { FGOutput::NvngxFG, Translation::Get("FSR3-FG Nukem/Enabler"), Translation::Get("Uses Game's DLSSG implementation.\nEnable DLSS-FG in-game") },
        { FGOutput::FSRFG, Translation::Get("FSR FG"), Translation::Get("FSR3/4 FG") },
        { FGOutput::DLSSG, Translation::Get("DLSSG"), Translation::Get("For 40xx and above") },
        { FGOutput::XeFG, Translation::Get("XeFG"), Translation::Get("XeFG") },
        { FGOutput::DLSSGWithNvngx, Translation::Get("DLSSG with Nvngx FG"), Translation::Get("Uses Opti's own DLSSG instance and adds NvngxFG on top\nDo not use if a game already has DLSSG\n\nIf a game has DLSSG then use the FG Input option:\n\"Nukem's/Artur's DLSSG\"") }
    };

    // clang-format on

    // DLSSG output requirements
    auto constexpr dlssgOutputIndex = (uint32_t) FGOutput::DLSSG;
    outputOptions[dlssgOutputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));
    outputOptions[dlssgOutputIndex].set_disabled(primaryGpu.nvidiaArchInfo.architecture_id < NV_GPU_ARCHITECTURE_AD100,
                                                 Translation::Get("Unsupported hardware"));

    // Nukem's FG mod requirements
    auto constexpr nvngxOutputIndex = (uint32_t) FGOutput::NvngxFG;
    if (state.activeFgOutput == FGOutput::NvngxFG)
    {
        if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
            outputOptions[nvngxOutputIndex].label = Translation::Get("FSR3-MFG via DLSS Enabler");
        else
            outputOptions[nvngxOutputIndex].label = Translation::Get("FSR3-FG via Nukem's");
    }
    if (!state.nvngxFgFilesAvailable)
    {
        inputOptions[nvngxInputIndex].set_disabled(
            true, Translation::Get("Missing dlssg_to_fsr3_amd_is_better.dll\nor dlss-enabler-headless.dll"));
        outputOptions[nvngxOutputIndex].set_disabled(
            true, Translation::Get("Missing dlssg_to_fsr3_amd_is_better.dll\nor dlss-enabler-headless.dll"));
    }

    // For that one case of DX11 DLSSG
    const auto streamlineVersion = state.streamlineVersion;
    const bool nukemsUnsupportedApi =
        state.swapchainApi == API::DX11 &&
        (streamlineVersion == feature_version { 0, 0, 0 } || streamlineVersion > feature_version { 2, 0, 1 });
    inputOptions[nvngxInputIndex].set_disabled(nukemsUnsupportedApi, Translation::Get("Unsupported API"));
    outputOptions[nvngxOutputIndex].set_disabled(nukemsUnsupportedApi, Translation::Get("Unsupported API"));

    auto constexpr DLSSGWithNvngxOutputIndex = (uint32_t) FGOutput::DLSSGWithNvngx;
    if (!state.nvngxFgFilesAvailable)
    {
        outputOptions[DLSSGWithNvngxOutputIndex].set_disabled(true, Translation::Get("Missing the dlssg_to_fsr3_amd_is_better.dll file"));
    }
    outputOptions[DLSSGWithNvngxOutputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));

    // FSR FG output requirements
    auto constexpr fsrfgOutputIndex = (uint32_t) FGOutput::FSRFG;
    outputOptions[fsrfgOutputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));

    // XeFG output requirements
    auto constexpr xefgOutputIndex = (uint32_t) FGOutput::XeFG;
    outputOptions[xefgOutputIndex].set_disabled(state.swapchainApi != API::DX12, Translation::Get("Unsupported API"));

    // Unsupported FG input selected
    const auto currentInputIndex = (uint32_t) state.activeFgInput;
    if (config->FGInput != FGInput::NoFG && inputOptions.size() > currentInputIndex &&
        inputOptions[currentInputIndex].disabled && state.activeFgInput == config->FGInput)
    {
        LOG_WARN("Resetting FGInput to NoFG: {}", inputOptions[currentInputIndex].label);
        config->FGInput = FGInput::NoFG;

        // Changing active can be dangerous but we are talking about an unsupported mode
        // which shouldn't even actually have taken affect
        state.activeFgInput = FGInput::NoFG;
    }

    // Unsupported FG output selected
    const auto currentOutputIndex = (uint32_t) state.activeFgOutput;
    if (config->FGOutput != FGOutput::NoFG && outputOptions.size() > currentOutputIndex &&
        outputOptions[currentOutputIndex].disabled && state.activeFgOutput == config->FGOutput)
    {
        LOG_WARN("Resetting FGOutput to NoFG: {}", outputOptions[currentOutputIndex].label);
        config->FGOutput = FGOutput::NoFG;
        state.activeFgOutput = FGOutput::NoFG;
    }

    if (!config->FGOutput.has_value())
        config->FGOutput = config->FGOutput.value_or_default(); // need to have a value before combo

    if (state.activeFgInput != FGInput::ForceXeLL)
    {
        ImGui::SeparatorText(Translation::Get("Frame Generation"));

        if (ImGui::BeginTable("fgSelection", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextColumn();

            PopulateCombo(Translation::Get("FG Input"), config->FGInput, inputOptions);
            ShowTooltip(Translation::Get("The data source to be used for FG\n"
                        "The native FG which the game supports"));

            ImGui::TableNextColumn();

            const bool disableOutputs = config->FGInput.value_or_default() == FGInput::NvngxFG;

            ImGui::BeginDisabled(disableOutputs);
            PopulateCombo(Translation::Get("FG Output"), config->FGOutput, outputOptions);
            ImGui::EndDisabled();

            if (disableOutputs)
                ShowTooltip(Translation::Get("Doesn't matter with the selected FG Source"));
            else
                ShowTooltip(Translation::Get("The FG that you will actually be using"));

            ImGui::EndTable();
        }

        auto static fgInputOverridden = false;

        if (config->FGOutput == FGOutput::NvngxFG && !fgInputOverridden)
        {
            config->FGInput = FGInput::NvngxFG;
            fgInputOverridden = true;
        }
        else if (config->FGInput != FGInput::NvngxFG && fgInputOverridden)
        {
            config->FGOutput = FGOutput::NoFG;
            fgInputOverridden = false;
        }

        state.fgSettingsChanged = state.activeFgOutput != config->FGOutput.value_or_default() ||
                                  state.activeFgInput != config->FGInput.value_or_default();

        if (state.fgSettingsChanged)
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.0f, 1.f)),
                               Translation::Get("Save Settings and restart to apply the changes"));
            ImGui::Spacing();
        }

        const bool dlssgInputOrOutput = state.activeFgOutput == FGOutput::DLSSG ||
                                        state.activeFgOutput == FGOutput::DLSSGWithNvngx ||
                                        state.activeFgInput == FGInput::DLSSG;

        ImGui::BeginDisabled(state.dlssgGameDMFGSupported && config->FGDLSSGOverrideForceDMFG.value_or_default());
        if (state.dlssgMfgMax.has_value() && state.dlssgMfgMax.value() >= 1 && !dlssgInputOrOutput)
        {
            auto maxInterpolationCount = state.dlssgMfgMax.value();

            if (maxInterpolationCount >= 1)
            {
                const char* intModes[] = { Translation::Get("Default"), "Off", "2X", "3X", "4X", "5X", "6X" };

                // Map config value to UI index
                int currentSet = 0;
                if (config->FGDLSSGOverrideInterpolationCount.has_value())
                {
                    currentSet = config->FGDLSSGOverrideInterpolationCount.value() + 1;
                }

                const char* currentIntCount = intModes[currentSet];

                ImGui::PushItemWidth(95.0f * menuResScale);

                if (ImGui::BeginCombo(Translation::Get("Override DLSSG"), currentIntCount))
                {
                    for (int i = 0; i <= maxInterpolationCount + 1; i++)
                    {
                        if (ImGui::Selectable(intModes[i], (currentSet == i)))
                        {
                            if (i == 0)
                            {
                                // Default, no override
                                config->FGDLSSGOverrideInterpolationCount.reset();
                            }
                            else
                            {
                                // UI index, store value
                                int framesToGenerate = i - 1;

                                LOG_DEBUG("DLSSG Interpolation Count set to: {}", framesToGenerate);
                                config->FGDLSSGOverrideInterpolationCount = framesToGenerate;
                            }

                            StreamlineHooks::updateDlssgOptions();
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::PopItemWidth();
            }
        }

        ImGui::EndDisabled();

        if (state.dlssgGameDMFGSupported && !dlssgInputOrOutput)
        {
            ImGui::SameLine(0.0f, 16.0f);

            if (bool dynamicMFG = config->FGDLSSGOverrideForceDMFG.value_or_default();
                ImGui::Checkbox(Translation::Get("Force Dynamic MFG"), &dynamicMFG))
            {
                config->FGDLSSGOverrideForceDMFG = dynamicMFG;
                StreamlineHooks::updateDlssgOptions();
            }

            ImGui::BeginDisabled(state.dlssgLastSetMode != sl::DLSSGMode::eDynamic);
            static float fpsTarget = config->FGDLSSGFramerateTargetDMFG.value_or_default();
            ImGui::SliderFloat(Translation::Get("DMFG FPS Target"), &fpsTarget, 0, 200, "%.0f");

            ShowHelpMarker(Translation::Get("An active limit of 0 means auto-detect the display refresh rate"));

            if (ImGui::Button(Translation::Get("Apply Target")))
            {
                config->FGDLSSGFramerateTargetDMFG = fpsTarget;
                StreamlineHooks::updateDlssgOptions();
            }

            ImGui::SameLine(0.0f, 16.0f);

            if (ImGui::Button(Translation::Get("Reset Target")))
            {
                fpsTarget = 0.0f;
                config->FGDLSSGFramerateTargetDMFG.reset();
            }

            ImGui::EndDisabled();
        }

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
        if (((state.activeFgOutput == FGOutput::FSRFG || state.activeFgOutput == FGOutput::XeFG ||
              state.activeFgOutput == FGOutput::DLSSG || state.activeFgOutput == FGOutput::DLSSGWithNvngx) &&
             state.activeFgInput != FGInput::NoFG && state.activeFgInput != FGInput::NvngxFG) &&
            fgOutput)
        {
            ImGui::Checkbox(Translation::Get("Show Detected UI"), &state.fgHudlessCompare);
            ShowHelpMarker(Translation::Get("Needs HUDless texture to compare with final image.\n"
                           "UI elements and ONLY UI elements should have a pink tint!"));

            const auto isUsingUIAny = fgOutput->IsUsingUIAny();

            ImGui::BeginDisabled(!isUsingUIAny);

            if (bool drawUIOverFG = config->FGDrawUIOverFG.value_or_default();
                ImGui::Checkbox(Translation::Get("Draw UI over"), &drawUIOverFG))
            {
                config->FGDrawUIOverFG = drawUIOverFG;
            }
            ShowHelpMarker(Translation::Get("Draws UI resource over the final image\n"
                           "If no UI visible, enable this!"));

            ImGui::EndDisabled();

            ImGui::SameLine(0.0f, 16.0f);

            ImGui::BeginDisabled(!isUsingUIAny || !config->FGDrawUIOverFG.value_or_default());

            if (bool uiPremultipliedAlpha = config->FGUIPremultipliedAlpha.value_or_default();
                ImGui::Checkbox(Translation::Get("UI Premult. alpha"), &uiPremultipliedAlpha))
            {
                config->FGUIPremultipliedAlpha = uiPremultipliedAlpha;
            }
            ShowHelpMarker(Translation::Get("If UI is too faint, disable this option"));

            ImGui::EndDisabled();
        }

        const bool showOutputSpecificFGSettings = state.activeFgInput == FGInput::DLSSG ||
                                                  state.activeFgInput == FGInput::FSRFG ||
                                                  state.activeFgInput == FGInput::FSRFG30;

        const bool showHudCutoff = state.activeFgInput == FGInput::NvngxFG ||
                                   state.activeFgOutput == FGOutput::DLSSGWithNvngx ||
                                   state.activeFgOutput == FGOutput::FSRFG;

        if (showOutputSpecificFGSettings || showHudCutoff)
        {
            ImGui::Spacing();

            if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced FG Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                if (showOutputSpecificFGSettings)
                {
                    auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
                    if (fgOutput)
                    {
                        ImGui::BeginDisabled(!fgOutput->IsActive());

                        const auto isUsingUIAny = fgOutput->IsUsingUIAny();
                        const auto isUsingHudlessAny = fgOutput->IsUsingHudlessAny();

                        bool disableUI = config->FGDisableUI.value_or_default();
                        ImGui::BeginDisabled(!isUsingUIAny && !disableUI);

                        if (ImGui::Checkbox(Translation::Get("Disable UI texture"), &disableUI))
                        {
                            config->FGDisableUI = disableUI;
                            fgOutput->UpdateTarget();
                        }

                        ShowHelpMarker(Translation::Get("For when the game sends a UI texture, but you want to disable it"));

                        ImGui::EndDisabled();

                        ImGui::SameLine(0.0f, 16.0f);

                        bool disableHudless = config->FGDisableHudless.value_or_default();
                        ImGui::BeginDisabled(!isUsingHudlessAny && !disableHudless);

                        if (ImGui::Checkbox(Translation::Get("Disable HUDless"), &disableHudless))
                        {
                            config->FGDisableHudless = disableHudless;
                        }

                        ShowHelpMarker(Translation::Get("For when the game sends HUDless, but you want to disable it"));

                        ImGui::EndDisabled();

                        bool depthValidNow = config->FGDepthValidNow.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Depth as ValidNow"), &depthValidNow))
                            config->FGDepthValidNow = depthValidNow;

                        ShowHelpMarker(Translation::Get("Will use more VRAM, but Uniscaler needs this\n"
                                       "Maybe some other games might need too"));

                        ImGui::SameLine(0.0f, 16.0f);

                        bool velocityValidNow = config->FGVelocityValidNow.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Velocity as ValidNow"), &velocityValidNow))
                            config->FGVelocityValidNow = velocityValidNow;

                        ShowHelpMarker(Translation::Get("Will use more VRAM, but Uniscaler needs this\n"
                                       "Maybe some other games might need too"));

                        bool hudlessValidNow = config->FGHudlessValidNow.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("HUDless as ValidNow"), &hudlessValidNow))
                            config->FGHudlessValidNow = hudlessValidNow;

                        ShowHelpMarker(Translation::Get("Will use more VRAM, but some games might need this"));

                        ImGui::SameLine(0.0f, 16.0f);

                        bool firstHudless = config->FGOnlyAcceptFirstHudless.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Accept First HUDless"), &firstHudless))
                            config->FGOnlyAcceptFirstHudless = firstHudless;

                        ShowHelpMarker(Translation::Get("If source tags more than one HUDless, only use the first one"));

                        if (bool skipReset = config->FGSkipReset.value_or_default();
                            ImGui::Checkbox(Translation::Get("Skip Reset"), &skipReset))
                        {
                            config->FGSkipReset = skipReset;
                        }

                        ShowHelpMarker(Translation::Get("Don't use reset signals from FG Inputs"));

                        ImGui::EndDisabled();

                        ImGui::PushItemWidth(80.0f * menuResScale);

                        auto frameAhead = config->FGAllowedFrameAhead.value_or_default();
                        if (ImGui::InputInt(Translation::Get("Frame Ahead"), &frameAhead, 1, 1) && frameAhead > 0 && frameAhead < 4)
                        {
                            config->FGAllowedFrameAhead = frameAhead;
                        }

                        ShowHelpMarker(Translation::Get("Number of frames the FG is allowed to be ahead of the game\n"
                                       "Might prevent FG on/off switching, but also might cause issues"));

                        ImGui::PopItemWidth();

                        ImGui::SameLine(0.0f, 16.0f);

    const char* ftSources[] = { Translation::Get("Input"), Translation::Get("Opti"), Translation::Get("Zero") };
                        const char* ftSourceInfos[] = { Translation::Get("Uses frametimes provided by\nDLSSG or FSR-FG"),
                                                        Translation::Get("Uses frametimes calculated by Opti"),
                                                        Translation::Get("Let XeFG to handle frametimes") };

                        auto currentSet = (int) config->FTInput.value_or_default();
                        auto currentSourceCount = state.activeFgOutput == FGOutput::XeFG ? 3 : 2;

                        ImGui::PushItemWidth(95.0f * menuResScale);

                        if (ImGui::BeginCombo(Translation::Get("FT Input"), ftSources[currentSet]))
                        {
                            for (size_t i = 0; i < currentSourceCount; i++)
                            {

                                if (ImGui::Selectable(ftSources[i], currentSet == i))
                                {
                                    LOG_DEBUG("FTInput has changed {} -> {}", ftSources[currentSet], ftSources[i]);
                                    config->FTInput = (FrameTimeSource) i;
                                }

                                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                                    ImGui::SetTooltip(ftSourceInfos[i]);
                            }

                            ImGui::EndCombo();
                        }

                        ImGui::PopItemWidth();

                        ShowHelpMarker(Translation::Get("Select source for frametime\n"
                                       "Might help frame pacing and stutter issues"));
                    }
                }

                if (showHudCutoff)
                {
                    float fgHudCutoff = config->FGHudCutoff.value_or_default();
                    if (ImGui::SliderFloat(Translation::Get("Hud Cutoff"), &fgHudCutoff, 0.00f, 1.0f, "%.2f"))
                        config->FGHudCutoff = fgHudCutoff;

                    ShowHelpMarker(Translation::Get("Cutoffs transparency from UI to help with interpolation\n"
                                   "You can use Show Detected UI to see the difference\n0.0 is auto"));
                }
            }
        }

        ImGui::Spacing();
    }
}

void MenuCommon::RenderFrameGenerationRuntimeSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;
    auto& primaryGpu = *ctx.primaryGpu;

    // FSR FG controls
    if (state.activeFgOutput == FGOutput::FSRFG && state.activeFgInput != FGInput::NoFG &&
        state.currentFGSwapchain != nullptr)
    {
        if (state.activeFgInput != FGInput::Upscaler ||
            (currentFeature != nullptr && !currentFeature->IsFrozen()) && FfxApiProxy::IsFGReady())
        {
            ImGui::SeparatorText(Translation::Get("Frame Generation (FSR FG)"));

            if (_ffxFGIndex < 0)
                _ffxFGIndex = config->FfxFGIndex.value_or_default();

            if (state.ffxFGVersionNames.size() > 0)
            {
                ImGui::PushItemWidth(135.0f * menuResScale);

                auto currentName = StrFmt("FSR %s", state.ffxFGVersionNames[_ffxFGIndex]);
                if (ImGui::BeginCombo(Translation::Get("FFX FG"), currentName.c_str()))
                {
                    for (int n = 0; n < state.ffxFGVersionIds.size(); n++)
                    {
                        auto name = StrFmt("FSR %s", state.ffxFGVersionNames[n]);
                        if (ImGui::Selectable(name.c_str(), config->FfxFGIndex.value_or_default() == n))
                            _ffxFGIndex = n;
                    }

                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                ShowHelpMarker(Translation::Get("List of FGs reported by FFX SDK"));

                ImGui::SameLine(0.0f, 6.0f);

                if (ImGui::Button(Translation::Get("Change FG")) && _ffxFGIndex != config->FfxFGIndex.value_or_default())
                {
                    config->FfxFGIndex = _ffxFGIndex;
                    state.fgChanged = true;
                    state.scChanged = true;
                }
            }

            bool fgActive = config->FGEnabled.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Active##2"), &fgActive))
            {
                config->FGEnabled = fgActive;
                LOG_DEBUG("FGEnabled set FGEnabled: {}", fgActive);

                if (config->FGEnabled.value_or_default())
                    state.fgChanged = true;
            }
            ShowHelpMarker(Translation::Get("Enable Frame Generation"));

            bool fgAsync = config->FGAsync.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Allow Async"), &fgAsync))
            {
                config->FGAsync = fgAsync;

                if (config->FGEnabled.value_or_default())
                {
                    state.fgChanged = true;
                    state.scChanged = true;
                    LOG_DEBUG("Async set FGChanged");
                }
            }
            ShowHelpMarker(Translation::Get("Enable Async for better FG performance\nMight cause crashes, especially with HUD Fix!"));

            ImGui::SameLine(0.0f, 16.0f);

            bool fgDV = config->FGDebugView.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Debug View##2"), &fgDV))
            {
                config->FGDebugView = fgDV;

                if (config->FGEnabled.value_or_default())
                {
                    state.fgChanged = true;
                    LOG_DEBUG("DebugView set FGChanged");
                }
            }
            ShowHelpMarker(Translation::Get("Enable FSR3.1-FG Debug view\n\n"
                           "Top left: Game Motion Vectors\n"
                           "Top middle: GMV Depth\n"
                           "Top right: Optical Flow MV\n"
                           "Middle: Interpolated frame only\n"
                           "Bottom left: Disocclusion mask\n"
                           "Bottom middle: Interpolation source (w/o UI)\n"
                           "Bottom right: HUDless resource"));

            ImGui::SameLine(0.0f, 16.0f);

            if (state.currentFG && state.currentFG->Version().major > 3)
            {
                if (bool fgwm = config->FSRFGEnableWatermark.value_or_default();
                    ImGui::Checkbox(Translation::Get("Enable Watermark"), &fgwm))
                {
                    LOG_DEBUG("FSRFGEnableWatermark set FGWatermark: {}", fgwm);
                    config->FSRFGEnableWatermark = fgwm;
                }

                ShowHelpMarker(Translation::Get("After changing this option, please Save Settings\n"
                               "It will be applied on next launch."));
            }

            ImGui::Spacing();
            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced FSR FG Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                ImGui::Checkbox(Translation::Get("FG Only Generated"), &state.fgOnlyGenerated);
                ShowHelpMarker(Translation::Get("Display only FSR 3.1 Generated frames"));

                ImGui::SameLine(0.0f, 16.0f);
                auto debugResetLines = config->FGDebugResetLines.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Debug Reset Lines"), &debugResetLines))
                {
                    config->FGDebugResetLines = debugResetLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugResetLines);
                }
                ShowHelpMarker(Translation::Get("Enables drawing of Interpolation skip lines"));

                auto debugTearLines = config->FGDebugTearLines.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Debug Tear Lines"), &debugTearLines))
                {
                    config->FGDebugTearLines = debugTearLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugTearLines);
                }
                ShowHelpMarker(Translation::Get("Enables drawing of Tear and Interpolation skip lines"));

                ImGui::SameLine(0.0f, 16.0f);
                auto debugPacingLines = config->FGDebugPacingLines.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Debug Pacing Lines"), &debugPacingLines))
                {
                    config->FGDebugPacingLines = debugPacingLines;
                    LOG_DEBUG("Enabled set FGDebugLines: {}", debugPacingLines);
                }
                ShowHelpMarker(Translation::Get("Enables drawing of Pacing lines"));

                ImGui::Spacing();
                if (ImGui::TreeNode(Translation::Get("FG Rectangle Settings")))
                {
                    ImGui::PushItemWidth(95.0f * menuResScale);
                    int rectLeft = config->FGRectLeft.value_or(0);
                    if (ImGui::InputInt(Translation::Get("Rect Left"), &rectLeft))
                        config->FGRectLeft = rectLeft;

                    ImGui::SameLine(0.0f, 16.0f);
                    int rectTop = config->FGRectTop.value_or(0);
                    if (ImGui::InputInt(Translation::Get("Rect Top"), &rectTop))
                        config->FGRectTop = rectTop;

                    int rectWidth = config->FGRectWidth.value_or(0);
                    if (ImGui::InputInt(Translation::Get("Rect Width"), &rectWidth))
                        config->FGRectWidth = rectWidth;

                    ImGui::SameLine(0.0f, 16.0f);
                    int rectHeight = config->FGRectHeight.value_or(0);
                    if (ImGui::InputInt(Translation::Get("Rect Height"), &rectHeight))
                        config->FGRectHeight = rectHeight;

                    ImGui::PopItemWidth();
                    ShowHelpMarker(Translation::Get("Frame generation rectangle, adjust for letterboxed content"));

                    ImGui::BeginDisabled(!config->FGRectLeft.has_value() && !config->FGRectTop.has_value() &&
                                         !config->FGRectWidth.has_value() && !config->FGRectHeight.has_value());

                    if (ImGui::Button(Translation::Get("Reset FG Rect")))
                    {
                        config->FGRectLeft.reset();
                        config->FGRectTop.reset();
                        config->FGRectWidth.reset();
                        config->FGRectHeight.reset();
                    }

                    ShowHelpMarker(Translation::Get("Resets Frame generation rectangle"));

                    ImGui::EndDisabled();
                    ImGui::TreePop();
                }

                auto fg = state.currentFG;
                if (fg != nullptr && strcmp(fg->Name(), "FSR-FG") == 0 &&
                    FfxApiProxy::VersionDx12_FG() >= feature_version { 3, 1, 3 })
                {
                    ImGui::Spacing();

                    if (ImGui::TreeNode(Translation::Get("Frame Pacing Tuning")))
                    {
                        auto fptEnabled = config->FGFramePacingTuning.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Enable Tuning"), &fptEnabled))
                        {
                            config->FGFramePacingTuning = fptEnabled;
                            state.fsrfgFramePaceTuningChanged = true;
                        }

                        ImGui::BeginDisabled(!config->FGFramePacingTuning.value_or_default());

                        ImGui::PushItemWidth(115.0f * menuResScale);
                        auto fptSafetyMargin = config->FGFPTSafetyMarginInMs.value_or_default();
                        if (ImGui::InputFloat(Translation::Get("Safety Margins in ms"), &fptSafetyMargin, 0.01f, 0.1f, "%.2f"))
                            config->FGFPTSafetyMarginInMs = fptSafetyMargin;
                        ShowHelpMarker(Translation::Get("Safety margins in millisecons\n"
                                       "FSR default value: 0.1ms\n"
                                       "Opti default value: 0.01ms"));

                        auto fptVarianceFactor = config->FGFPTVarianceFactor.value_or_default();
                        if (ImGui::SliderFloat(Translation::Get("Variance Factor"), &fptVarianceFactor, 0.0f, 1.0f, "%.2f"))
                            config->FGFPTVarianceFactor = fptVarianceFactor;
                        ShowHelpMarker(Translation::Get("Variance factor\n"
                                       "FSR default value: 0.1\n"
                                       "Opti default value: 0.3"));
                        ImGui::PopItemWidth();

                        auto fpHybridSpin = config->FGFPTAllowHybridSpin.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Enable Hybrid Spin"), &fpHybridSpin))
                            config->FGFPTAllowHybridSpin = fpHybridSpin;
                        ShowHelpMarker(Translation::Get("Allows pacing spinlock to sleep, should reduce CPU usage\n"
                                       "Might cause slow ramp up of FPS"));

                        ImGui::PushItemWidth(115.0f * menuResScale);
                        auto fptHybridSpinTime = config->FGFPTHybridSpinTime.value_or_default();
                        if (ImGui::SliderInt(Translation::Get("Hybrid Spin Time"), &fptHybridSpinTime, 0, 100))
                            config->FGFPTHybridSpinTime = fptHybridSpinTime;
                        ShowHelpMarker(Translation::Get("How long to spin if FPTHybridSpin is true. Measured in timer "
                                       "resolution units.\n"
                                       "Not recommended to go below 2. Will result in frequent overshoots"));
                        ImGui::PopItemWidth();

                        auto fpWaitForSingleObjectOnFence =
                            config->FGFPTAllowWaitForSingleObjectOnFence.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Enable WaitForSingleObjectOnFence"), &fpWaitForSingleObjectOnFence))
                        {
                            config->FGFPTAllowWaitForSingleObjectOnFence = fpWaitForSingleObjectOnFence;
                        }
                        ShowHelpMarker(Translation::Get("Allows WaitForSingleObject instead of spinning for fence value"));

                        if (ImGui::Button(Translation::Get("Apply Timing Changes")))
                            state.fsrfgFramePaceTuningChanged = true;

                        ImGui::EndDisabled();
                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }
    }

    // XeFG controls
    if (state.activeFgOutput == FGOutput::XeFG && state.activeFgInput != FGInput::NoFG &&
        state.activeFgInput != FGInput::ForceXeLL && state.currentFGSwapchain != nullptr && XeFGProxy::InitXeFG())
    {
        ImGui::SeparatorText(Translation::Get("Frame Generation (XeFG)"));

        bool ignoreChecks = config->FGXeFGIgnoreInitChecks.value_or_default();

        bool nativeAA = false;
        if (state.activeFgInput == FGInput::Upscaler && currentFeature != nullptr)
            nativeAA = currentFeature->RenderWidth() == currentFeature->DisplayWidth();

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
        const bool correctMVs = fgOutput && fgOutput->IsLowResMV() || nativeAA ||
                                (State::Instance().gameQuirks & GameQuirk::ForceFGRenderSizeMVs) || ignoreChecks;

        if (!correctMVs || state.realExclusiveFullscreen)
        {
            config->FGEnabled.reset();
            config->FGXeFGDebugView.reset();
        }

        const bool restartNeeded =
            fgOutput && (config->FGXeFGDepthInverted.value_or_default() != fgOutput->IsInvertedDepth() ||
                         config->FGXeFGJitteredMV.value_or_default() != fgOutput->IsJitteredMVs() ||
                         config->FGXeFGHighResMV.value_or_default() == fgOutput->IsLowResMV());

        bool cantActivate = false;
        if (restartNeeded)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                               Translation::Get("Restart the game to apply correct XeFG settings!"));
        }
        else
        {
            if (!correctMVs)
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                                   Translation::Get("Requires disabling dilated motion vectors"));

            if (!ignoreChecks && state.realExclusiveFullscreen)
            {
                cantActivate = true;
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("Borderless display mode required!"));
            }

            if (!ignoreChecks && state.isHdrActive)
            {
                if (state.currentSwapchainDesc.BufferDesc.Format > 0 &&
                    state.currentSwapchainDesc.BufferDesc.Format < 15)
                {
                    cantActivate = true;
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.0f, 0.0f, 1.f)), Translation::Get("XeFG only supports HDR10"));
                }
            }
        }

        if (!correctMVs || cantActivate || ignoreChecks)
        {
            if (ImGui::Checkbox(Translation::Get("Ignore Init Checks"), &ignoreChecks))
                config->FGXeFGIgnoreInitChecks = ignoreChecks;

            ShowHelpMarker(Translation::Get("Ignores all prechecks for XeFG\n"
                           "Don't use this option to skip MV size warning for UE games!\n"
                           "It might cause crashes and bad IQ!"));
        }

        ImGui::BeginDisabled(!correctMVs || cantActivate);

        bool fgActive = config->FGEnabled.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Active##3"), &fgActive))
        {
            config->FGEnabled = fgActive;
            LOG_DEBUG("Enabled set FGEnabled: {}", fgActive);

            if (config->FGEnabled.value_or_default())
                state.fgChanged = true;
        }

        ShowHelpMarker(Translation::Get("Enable Frame Generation"));

        auto maxInterpolationCount = state.xefgMaxInterpolationCount;

        if (maxInterpolationCount > 1)
        {
            ImGui::SameLine(0.0f, 16.0f);

        const char* intModes[] = { "2X", "3X", "4X", "5X", "6X" };
        auto currentSet = config->FGXeFGInterpolationCount.value_or_default() - 1;
            auto currentIntCount = intModes[currentSet];

            ImGui::PushItemWidth(95.0f * menuResScale);

            if (ImGui::BeginCombo(Translation::Get("MFG"), currentIntCount))
            {
                for (int i = 0; i < maxInterpolationCount; i++)
                {
                    if (ImGui::Selectable(intModes[i], (currentSet == i)))
                    {
                        LOG_DEBUG("XeFG Interpolation Count set to: {}", i + 1);
                        state.fgChanged = true;
                        config->FGXeFGInterpolationCount = i + 1;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            ShowHelpMarker(Translation::Get("Set XeFG interpolation count"));
        }

        ImGui::SameLine(0.0f, 16.0f);
        ImGui::BeginDisabled(!fgOutput->IsUsingHudlessAny() || XeFGProxy::SetUiCompositionState() == nullptr);
        bool fgCompositeUI = config->FGXeFGUIComposition.value_or_default();
        if (ImGui::Checkbox(Translation::Get("UI Composition"), &fgCompositeUI))
            config->FGXeFGUIComposition = fgCompositeUI;

        ShowHelpMarker(Translation::Get("Disable HUD/UI interpolation\n"
                       "Reverts back to previous XeFG 2 behaviour\n\n"
                       "Fixes artifacting transparent HUD/UI"));
        ImGui::EndDisabled();

        bool fgDV = config->FGXeFGDebugView.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Debug View##2"), &fgDV))
        {
            config->FGXeFGDebugView = fgDV;

            if (config->FGXeFGDebugView.value_or_default())
            {
                state.fgChanged = true;
                LOG_DEBUG("DebugView set FGChanged");
            }
        }
        ShowHelpMarker(Translation::Get("Enable XeFG Debug view"));

        ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 16.0f);
        bool fgBorderless = config->FGXeFGForceBorderless.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Force Borderless"), &fgBorderless))
            config->FGXeFGForceBorderless = fgBorderless;

        ShowHelpMarker(Translation::Get("Forces Borderless display mode\n\n"
                       "For best results, set fullscreen \n"
                       "resolution to your display resolution\n"
                       "Might cause some instability issues.\n\n"
                       "NEEDS GAME RESTART TO BE ACTIVE!"));

        // Disable this for now
        // ImGui::SameLine(0.0f, 16.0f);
        // ImGui::Checkbox("Only Generated##2", &state.fgOnlyGenerated);
        // ShowHelpMarker("Display only XeFG generated frames");

        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced XeFG Settings")); ch.IsHeaderOpen())
        {
            ImGui::Spacing();
            if (ImGui::TreeNode(Translation::Get("Rectangle Settings")))
            {
                ImGui::PushItemWidth(95.0f * menuResScale);
                int rectLeft = config->FGRectLeft.value_or(0);
                if (ImGui::InputInt(Translation::Get("Rect Left##2"), &rectLeft))
                    config->FGRectLeft = rectLeft;

                ImGui::SameLine(0.0f, 16.0f);
                int rectTop = config->FGRectTop.value_or(0);
                if (ImGui::InputInt(Translation::Get("Rect Top##2"), &rectTop))
                    config->FGRectTop = rectTop;

                int rectWidth = config->FGRectWidth.value_or(0);
                if (ImGui::InputInt(Translation::Get("Rect Width##2"), &rectWidth))
                    config->FGRectWidth = rectWidth;

                ImGui::SameLine(0.0f, 16.0f);
                int rectHeight = config->FGRectHeight.value_or(0);
                if (ImGui::InputInt(Translation::Get("Rect Height##2"), &rectHeight))
                    config->FGRectHeight = rectHeight;

                ImGui::PopItemWidth();
                ShowHelpMarker(Translation::Get("Frame generation rectangle, adjust for letterboxed content##2"));

                ImGui::BeginDisabled(!config->FGRectLeft.has_value() && !config->FGRectTop.has_value() &&
                                     !config->FGRectWidth.has_value() && !config->FGRectHeight.has_value());

                if (ImGui::Button(Translation::Get("Reset FG Rect##2")))
                {
                    config->FGRectLeft.reset();
                    config->FGRectTop.reset();
                    config->FGRectWidth.reset();
                    config->FGRectHeight.reset();
                }

                ShowHelpMarker(Translation::Get("Resets Frame generation rectangle##2"));

                ImGui::EndDisabled();
                ImGui::TreePop();
            }

            ImGui::Spacing();
            ImGui::Spacing();
        }
    }

    // DLSSG controls
    if ((state.activeFgOutput == FGOutput::DLSSG || state.activeFgOutput == FGOutput::DLSSGWithNvngx) &&
        state.activeFgInput != FGInput::NoFG && state.currentFGSwapchain != nullptr &&
        StreamlineProxy::LoadStreamline())
    {

        ImGui::SeparatorText(Translation::Get("Frame Generation (DLSSG)"));

        ImGui::Text(Translation::Get("Current DLSSG state:"));
        ImGui::SameLine();
        if (auto count = state.dlssgDetectedInterpolationCount; count > 0)
        {
            auto onText = std::string(Translation::Get("ON {}x"));
            onText.replace(onText.find("{}"), 2, std::to_string(static_cast<int>(count + 1)));
            ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), onText.c_str());
        }
        else
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("OFF"));
        }

        bool fgActive = config->FGEnabled.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Active##4"), &fgActive))
        {
            config->FGEnabled = fgActive;
            LOG_DEBUG("Enabled set FGEnabled: {}", fgActive);

            if (config->FGEnabled.value_or_default())
                state.fgChanged = true;
        }

        ShowHelpMarker(Translation::Get("Enable Frame Generation"));

        auto maxInterpolationCount = state.dlssgMaxInterpolationCount;

        if (maxInterpolationCount > 1)
        {
            ImGui::SameLine(0.0f, 16.0f);

            ImGui::BeginDisabled(config->FGDLSSGForceDMFG.value_or_default());

            const char* intModes[] = { "2X", "3X", "4X", "5X", "6X" };
            auto currentSet = config->FGDLSSGInterpolationCount.value_or_default() - 1;
            auto currentIntCount = intModes[currentSet];

            ImGui::PushItemWidth(95.0f * menuResScale);

            if (ImGui::BeginCombo(Translation::Get("MFG"), currentIntCount))
            {
                for (int i = 0; i < maxInterpolationCount; i++)
                {
                    if (ImGui::Selectable(intModes[i], (currentSet == i)))
                    {
                        LOG_DEBUG("DLSSG Interpolation Count set to: {}", i + 1);
                        config->FGDLSSGInterpolationCount = i + 1;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            ShowHelpMarker(Translation::Get("Set DLSSG interpolation count"));

            ImGui::EndDisabled();

            if (state.dlssgOptiDMFGSupported)
            {
                ImGui::SameLine(0.0f, 16.0f);

                if (bool dynamicMFG = config->FGDLSSGForceDMFG.value_or_default();
                    ImGui::Checkbox(Translation::Get("Force Dynamic MFG"), &dynamicMFG))
                {
                    config->FGDLSSGForceDMFG = dynamicMFG;
                }

                ImGui::BeginDisabled(!config->FGDLSSGForceDMFG.value_or_default());
                static float fpsTarget = config->FGDLSSGFramerateTargetDMFG.value_or_default();
                ImGui::SliderFloat(Translation::Get("DMFG FPS Target"), &fpsTarget, 0, 200, "%.0f");

                ShowHelpMarker(Translation::Get("An active limit of 0 means auto-detect the display refresh rate"));

                if (ImGui::Button(Translation::Get("Apply Target")))
                {
                    config->FGDLSSGFramerateTargetDMFG = fpsTarget;
                }

                ImGui::SameLine(0.0f, 16.0f);

                if (ImGui::Button(Translation::Get("Reset Target")))
                {
                    fpsTarget = 0.0f;
                    config->FGDLSSGFramerateTargetDMFG.reset();
                }

                ImGui::EndDisabled();
            }
        }

        bool useGamesMarkers = config->FGDLSSGUseGamesReflexMarkers.value_or_default();
        ImGui::BeginDisabled(!ReflexHooks::gameIsSendingMarkers());
        if (ImGui::Checkbox(Translation::Get("Use Game's Reflex Markers"), &useGamesMarkers))
        {
            config->FGDLSSGUseGamesReflexMarkers = useGamesMarkers;
            LOG_DEBUG("Changed set FGDLSSGUseGamesReflexMarkers: {}", useGamesMarkers);
        }
        ImGui::EndDisabled();
    }

    // OptiFG
    if (state.api != API::Vulkan && state.currentFGSwapchain != nullptr && state.activeFgInput == FGInput::Upscaler)
    {
        SeparatorWithHelpMarker(Translation::Get("Frame Generation (OptiFG)"), Translation::Get("Using upscaler data for FG"));

        if (currentFeature != nullptr && !currentFeature->IsFrozen() &&
            ((state.activeFgOutput == FGOutput::FSRFG && FfxApiProxy::IsFGReady()) ||
             (state.activeFgOutput == FGOutput::XeFG && XeFGProxy::Module() != nullptr) ||
             ((state.activeFgOutput == FGOutput::DLSSG || state.activeFgOutput == FGOutput::DLSSGWithNvngx) &&
              StreamlineProxy::Module() != nullptr)))
        {
            if (!Config::Instance()->FGDisableHUDFix.value_or_default() &&
                state.swapchainInteropApi == SwapchainInteropApi::None)
            {
                bool fgHudfix = config->FGHUDFix.value_or_default();

                if (ImGui::Checkbox(Translation::Get("HUDFix"), &fgHudfix))
                {
                    config->FGHUDFix = fgHudfix;
                    LOG_DEBUG("Enabled set FGHUDFix: {}", fgHudfix);
                    state.clearCapturedHudlesses = true;
                    state.fgChanged = true;
                }

                ShowHelpMarker(Translation::Get("Enable HUD stability fix, might cause crashes!"));

                ImGui::BeginDisabled(!config->FGHUDFix.value_or_default());

                ImGui::SameLine(0.0f, 16.0f);
                ImGui::PushItemWidth(95.0f * menuResScale);
                int hudFixLimit = config->FGHUDLimit.value_or_default();
                if (ImGui::InputInt(Translation::Get("Limit"), &hudFixLimit))
                {
                    if (hudFixLimit < 1)
                        hudFixLimit = 1;
                    else if (hudFixLimit > 999)
                        hudFixLimit = 999;

                    config->FGHUDLimit = hudFixLimit;
                    LOG_DEBUG("Enabled set FGHUDLimit: {}", hudFixLimit);
                }
                ShowHelpMarker(Translation::Get("Delay HUDless capture, high values might cause crash!"));

                ImGui::SameLine(0.0f, 16.0f);
                if (ImGui::Button(Translation::Get("Res##2")))
                    _showHudlessWindow = !_showHudlessWindow;

                ImGui::EndDisabled();

                auto hudExtended = config->FGHUDFixExtended.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Extended"), &hudExtended))
                {
                    LOG_DEBUG("Enabled set FGHUDFixExtended: {}", hudExtended);
                    config->FGHUDFixExtended = hudExtended;
                }
                ShowHelpMarker(Translation::Get("Extended format checks for possible Hudless\nMight cause crashes and slowdowns!"));
                ImGui::SameLine(0.0f, 16.0f);

                ImGui::BeginDisabled(!config->FGHUDFix.value_or_default());

                auto immediate = config->FGImmediateCapture.value_or_default();
                if (ImGui::Checkbox(Translation::Get("Immediate Capture"), &immediate))
                {
                    LOG_DEBUG("Enabled set FGImmediateCapture: {}", immediate);
                    config->FGImmediateCapture = immediate;
                }
                ShowHelpMarker(Translation::Get("Enables capturing of resources before shader execution.\nIncrease Hudless "
                               "capture chances, but might cause capturing of unnecessary resources."));

                ImGui::PopItemWidth();

                ImGui::EndDisabled();
            }
            bool depthScale = config->FGEnableDepthScale.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Scale Depth to fix DLSS RR"), &depthScale))
                config->FGEnableDepthScale = depthScale;
            ShowHelpMarker(Translation::Get("Fix for DLSS-D wrong depth inputs"));

            bool resourceFlip = config->FGResourceFlip.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Flip (Unity)"), &resourceFlip))
                config->FGResourceFlip = resourceFlip;
            ShowHelpMarker(Translation::Get("Flip Velocity & Depth resources of Unity games"));

            ImGui::SameLine(0.0f, 16.0f);

            bool resourceFlipOffset = config->FGResourceFlipOffset.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Flip Use Offset"), &resourceFlipOffset))
                config->FGResourceFlipOffset = resourceFlipOffset;
            ShowHelpMarker(Translation::Get("Use height difference as offset"));

            ImGui::Spacing();

            if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced OptiFG Settings")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};

                if (!Config::Instance()->FGDisableHUDFix.value_or_default() &&
                    state.swapchainInteropApi == SwapchainInteropApi::None)
                {
                    ImGui::Spacing();

                    auto rb = config->FGResourceBlocking.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("Resource Blocking"), &rb))
                    {
                        config->FGResourceBlocking = rb;
                        LOG_DEBUG("Enabled set FGResourceBlocking: {}", rb);
                    }
                    ShowHelpMarker(Translation::Get("Block rarely used resources from using as Hudless \n"
                                   "to prevent flickers and other issues\n\n"
                                   "HUDfix enable/disable will reset the block list!"));

                    ImGui::SameLine(0.0f, 16.0f);

                    auto rrc = config->FGRelaxedResolutionCheck.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("Relaxed Resource Check"), &rrc))
                    {
                        config->FGRelaxedResolutionCheck = rrc;
                        LOG_DEBUG("Enabled set FGRelaxedResolutionCheck: {}", rrc);
                    }
                    ShowHelpMarker(Translation::Get("Relax resolution checks for Hudless by 32 pixels \n"
                                   "Helps games which use black borders for some \n"
                                   "resolutions and screen ratios (e.g. Witcher 3)"));

                    ImGui::BeginDisabled(state.fgResetCapturedResources);
                    ImGui::PushItemWidth(95.0f * menuResScale);
                    if (ImGui::Checkbox(Translation::Get("FG Create List"), &state.fgCaptureResources))
                    {
                        if (!state.fgCaptureResources)
                            config->FGHUDLimit = 1;
                        else
                            state.fgOnlyUseCapturedResources = false;
                    }

                    ImGui::SameLine(0.0f, 16.0f);
                    if (ImGui::Checkbox(Translation::Get("FG Use List"), &state.fgOnlyUseCapturedResources))
                    {
                        if (state.fgCaptureResources)
                        {
                            state.fgCaptureResources = false;
                            config->FGHUDLimit = 1;
                        }
                    }

                    ImGui::SameLine(0.0f, 8.0f);
                    ImGui::Text(Translation::Get("(%d)"), state.fgCapturedResourceCount);

                    ImGui::PopItemWidth();

                    ImGui::SameLine(0.0f, 16.0f);

                    if (ImGui::Button(Translation::Get("Reset List")))
                    {
                        LOG_DEBUG("Resetting captured resource list");

                        state.fgResetCapturedResources = true;
                        state.fgOnlyUseCapturedResources = false;
                    }

                    ImGui::EndDisabled();

                    ImGui::Spacing();
                    ImGui::Spacing();
                    if (ImGui::TreeNode(Translation::Get("Tracking Settings")))
                    {
                        auto ath = config->FGAlwaysTrackHeaps.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Always Track Heaps"), &ath))
                        {
                            config->FGAlwaysTrackHeaps = ath;
                            LOG_DEBUG("Enabled set FGAlwaysTrackHeaps: {}", ath);
                        }
                        ShowHelpMarker(Translation::Get("Always track resources, might cause performance issues\n, but also might "
                                       "fix HUDFix related crashes!"));

                        auto disableRTV = config->FGHudfixDisableRTV.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable RTV Tracking"), &disableRTV))
                            config->FGHudfixDisableRTV = disableRTV;
                        ShowHelpMarker(Translation::Get("Disable tracking of CreateRenderTargetView\n"
                                       "This might help filtering of wrong hudless resources"));

                        ImGui::SameLine(0.0f, 16.0f);

                        auto disableSRV = config->FGHudfixDisableSRV.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable SRV Tracking"), &disableSRV))
                            config->FGHudfixDisableSRV = disableSRV;
                        ShowHelpMarker(Translation::Get("Disable tracking of CreateShaderResourceView\n"
                                       "This might help filtering of wrong Hudless resources"));

                        auto disableUAV = config->FGHudfixDisableUAV.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable UAV Tracking"), &disableUAV))
                            config->FGHudfixDisableUAV = disableUAV;
                        ShowHelpMarker(Translation::Get("Disable tracking of CreateUnorderedAccessView\n"
                                       "This might help filtering of wrong Hudless resources"));

                        ImGui::SameLine(0.0f, 16.0f);

                        auto disableOM = config->FGHudfixDisableOM.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable OM Tracking"), &disableOM))
                            config->FGHudfixDisableOM = disableOM;
                        ShowHelpMarker(Translation::Get("Disable tracking of OMSetRenderTargets\n"
                                       "This might help filtering of wrong Hudless resources"));

                        auto disableSCR = config->FGHudfixDisableSCR.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable SCR Tracking"), &disableSCR))
                            config->FGHudfixDisableSCR = disableSCR;
                        ShowHelpMarker(Translation::Get("Disable tracking of SetComputeRootDescriptorTable\n"
                                       "This might help filtering of wrong Hudless resources"));

                        ImGui::SameLine(0.0f, 16.0f);

                        auto disableSGR = config->FGHudfixDisableSGR.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable SGR Tracking"), &disableSGR))
                            config->FGHudfixDisableSGR = disableSGR;
                        ShowHelpMarker(Translation::Get("Disable tracking of SetGraphicsRootDescriptorTable\n"
                                       "This might help filtering of wrong Hudless resources"));

                        ImGui::Spacing();

                        auto disableDI = config->FGHudfixDisableDI.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable DI Tracking"), &disableDI))
                            config->FGHudfixDisableDI = disableDI;
                        ShowHelpMarker(Translation::Get("Disable tracking of DrawInstanced\n"
                                       "This might help filtering of wrong Hudless resources"));

                        ImGui::SameLine(0.0f, 16.0f);

                        auto disableDII = config->FGHudfixDisableDII.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable DII Tracking"), &disableDII))
                            config->FGHudfixDisableDII = disableDII;
                        ShowHelpMarker(Translation::Get("Disable tracking of DrawIndexedInstanced\n"
                                       "This might help filtering of wrong Hudless resources"));

                        auto disableDispatch = config->FGHudfixDisableDispatch.value_or_default();
                        if (ImGui::Checkbox(Translation::Get("Disable Dispatch Tracking"), &disableDispatch))
                            config->FGHudfixDisableDispatch = disableDispatch;
                        ShowHelpMarker(Translation::Get("Disable tracking of Dispatch\n"
                                       "This might help filtering of wrong Hudless resources"));

                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing();
                if (ImGui::TreeNode(Translation::Get("Resource Settings")))
                {
                    bool makeMVCopies = config->FGMakeMVCopy.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("FG Make MV Copies"), &makeMVCopies))
                        config->FGMakeMVCopy = makeMVCopies;
                    ShowHelpMarker(Translation::Get("Make a copy of motion vectors to use with OptiFG\n"
                                   "For preventing corruptions that might happen"));

                    bool makeDepthCopies = config->FGMakeDepthCopy.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("FG Make Depth Copies"), &makeDepthCopies))
                        config->FGMakeDepthCopy = makeDepthCopies;
                    ShowHelpMarker(Translation::Get("Make a copy of depth to use with OptiFG\n"
                                   "For preventing corruptions that might happen"));

                    ImGui::PushItemWidth(115.0f * menuResScale);
                    float depthScaleMax = config->FGDepthScaleMax.value_or_default();
                    if (ImGui::InputFloat(Translation::Get("FG Scale Depth Max"), &depthScaleMax, 10.0f, 100.0f, "%.1f"))
                        config->FGDepthScaleMax = depthScaleMax;
                    ShowHelpMarker(Translation::Get("Depth values will be divided to this value"));
                    ImGui::PopItemWidth();

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                if (ImGui::TreeNode(Translation::Get("Syncing Settings")))
                {
                    bool useMutexForPresent = config->FGUseMutexForSwapchain.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("FG Use Mutex for Present"), &useMutexForPresent))
                        config->FGUseMutexForSwapchain = useMutexForPresent;
                    ShowHelpMarker(Translation::Get("Use mutex to prevent desync of FG and crashes\n"
                                   "Disabling might improve the perf but decrease stability"));

                    ImGui::TreePop();
                }

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }
        else if (currentFeature == nullptr || currentFeature->IsFrozen())
        {
            ImGui::Text(Translation::Get("Upscaler is not active")); // Probably never will be visible
        }
        else if (state.activeFgOutput == FGOutput::FSRFG && !FfxApiProxy::IsFGReady())
        {
            ImGui::TextColored(toneMapColor({ 1.0f, 0.0f, 0.0f, 1.0f }),
                               Translation::Get("amd_fidelityfx_dx12.dll is missing!")); // Probably never will be visible
        }
        else if (state.activeFgOutput == FGOutput::XeFG && XeFGProxy::Module() == nullptr)
        {
            ImGui::TextColored(toneMapColor({ 1.0f, 0.0f, 0.0f, 1.0f }),
                               Translation::Get("libxess_fg.dll is missing!")); // Probably never will be visible
        }
    }

    // Nvngx FG Mods
    if ((state.activeFgInput == FGInput::NvngxFG && state.activeFgOutput == FGOutput::NvngxFG) ||
        state.activeFgOutput == FGOutput::DLSSGWithNvngx)
    {
        if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
        {
            SeparatorWithHelpMarker(Translation::Get("Frame Generation (FSR3-MFG via DLSS Enabler)"),
                                    Translation::Get("DLSS Enabler as dlss-enabler-headless.dll\nSelect DLSS-FG in-game"));
        }
        else
        {
            SeparatorWithHelpMarker(Translation::Get("Frame Generation (FSR3-FG via Nukem's DLSSG)"),
                                    Translation::Get("Requires Nukem's dlssg_to_fsr3 dll\nSelect DLSS-FG in-game"));
        }

        if (!state.nvngxFgFilesAvailable)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                               Translation::Get("Please put dlssg_to_fsr3_amd_is_better.dll or dlss-enabler-headless.dll next to OptiScaler"));
        }

        if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                               Translation::Get("Using a subset of features from DLSS Enabler"));
        }

        if (state.activeFgOutput != FGOutput::DLSSGWithNvngx)
        {

            bool dmfgActive = state.dlssgGameDMFGSupported && config->FGDLSSGOverrideForceDMFG.value_or_default();

            if (!ReflexHooks::isReflexHooked())
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("Reflex not hooked"));
                ImGui::Text(Translation::Get("If you are using an AMD/Intel GPU, then make sure you have Fakenvapi"));
            }
            else if (ReflexHooks::dlssgFrameCountToGenerate() == 0 && !dmfgActive)
            {
                ImGui::Text(Translation::Get("Please select DLSS Frame Generation in the game options\n"
                            "You might need to select DLSS first"));
            }

            if (state.swapchainApi == DX12)
            {
                ImGui::Text(Translation::Get("Current DLSSG state:"));
                ImGui::SameLine();
                if (auto count = state.dlssgDetectedInterpolationCount; count > 0)
                {
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)),
                                       StrFmt(Translation::Get("ON %dx"), count + 1).c_str());
                }
                else
                {
                    ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("OFF"));
                }

                // Issue mostly shows up on AMD on Windows on pre-RDNA3 in some non-UE games
                // Hide to reduce confusion, config is still read
                bool isUnrealEngine = State::Instance().NVNGX_Engine == NVSDK_NGX_ENGINE_TYPE_UNREAL ||
                                      State::Instance().gameQuirks & GameQuirk::ForceUnrealEngine;
                if (!primaryGpu.dlssCapable && primaryGpu.fsr4Support == FSR4Support::None &&
                    !primaryGpu.usesVkd3dProton && !isUnrealEngine)
                {
                    if (bool makeDepthCopy = config->NvngxFGMakeDepthCopy.value_or_default();
                        ImGui::Checkbox(Translation::Get("Fix broken visuals"), &makeDepthCopy))
                    {
                        config->NvngxFGMakeDepthCopy = makeDepthCopy;
                    }
                    ShowHelpMarker(Translation::Get("Makes a copy of the depth buffer\nCan fix broken visuals in some games on AMD "
                                   "GPUs under Windows\nCan cause stutters, so best to use only when necessary"));
                }
            }
            else if (state.swapchainApi == Vulkan)
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                                   Translation::Get("DLSSG is purposefully disabled when this menu is visible"));
                ImGui::Spacing();
            }
        }

        if (Nvngx_FG::isLoaded(state.swapchainApi))
        {
            if (bool disableHudless = config->NvngxFGDisableHudless.value_or_default();
                ImGui::Checkbox(Translation::Get("Disable Hudless"), &disableHudless))
            {
                config->NvngxFGDisableHudless = disableHudless;
            }
            ShowHelpMarker(Translation::Get("Might be required for some sets of DispatchFlags"));

            if (Nvngx_FG::getMaxFakeFramesCount(state.swapchainApi) > 1)
            {
                if (bool showDebug = config->NvngxFGShowDebug.value_or_default();
                    ImGui::Checkbox(Translation::Get("Show Debug"), &showDebug))
                {
                    config->NvngxFGShowDebug = showDebug;
                }
                ShowHelpMarker(Translation::Get("Required for Debug flags to work correctly"));

                static std::vector<FlagDefinition> known_flags = {
                    { "FRAME_INDEX_LINE", 0x00010000, "" },
                    { "HUD_DETECTION", 0x00020000, "" },
                    { "DISOCCLUSION_TINT", 0x00040000, "" },
                    { "ARTIFACTS_DETECTION", 0x00080000, "" },
                    { "ANTIGHOSTING_ENABLE", 0x00100000, Translation::Get("Enable anti-ghosting correction") },
                    { "ANTIGHOSTING_RED_TINT", 0x00200000, Translation::Get("Debug: red tint on corrected pixels") },
                    { "ANTIGHOSTING_SPLIT_SCREEN", 0x00400000, Translation::Get("Debug: split screen comparison") },
                    { "CAMERA_MV_DEBUG", 0x00800000, Translation::Get("Debug: blue tint where camera MV fallback is used") },
                    { "TRAPEZOID_VIS", 0x01000000, Translation::Get("Debug: trapezoid zone visualization") },
                    { "HUDLESS_UI_MASK", 0x02000000, Translation::Get("Use HUD-less as UI mask (DL2 inverted semantics)") },
                    { "TEMPORAL_HUD_PIN", 0x04000000, Translation::Get("Enable temporal HUD pinning (present-backbuffer stability)") },
                    { "HUD_INTERPOLATION", 0x08000000, Translation::Get("HUD OF interpolation (0=legacy pin-present, 1=OF warp)") },
                    { "IGNORE_UI_TEXTURE", 0x10000000, Translation::Get("Ignore dedicated DLSSG.UI texture (force legacy HUD path)") },
                    { "DP4A_ACTIVE", 0x20000000, Translation::Get("OF pipeline using dp4a-accelerated SSD (SM 6.4+)") },
                    { "PIN_BACKBUFFER", 0x40000000, Translation::Get("Pin DLSSG.Backbuffer to subframe-1 snapshot across MFG frame") }
                };

                uint32_t temp_flags = config->NvngxFGDispatchFlags.value_or_default();
                bool changed = false;

                ImGui::Text(Translation::Get("Raw DispatchFlags:"));
                changed |= ImGui::InputScalar("##RawFlags", ImGuiDataType_U32, &temp_flags, NULL, NULL, "%08X",
                                              ImGuiInputTextFlags_CharsHexadecimal);

                ImGui::Spacing();

                if (auto ch = ScopedCollapsingHeader(Translation::Get("Active DispatchFlags")); ch.IsHeaderOpen())
                {
                    ScopedIndent indent {};
                    for (const auto& flag : known_flags)
                    {
                        changed |= ImGui::CheckboxFlags(flag.name.c_str(), &temp_flags, flag.mask);

                        if (ImGui::IsItemHovered() && !flag.description.empty())
                        {
                            ImGui::SetTooltip("%s", flag.description.c_str());
                        }
                    }
                }

                if (changed)
                {
                    config->NvngxFGDispatchFlags = temp_flags;
                }
            }
            else
            {

                if (Nvngx_FG::is120orNewer())
                {
                    if (ImGui::Checkbox(Translation::Get("Enable Debug View"), &state.dlssgDebugView))
                    {
                        Nvngx_FG::setDebugView(state.dlssgDebugView);
                    }
                    if (ImGui::Checkbox(Translation::Get("Interpolated frames only"), &state.dlssgInterpolatedOnly))
                    {
                        Nvngx_FG::setInterpolatedOnly(state.dlssgInterpolatedOnly);
                    }
                }
                else if (Nvngx_FG::FSRDebugView() != nullptr)
                {
                    if (ImGui::Checkbox(Translation::Get("Enable Debug View"), &state.dlssgDebugView))
                    {
                        Nvngx_FG::FSRDebugView()(state.dlssgDebugView);
                    }
                }
            }
        }
    }

    // FSR-FG Inputs
    if (state.currentFGSwapchain != nullptr &&
        (state.activeFgInput == FGInput::FSRFG || state.activeFgInput == FGInput::FSRFG30))
    {
        SeparatorWithHelpMarker(Translation::Get("Frame Generation (FSR-FG Inputs)"), Translation::Get("Select FSR-FG in-game"));

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);
        if (fgOutput != nullptr)
        {
            ImGui::Text(Translation::Get("Current FSR-FG state:"));
            ImGui::SameLine();
            if (state.fsrfgInputActive)
            {
                if (fgOutput->IsActive())
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), Translation::Get("ON"));
                else
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.647f, 0.0f, 1.f)), Translation::Get("ACTIVATE FG"));
            }
            else
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("OFF"));
                ImGui::Text(Translation::Get("Please select FSR Frame Generation in the game options\n"
                            "You might need to select FSR first"));
            }
        }

        bool skipConfig = config->FSRFGSkipConfigForHudless.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Skip Config for Hudless"), &skipConfig))
            config->FSRFGSkipConfigForHudless = skipConfig;

        ShowHelpMarker(Translation::Get("Do not use Hudless set at ffxConfig"));

        ImGui::SameLine(0.0f, 6.0f);

        bool skipDispatch = config->FSRFGSkipDispatchForHudless.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Skip Dispatch for Hudless"), &skipDispatch))
            config->FSRFGSkipDispatchForHudless = skipDispatch;

        ShowHelpMarker(Translation::Get("Do not use Hudless set at ffxDispatch"));
    }

    // Streamline FG Inputs
    if (state.currentFGSwapchain != nullptr && state.activeFgInput == FGInput::DLSSG)
    {
        SeparatorWithHelpMarker(Translation::Get("Frame Generation (Streamline FG Inputs)"), Translation::Get("Select DLSS-FG in-game"));

        auto fgOutput = reinterpret_cast<IFGFeature_Dx12*>(state.currentFG);

        if (!ReflexHooks::isReflexHooked())
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("Reflex not hooked"));
            ImGui::Text(Translation::Get("If you are using an AMD/Intel GPU then make sure you have fakenvapi"));
        }
        else if (fgOutput != nullptr)
        {
            ImGui::Text(Translation::Get("Current Streamline FG state:"));
            ImGui::SameLine();
            if ((state.fgLastFrame - state.dlssgLastFrame) < 3)
            {
                if (fgOutput->IsActive())
                    ImGui::TextColored(toneMapColor(ImVec4(0.f, 1.f, 0.25f, 1.f)), Translation::Get("ON"));
                else
                    ImGui::TextColored(toneMapColor(ImVec4(1.0f, 0.647f, 0.0f, 1.f)), Translation::Get("ACTIVATE FG"));
            }
            else
            {
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)), Translation::Get("OFF"));
                ImGui::Text(Translation::Get("Please select DLSS Frame Generation in the game options\n"
                            "You might need to select DLSS first"));
            }
        }
    }
}

void MenuCommon::RenderFsrCommonSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // FSR Common -----------------
        if (currentFeature != nullptr && !currentFeature->IsFrozen() &&
            (state.activeFgOutput == FGOutput::FSRFG || IsFsr(currentBackend)))
        {
            SeparatorWithHelpMarker(Translation::Get("FSR Common Settings"), Translation::Get("Affects both FSR-FG & Upscalers"));

            bool useFsrVales = config->FsrUseFsrInputValues.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Use FSR Input Values"), &useFsrVales))
                config->FsrUseFsrInputValues = useFsrVales;

            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("FoV & Camera Values")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                bool useVFov = config->FsrVerticalFov.has_value() || !config->FsrHorizontalFov.has_value();

                float vfov = config->FsrVerticalFov.value_or_default();
                float hfov = config->FsrHorizontalFov.value_or(90.0f);

                if (useVFov && !config->FsrVerticalFov.has_value())
                    config->FsrVerticalFov = vfov;
                else if (!useVFov && !config->FsrHorizontalFov.has_value())
                    config->FsrHorizontalFov = hfov;

                if (ImGui::RadioButton(Translation::Get("Use Vert. Fov"), useVFov))
                {
                    config->FsrHorizontalFov.reset();
                    config->FsrVerticalFov = vfov;
                    useVFov = true;
                }

                ImGui::SameLine(0.0f, 6.0f);

                if (ImGui::RadioButton(Translation::Get("Use Horz. Fov"), !useVFov))
                {
                    config->FsrVerticalFov.reset();
                    config->FsrHorizontalFov = hfov;
                    useVFov = false;
                }

                if (useVFov)
                {
                    if (ImGui::SliderFloat(Translation::Get("Vert. FOV"), &vfov, 0.0f, 180.0f, "%.1f"))
                        config->FsrVerticalFov = vfov;

                    ShowHelpMarker(Translation::Get("Might help achieve better image quality"));
                }
                else
                {
                    if (ImGui::SliderFloat(Translation::Get("Horz. FOV"), &hfov, 0.0f, 180.0f, "%.1f"))
                        config->FsrHorizontalFov = hfov;

                    ShowHelpMarker(Translation::Get("Might help achieve better image quality"));
                }

                float cameraNear;
                float cameraFar;

                cameraNear = config->FsrCameraNear.value_or_default();
                cameraFar = config->FsrCameraFar.value_or_default();

                if (ImGui::SliderFloat(Translation::Get("Camera Near"), &cameraNear, 0.1f, 500000.0f, "%.1f"))
                    config->FsrCameraNear = cameraNear;
                ShowHelpMarker(Translation::Get("Might help achieve better image quality\n"
                               "And potentially less ghosting"));

                if (ImGui::SliderFloat(Translation::Get("Camera Far"), &cameraFar, 0.1f, 500000.0f, "%.1f"))
                    config->FsrCameraFar = cameraFar;
                ShowHelpMarker(Translation::Get("Might help achieve better image quality\n"
                               "And potentially less ghosting"));

                if (ImGui::Button(Translation::Get("Reset Camera Values")))
                {
                    config->FsrVerticalFov.reset();
                    config->FsrHorizontalFov.reset();
                    config->FsrCameraNear.reset();
                    config->FsrCameraFar.reset();
                }

                ImGui::SameLine(0.0f, 6.0f);
                ImGui::Text(Translation::Get("Near: %.1f Far: %.1f"),
                            state.lastFsrCameraNear < 500000.0f ? state.lastFsrCameraNear : 500000.0f,
                            state.lastFsrCameraFar < 500000.0f ? state.lastFsrCameraFar : 500000.0f);

                ImGui::Spacing();
                ImGui::Spacing();
            }
        }
    }
}

void MenuCommon::RenderFramerateSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& menuResScale = ctx.menuResScale;

    // Framerate ---------------------
    if (state.reflexLimitsFps || config->OverlayMenu.value_or_default())
    {
        SeparatorWithHelpMarker(
            Translation::Get("Framerate"), Translation::Get("Uses Reflex when possible\nOn AMD/Intel cards, you can use Fakenvapi to substitute Reflex"));

        static std::string currentMethod {};
        LowLatencyMode fakenvapiMode = {};
        if (state.reflexLimitsFps)
        {
            fakenvapiMode = fakenvapi::getCurrentMode();

            if (fakenvapiMode == LowLatencyMode::AntiLag2)
                currentMethod = Translation::Get("FSR Anti-Lag 2.0");
            else if (fakenvapiMode == LowLatencyMode::LatencyFlex)
                currentMethod = Translation::Get("LatencyFlex");
            else if (fakenvapiMode == LowLatencyMode::XeLL)
                currentMethod = Translation::Get("XeLL");
            else if (fakenvapiMode == LowLatencyMode::AntiLagVk)
                currentMethod = Translation::Get("Vulkan AntiLag");
            else if (fakenvapiMode == LowLatencyMode::None)
            {
                if (fakenvapi::isUsingAsMainNvapi())
                    currentMethod = Translation::Get("None");
                else
                    currentMethod = Translation::Get("Reflex");
            }

            if (state.rtssReflexInjection && fakenvapiMode == LowLatencyMode::AntiLag2 &&
                config->FGOutput.value_or_default() == FGOutput::FSRFG)
                ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.8f, 0.f, 1.f)),
                                   Translation::Get("Using RTSS Reflex injection with FSR Anti-Lag 2.0 and FSR FG might cause issues"));
        }
        else
        {
            if (XellHooks::canLimit())
                currentMethod = Translation::Get("Game's XeLL");
            else
                currentMethod = Translation::Get("Fallback");
        }

        if (state.rtssReflexInjection)
            currentMethod.append(Translation::Get(" (RTSS)"));

        const bool fakenvapiInactive = (fakenvapi::isUsingAsMainNvapi() || fakenvapiMode == LowLatencyMode::XeLL) &&
                                       !fakenvapi::isLowLatencyActive() && state.reflexLimitsFps;

        if (fakenvapiInactive)
            currentMethod.append(Translation::Get(" (inactive)"));

        ImGui::Text(Translation::Get("Current method: %s"), currentMethod.c_str());

        if (fakenvapiMode == LowLatencyMode::AntiLag2)
            ShowHelpMarker(Translation::Get("FSR Anti-Lag 2.0 is the new name for AntiLag 2\nDon't ask me why"));

        if (state.reflexShowWarning)
        {
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                               Translation::Get("Using Reflex's limit with FSR FG has performance overhead"));

            ImGui::Spacing();
        }

        // set initial value
        if (std::isinf(_limitFps))
            _limitFps = config->FramerateLimit.value_or_default();

        ImGui::SliderFloat(Translation::Get("FPS Limit"), &_limitFps, 0, 200, "%.0f");

        if (ImGui::Button(Translation::Get("Apply Limit")))
        {
            config->FramerateLimit = _limitFps;
        }

        ImGui::SameLine(0.0f, 16.0f);

        if (ImGui::Button(Translation::Get("Reset Limit")))
        {
            _limitFps = 0.0f;
            config->FramerateLimit = _limitFps;
        }

        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(Translation::Get("VRR Frame Cap Calculator")); ch.IsHeaderOpen())
        {
            ScopedIndent indent {};
            ImGui::Spacing();

            ImGui::PushItemWidth(105.0f * menuResScale);
            ImGui::InputInt(Translation::Get("Refresh Rate"), &refreshRate, 1, 1, ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();

            float refreshRateF = static_cast<float>(refreshRate);
            // it's fine to use with real reflex, we only care about antilag
            auto fpsLimitTech = fakenvapi::getCurrentMode();
            constexpr float margin = 0.3f; // in ms
            float frameCap = std::round(10000.f / (1000.f / refreshRateF + margin)) / 10.f;

            if (fpsLimitTech == LowLatencyMode::AntiLag2 || fpsLimitTech == LowLatencyMode::AntiLagVk)
                frameCap = std::round(frameCap);

            ImGui::Text(Translation::Get("Calculated Cap: %.1f"), frameCap);

            ImGui::SameLine(0.0f, 16.0f);

            if (ImGui::Button(Translation::Get("Set as FPS Limit")))
            {
                _limitFps = frameCap;
                config->FramerateLimit = _limitFps;
            }
        }
    }
}

void MenuCommon::RenderFakenvapiSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;

    // FAKENVAPI ---------------------------
    if (fakenvapi::isUsingAsMainNvapi() || (state.activeFgOutput == FGOutput::XeFG && state.reflexLimitsFps))
    {
        // Using state.reflexLimitsFps as a detection for Reflex being used on Nvidia

        ImGui::SeparatorText(Translation::Get("fakenvapi"));
#ifdef LOW_LATENCY_INPUTS
        if (ImGui::BeginTable("lowLatencySelection", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextColumn();

            auto avalibleInputs = InputCommon::get_avaliable_inputs();
            static std::vector<MenuOption<LowLatencyInput>> lowLatencyInput = {
                { LowLatencyInput::None, Translation::Get("None (Off)") },
                { LowLatencyInput::Auto, Translation::Get("Auto") },
                { LowLatencyInput::AntiLag2, Translation::Get("AntiLag 2") },
                { LowLatencyInput::Reflex, Translation::Get("Reflex") },
                { LowLatencyInput::XeLL, Translation::Get("XeLL") },
                { LowLatencyInput::UeLowLatency, Translation::Get("UeLowLatency") },
            };

            lowLatencyInput[(uint32_t) LowLatencyInput::AntiLag2].set_disabled(
                !avalibleInputs[LowLatencyInput::AntiLag2]);
            lowLatencyInput[(uint32_t) LowLatencyInput::Reflex].set_disabled(!avalibleInputs[LowLatencyInput::Reflex]);
            lowLatencyInput[(uint32_t) LowLatencyInput::XeLL].set_disabled(!avalibleInputs[LowLatencyInput::XeLL]);
            lowLatencyInput[(uint32_t) LowLatencyInput::UeLowLatency].set_disabled(
                !avalibleInputs[LowLatencyInput::UeLowLatency]);

            // need to have a value before combo
            if (!config->LowLatencyInput.has_value())
                config->LowLatencyInput = config->LowLatencyInput.value_or_default();

            PopulateCombo(Translation::Get("Input"), config->LowLatencyInput, lowLatencyInput);

            ImGui::TableNextColumn();

            static std::vector<MenuOption<LowLatencyMode>> lowLatencyOutput = {
                { LowLatencyMode::None, Translation::Get("None (Off)") },
                { LowLatencyMode::Auto, Translation::Get("Auto") },
                { LowLatencyMode::LatencyFlex, Translation::Get("LatencyFlex") },
                { LowLatencyMode::AntiLag2, Translation::Get("AntiLag 2") },
                { LowLatencyMode::XeLL, Translation::Get("XeLL") },
                { LowLatencyMode::AntiLagVk, Translation::Get("AntiLag Vk") },
                { LowLatencyMode::Reflex, Translation::Get("Reflex") },
            };

            lowLatencyOutput[(uint32_t) LowLatencyMode::AntiLagVk].set_disabled(true, Translation::Get("No support"));
            lowLatencyOutput[(uint32_t) LowLatencyMode::Reflex].set_disabled(true, Translation::Get("No support"));

            // need to have a value before combo
            if (!config->LowLatencyOutput.has_value())
                config->LowLatencyOutput = config->LowLatencyOutput.value_or_default();

            PopulateCombo(Translation::Get("Output"), config->LowLatencyOutput, lowLatencyOutput);

            ImGui::EndTable();
        }
#endif

        ImGui::BeginDisabled(state.activeFgOutput == FGOutput::XeFG || state.activeFgInput == FGInput::ForceXeLL);
        if (bool forceLFX = config->FN_ForceLatencyFlex.value_or_default();
            ImGui::Checkbox(Translation::Get("Force LatencyFlex"), &forceLFX))
        {
            config->FN_ForceLatencyFlex = forceLFX;
        }
        ShowHelpMarker(Translation::Get("By default, FSR Anti-Lag 2.0/XeLL is used when available.\n"
                       "This setting lets you force LatencyFlex instead"));
        ImGui::EndDisabled();

        bool forceXell = config->ForceXeLL.value_or_default();
        static bool activeForceXeLL = forceXell;

        if (fakenvapi::isUsingAsMainNvapi())
        {
            ImGui::SameLine(0.0f, 16.0f);

            if (ImGui::Checkbox(Translation::Get("Force XeLL"), &forceXell))
            {
                config->ForceXeLL = forceXell;
            }
            ShowHelpMarker(Translation::Get("Allows XeLL to work without FG on non-Intel cards.\n\nDisables FG "
                           "options\n\nRequires a restart"));
        }

        if (activeForceXeLL != forceXell)
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.0f, 1.f)), Translation::Get("Save INI and restart to apply the changes"));
            ImGui::Spacing();
        }

        // clang-format off
        static const std::vector<MenuOption<LFXMode>> lfx_modes = {
            { LFXMode::Conservative, Translation::Get("Conservative"),
                Translation::Get("The safest, but might not reduce latency well") },
            { LFXMode::Aggressive, Translation::Get("Aggressive"),
                Translation::Get("Improves latency, but in some cases will lower FPS more than expected") },
            { LFXMode::ReflexIDs, Translation::Get("Reflex ID"),
                Translation::Get("Best when can be used, some games are not compatible (e.g. Cyberpunk)\nand will fallback to Aggressive") }
        };

        bool usingLFX = fakenvapi::getCurrentMode() == LowLatencyMode::LatencyFlex;

        ImGui::BeginDisabled(!usingLFX);
        PopulateCombo(Translation::Get("LatencyFlex mode"), config->FN_LatencyFlexMode, lfx_modes);
        ImGui::EndDisabled();

        static std::vector<MenuOption<ForceReflex>> reflex_modes = { { ForceReflex::InGame, Translation::Get("Follow in-game") },
        { ForceReflex::ForceDisable, Translation::Get("Force Disable") },
        { ForceReflex::ForceEnable, Translation::Get("Force Enable") } };

        PopulateCombo(Translation::Get("Force Reflex"), config->FN_ForceReflex, reflex_modes);
        // clang-format on
    }
}

void MenuCommon::RenderActiveImageSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;

    bool rcasEnabled = false;

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        // SHARPNESS -----------------------------
        ImGui::SeparatorText(Translation::Get("Sharpness"));

        if (bool overrideSharpness = config->OverrideSharpness.value_or_default();
            ImGui::Checkbox(Translation::Get("Override"), &overrideSharpness))
        {
            config->OverrideSharpness = overrideSharpness;

            if (currentBackend == Upscaler::DLSS && currentFeature->Version().major < 3)
            {
                state.newBackend = currentBackend;
                MARK_ALL_BACKENDS_CHANGED();
            }
        }
        ShowHelpMarker(Translation::Get("Ignores the value sent by the game\n"
                       "and uses the value set below"));

        ImGui::BeginDisabled(!config->OverrideSharpness.value_or_default());

        float sharpness = config->Sharpness.value_or_default();

        if (ImGui::SliderFloat(Translation::Get("Sharpness"), &sharpness, 0.0f, 1.0f))
            config->Sharpness = sharpness;

        ImGui::EndDisabled();

        // RCAS
        // if (state.api == DX12 || state.api == DX11)
        {
            // xess or dlss version >= 2.5.1
            constexpr feature_version requiredDlssVersion = { 2, 5, 1 };
            rcasEnabled = (currentBackend == Upscaler::XeSS ||
                           (currentBackend == Upscaler::DLSS && currentFeature->Version() >= requiredDlssVersion));

            ImGui::Spacing();
            ImGui::Spacing();

            if (bool rcas = config->RcasEnabled.value_or(rcasEnabled); ImGui::Checkbox(Translation::Get("Enable RCAS/DA"), &rcas))
                config->RcasEnabled = rcas;

            ShowHelpMarker(Translation::Get("Enable OptiScaler's sharpening filter\n"
                           "By default uses a sharpening value provided by the game\n"
                           "Select 'Override' under 'Sharpness' and adjust the slider\n"
                           "to change it\n\n"
                           "Some upscalers have their own sharpness filter, so this\n"
                           "option is not always needed"));

            ImGui::BeginDisabled(!config->RcasEnabled.value_or(rcasEnabled));

            auto sharpnessShader = (int32_t) Config::Instance()->SharpnessShader.value_or_default();

            if (ImGui::RadioButton(Translation::Get("RCAS"), &sharpnessShader, (int32_t) SharpenShader::RCAS))
            {
                Config::Instance()->SharpnessShader = SharpenShader::RCAS;
            }

            ShowHelpMarker(Translation::Get("Use AMD's RCAS\n"
                           "Modified to add Contrast parameter\n"
                           "and MAS support"));

            ImGui::SameLine(0.0f, 6.0f);

            if (ImGui::RadioButton(Translation::Get("Depth Aware (RCAS)"), &sharpnessShader, (int32_t) SharpenShader::DepthAware))
            {
                Config::Instance()->SharpnessShader = SharpenShader::DepthAware;
            }

            ShowHelpMarker(Translation::Get("Use Depth Aware Sharpening (RCAS)\n"
                           "Smarter sharpening with less artifacts,\n"
                           "but also heavier\n\n"
                           "The farther away is the object, the more\n"
                           "sharpening is applied"));

            ImGui::SameLine(0.0f, 6.0f);

            if (ImGui::RadioButton(Translation::Get("Depth Aware (DAS)"), &sharpnessShader,
                                   (int32_t) SharpenShader::LocalContrastDepthAware))
            {
                Config::Instance()->SharpnessShader = SharpenShader::LocalContrastDepthAware;
            }

            ShowHelpMarker(Translation::Get("Use Depth Aware Sharpening (DAS)\n"
                           "Depth-aware directional adaptive luma sharpener\n"
                           "Smarter sharpening with less artifacts,\n"
                           "but also heavier\n\n"
                           "The farther away is the object, the more\n"
                           "sharpening is applied"));

            ImGui::Spacing();

            if (bool overrideMotionSharpness = config->MotionSharpnessEnabled.value_or_default();
                ImGui::Checkbox(Translation::Get("Enable Motion Adaptive Sharpness"), &overrideMotionSharpness))
                config->MotionSharpnessEnabled = overrideMotionSharpness;
            ShowHelpMarker(Translation::Get("Enables sharpness adjustments according to the motion"));

            if (Config::Instance()->SharpnessShader.value_or_default() != SharpenShader::RCAS)
            {
                if (bool overrideMSDebug = config->MotionSharpnessDebug.value_or_default();
                    ImGui::Checkbox(Translation::Get("DA + MAS Debug"), &overrideMSDebug))
                    config->MotionSharpnessDebug = overrideMSDebug;

                ShowHelpMarker(Translation::Get("Enable DA + MAS debug views\n"
                               "Blue tint for DA detected edges\n\n"
                               "More red areas will have more sharpness applied\n"
                               "Green areas will get reduced sharpness"));

                if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced DA Parameters")); ch.IsHeaderOpen())
                {
                    ScopedIndent indent {};
                    ImGui::Spacing();

                    if (bool clamp = config->DAClampOutput.value_or(false); ImGui::Checkbox(Translation::Get("Clamp Output"), &clamp))
                    {
                        if (clamp)
                            config->DAClampOutput = true;
                        else
                            config->DAClampOutput.reset();
                    }

                    ShowHelpMarker(Translation::Get("Clamps the final image to the [0, 1] range.\n\n"
                                   "Prevents overshoot artifacts such as bright halos or negative colors.\n"
                                   "Recommended for LDR pipelines; optional for HDR depending on tone-mapping.\n\n"
                                   "When not set OptiScaler controls it via upscalers HDR flag"));

                    if (currentFeature->DepthLinear())
                    {
                        float depthBias = config->DADepthBias.value_or(0.0015f);
                        if (ImGui::SliderFloat(Translation::Get("Depth Bias"), &depthBias, 0.005f, 0.03f, "%.4f"))
                            config->DADepthBias = depthBias;

                        ShowHelpMarker(Translation::Get("Ignores small depth differences before edge detection.\n\n"
                                       "Higher values reduce flickering and noise from minor depth changes, but may "
                                       "soften real geometry edges.\n"
                                       "Lower values preserve fine detail but can cause unstable or noisy edge "
                                       "detection."));

                        float depthScale = config->DADepthScale.value_or(250.0f);
                        if (ImGui::SliderFloat(Translation::Get("Depth Scale"), &depthScale, 100.0f, 600.0f, "%.1f"))
                            config->DADepthScale = depthScale;

                        ShowHelpMarker(Translation::Get("Controls how strongly sharpening is reduced across depth edges.\n\n"
                                       "Higher values more aggressively prevent sharpening across object boundaries "
                                       "(reduces halos).\n"
                                       "Lower values allow more sharpening to pass across edges (sharper but "
                                       "riskier)."));
                    }
                    else
                    {
                        float depthBias = config->DADepthBias.value_or(0.001f);
                        if (ImGui::SliderFloat(Translation::Get("Depth Bias"), &depthBias, 0.0001f, 0.003f, "%.4f"))
                            config->DADepthBias = depthBias;

                        ShowHelpMarker(Translation::Get("Ignores small depth differences before edge detection.\n\n"
                                       "Higher values reduce flickering and noise from minor depth changes, but may "
                                       "soften real geometry edges.\n"
                                       "Lower values preserve fine detail but can cause unstable or noisy edge "
                                       "detection."));

                        float depthScale = config->DADepthScale.value_or(35.0f);
                        if (ImGui::SliderFloat(Translation::Get("Depth Scale"), &depthScale, 25.0f, 400.0f, "%.1f"))
                            config->DADepthScale = depthScale;

                        ShowHelpMarker(Translation::Get("Controls how strongly sharpening is reduced across depth edges.\n\n"
                                       "Higher values more aggressively prevent sharpening across object boundaries "
                                       "(reduces halos).\n"
                                       "Lower values allow more sharpening to pass across edges (sharper but "
                                       "riskier)."));
                    }

                    if (ImGui::Button(Translation::Get("Reset Depth Values")))
                    {
                        config->DADepthBias.reset();
                        config->DADepthScale.reset();
                    }
                }
            }
            else
            {
                if (bool contrastEnabled = config->ContrastEnabled.value_or_default();
                    ImGui::Checkbox(Translation::Get("Contrast Enabled"), &contrastEnabled))
                    config->ContrastEnabled = contrastEnabled;

                ShowHelpMarker(Translation::Get("Controls sharpness at high contrast areas."));

                ImGui::BeginDisabled(!config->ContrastEnabled.value_or_default());

                float contrast = config->Contrast.value_or_default();
                if (ImGui::SliderFloat(Translation::Get("Contrast"), &contrast, -2.0f, 2.0f, "%.2f"))
                    config->Contrast = contrast;

                ShowHelpMarker(Translation::Get("Positive values decrease sharpness at high contrast areas.\n"
                               "Negative values increase sharpness at high contrast areas."));

                ImGui::EndDisabled();
            }

            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Motion Adaptive Sharpness##2")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                ImGui::BeginDisabled(!config->MotionSharpnessEnabled.value_or_default());

                if (Config::Instance()->SharpnessShader.value_or_default() == SharpenShader::RCAS)
                {
                    if (bool overrideMSDebug = config->MotionSharpnessDebug.value_or_default();
                    ImGui::Checkbox(Translation::Get("MAS Debug"), &overrideMSDebug))
                        config->MotionSharpnessDebug = overrideMSDebug;
                    ShowHelpMarker(Translation::Get("Areas that are more red will have more sharpness applied\n"
                                   "Green areas will get reduced sharpness"));
                }

                float motionSharpness = config->MotionSharpness.value_or_default();
                ImGui::SliderFloat(Translation::Get("MotionSharpness"), &motionSharpness, -1.0f, 1.0f, "%.3f");
                config->MotionSharpness = motionSharpness;

                ShowHelpMarker(Translation::Get("Maximum amount of sharpness that motion can add or remove.\n\n"
                               "Negative values reduce sharpening in motion (recommended).\n"
                               "Positive values increase sharpening in motion.\n\n"
                               "The final adjustment scales with motion and is capped at this value."));

                float motionThreshod = config->MotionThreshold.value_or_default();
                ImGui::SliderFloat(Translation::Get("MotionThreshod"), &motionThreshod, 0.0f, 100.0f, "%.2f");
                config->MotionThreshold = motionThreshod;

                ShowHelpMarker(Translation::Get("Minimum motion required before motion-based sharpening adjustment begins.\n\n"
                               "Higher values ignore small movements (more stable).\n"
                               "Lower values react to subtle motion (more sensitive)."));

                float motionScale = config->MotionScaleLimit.value_or_default();
                ImGui::SliderFloat(Translation::Get("MotionRange"), &motionScale, 0.01f, 100.0f, "%.2f");
                config->MotionScaleLimit = motionScale;

                ShowHelpMarker(Translation::Get("Defines the motion range over which the effect ramps from zero to full strength.\n\n"
                               "Values above the threshold are mapped into this range.\n"
                               "Larger values make the response smoother and more gradual.\n"
                               "Smaller values make the effect react more quickly and aggressively."));

                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::Spacing();
            }

            ImGui::EndDisabled();
        }

        // UPSCALE RATIO OVERRIDE -----------------

        auto minSliderLimit = config->ExtendedLimits.value_or_default() ? 0.1f : 1.0f;
        auto maxSliderLimit = config->ExtendedLimits.value_or_default() ? 6.0f : 3.0f;

        ImGui::SeparatorText(Translation::Get("Upscale Ratio Override"));

        if (bool upOverride = config->UpscaleRatioOverrideEnabled.value_or_default();
            ImGui::Checkbox(Translation::Get("Override all"), &upOverride))
        {
            config->UpscaleRatioOverrideEnabled = upOverride;

            if (upOverride)
                config->QualityRatioOverrideEnabled = false;
        }
        ShowHelpMarker(Translation::Get("Overrides every upscaler preset with the set value\n\n"
                       "1.5x on a 1080p screen means an internal res of 720p\n"
                       "1080 / 1.5 = 720"));

        if (bool qOverride = config->QualityRatioOverrideEnabled.value_or_default();
            ImGui::Checkbox(Translation::Get("Override per quality preset"), &qOverride))
        {
            config->QualityRatioOverrideEnabled = qOverride;

            if (qOverride)
                config->UpscaleRatioOverrideEnabled = false;
        }

        ShowHelpMarker(Translation::Get("Lets you override each preset's ratio individually\n"
                       "Note that not every game supports every quality preset\n\n"
                       "1.5x on a 1080p screen means internal resolution of 720p\n"
                       "1080 / 1.5 = 720"));

        if (config->UpscaleRatioOverrideEnabled.value_or_default())
        {
            float urOverride = config->UpscaleRatioOverrideValue.value_or_default();
            ImGui::SliderFloat(Translation::Get("All Ratios"), &urOverride, minSliderLimit, maxSliderLimit, "%.3f");
            config->UpscaleRatioOverrideValue = urOverride;
        }

        if (config->QualityRatioOverrideEnabled.value_or_default())
        {
            float qDlaa = config->QualityRatio_DLAA.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("DLAA"), &qDlaa, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_DLAA = qDlaa;

            float qUq = config->QualityRatio_UltraQuality.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Ultra Quality"), &qUq, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_UltraQuality = qUq;

            float qQ = config->QualityRatio_Quality.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Quality"), &qQ, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Quality = qQ;

            float qB = config->QualityRatio_Balanced.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Balanced"), &qB, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Balanced = qB;

            float qP = config->QualityRatio_Performance.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Performance"), &qP, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_Performance = qP;

            float qUp = config->QualityRatio_UltraPerformance.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Ultra Performance"), &qUp, minSliderLimit, maxSliderLimit, "%.3f"))
                config->QualityRatio_UltraPerformance = qUp;
        }

        if (currentFeature != nullptr && !currentFeature->IsFrozen())
        {
            // OUTPUT SCALING -----------------------------
            // if (state.api == DX12 || state.api == DX11)
            {
                // if motion vectors are not display size
                ImGui::BeginDisabled(!currentFeature->LowResMV() &&
                                     currentFeature->RenderWidth() != currentFeature->DisplayWidth());

                ImGui::SeparatorText(Translation::Get("Output Scaling"));

                float defaultRatio = 1.5f;

                if (_ssRatio == 0.0f)
                {
                    _ssRatio = config->OutputScalingMultiplier.value_or(defaultRatio);
                    _ssEnabled = config->OutputScalingEnabled.value_or_default();
                    _ssDownsampler = config->OutputScalingDownscaler.value_or_default();
                }

                ImGui::BeginDisabled((currentBackend == Upscaler::XeSS || currentBackend == Upscaler::DLSS) &&
                                     currentFeature->RenderWidth() > currentFeature->DisplayWidth());
                ImGui::Checkbox(Translation::Get("Enable"), &_ssEnabled);
                ImGui::EndDisabled();

                ShowHelpMarker(Translation::Get("Upscales the image internally to a higher output resolution\n"
                               "then downscales it back to your display resolution\n\n"
                               "Values <1.0 make the upscaler cheaper\n"
                               "Values >1.0 make image sharper at the cost of performance\n\n"
                               "If greyed out, please check Git Wiki - Unreal Engine tweaks\n\n"
                               "Target res and total ratio at the bottom (max. total 3.0!)"));

                ImGui::SameLine(0.0f, 6.0f);

                ImGui::BeginDisabled(!_ssEnabled);
                {
                    ImGui::PushItemWidth(95.0f * menuResScale);

                    // clang-format off
                    std::vector<MenuOption<Scaler>> ds_options = {
                        { Scaler::FSR1, Translation::Get("FSR1"),
                            Translation::Get("Default option.\nGood enough image quality and very fast.") },
                        { Scaler::Bicubic, Translation::Get("Bicubic"),
                            Translation::Get("Fastest traditional option.\nProduces a very soft/blurry image, but might be okay for downscaling.") },
                        { Scaler::CatmullRom, Translation::Get("Catmull-Rom"),
                            Translation::Get("Designed primarily for downscaling.\nRetains good contrast with minimal artefacts, but softer than Lanczos.") },
                        { Scaler::Lanczos2, Translation::Get("Lanczos2"),
                            Translation::Get("Lighter and faster than Lanczos3.\nLess prone to ringing artefacts, but slightly blurrier.") },
                        { Scaler::Lanczos3, Translation::Get("Lanczos3"),
                            Translation::Get("Heavier version of Lanczos2.\nOffers the sharpest image, but is the most prone to ringing.") },
                        { Scaler::Kaiser2, Translation::Get("Kaiser2"),
                            Translation::Get("Similar to Lanczos2.\nSmoother and less prone to artefacts than Lanczos, but slightly blurrier.") },
                        { Scaler::Kaiser3, Translation::Get("Kaiser3"),
                            Translation::Get("Similar to Lanczos3.\nFar less prone to artefacting than Lanczos3, but much heavier on the GPU.") },
                        { Scaler::Magic, Translation::Get("MAGIC"),
                            Translation::Get("Specialised to prevent artifacts.\nEliminates harsh halos for a natural look, but can appear slightly soft.") }
                    };
                    // clang-format on

                    const bool isUpsampleRatio = _ssRatio < 1.0f;
                    const std::string disabledReason = Translation::Get("Only FSR1 and Bicubic are supported when Ratio is below 1.0.");

                    for (auto& opt : ds_options)
                    {
                        if (isUpsampleRatio && opt.value > Scaler::Bicubic)
                            opt.set_disabled(true, opt.tooltip + "\n\n" + disabledReason);
                    }

                    if (isUpsampleRatio && _ssDownsampler > Scaler::Bicubic)
                        _ssDownsampler = Scaler::FSR1;

                    PopulateCombo(Translation::Get("Downscaler"), _ssDownsampler, ds_options);

                    ImGui::PopItemWidth();
                }
                ImGui::EndDisabled();

                bool applyEnabled = _ssEnabled != config->OutputScalingEnabled.value_or_default() ||
                                    _ssRatio != config->OutputScalingMultiplier.value_or(defaultRatio) ||
                                    _ssDownsampler != config->OutputScalingDownscaler.value_or_default();

                ImGui::BeginDisabled(!applyEnabled);
                if (ImGui::Button(Translation::Get("Apply Change")))
                {
                    config->OutputScalingEnabled = _ssEnabled;
                    config->OutputScalingMultiplier = _ssRatio;

                    if (_ssRatio < 1.0f && _ssDownsampler > Scaler::Bicubic)
                        _ssDownsampler = Scaler::FSR1;

                    config->OutputScalingDownscaler = _ssDownsampler;

                    const bool usesDlssd = currentFeature->GetUpscalerType() == Upscaler::DLSSD;
                    if (usesDlssd)
                        state.newBackend = Upscaler::DLSSD;
                    else
                        state.newBackend = currentBackend;

                    MARK_ALL_BACKENDS_CHANGED();
                }
                ImGui::EndDisabled();

                ImGui::BeginDisabled(!_ssEnabled || currentFeature->RenderWidth() > currentFeature->DisplayWidth());
                ImGui::SliderFloat(Translation::Get("Ratio"), &_ssRatio, 0.5f, 3.0f, "%.2f");
                ImGui::EndDisabled();

                if (currentFeature != nullptr && !currentFeature->IsFrozen())
                {
                    ImGui::Text(Translation::Get("Output Scaling is %s, Target Res: %dx%d (%.2f)\nJitter Count: %d"),
            config->OutputScalingEnabled.value_or_default() ? Translation::Get("ENABLED") : Translation::Get("DISABLED"),
                                (uint32_t) (currentFeature->DisplayWidth() * _ssRatio),
                                (uint32_t) (currentFeature->DisplayHeight() * _ssRatio),
                                ((float) currentFeature->DisplayWidth() * _ssRatio) /
                                    (float) currentFeature->RenderWidth(),
                                currentFeature->JitterCount());
                }

                ImGui::EndDisabled();
            }
        }

        // INIT -----------------------------
        ImGui::SeparatorText(Translation::Get("Init Flags"));
        if (ImGui::BeginTable("init", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableNextColumn();

            // AutoExposure is always enabled for XeSS with native Dx11
            bool autoExposureDisabled = state.api == API::DX11 && currentBackend == Upscaler::XeSS;
            ImGui::BeginDisabled(autoExposureDisabled);

            if (bool autoExposure = currentFeature->AutoExposure(); ImGui::Checkbox(Translation::Get("Auto Exposure"), &autoExposure))
            {
                config->AutoExposure = autoExposure;
                ReInitUpscaler();
            }
            ShowResetButton(&config->AutoExposure, "R");
            ShowHelpMarker(Translation::Get("Some Unreal Engine games need this\n\n"
                           "Try using if colours flickering or\n"
                           "objects have ghosting trails"));

            ImGui::EndDisabled();

            ImGui::TableNextColumn();
            auto accessToReactiveMask = currentFeature->AccessToReactiveMask();
            ImGui::BeginDisabled(!accessToReactiveMask);

            bool canUseReactiveMask =
                accessToReactiveMask && currentBackend != Upscaler::DLSS &&
                (currentBackend != Upscaler::XeSS || currentFeature->Version() >= feature_version { 2, 0, 1 });

            bool disableReactiveMask = config->DisableReactiveMask.value_or(!canUseReactiveMask);

            if (ImGui::Checkbox(Translation::Get("Disable Reactive Mask"), &disableReactiveMask))
            {
                config->DisableReactiveMask = disableReactiveMask;

                if (currentBackend == Upscaler::XeSS)
                {
                    state.newBackend = currentBackend;
                    MARK_ALL_BACKENDS_CHANGED();
                }
            }

            ImGui::EndDisabled();

            if (accessToReactiveMask)
                ShowHelpMarker(Translation::Get("Allows the use of a Reactive mask\n"
                               "Keep in mind that a Reactive mask sent to DLSS\n"
                               "will not produce a good image in combination with FSR/XeSS"));
            else
                ShowHelpMarker(Translation::Get("Option disabled because the game doesn't provide a Reactive mask"));

            ImGui::EndTable();

            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced Init Flags")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                if (ImGui::BeginTable("init2", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableNextColumn();
                    if (bool depth = currentFeature->DepthInverted(); ImGui::Checkbox(Translation::Get("Depth Inverted"), &depth))
                    {
                        config->DepthInverted = depth;
                        ReInitUpscaler();
                    }
                    ShowResetButton(&config->DepthInverted, "R##2");
                    ShowHelpMarker(Translation::Get("You shouldn't need to change it"));

                    ImGui::TableNextColumn();
                    if (bool hdr = currentFeature->IsHdr(); ImGui::Checkbox(Translation::Get("HDR"), &hdr))
                    {
                        config->HDR = hdr;
                        ReInitUpscaler();
                    }
                    ShowResetButton(&config->HDR, "R##1");
                    ShowHelpMarker(Translation::Get("Might help with purple hue in some games"));

                    ImGui::TableNextColumn();
                    if (bool mv = !currentFeature->LowResMV(); ImGui::Checkbox(Translation::Get("Display Res. MV"), &mv))
                    {
                        config->DisplayResolution = mv;

                        // Disable output scaling when
                        // Display res MV is active
                        if (mv)
                        {
                            config->OutputScalingEnabled = false;
                            _ssEnabled = false;
                        }

                        ReInitUpscaler();
                    }
                    ShowResetButton(&config->DisplayResolution, "R##4");
                    ShowHelpMarker(Translation::Get("Mostly a fix for Unreal Engine games\n"
                                   "Top left part of the screen will be blurry"));

                    ImGui::TableNextColumn();

                    if (bool jitter = currentFeature->JitteredMV(); ImGui::Checkbox(Translation::Get("Jitter Cancellation"), &jitter))
                    {
                        config->JitterCancellation = jitter;
                        ReInitUpscaler();
                    }
                    ShowResetButton(&config->JitterCancellation, "R##3");
                    ShowHelpMarker(Translation::Get("Fix for games that send motion data with preapplied jitter"));

                    ImGui::TableNextColumn();
                    ImGui::EndTable();
                }

                if (currentFeature->AccessToReactiveMask() && currentBackend != Upscaler::DLSS)
                {
                    ImGui::BeginDisabled(config->DisableReactiveMask.value_or(currentBackend == Upscaler::XeSS));

                    bool binaryMask = state.api == Vulkan || currentBackend == Upscaler::XeSS;
                    auto defaultBias = binaryMask ? 0.0f : 0.45f;
                    auto maskBias = config->DlssReactiveMaskBias.value_or(defaultBias);

                    if (!binaryMask)
                    {
                        if (ImGui::SliderFloat(Translation::Get("React. Mask Bias"), &maskBias, 0.0f, 0.9f, "%.2f"))
                            config->DlssReactiveMaskBias = maskBias;

                        ShowHelpMarker(Translation::Get("Values above 0 activate usage of Reactive mask"));
                    }
                    else
                    {
                        bool useRM = maskBias > 0.0f;
                        if (ImGui::Checkbox(Translation::Get("Use Binary Reactive Mask"), &useRM))
                        {
                            if (useRM)
                                config->DlssReactiveMaskBias = 0.45f;
                            else
                                config->DlssReactiveMaskBias.reset();
                        }
                    }

                    ImGui::EndDisabled();
                }
            }
        }
    }
}

void MenuCommon::RenderMagnifierSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Magnifier")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        bool enabled = config->MagnifierEnabled.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Enable Magnifier"), &enabled))
            config->MagnifierEnabled = enabled;

        ImGui::BeginDisabled(!enabled);

        float size = config->MagnifierSize.value_or_default();
        if (ImGui::SliderFloat(Translation::Get("Size##Magnifier"), &size, 5.0f, 50.0f, "%.1f%%"))
            config->MagnifierSize = size;

        int zoom = config->MagnifierZoomFactor.value_or_default();
        if (ImGui::SliderInt(Translation::Get("Zoom Factor##Magnifier"), &zoom, 2, 20, "%dx"))
            config->MagnifierZoomFactor = zoom;

        float border = config->MagnifierBorderSize.value_or_default();
        if (ImGui::SliderFloat(Translation::Get("Border Size##Magnifier"), &border, 0.0f, 2.0f, "%.2f%%"))
            config->MagnifierBorderSize = border;

        bool staticMode = config->MagnifierStaticPosX.has_value() && config->MagnifierStaticPosY.has_value();
        if (staticMode)
        {
            float x = config->MagnifierStaticPosX.value();
            if (ImGui::SliderFloat(Translation::Get("Static Pos X##Magnifier"), &x, 0.0f, 100.0f, "%.1f%%"))
                config->MagnifierStaticPosX = x;

            float y = config->MagnifierStaticPosY.value();
            if (ImGui::SliderFloat(Translation::Get("Static Pos Y##Magnifier"), &y, 0.0f, 100.0f, "%.1f%%"))
                config->MagnifierStaticPosY = y;

            if (ImGui::Button(Translation::Get("Follow Cursor##Magnifier")))
            {
                config->MagnifierStaticPosX.reset();
                config->MagnifierStaticPosY.reset();
            }
        }
        else
        {
            if (ImGui::Button(Translation::Get("Set Static Position##Magnifier")))
            {
                config->MagnifierStaticPosX = 50.0f;
                config->MagnifierStaticPosY = 50.0f;
            }

            float x = config->MagnifierCursorOffsetX.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Cursor Offset X##Magnifier"), &x, -300.0f, 300.0f, "%.0f px"))
                config->MagnifierCursorOffsetX = x;

            float y = config->MagnifierCursorOffsetY.value_or_default();
            if (ImGui::SliderFloat(Translation::Get("Cursor Offset Y##Magnifier"), &y, -300.0f, 300.0f, "%.0f px"))
                config->MagnifierCursorOffsetY = y;
        }

        ImGui::EndDisabled();
    }
}

void MenuCommon::RenderQuirksSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;

    // QUIRKS -----------------------------
    if (state.detectedQuirks.size() > 0)
    {
        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(Translation::Get("Active Quirks")); ch.IsHeaderOpen())
        {
            ScopedIndent indent {};
            ImGui::Spacing();

            for (const auto& quirk : state.detectedQuirks)
            {
                ImGui::TextWrapped("%s", Translation::Get(quirk.c_str()));
            }
        }
    }
}

void MenuCommon::RenderAdvancedSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    // ADVANCED SETTINGS -----------------------------
    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Advanced Settings")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        if (currentFeature != nullptr && !currentFeature->IsFrozen())
        {
            bool extendedLimits = config->ExtendedLimits.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Enable Extended Limits"), &extendedLimits))
                config->ExtendedLimits = extendedLimits;

            ShowHelpMarker(Translation::Get("Extended sliders limit for quality presets\n\n"
                           "Using this option changes resolution detection logic\n"
                           "and might cause issues and crashes!"));
        }

        bool pcShaders = config->UsePrecompiledShaders.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Use Precompiled Shaders"), &pcShaders))
        {
            config->UsePrecompiledShaders = pcShaders;
            state.newBackend = currentBackend;
            MARK_ALL_BACKENDS_CHANGED();
        }

        // DRS
        ImGui::SeparatorText(Translation::Get("DRS (Dynamic Resolution Scaling)"));
        if (ImGui::BeginTable("drs", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableNextColumn();
            if (bool drsMin = config->DrsMinOverrideEnabled.value_or_default();
                ImGui::Checkbox(Translation::Get("Override Minimum"), &drsMin))
                config->DrsMinOverrideEnabled = drsMin;
            ShowHelpMarker(Translation::Get("Fix for games ignoring official DRS limits"));

            ImGui::TableNextColumn();
            if (bool drsMax = config->DrsMaxOverrideEnabled.value_or_default();
                ImGui::Checkbox(Translation::Get("Override Maximum"), &drsMax))
                config->DrsMaxOverrideEnabled = drsMax;
            ShowHelpMarker(Translation::Get("Fix for games ignoring official DRS limits"));

            ImGui::EndTable();
        }

        // Non-DLSS hotfixes -----------------------------
        if (currentFeature != nullptr && !currentFeature->IsFrozen() && currentBackend != Upscaler::DLSS)
        {
            // BARRIERS -----------------------------
            ImGui::Spacing();
            if (auto ch = ScopedCollapsingHeader(Translation::Get("Resource Barriers")); ch.IsHeaderOpen())
            {
                ScopedIndent indent {};
                ImGui::Spacing();

                AddResourceBarrier(Translation::Get("Color"), &config->ColorResourceBarrier);
                AddResourceBarrier(Translation::Get("Depth"), &config->DepthResourceBarrier);
                AddResourceBarrier(Translation::Get("Motion"), &config->MVResourceBarrier);
                AddResourceBarrier(Translation::Get("Exposure"), &config->ExposureResourceBarrier);
                AddResourceBarrier(Translation::Get("Mask"), &config->MaskResourceBarrier);
                AddResourceBarrier(Translation::Get("Output"), &config->OutputResourceBarrier);
            }

            // HOTFIXES -----------------------------
            if (state.api == DX12)
            {
                ImGui::Spacing();
                if (auto ch = ScopedCollapsingHeader(Translation::Get("Root Signatures")); ch.IsHeaderOpen())
                {
                    ScopedIndent indent {};
                    ImGui::Spacing();

                    if (bool crs = config->RestoreComputeSignature.value_or_default();
                        ImGui::Checkbox(Translation::Get("Restore Compute Root Signature"), &crs))
                        config->RestoreComputeSignature = crs;

                    if (bool grs = config->RestoreGraphicSignature.value_or_default();
                        ImGui::Checkbox(Translation::Get("Restore Graphic Root Signature"), &grs))
                        config->RestoreGraphicSignature = grs;
                }
            }
        }
    }
}

void MenuCommon::RenderLoggingSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // LOGGING -----------------------------
    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Logging")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        if (config->LogToConsole.value_or_default() || config->LogToFile.value_or_default() ||
            config->LogToNGX.value_or_default())
            spdlog::default_logger()->set_level((spdlog::level::level_enum) config->LogLevel.value_or_default());
        else
            spdlog::default_logger()->set_level(spdlog::level::off);

        if (bool toFile = config->LogToFile.value_or_default(); ImGui::Checkbox(Translation::Get("To File"), &toFile))
        {
            config->LogToFile = toFile;
            PrepareLogger();
        }

        ImGui::SameLine(0.0f, 6.0f);
        if (bool toConsole = config->LogToConsole.value_or_default(); ImGui::Checkbox(Translation::Get("To Console"), &toConsole))
        {
            config->LogToConsole = toConsole;
            PrepareLogger();
        }

        const char* logLevels[] = { Translation::Get("Trace"), Translation::Get("Debug"), Translation::Get("Information"), Translation::Get("Warning"), Translation::Get("Error") };
        const char* selectedLevel = logLevels[config->LogLevel.value_or_default()];

        if (ImGui::BeginCombo(Translation::Get("Log Level"), selectedLevel))
        {
            for (int n = 0; n < 5; n++)
            {
                if (ImGui::Selectable(logLevels[n], (config->LogLevel.value_or_default() == n)))
                {
                    config->LogLevel = n;
                    spdlog::default_logger()->set_level(
                        (spdlog::level::level_enum) config->LogLevel.value_or_default());
                }
            }

            ImGui::EndCombo();
        }
    }
}

void MenuCommon::RenderThemeSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // THEME -----------------------------
    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Menu Theme and Color")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        bool lightTheme = config->LightTheme.value_or_default();

        const ImVec4 bgDark = lightTheme ? ImVec4(0.80f, 0.82f, 0.86f, 1.00f) : ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
        const ImVec4 bgMid = lightTheme ? ImVec4(0.89f, 0.91f, 0.95f, 1.00f) : ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
        const ImVec4 bgLight = lightTheme ? ImVec4(0.96f, 0.97f, 0.99f, 1.00f) : ImVec4(0.14f, 0.14f, 0.15f, 1.00f);

        auto Mix = [](const ImVec4& a, const ImVec4& b, float t, float alpha = 1.0f)
        { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, alpha); };

        auto AccentSoft = [&](ImVec4 accent, float alpha = 1.0f)
        { return toneMapColor(lightTheme ? Mix(bgLight, accent, 0.24f, alpha) : Mix(bgDark, accent, 0.32f, alpha)); };

        auto AccentMed = [&](ImVec4 accent, float alpha = 1.0f)
        { return toneMapColor(lightTheme ? Mix(bgLight, accent, 0.42f, alpha) : Mix(bgDark, accent, 0.55f, alpha)); };

        auto AccentStrong = [&](ImVec4 accent, float alpha = 1.0f)
        { return toneMapColor(ImVec4(accent.x, accent.y, accent.z, alpha)); };

        if (ImGui::Checkbox(Translation::Get("Light Theme"), &lightTheme))
        {
            config->LightTheme = lightTheme;
            ApplyThemeStyle();
        }

        ImGui::SeparatorText(Translation::Get("Accent Colour"));

        // Color palette grid
        static const struct { ImVec4 c; const char* label; } accentPresets[] = {
            { { 0.000f, 0.400f, 0.770f, 1.0f }, "Blue" },
            { { 0.000f, 1.000f, 0.910f, 1.0f }, "Teal" },
            { { 0.250f, 1.000f, 0.000f, 1.0f }, "Green" },
            { { 1.000f, 0.890f, 0.000f, 1.0f }, "Yellow" },
            { { 1.000f, 0.520f, 0.000f, 1.0f }, "Orange" },
            { { 1.000f, 0.000f, 0.000f, 1.0f }, "Red" },
            { { 0.576f, 0.000f, 1.000f, 1.0f }, "Purple" },
            { { 0.540f, 0.540f, 0.540f, 1.0f }, "Gray" },
        };

        constexpr int cols = 4;
        float btnSize = ImGui::GetFrameHeight();

        for (int i = 0; i < IM_ARRAYSIZE(accentPresets); i++)
        {
            if (i % cols != 0) ImGui::SameLine(0.0f, 4.0f);
            ImGui::PushID(i);
            ImVec4 col = toneMapColor(accentPresets[i].c);
            if (ImGui::ColorButton("##accent", col, ImGuiColorEditFlags_NoTooltip, ImVec2(btnSize, btnSize)))
            {
                config->MenuAccentColorR = accentPresets[i].c.x;
                config->MenuAccentColorG = accentPresets[i].c.y;
                config->MenuAccentColorB = accentPresets[i].c.z;
                ApplyThemeStyle();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(Translation::Get(accentPresets[i].label));
            ImGui::PopID();
        }

        ImGui::Spacing();

        float accentColor[3] = { config->MenuAccentColorR.value_or_default(),
                                 config->MenuAccentColorG.value_or_default(),
                                 config->MenuAccentColorB.value_or_default() };

    if (ImGui::ColorEdit3(Translation::Get("Custom Accent Color"), accentColor))
    {
            config->MenuAccentColorR = accentColor[0];
            config->MenuAccentColorG = accentColor[1];
            config->MenuAccentColorB = accentColor[2];
            ApplyThemeStyle();
        }

        ImGui::Spacing();

        if (ImGui::Button(Translation::Get("Reset Accent Color")))
        {
            config->MenuAccentColorR.reset();
            config->MenuAccentColorG.reset();
            config->MenuAccentColorB.reset();
            ApplyThemeStyle();
        }

        ImGui::Spacing();

        ImGui::SeparatorText(Translation::Get("Background Colour"));

        static const struct { ImVec4 c; const char* label; } bgPresets[] = {
            { { 0.000f, 0.400f, 0.770f, 1.0f }, "Blue" },
            { { 0.000f, 1.000f, 0.910f, 1.0f }, "Teal" },
            { { 0.250f, 1.000f, 0.000f, 1.0f }, "Green" },
            { { 1.000f, 0.890f, 0.000f, 1.0f }, "Yellow" },
            { { 1.000f, 0.520f, 0.000f, 1.0f }, "Orange" },
            { { 1.000f, 0.000f, 0.000f, 1.0f }, "Red" },
            { { 0.576f, 0.000f, 1.000f, 1.0f }, "Purple" },
            { { 0.540f, 0.540f, 0.540f, 1.0f }, "Gray" },
        };

        for (int i = 0; i < IM_ARRAYSIZE(bgPresets); i++)
        {
            if (i % 4 != 0) ImGui::SameLine(0.0f, 4.0f);
            ImGui::PushID(100 + i);
            ImVec4 col = toneMapColor(bgPresets[i].c);
            if (ImGui::ColorButton("##bg", col, ImGuiColorEditFlags_NoTooltip, ImVec2(btnSize, btnSize)))
            {
                config->MenuBGColorR = bgPresets[i].c.x;
                config->MenuBGColorG = bgPresets[i].c.y;
                config->MenuBGColorB = bgPresets[i].c.z;
                ApplyThemeStyle();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(Translation::Get(bgPresets[i].label));
            ImGui::PopID();
        }

        float bgColor[3] = { config->MenuBGColorR.value_or_default(), config->MenuBGColorG.value_or_default(),
                             config->MenuBGColorB.value_or_default() };

    if (ImGui::ColorEdit3(Translation::Get("Custom BG Colour"), bgColor))
    {
            config->MenuBGColorR = bgColor[0];
            config->MenuBGColorG = bgColor[1];
            config->MenuBGColorB = bgColor[2];
            ApplyThemeStyle();
        }

        ImGui::Spacing();

        auto alpha = config->MenuBGColorA.value_or_default();
        if (ImGui::SliderFloat(Translation::Get("Background Alpha"), &alpha, 0.0f, 1.0f))
        {
            config->MenuBGColorA = alpha;
            ApplyThemeStyle();
        }

        ImGui::Spacing();

    if (ImGui::Button(Translation::Get("Reset BG Colour")))
        {
            config->MenuBGColorR.reset();
            config->MenuBGColorG.reset();
            config->MenuBGColorB.reset();
            config->MenuBGColorA.reset();
            ApplyThemeStyle();
        }

        ImGui::Spacing();
    }
}

void MenuCommon::RenderFpsOverlaySettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    // FPS OVERLAY -----------------------------
    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("FPS Overlay")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        bool fpsEnabled = config->ShowFps.value_or_default();
        if (ImGui::Checkbox(Translation::Get("FPS Overlay Enabled"), &fpsEnabled))
            config->ShowFps = fpsEnabled;

        ImGui::SameLine(0.0f, 6.0f);

        bool fpsHorizontal = config->FpsOverlayHorizontal.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Horizontal"), &fpsHorizontal))
            config->FpsOverlayHorizontal = fpsHorizontal;

        const char* fpsPosition[] = { Translation::Get("Top Left"), Translation::Get("Top Right"), Translation::Get("Bottom Left"), Translation::Get("Bottom Right") };
        const char* selectedPosition = fpsPosition[config->FpsOverlayPosition.value_or_default()];

        if (ImGui::BeginCombo(Translation::Get("Overlay Position"), selectedPosition))
        {
            for (int n = 0; n < FpsOverlayPos_COUNT; n++)
            {
                if (ImGui::Selectable(fpsPosition[n], (config->FpsOverlayPosition.value_or_default() == n)))
                    config->FpsOverlayPosition = (FpsOverlayPos)n;
            }

            ImGui::EndCombo();
        }

        const char* fpsType[] = { Translation::Get("Just FPS"), Translation::Get("Simple"),       Translation::Get("Detailed"),      Translation::Get("Detailed + Graph"),
                                  Translation::Get("Full"),     Translation::Get("Full + Graph"), Translation::Get("Reflex timings") };
        const char* selectedType = fpsType[config->FpsOverlayType.value_or_default()];

        if (ImGui::BeginCombo(Translation::Get("Overlay Type"), selectedType))
        {
            for (int n = 0; n < std::size(fpsType); n++)
            {
                if (ImGui::Selectable(fpsType[n], (config->FpsOverlayType.value_or_default() == n)))
                    config->FpsOverlayType = (FpsOverlay) n;
            }

            ImGui::EndCombo();
        }

        float fpsAlpha = config->FpsOverlayAlpha.value_or_default();
        if (ImGui::SliderFloat(Translation::Get("Background Alpha"), &fpsAlpha, 0.0f, 1.0f, "%.2f"))
            config->FpsOverlayAlpha = fpsAlpha;

        const char* options[] = { Translation::Get("Same as menu"), "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1", "1.2",
                                  "1.3",          "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0" };
        int currentIndex = std::max(((int) (config->FpsScale.value_or(0.0f) * 10.0f)) - 4, 0);
        float values[] = { 0.0f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f,
                           1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f };

        if (ImGui::SliderInt(Translation::Get("Scale"), &currentIndex, 0, IM_ARRAYSIZE(options) - 1, options[currentIndex],
                             ImGuiSliderFlags_ClampOnInput))
        {
            if (currentIndex == 0)
                config->FpsScale.reset();
            else
                config->FpsScale = values[currentIndex];
        }

        bool useTheme = config->OverlaysUseTheme.value_or_default();
        if (ImGui::Checkbox(Translation::Get("Use Theme Colors"), &useTheme))
            config->OverlaysUseTheme = useTheme;
    }
}

void MenuCommon::RenderUpscalerInputsSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;

    // UPSCALER INPUTS -----------------------------
    ImGui::Spacing();
    auto uiStateOpen = currentFeature == nullptr || currentFeature->IsFrozen();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Upscaler Inputs"), uiStateOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

        if (config->EnableFsr2Inputs.value_or_default())
        {
            bool fsr2Inputs = config->UseFsr2Inputs.value_or_default();
            bool fsr2Pattern = config->Fsr2Pattern.value_or_default();

            if (ImGui::Checkbox(Translation::Get("Use Fsr2 Inputs"), &fsr2Inputs))
                config->UseFsr2Inputs = fsr2Inputs;

            if (ImGui::Checkbox(Translation::Get("Use Fsr2 Pattern Matching"), &fsr2Pattern))
                config->Fsr2Pattern = fsr2Pattern;
            ShowTooltip(Translation::Get("This setting will become active on next boot!"));
        }

        if (config->EnableFsr3Inputs.value_or_default())
        {
            bool fsr3Inputs = config->UseFsr3Inputs.value_or_default();
            bool fsr3Pattern = config->Fsr3Pattern.value_or_default();

            if (ImGui::Checkbox(Translation::Get("Use Fsr3 Inputs"), &fsr3Inputs))
                config->UseFsr3Inputs = fsr3Inputs;

            if (ImGui::Checkbox(Translation::Get("Use Fsr3 Pattern Matching"), &fsr3Pattern))
                config->Fsr3Pattern = fsr3Pattern;
            ShowTooltip(Translation::Get("This setting will become active on next boot!"));
        }

        if (config->EnableFfxInputs.value_or_default())
        {
            bool ffxInputs = config->UseFfxInputs.value_or_default();

            if (ImGui::Checkbox(Translation::Get("Use Ffx Inputs"), &ffxInputs))
                config->UseFfxInputs = ffxInputs;
        }
    }
}

void MenuCommon::RenderApiAndTextureSettings(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;

    // DX11 & DX12 -----------------------------
    if (state.swapchainApi != Vulkan)
    {
        // V-SYNC -----------------------------
        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(Translation::Get("V-Sync Settings")); ch.IsHeaderOpen())
        {
            ScopedIndent indent {};
            ImGui::Spacing();

            auto forceVsyncOn = config->ForceVsync.has_value() && config->ForceVsync.value();
            auto forceVsyncOff = config->ForceVsync.has_value() && !config->ForceVsync.value();
            bool vsyncChanged = false;

            if (ImGui::Checkbox(Translation::Get("V-Sync On"), &forceVsyncOn))
            {
                if (forceVsyncOn)
                {
                    config->ForceVsync = true;
                    vsyncChanged = true;
                }
                else
                {
                    config->ForceVsync.reset();
                    vsyncChanged = true;
                }
            }
            ImGui::SameLine(0.0f, 16.0f);

            if (ImGui::Checkbox(Translation::Get("V-Sync Off"), &forceVsyncOff))
            {
                if (forceVsyncOff)
                {
                    config->ForceVsync = false;
                    vsyncChanged = true;
                }
                else
                {
                    config->ForceVsync.reset();
                    vsyncChanged = true;
                }
            }
            ImGui::SameLine(0.0f, 16.0f);

            ImGui::BeginDisabled(!forceVsyncOn);

            ImGui::PushItemWidth(50.0f * menuResScale);

        auto vsyncBuf = StrFmt(Translation::Get("%d"), config->VsyncInterval.value_or_default());
            if (ImGui::BeginCombo(Translation::Get("Sync Int."), vsyncBuf.c_str()))
            {
                if (ImGui::Selectable("0", config->VsyncInterval.value_or_default() == 0))
                {
                    config->VsyncInterval = 0;
                    vsyncChanged = true;
                }

                if (ImGui::Selectable("1", config->VsyncInterval.value_or_default() == 1))
                {
                    config->VsyncInterval = 1;
                    vsyncChanged = true;
                }

                if (ImGui::Selectable("2", config->VsyncInterval.value_or_default() == 2))
                {
                    config->VsyncInterval = 2;
                    vsyncChanged = true;
                }

                if (ImGui::Selectable("3", config->VsyncInterval.value_or_default() == 3))
                {
                    config->VsyncInterval = 3;
                    vsyncChanged = true;
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ShowHelpMarker("Controls the DXGI Present sync interval, which determines how\n"
                           "the swap chain waits for vertical refresh.\n\n"
                           "0  = Present immediately, no VSync wait.\n"
                           "1  = Sync to every refresh, normal VSync.\n"
                           "2+ = Present every N refreshes, reducing effective frame rate.\n\n"
                           "Higher values can reduce tearing but may increase latency and cap FPS.\n"
                           "For most games, use 0 for lowest latency or 1 for normal VSync.");

            ImGui::EndDisabled();
            ImGui::SameLine(0.0f, 16.0f);

            if (ImGui::Button(Translation::Get("Reset##10")))
            {
                config->ForceVsync.reset();
                vsyncChanged = true;
            }

            ShowHelpMarker(Translation::Get("Force V-Sync On/Off & Sync Interval options"));

            if (vsyncChanged && state.activeFgOutput == FGOutput::XeFG && state.currentFG != nullptr)
            {
                // To prevent XeLL issues
                LOG_DEBUG("V-Sync change detected, forcing XeFG reset");
                state.WAR_xefgRequestFGToggle = true;
            }
        }

        // MIPMAP BIAS & Anisotropy -----------------------------
        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(Translation::Get("Mipmap Bias"), (currentFeature == nullptr || currentFeature->IsFrozen())
                                                                ? ImGuiTreeNodeFlags_DefaultOpen
                                                                : 0);
            ch.IsHeaderOpen())
        {
            ScopedIndent indent {};
            ImGui::Spacing();
            if (config->MipmapBiasOverride.has_value() && _mipBias == 0.0f)
                _mipBias = config->MipmapBiasOverride.value();

            ImGui::SliderFloat(Translation::Get("Mipmap Bias##2"), &_mipBias, -15.0f, 15.0f, "%.6f");
            ShowHelpMarker(Translation::Get("Can help with blurry textures in broken games\n"
                           "Negative values will make textures sharper\n"
                           "Positive values will make textures more blurry\n\n"
                           "Has a small performance impact"));

            ImGui::BeginDisabled(!config->MipmapBiasOverride.has_value());
            {
                ImGui::BeginDisabled(config->MipmapBiasScaleOverride.has_value() &&
                                     config->MipmapBiasScaleOverride.value());
                {
                    bool mbFixed = config->MipmapBiasFixedOverride.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("MB Fixed Override"), &mbFixed))
                    {
                        config->MipmapBiasScaleOverride.reset();
                        config->MipmapBiasFixedOverride = mbFixed;
                    }

                    ShowHelpMarker(Translation::Get("Apply same override value to all textures"));
                }
                ImGui::EndDisabled();

                ImGui::SameLine(0.0f, 6.0f);

                ImGui::BeginDisabled(config->MipmapBiasFixedOverride.has_value() &&
                                     config->MipmapBiasFixedOverride.value());
                {
                    bool mbScale = config->MipmapBiasScaleOverride.value_or_default();
                    if (ImGui::Checkbox(Translation::Get("MB Scale Override"), &mbScale))
                    {
                        config->MipmapBiasFixedOverride.reset();
                        config->MipmapBiasScaleOverride = mbScale;
                    }

                    ShowHelpMarker(Translation::Get("Apply override value as scale multiplier\n"
                                   "When using scale mode, please use positive\n"
                                   "override values to increase sharpness!"));
                }
                ImGui::EndDisabled();

                bool mbAll = config->MipmapBiasOverrideAll.value_or_default();
                if (ImGui::Checkbox(Translation::Get("MB Override All Textures"), &mbAll))
                    config->MipmapBiasOverrideAll = mbAll;

                ShowHelpMarker(Translation::Get("Override all textures mipmap values\n"
                               "Normally OptiScaler only overrides\n"
                               "below zero mipmap values!"));
            }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(config->MipmapBiasOverride.has_value() &&
                                 config->MipmapBiasOverride.value() == _mipBias);
            {
                if (ImGui::Button(Translation::Get("Set")))
                {
                    config->MipmapBiasOverride = _mipBias;
                    state.lastMipBias = 100.0f;
                    state.lastMipBiasMax = -100.0f;
                }
            }
            ImGui::EndDisabled();

            ImGui::SameLine(0.0f, 6.0f);

            ImGui::BeginDisabled(!config->MipmapBiasOverride.has_value());
            {
                if (ImGui::Button(Translation::Get("Reset")))
                {
                    config->MipmapBiasOverride.reset();
                    _mipBias = 0.0f;
                    state.lastMipBias = 100.0f;
                    state.lastMipBiasMax = -100.0f;
                }
            }
            ImGui::EndDisabled();

            if (currentFeature != nullptr && !currentFeature->IsFrozen())
            {
                ImGui::SameLine(0.0f, 6.0f);

                if (ImGui::Button(Translation::Get("Calculate Mipmap Bias")))
                    _showMipmapCalcWindow = true;
            }

            if (config->MipmapBiasOverride.has_value())
            {
                if (config->MipmapBiasFixedOverride.value_or_default())
                {
                    ImGui::Text(Translation::Get("Current : %.3f / %.3f, Target: %.3f"), state.lastMipBias, state.lastMipBiasMax,
                                config->MipmapBiasOverride.value());
                }
                else if (config->MipmapBiasScaleOverride.value_or_default())
                {
                    ImGui::Text(Translation::Get("Current : %.3f / %.3f, Target: Base * %.3f"), state.lastMipBias, state.lastMipBiasMax,
                                config->MipmapBiasOverride.value());
                }
                else
                {
                    ImGui::Text(Translation::Get("Current : %.3f / %.3f, Target: Base + %.3f"), state.lastMipBias, state.lastMipBiasMax,
                                config->MipmapBiasOverride.value());
                }
            }
            else
            {
                ImGui::Text(Translation::Get("Current : %.3f / %.3f"), state.lastMipBias, state.lastMipBiasMax);
            }

            ImGui::Text(Translation::Get("Will be applied after RESOLUTION/PRESET change !!!"));
        }

        ImGui::Spacing();
        if (auto ch = ScopedCollapsingHeader(
                Translation::Get("Anisotropic Filtering"),
                (currentFeature == nullptr || currentFeature->IsFrozen()) ? ImGuiTreeNodeFlags_DefaultOpen : 0);
            ch.IsHeaderOpen())
        {
            ScopedIndent indent {};
            ImGui::Spacing();
            ImGui::PushItemWidth(65.0f * menuResScale);

            auto selectedAF =
                config->AnisotropyOverride.has_value() ? std::to_string(config->AnisotropyOverride.value()) : "Auto";
            if (ImGui::BeginCombo(Translation::Get("Force Anisotropic Filtering"), selectedAF.c_str()))
            {
                if (ImGui::Selectable(Translation::Get("Auto"), !config->AnisotropyOverride.has_value()))
                    config->AnisotropyOverride.reset();

                if (ImGui::Selectable("1", config->AnisotropyOverride.value_or(0) == 1))
                    config->AnisotropyOverride = 1;

                if (ImGui::Selectable("2", config->AnisotropyOverride.value_or(0) == 2))
                    config->AnisotropyOverride = 2;

                if (ImGui::Selectable("4", config->AnisotropyOverride.value_or(0) == 4))
                    config->AnisotropyOverride = 4;

                if (ImGui::Selectable("8", config->AnisotropyOverride.value_or(0) == 8))
                    config->AnisotropyOverride = 8;

                if (ImGui::Selectable("16", config->AnisotropyOverride.value_or(0) == 16))
                    config->AnisotropyOverride = 16;

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();

            bool afComp = config->AnisotropyModifyComp.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Modify Compare"), &afComp))
                config->AnisotropyModifyComp = afComp;

            ShowHelpMarker(Translation::Get("Update comparison filters"));

            ImGui::SameLine(0.0f, 6.0f);

            bool afMinMax = config->AnisotropyModifyMinMax.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Modify Min/Max"), &afMinMax))
                config->AnisotropyModifyMinMax = afMinMax;

            ShowHelpMarker(Translation::Get("Update min/max filters"));

            bool afSkipPoint = config->AnisotropySkipPointFilter.value_or_default();
            if (ImGui::Checkbox(Translation::Get("Skip Point Filters"), &afSkipPoint))
                config->AnisotropySkipPointFilter = afSkipPoint;

            ShowHelpMarker(Translation::Get("Skip updating of point filters"));

            ImGui::Text(Translation::Get("Will might be applied after RESOLUTION/PRESET change !!!"));
        }
    }
}

void MenuCommon::RenderKeybindSettings(RenderMenuContext& ctx)
{
    auto config = ctx.config;

    ImGui::Spacing();
    if (auto ch = ScopedCollapsingHeader(Translation::Get("Keybinds")); ch.IsHeaderOpen())
    {
        ScopedIndent indent {};
        ImGui::Spacing();

    ImGui::Text(Translation::Get("Key combinations are currently NOT supported!"));
    ImGui::Text(Translation::Get("Escape to cancel, Backspace to unbind"));
        ImGui::Spacing();

    static auto menu = Keybind(Translation::Get("Menu"), 10);
        static auto fpsOverlay = Keybind(Translation::Get("FPS Overlay"), 11);
        static auto fpsOverlayCycle = Keybind(Translation::Get("FPS Overlay Cycle"), 12);
        static auto fgEnable = Keybind(Translation::Get("Frame Generation"), 13);

        menu.Render(config->ShortcutKey);
        fpsOverlay.Render(config->FpsShortcutKey);
        fpsOverlayCycle.Render(config->FpsCycleShortcutKey);
        fgEnable.Render(config->FGShortcutKey);
    }
}

void MenuCommon::RenderMainMenuTable(RenderMenuContext& ctx)
{
    auto& state = ctx.state;

    static const char* sections[] = {
        "Upscaling",
        "Frame Gen",
        "Image",
        "Latency",
        "Display",
        "Advanced",
        "Settings",
    };
    const int sectionCount = IM_ARRAYSIZE(sections);

    auto& style = ImGui::GetStyle();
    // io.FontGlobalScale is never assigned (stays 1.0), so hand-placed geometry
    // must scale off menuResScale like the rest of the menu does.
    float uiScale = ctx.menuResScale;
    float sidebarW = 220.0f * uiScale;
    float availH = ImGui::GetContentRegionAvail().y - style.ItemSpacing.y * 2.0f;
    auto accentCol = toneMapColor(ImVec4(ctx.config->MenuAccentColorR.value_or_default(),
                                         ctx.config->MenuAccentColorG.value_or_default(),
                                         ctx.config->MenuAccentColorB.value_or_default(), 1.f));

    // === SIDEBAR ===
    // Recessed rail: darker than the window so the content panel reads as raised.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.22f)));
    ImGui::BeginChild("Sidebar", ImVec2(sidebarW, availH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();
    {
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 sbp = ImGui::GetCursorScreenPos();
        float cornerRadius = 6.0f;
        float itemH = ImGui::GetFontSize() + style.FramePadding.y * 3.5f;

        auto DrawNavIcon = [&](int idx, ImVec2 center, ImU32 col, float size)
        {
            float s = size * 0.5f;
            switch (idx)
            {
            case 0: // Upscaling: upward arrow
                dl->AddTriangleFilled({ center.x, center.y - s }, { center.x - s, center.y + s * 0.3f },
                                      { center.x + s, center.y + s * 0.3f }, col);
                break;
            case 1: // Frame Gen: circular arrow
                dl->AddCircle(center, s, col, 16, 2.0f);
                dl->AddTriangleFilled({ center.x + s * 0.2f, center.y - s },
                                      { center.x - s * 0.3f, center.y - s * 0.7f },
                                      { center.x + s * 0.5f, center.y - s * 0.3f }, col);
                break;
            case 2: // Image: picture frame with mountain
                dl->AddRect({ center.x - s, center.y - s * 0.7f }, { center.x + s, center.y + s * 0.7f },
                            col, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
                dl->AddLine({ center.x - s * 0.5f, center.y + s * 0.2f }, { center.x - s * 0.1f, center.y - s * 0.1f }, col, 2.0f);
                dl->AddLine({ center.x - s * 0.1f, center.y - s * 0.1f }, { center.x + s * 0.4f, center.y + s * 0.5f }, col, 2.0f);
                break;
            case 3: // Latency: lightning bolt
            {
                ImVec2 pts[5] = {
                    { center.x + s * 0.4f, center.y - s },
                    { center.x - s * 0.3f, center.y - s * 0.1f },
                    { center.x + s * 0.1f, center.y - s * 0.1f },
                    { center.x - s * 0.2f, center.y + s },
                    { center.x + s * 0.3f, center.y + s * 0.1f }
                };
                dl->AddPolyline(pts, 5, col, ImDrawFlags_None, 2.5f);
                break;
            }
            case 4: // Display: monitor
                dl->AddRect({ center.x - s, center.y - s * 0.6f }, { center.x + s, center.y + s * 0.4f },
                            col, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
                dl->AddLine({ center.x - s * 0.3f, center.y + s * 0.4f }, { center.x - s * 0.3f, center.y + s * 0.75f }, col, 2.0f);
                dl->AddLine({ center.x + s * 0.3f, center.y + s * 0.4f }, { center.x + s * 0.3f, center.y + s * 0.75f }, col, 2.0f);
                dl->AddLine({ center.x - s * 0.5f, center.y + s * 0.75f }, { center.x + s * 0.5f, center.y + s * 0.75f }, col, 2.0f);
                break;
            case 5: // Advanced: gear
            case 6: // Settings: gear
            {
                float outer = s * 0.85f;
                float inner = s * 0.45f;
                int teeth = 8;
                for (int t = 0; t < teeth; t++)
                {
                    float a0 = (float)t / teeth * 2.0f * IM_PI;
                    float a1 = (float)(t + 0.35f) / teeth * 2.0f * IM_PI;
                    float a2 = (float)(t + 0.65f) / teeth * 2.0f * IM_PI;
                    float a3 = (float)(t + 1.0f) / teeth * 2.0f * IM_PI;
                    ImVec2 p0 = { center.x + cosf(a0) * inner, center.y + sinf(a0) * inner };
                    ImVec2 p1 = { center.x + cosf(a1) * inner, center.y + sinf(a1) * inner };
                    ImVec2 p2 = { center.x + cosf(a1) * outer, center.y + sinf(a1) * outer };
                    ImVec2 p3 = { center.x + cosf(a2) * outer, center.y + sinf(a2) * outer };
                    ImVec2 p4 = { center.x + cosf(a2) * inner, center.y + sinf(a2) * inner };
                    ImVec2 p5 = { center.x + cosf(a3) * inner, center.y + sinf(a3) * inner };
                    dl->AddLine(p0, p1, col, 2.0f);
                    dl->AddLine(p1, p2, col, 2.0f);
                    dl->AddLine(p2, p3, col, 2.0f);
                    dl->AddLine(p3, p4, col, 2.0f);
                    dl->AddLine(p4, p5, col, 2.0f);
                }
                dl->AddCircleFilled(center, s * 0.22f, col, 8);
                break;
            }
            default:
                dl->AddCircleFilled(center, s * 0.4f, col, 8);
                break;
            }
        };

        // Resting plate for nav items. The rail is a recessed (darkened) surface, so
        // lift the plates with white on dark themes and darken them on light themes;
        // FrameBg alone is too close to the rail to be visible.
        bool navLightTheme = ctx.config->LightTheme.value_or_default();
        ImU32 navPlateCol = navLightTheme ? ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.10f))
                                          : ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
        ImU32 navPlateBorderCol = navLightTheme ? ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.22f))
                                                : ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.20f));

        // Evenly distribute nav items over the full rail height.
        float totalItemsH = sectionCount * itemH;
        float itemSpacing =
            (availH > totalItemsH) ? (availH - totalItemsH) / (sectionCount + 1) : (6.0f * uiScale);

        ImGui::Dummy(ImVec2(0.0f, itemSpacing));

        for (int i = 0; i < sectionCount; i++)
        {
            if (i > 0)
                ImGui::Dummy(ImVec2(0.0f, itemSpacing));

            ImGui::PushID(i);
            bool active = (_activeSection == i);
            const char* translatedLabel = Translation::Get(sections[i]);
            ImVec2 labelSize = ImGui::CalcTextSize(translatedLabel);
            float btnH = std::max(itemH, labelSize.y + style.FramePadding.y * 2.0f);
            float btnW = sidebarW - 24.0f * uiScale;

            // Reserve the full button area with an invisible button
            ImVec2 cursorBefore = ImGui::GetCursorScreenPos();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            bool clicked = ImGui::Button("##navbtn", ImVec2(btnW, btnH));
            ImGui::PopStyleColor(3);
            if (clicked)
                _activeSection = i;

            ImVec2 btnMin = cursorBefore;
            ImVec2 btnMax = ImVec2(btnMin.x + btnW, btnMin.y + btnH);

            bool hovered = ImGui::IsItemHovered();

            // Hover floating effect: shift right and scale up slightly
            float hoverOffset = hovered ? 4.0f * uiScale : 0.0f;
            float hoverScale = hovered ? 1.03f : 1.0f;
            ImVec2 hoverMin = { btnMin.x + hoverOffset - (btnW * (hoverScale - 1.0f) * 0.5f),
                                btnMin.y - (btnH * (hoverScale - 1.0f) * 0.5f) };
            ImVec2 hoverMax = { hoverMin.x + btnW * hoverScale, hoverMin.y + btnH * hoverScale };

            // Every item gets a visible resting plate. Two layers: a neutral
            // lift against the recessed rail (FrameBg is too dark here to read),
            // then an accent tint on top for hover / active.
            dl->AddRectFilled(hoverMin, hoverMax, navPlateCol, cornerRadius);
            if (active)
                dl->AddRectFilled(hoverMin, hoverMax,
                                  ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.30f)),
                                  cornerRadius);
            else if (hovered)
                dl->AddRectFilled(hoverMin, hoverMax,
                                  ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.18f)),
                                  cornerRadius);

            // Outline every item so each row reads as a distinct frame.
            ImU32 navBorderCol = (active || hovered)
                                     ? ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z,
                                                                 active ? 0.70f : 0.40f))
                                     : navPlateBorderCol;
            dl->AddRect(hoverMin, hoverMax, navBorderCol, cornerRadius, ImDrawFlags_RoundCornersAll,
                        active ? 1.5f * uiScale : 1.0f * uiScale);

            // Active indicator bar on the left
            if (active)
            {
                ImVec2 barMin = { hoverMin.x + 4.0f * uiScale, hoverMin.y + 10.0f * uiScale };
                ImVec2 barMax = { barMin.x + 4.0f * uiScale, hoverMax.y - 10.0f * uiScale };
                dl->AddRectFilled(barMin, barMax, ImGui::GetColorU32(accentCol), 2.0f * uiScale);
            }

            // Nav icon on the left, vertically centered within the button
            ImVec2 iconCenter = { hoverMin.x + 20.0f * uiScale, hoverMin.y + btnH * hoverScale * 0.5f };
            ImU32 iconCol = active ? ImGui::GetColorU32(accentCol)
                                   : (hovered ? ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.85f))
                                              : ImGui::GetColorU32(ImGuiCol_TextDisabled));
            DrawNavIcon(i, iconCenter, iconCol, 16.0f * uiScale);

            // Text on the right of the icon, vertically centered
            ImVec2 textPos = { hoverMin.x + 38.0f * uiScale, hoverMin.y + (btnH * hoverScale - labelSize.y) * 0.5f };
            ImU32 textCol = active ? ImGui::GetColorU32(accentCol)
                                   : (hovered ? ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.9f))
                                              : ImGui::GetColorU32(ImGuiCol_Text));
            dl->AddText(textPos, textCol, translatedLabel);

            ImGui::PopID();
        }

    }
    ImGui::EndChild();

    ImGui::SameLine(0.0f, style.ItemSpacing.x);

    // === CONTENT PANEL ===
    // Raised surface: lighter than the window, opposite the recessed sidebar.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.035f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * uiScale, 14.0f * uiScale));
    ImGui::BeginChild("ContentBody", ImVec2(0.0f, availH), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f * uiScale, 7.0f * uiScale));
        ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(12.0f * uiScale, 6.0f * uiScale));

        // Section content
        switch (_activeSection)
        {
        case 0: RenderActiveUpscalerSettings(ctx); RenderFsrCommonSettings(ctx);
                RenderUpscalerInputsSettings(ctx); RenderApiAndTextureSettings(ctx); break;
        case 1: RenderFrameGenerationSelection(ctx); RenderFrameGenerationRuntimeSettings(ctx); break;
        case 2: RenderActiveImageSettings(ctx); RenderMagnifierSettings(ctx); RenderFramerateSettings(ctx); break;
        case 3: RenderFakenvapiSettings(ctx); break;
        case 4: RenderFpsOverlaySettings(ctx); RenderThemeSettings(ctx); RenderKeybindSettings(ctx); break;
        case 5: RenderQuirksSettings(ctx); RenderAdvancedSettings(ctx); RenderLoggingSettings(ctx); break;
        case 6: RenderSettingsSection(ctx); break;
        }

        // nvngx.ini warning
        if (state.nvngxIniDetected)
        {
            ImGui::Spacing();
            ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.2f, 0.2f, 1.f)),
                               "%s", Translation::Get("nvngx.ini detected, please move over to using OptiScaler.ini and delete the old config"));
        }

        ImGui::PopStyleVar(2);
    }
    ImGui::EndChild();
}

void MenuCommon::RenderMainMenuGraphs(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto& currentFeature = ctx.currentFeature;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("plots", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextColumn();
        ImGui::Text(Translation::Get("FrameTime"));
        auto ft = StrFmt("%7.2f ms / %6.1f fps", frameTime, frameRate);
        ImGui::PlotLines(
            ft.c_str(), [](void* rb, int idx) -> float
            { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); }, &gFrameTimes, plotWidth);

        if (currentFeature != nullptr && !currentFeature->IsFrozen())
        {
            ImGui::TableNextColumn();
        ImGui::Text(Translation::Get("Upscaler"));
            auto ups = StrFmt("%7.2f ms", state.upscaleTimes.back());
            ImGui::PlotLines(
                ups.c_str(), [](void* rb, int idx) -> float
                { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); }, &gUpscalerTimes, plotWidth);
        }

        ImGui::EndTable();
    }
}

void MenuCommon::RenderSettingsSection(RenderMenuContext& ctx)
{
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& menuResScale = ctx.menuResScale;

    ImGui::SeparatorText(Translation::Get("Settings"));
    ImGui::Spacing();

    float availW = ImGui::GetContentRegionAvail().x;

    auto drawCard = [&](const char* title, std::function<void()> body)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.03f)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13.0f * menuResScale, 11.0f * menuResScale));
        ImGui::BeginChild(title, ImVec2(availW, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        {
            // Card titles lead the group, so keep them at full text weight and
            // underline the header instead of dimming it.
            ImGui::TextUnformatted(title);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            body();
        }
        ImGui::EndChild();
    };

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * menuResScale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f * menuResScale);

    drawCard(Translation::Get("Preferences"), [&]()
    {
        const char* selectedLangName = Translation::GetLanguageName(Translation::GetLanguage());
        if (ImGui::BeginCombo(Translation::Get("Language"), selectedLangName))
        {
            for (auto lang : Translation::GetAllLanguages())
            {
                const char* name = Translation::GetLanguageName(lang);
                if (ImGui::Selectable(name, Translation::GetLanguage() == lang))
                {
                    Translation::SetLanguage(lang);
                    config->Language = static_cast<int>(lang);
                }
            }
            ImGui::EndCombo();
        }

        auto autoText = config->MenuScale.has_value() ? Translation::Get("Auto") : StrFmt("Auto (%3.1f)", menuResScale);
        const char* uiScales[] = { autoText.c_str(), "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1",
                                   "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0" };

        if (ImGui::BeginCombo(Translation::Get("Menu Scale"), uiScales[_selectedScale]))
        {
            for (int n = 0; n < IM_ARRAYSIZE(uiScales); n++)
            {
                if (ImGui::Selectable(uiScales[n], (_selectedScale == n)))
                {
                    _selectedScale = n;
                    if (n == 0) config->MenuScale.reset();
                    else        config->MenuScale = 0.4f + (float)n / 10.0f;
                }
            }
            ImGui::EndCombo();
        }
    });

    ImGui::Spacing();

    drawCard(Translation::Get("Actions"), [&]()
    {
        auto btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
        if (ImGui::Button(Translation::Get("Save Settings"), ImVec2(btnW, 0.0f)))
            config->SaveIni();
        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::Button(Translation::Get("Close"), ImVec2(btnW, 0.0f)))
        {
            _isVisible = false;
            hasGamepad = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
            io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
            io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;
            _showMipmapCalcWindow = false;
            _showHudlessWindow = false;
            io.MouseDrawCursor = false;
            io.WantCaptureKeyboard = false;
            io.WantCaptureMouse = false;
        }

        if (ImGui::Button(Translation::Get("Open Wiki"), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
        {
            auto pIO = &ImGui::GetPlatformIO();
            pIO->Platform_OpenInShellFn(ImGui::GetCurrentContext(), "https://github.com/optiscaler/OptiScaler/wiki");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Translation::Get("Open OptiScaler Wiki"));
    });

    ImGui::PopStyleVar(2);
}

void MenuCommon::RenderMainMenuBottomBar(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& currentFeature = ctx.currentFeature;
    auto& menuResScale = ctx.menuResScale;

    // BOTTOM LINE ---------------
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (currentFeature != nullptr && !currentFeature->IsFrozen())
    {
        ImGui::Text(Translation::Get("%dx%d -> %dx%d (%.1f) [%dx%d (%.1f)]"), currentFeature->RenderWidth(),
                    currentFeature->RenderHeight(), currentFeature->TargetWidth(), currentFeature->TargetHeight(),
                    (float) currentFeature->TargetWidth() / (float) currentFeature->RenderWidth(),
                    currentFeature->DisplayWidth(), currentFeature->DisplayHeight(),
                    (float) currentFeature->DisplayWidth() / (float) currentFeature->RenderWidth());

        ImGui::SameLine(0.0f, 4.0f);

    ImGui::Text(Translation::Get("%d"), currentFeature->FrameCount());

        ImGui::SameLine(0.0f, 10.0f);
    }

    {
        const char* selectedLangName = Translation::GetLanguageName(Translation::GetLanguage());
        if (ImGui::BeginCombo(Translation::Get("Language"), selectedLangName))
        {
            for (auto lang : Translation::GetAllLanguages())
            {
                const char* name = Translation::GetLanguageName(lang);
                if (ImGui::Selectable(name, Translation::GetLanguage() == lang))
                {
                    Translation::SetLanguage(lang);
                    config->Language = static_cast<int>(lang);
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::PushItemWidth(100.0f * menuResScale);

    auto autoText = config->MenuScale.has_value() ? Translation::Get("Auto") : StrFmt("Auto (%3.1f)", menuResScale);
    // clang-format off
    const char* uiScales[] = { autoText.c_str(), "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1",
                               "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0" };
    // clang-format on

    const char* selectedScaleName = uiScales[_selectedScale];

    if (ImGui::BeginCombo(Translation::Get("Menu Scale"), selectedScaleName))
    {
        for (int n = 0; n < std::size(uiScales); n++)
        {
            if (ImGui::Selectable(uiScales[n], (_selectedScale == n)))
            {
                _selectedScale = n;

                if (n == 0)
                    config->MenuScale.reset();
                else
                    config->MenuScale = 0.4f + (float) n / 10.0f;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, 15.0f);

    if (ImGui::Button(Translation::Get("Save Settings")))
        config->SaveIni();

    ImGui::SameLine(0.0f, 6.0f);

    if (ImGui::Button(Translation::Get("Close")))
    {
        _isVisible = false;
        hasGamepad = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;

        _showMipmapCalcWindow = false;
        _showHudlessWindow = false;
        io.MouseDrawCursor = false;
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;
    }

    auto winSize = ImGui::GetWindowSize();
    auto winPos = ImGui::GetWindowPos();

    ImGui::SameLine();

    auto textSize = ImGui::CalcTextSize("Open Wiki (?)");
    auto& style = ImGui::GetStyle();
    textSize.x += style.FramePadding.x * 2.0f;
    textSize.x += style.ItemSpacing.x;

    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textSize.x);

    // Make button text underline
    if (ImGui::Button(Translation::Get("Open Wiki")))
    {
        auto pIO = &ImGui::GetPlatformIO();
        auto ctx = ImGui::GetCurrentContext();
        pIO->Platform_OpenInShellFn(ctx, "https://github.com/optiscaler/OptiScaler/wiki");
    }
    ShowHelpMarker(Translation::Get("Click to open the OptiScaler Wiki page\nin your default browser\n\n"
                   "Compatibility list with known game issues\nand workarounds, FG options explained\n"
                   "and other useful info"));

    ImGui::Spacing();
    ImGui::Separator();

    if (state.nvngxIniDetected)
    {
        ImGui::Spacing();
        ImGui::TextColored(toneMapColor(ImVec4(1.f, 0.f, 0.f, 1.f)),
                           Translation::Get("nvngx.ini detected, please move over to using OptiScaler.ini and delete the old config"));
        ImGui::Spacing();
    }

    if (lastPosition.x < -900.0f || (lastPosition.x >= winPos.x - 1.0f && lastPosition.y >= winPos.y - 1.0f &&
                                     lastPosition.x <= winPos.x + 1.0f && lastPosition.y <= winPos.y + 1.0f))
    {
        float posX;
        float posY;

        posX = ((float) io.DisplaySize.x - winSize.x) / 2.0f;
        posY = ((float) io.DisplaySize.y - winSize.y) / 2.0f;

        // don't position menu outside of screen
        if (posX < 0.0 || posY < 0.0)
        {
            posX = 50;
            posY = 50;
        }

        ImGui::SetWindowPos(ImVec2 { posX, posY });
        lastPosition.x = posX;
        lastPosition.y = posY;
    }
}

void MenuCommon::RenderMipmapBiasWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags)
{
    auto config = ctx.config;
    auto& io = ctx.io;
    auto& currentFeature = ctx.currentFeature;

    // Metrics window (for debug)
    // ImGui::ShowMetricsWindow();

    // Mipmap calculation window
    if (_showMipmapCalcWindow && currentFeature != nullptr && !currentFeature->IsFrozen() && currentFeature->IsInited())
    {
        auto posX = (io.DisplaySize.x - 450.0f) / 2.0f;
        auto posY = (io.DisplaySize.y - 200.0f) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2 { posX, posY }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2 { 450.0f, 200.0f }, ImGuiCond_FirstUseEver);

        if (_displayWidth == 0)
        {
            if (config->OutputScalingEnabled.value_or_default())
            {
                _displayWidth = static_cast<uint32_t>(currentFeature->DisplayWidth() *
                                                      config->OutputScalingMultiplier.value_or_default());
            }
            else
            {
                _displayWidth = currentFeature->DisplayWidth();
            }

            _renderWidth = static_cast<uint32_t>(_displayWidth / 3.0f);
            _mipmapUpscalerQuality = 0;
            _mipmapUpscalerRatio = 3.0f;
            _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
        }

        if (ImGui::Begin(Translation::Get("Mipmap Bias"), nullptr, flags))
        {
            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
                ImGui::SetWindowFocus();

            if (ImGui::InputScalar(Translation::Get("Display Width"), ImGuiDataType_U32, &_displayWidth, NULL, NULL, "%u"))
            {
                if (_displayWidth <= 0)
                {
                    if (config->OutputScalingEnabled.value_or_default())
                    {
                        _displayWidth = static_cast<uint32_t>(currentFeature->DisplayWidth() *
                                                              config->OutputScalingMultiplier.value_or_default());
                    }
                    else
                    {
                        _displayWidth = currentFeature->DisplayWidth();
                    }
                }

                _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
            }

            const char* q[] = { Translation::Get("Ultra Performance"), Translation::Get("Performance"), Translation::Get("Balanced"), Translation::Get("Quality"), Translation::Get("Ultra Quality"), Translation::Get("DLAA") };
            float fr[] = { 3.0f, 2.0f, 1.7f, 1.5f, 1.3f, 1.0f };
            auto configQ = _mipmapUpscalerQuality;

            const char* selectedQ = q[configQ];

            ImGui::BeginDisabled(config->UpscaleRatioOverrideEnabled.value_or_default());

            if (ImGui::BeginCombo(Translation::Get("Upscaler Quality"), selectedQ))
            {
                for (int n = 0; n < 6; n++)
                {
                    if (ImGui::Selectable(q[n], (_mipmapUpscalerQuality == n)))
                    {
                        _mipmapUpscalerQuality = n;

                        float ov = -1.0f;

                        if (config->QualityRatioOverrideEnabled.value_or_default())
                        {
                            switch (n)
                            {
                            case 0:
                                ov = config->QualityRatio_UltraPerformance.value_or(-1.0f);
                                break;

                            case 1:
                                ov = config->QualityRatio_Performance.value_or(-1.0f);
                                break;

                            case 2:
                                ov = config->QualityRatio_Balanced.value_or(-1.0f);
                                break;

                            case 3:
                                ov = config->QualityRatio_Quality.value_or(-1.0f);
                                break;

                            case 4:
                                ov = config->QualityRatio_UltraQuality.value_or(-1.0f);
                                break;
                            }
                        }

                        if (ov > 0.0f)
                            _mipmapUpscalerRatio = ov;
                        else
                            _mipmapUpscalerRatio = fr[n];

                        _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                        _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::EndDisabled();

            auto minLimit = config->ExtendedLimits.value_or_default() ? 0.1f : 1.0f;
            auto maxLimit = config->ExtendedLimits.value_or_default() ? 6.0f : 3.0f;
            if (ImGui::SliderFloat(Translation::Get("Upscaler Ratio"), &_mipmapUpscalerRatio, minLimit, maxLimit, "%.2f"))
            {
                _renderWidth = static_cast<uint32_t>(_displayWidth / _mipmapUpscalerRatio);
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);
            }

            if (ImGui::InputScalar(Translation::Get("Render Width"), ImGuiDataType_U32, &_renderWidth, NULL, NULL, "%u"))
                _mipBiasCalculated = log2((float) _renderWidth / (float) _displayWidth);

            ImGui::SliderFloat(Translation::Get("Mipmap Bias"), &_mipBiasCalculated, -15.0f, 0.0f, "%.6f");

            // BOTTOM LINE
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SameLine();
            ImGui::Spacing();

            constexpr float spacing = 6.0f;
            auto textSize = ImGui::CalcTextSize("Use Value");
            textSize += ImGui::CalcTextSize("Close");
            textSize.x += ImGui::GetStyle().FramePadding.x * 5.0f + spacing; // 2 sides * 2 buttons + 1

            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textSize.x);

            if (ImGui::Button(Translation::Get("Use Value")))
            {
                _mipBias = _mipBiasCalculated;
                _showMipmapCalcWindow = false;
            }

            ImGui::SameLine(0.0f, spacing);

            if (ImGui::Button(Translation::Get("Close")))
                _showMipmapCalcWindow = false;

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::End();
        }
    }
}

void MenuCommon::RenderHudlessResourcesWindow(RenderMenuContext& ctx, ImGuiWindowFlags flags)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& io = ctx.io;

    auto fg = state.currentFG;
    if (_showHudlessWindow && config->FGHUDFix.value_or_default() && fg != nullptr && fg->IsActive())
    {
        auto posX = (io.DisplaySize.x - 400.0f) / 2.0f;
        auto posY = (io.DisplaySize.y - 300.0f) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2 { posX, posY }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2 { 400.0f, 300.0f });

        if (ImGui::Begin(Translation::Get("Hudless Resources"), nullptr, flags))
        {
            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
                ImGui::SetWindowFocus();

            int btnCount = 100;

            if (ImGui::BeginTable("HudlessTable", 2, ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("##1", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##2", ImGuiTableColumnFlags_WidthFixed);

                ankerl::unordered_dense::map<void*, CapturedHudlessInfo>::iterator it;

                for (it = state.capturedHudlesses.begin(); it != state.capturedHudlesses.end(); it++)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);

                    ImGui::Text(Translation::Get("%08x, %s->%s, Count: %llu, %s"), (size_t) it->first,
                                GetSourceString(it->second.captureInfo & 0xFF).c_str(),
                                GetDispatchString(it->second.captureInfo & 0xFF00).c_str(), it->second.usageCount,
                                it->second.enabled ? Translation::Get("Active") : Translation::Get("Passive"));

                    ImGui::TableSetColumnIndex(1);

                    btnCount++;
                    std::string text;

                    if (it->second.enabled)
                        text = StrFmt(Translation::Get("Disable##%d"), btnCount);
                    else
                        text = StrFmt(Translation::Get("Enable##%d"), btnCount);

                    if (ImGui::Button(text.c_str()))
                    {
                        LOG_DEBUG("Hudless {:X}: {}", (size_t) it->first,
                                  it->second.enabled ? "Disabling" : "Enabling");
                        it->second.enabled = !it->second.enabled;
                    }
                }

                ImGui::EndTable();
            }

            if (ImGui::Button(Translation::Get("Clear##4")))
            {
                LOG_DEBUG("Clearing captured hudless resources");
                state.clearCapturedHudlesses = true;
            }

            ImGui::SameLine(0.0f, 8.0f);

            if (ImGui::Button(Translation::Get("Close##4")))
                _showHudlessWindow = false;

            ImGui::End();
        }
    }
}

void MenuCommon::RenderMainMenuWindow(RenderMenuContext& ctx)
{
    auto& state = ctx.state;
    auto config = ctx.config;
    auto& frameTime = ctx.frameTime;
    auto& frameRate = ctx.frameRate;
    auto& frameTimesCalculated = ctx.frameTimesCalculated;
    auto& menuResScale = ctx.menuResScale;

    if (!_isVisible)
        return;

    // Check for GPU support once and reuse the result in all menu sections.
    // DXVK might call Vulkan device creation, which would destroy our objects.
    State::Instance().vulkanSkipHooks = true;
    ctx.primaryGpu =
        std::make_unique<std::decay_t<decltype(IdentifyGpu::getPrimaryGpu())>>(IdentifyGpu::getPrimaryGpu());
    State::Instance().vulkanSkipHooks = false;

    // Overlay font
    if (config->UseHQFont.value_or_default())
        ImGui::PushFontSize(std::round(menuResScale * fontSize));

    // If overlay is not visible frame needs to be inited
    if (!frameTimesCalculated)
    {
        float frameCnt = 0;
        frameTime = 0;
        for (size_t i = 299; i > 199; i--)
        {
            if (state.frameTimes[i] > 0.0)
            {
                frameTime += state.frameTimes[i];
                frameCnt++;
            }
        }

        frameTime /= frameCnt;
        frameRate = 1000.0 / frameTime;
    }

    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoSavedSettings;
    flags |= ImGuiWindowFlags_NoCollapse;

    if (lastMenuScale != menuResScale)
    {
        lastMenuScale = menuResScale;

        // ApplyThemeStyle resets to unscaled defaults and applies the current
        // scale itself, so it fully rebuilds the style here.
        ApplyThemeStyle();

        // Always, not FirstUseEver: the latter only applies on the very first frame,
        // so the window kept its old size whenever the scale changed afterwards.
        ImGui::SetNextWindowSize(ImVec2(760.0f * menuResScale, 540.0f * menuResScale), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f * menuResScale, 240.0f * menuResScale),
                                            ImVec2(FLT_MAX, FLT_MAX));
    }

    // Main menu window
    if (windowTitle.empty())
    {
        windowTitle = StrFmt("%s - %s %s %s %s", VER_PRODUCT_NAME, state.gameExe.c_str(),
                             state.gameName.empty() ? "" : StrFmt("- %s", state.gameName.c_str()).c_str(),
                             (state.detectedQuirks.size() > 0) ? "(Q)" : "", state.isOptiPatcherSucceed ? "(OP)" : "");
    }

    if (ImGui::Begin(windowTitle.c_str(), NULL, flags))
    {
        // Window drop shadow
        {
            auto* dl = ImGui::GetWindowDrawList();
            ImVec2 wp = ImGui::GetWindowPos();
            ImVec2 ws = ImGui::GetWindowSize();
            float sr = 5.0f * menuResScale;
            float so = 3.0f * menuResScale;
            for (int i = 3; i >= 0; i--)
                dl->AddRectFilled({ wp.x + so * (i + 1), wp.y + so * (i + 1) },
                                  { wp.x + ws.x + so * (i + 1), wp.y + ws.y + so * (i + 1) },
                                  IM_COL32(0, 0, 0, (int)(12 * (4 - i))), sr, ImDrawFlags_RoundCornersAll);
        }

        RenderMainMenuHeaderMessages(ctx);

        // === STATUS BAR (full window width, top) ===
        float statusPad = 14.0f * menuResScale;
        float statusPlotH = 42.0f * menuResScale;
        float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
        float statusH = 138.0f * menuResScale;
        bool hasFeature = (ctx.currentFeature != nullptr && !ctx.currentFeature->IsFrozen());

        float statusBarW = 0.0f;
        if (hasFeature)
        {
            statusBarW = ImGui::GetContentRegionAvail().x;
            float reserveH = statusH + rowSpacing;
            ImVec2 topPos = ImGui::GetCursorPos();

            // Reserve layout space so sidebar/content begin below the status bar.
            ImGui::Dummy(ImVec2(0.0f, reserveH));
            ImVec2 bottomPos = ImGui::GetCursorPos();

            // Move cursor back to the reserved rectangle and draw status bar contents.
            ImGui::SetCursorPos(topPos);

            auto accentCol = toneMapColor(ImVec4(
                ctx.config->MenuAccentColorR.value_or_default(),
                ctx.config->MenuAccentColorG.value_or_default(),
                ctx.config->MenuAccentColorB.value_or_default(), 1.f));

            auto* dl = ImGui::GetWindowDrawList();
            ImVec2 cp = ImGui::GetCursorScreenPos();
            ImU32 cardBg = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.045f));
            ImU32 statusBorder = ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.28f));
            dl->AddRectFilled(cp, ImVec2(cp.x + statusBarW, cp.y + statusH), cardBg, 8.0f);
            dl->AddRect(cp, ImVec2(cp.x + statusBarW, cp.y + statusH), statusBorder, 8.0f, ImDrawFlags_RoundCornersAll, 1.0f);

            ImGui::SetCursorPosY(topPos.y + statusPad);
            ImGui::SetCursorPosX(statusPad);

            // "Status" pill badge with accent background
            {
                const char* badgeText = Translation::Get("Status");
                ImVec2 textSize = ImGui::CalcTextSize(badgeText);
                ImVec2 pad = ImVec2(10.0f, 4.0f);
                ImVec2 badgeMin = ImGui::GetCursorScreenPos();
                ImVec2 badgeMax = ImVec2(badgeMin.x + textSize.x + pad.x * 2.0f, badgeMin.y + textSize.y + pad.y * 2.0f);
                dl->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(ImVec4(accentCol.x, accentCol.y, accentCol.z, 0.90f)), 6.0f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad.x);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + pad.y);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", badgeText);
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::Dummy(ImVec2(pad.x, 0.0f));
                ImGui::SameLine(0.0f, 14.0f);
            }

            ImGui::Text("%dx%d -> %dx%d (%.1f)  |  %dx%d (%.1f)",
                       ctx.currentFeature->RenderWidth(), ctx.currentFeature->RenderHeight(),
                       ctx.currentFeature->TargetWidth(), ctx.currentFeature->TargetHeight(),
                       (float)ctx.currentFeature->TargetWidth() / (float)ctx.currentFeature->RenderWidth(),
                       ctx.currentFeature->DisplayWidth(), ctx.currentFeature->DisplayHeight(),
                       (float)ctx.currentFeature->DisplayWidth() / (float)ctx.currentFeature->RenderWidth());

            float cw = ImGui::GetContentRegionAvail().x - statusPad * 2.0f + 6.0f;
            float plotW = (cw - 14.0f) * 0.5f;

            // Frame plot card
            {
                ImGui::SetCursorPosX(statusPad);
                ImVec2 plotCardMin = ImGui::GetCursorScreenPos();
                ImVec2 plotCardMax = ImVec2(plotCardMin.x + plotW, plotCardMin.y + statusPlotH + 8.0f);
                dl->AddRectFilled(plotCardMin, plotCardMax, ImGui::GetColorU32(ImGuiCol_WindowBg, 0.5f), 8.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                auto ft = StrFmt("Frame  %7.2f ms / %6.1f fps", ctx.frameTime, ctx.frameRate);
                ImGui::PlotLines("##ft", [](void* rb, int idx) -> float
                    { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gFrameTimes, plotWidth, 0, ft.c_str(), 0.0f, FLT_MAX,
                    ImVec2(plotW, statusPlotH));
                ImGui::PopStyleColor();
            }

            ImGui::SameLine(0.0f, 14.0f);

            // Upscale plot card
            {
                ImVec2 plotCardMin = ImGui::GetCursorScreenPos();
                ImVec2 plotCardMax = ImVec2(plotCardMin.x + plotW, plotCardMin.y + statusPlotH + 8.0f);
                dl->AddRectFilled(plotCardMin, plotCardMax, ImGui::GetColorU32(ImGuiCol_WindowBg, 0.5f), 8.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                auto usLabel = StrFmt("Upscale  %7.2f ms", ctx.state.upscaleTimes.back());
                ImGui::PlotLines("##ups", [](void* rb, int idx) -> float
                    { return static_cast<RingBuffer<float, plotWidth>*>(rb)->At(idx); },
                    &gUpscalerTimes, plotWidth, 0, usLabel.c_str(), 0.0f, FLT_MAX,
                    ImVec2(plotW, statusPlotH));
                ImGui::PopStyleColor();
            }

            // Ensure subsequent widgets start below the reserved status bar area.
            ImGui::SetCursorPos(bottomPos);
        }

        // Sidebar + content (footer controls in sidebar, settings in scrollable body)
        RenderMainMenuTable(ctx);

        ImGui::End();

        // Center window on first show
        auto winSize = ImGui::GetWindowSize();
        auto winPos = ImGui::GetWindowPos();
        if (lastPosition.x < -900.0f || (lastPosition.x >= winPos.x - 1.0f && lastPosition.y >= winPos.y - 1.0f &&
                                         lastPosition.x <= winPos.x + 1.0f && lastPosition.y <= winPos.y + 1.0f))
        {
            float posX = ((float)ctx.io.DisplaySize.x - winSize.x) / 2.0f;
            float posY = ((float)ctx.io.DisplaySize.y - winSize.y) / 2.0f;
            if (posX < 0.0f || posY < 0.0f) { posX = 50.0f; posY = 50.0f; }
            ImGui::SetWindowPos(ImVec2 { posX, posY });
            lastPosition.x = posX;
            lastPosition.y = posY;
        }
    }

    // Detached utility windows owned by the main menu.
    RenderMipmapBiasWindow(ctx, flags);
    RenderHudlessResourcesWindow(ctx, flags);

    if (config->UseHQFont.value_or_default())
        ImGui::PopFontSize();
}

void KeyUp(UINT vKey)
{
    inputMenu = vKey == Config::Instance()->ShortcutKey.value_or_default();
    inputFps = vKey == Config::Instance()->FpsShortcutKey.value_or_default();
    inputFG = vKey == Config::Instance()->FGShortcutKey.value_or_default();
    inputFpsCycle = vKey == Config::Instance()->FpsCycleShortcutKey.value_or_default();
}

bool MenuCommon::RenderMenu()
{
    if (!_isInited)
        return false;

    RenderMenuContext ctx { State::Instance(), Config::Instance(), ImGui::GetIO() };
    ctx.now = Util::MillisecondsNow();
    ctx.currentFeature = ctx.state.currentFeature;

    // 1) Collect timing and input state before any ImGui drawing.
    UpdateRenderTiming(ctx);
    UpdateMenuInputMode(ctx);
    HandleMenuShortcuts(ctx);

    // 2) Prepare one-shot notifications and start a new ImGui frame only when needed.
    UpdateVersionAndStartupNotifications(ctx);
    BeginMenuFrameIfNeeded(ctx);
    OptiInput::EndFrame(_isVisible);

    // 3) Draw lightweight overlay windows first, preserving the original order.
    ctx.menuResScale = MenuResolutionScale(ctx.io);
    RenderSplashWindow(ctx);
    RenderNotifications(ctx);
    UpdateFrameTimeAverages(ctx);
    RenderPerformanceOverlay(ctx);
    RenderCursor(ctx);

    // 4) Draw the full settings menu last so popups and child windows keep their existing behavior.
    RenderMainMenuWindow(ctx);

    if (ctx.newFrame)
        ImGui::EndFrame();

    return ctx.newFrame;
}

void MenuCommon::Init(HWND InHwnd, bool isUWP)
{
    Translation::Init();
    auto config = Config::Instance();
    Translation::SetLanguage(static_cast<Language>(config->Language.value_or_default()));

    // Reset shutdown flag in case of re-init
    State::Instance().isShuttingDown = false;

    HWND oldHandle = nullptr;

    if (_handle != nullptr)
    {
        oldHandle = _handle;
        LOG_DEBUG("Old Handle: {:X}, ImGui Handle: {:X}", (size_t) oldHandle,
                  (size_t) ImGui::GetMainViewport()->PlatformHandleRaw);
    }

    _handle = InHwnd;
    _isVisible = false;
    _isUWP = isUWP;
    lastPosition = { -1000.0f, -1000.0f };

    LOG_DEBUG("Handle: {0:X}", (size_t) _handle);

    // In case d3d12 wasn't yet used up to this point, try to update GPU info late here
    IdentifyGpu::updateD3d12Capabilities();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    (void) io;

    hasGamepad = (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    io.ConfigFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NoKeyboard;

    io.MouseDrawCursor = false;
    io.WantCaptureKeyboard = _isVisible;
    io.WantCaptureMouse = _isVisible;
    io.WantSetMousePos = _isVisible;

    io.IniFilename = io.LogFilename = nullptr;

    bool initResult = false;

    if (io.BackendPlatformUserData == nullptr)
    {
        if (!isUWP)
        {
            initResult = ImGui_ImplWin32_Init(InHwnd);
            LOG_DEBUG("ImGui_ImplWin32_Init result: {0}", initResult);
        }
        else
        {
            initResult = ImGui_ImplUwp_Init(InHwnd);
            ImGui_BindUwpKeyUp(KeyUp);
            LOG_DEBUG("ImGui_ImplUwp_Init result: {0}", initResult);
        }
    }

    if (io.Fonts->Fonts.empty() && Config::Instance()->UseHQFont.value_or_default())
    {
        ImFontAtlas* atlas = io.Fonts;
        atlas->Clear();

        // This automatically becomes the next default font
        ImFontConfig fontConfig;

        if (Config::Instance()->FontSize.has_value())
            fontSize = Config::Instance()->FontSize.value();

        if (Config::Instance()->TTFFontPath.has_value())
        {
            io.FontDefault =
                atlas->AddFontFromFileTTF(wstring_to_string(Config::Instance()->TTFFontPath.value()).c_str(), fontSize,
                                          &fontConfig, io.Fonts->GetGlyphRangesDefault());
        }
        else
        {
            io.FontDefault = atlas->AddFontFromMemoryCompressedBase85TTF(hack_compressed_compressed_data_base85,
                                                                         fontSize, &fontConfig);
        }

        // Try to load system CJK font for Chinese character support
        // If available, merge CJK glyphs into the same font
        const char* cjkFontPaths[] = {
            "C:\\Windows\\Fonts\\msyh.ttc",
            "C:\\Windows\\Fonts\\simsun.ttc",
            "C:\\Windows\\Fonts\\deng.ttf",
        };
        for (const char* cjkPath : cjkFontPaths)
        {
            FILE* f = nullptr;
            if (fopen_s(&f, cjkPath, "rb") == 0 && f != nullptr)
            {
                fclose(f);
                ImFontConfig mergeConfig;
                mergeConfig.MergeMode = true;
                mergeConfig.GlyphRanges = GetCJKGlyphRanges();
                atlas->AddFontFromFileTTF(cjkPath, fontSize, &mergeConfig, GetCJKGlyphRanges());
                break;
            }
        }
    }

    if (!Config::Instance()->OverlayMenu.value_or_default())
    {
        _hdrTonemapApplied = false;
    }

    DWORD hwndPid = 0;
    DWORD hwndTid = GetWindowThreadProcessId(_handle, &hwndPid);

    LOG_DEBUG("HWND: {:X}, IsWindow: {}, HWND PID: {}, Current PID: {}, HWND TID: {}, Current TID: {}",
              (ULONG64) _handle, IsWindow(_handle), hwndPid, GetCurrentProcessId(), hwndTid, GetCurrentThreadId());

    OptiInput::Initialize(_handle, isUWP);

    ApplyThemeStyle();

    // Install low-level keyboard hook to bypass game input interception
    if (_llKeyboardHook == nullptr)
        _llKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);

    _isInited = true;
}

void MenuCommon::Shutdown()
{
    if (!MenuCommon::_isInited)
        return;

    // if (_oWndProc != nullptr)
    //{
    //     auto handle = (HWND) ImGui::GetMainViewport()->PlatformHandleRaw;
    //     SetLastError(0);
    //     auto restoreResult = SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR) _oWndProc);
    //     auto error = GetLastError();

    //    if (restoreResult == 0 && error != 0)
    //    {
    //        LOG_ERROR("Failed to restore old WndProc. Error: {:X}", error);
    //    }

    //    _oWndProc = nullptr;
    //}

    if (!_isUWP)
        ImGui_ImplWin32_Shutdown();
    else
        ImGui_ImplUwp_Shutdown();

    ImGui::DestroyContext();

    // Remove low-level keyboard hook
    if (_llKeyboardHook != nullptr)
    {
        UnhookWindowsHookEx(_llKeyboardHook);
        _llKeyboardHook = nullptr;
    }

    _handle = nullptr;
    _isInited = false;
    _isVisible = false;
}

void MenuCommon::HideMenu()
{
    if (!_isVisible)
        return;

    _isVisible = false;

    ImGuiIO& io = ImGui::GetIO();
    (void) io;

    _showMipmapCalcWindow = false;
    _showHudlessWindow = false;

    io.MouseDrawCursor = false;
    io.WantCaptureKeyboard = _isVisible;
    io.WantCaptureMouse = _isVisible;
}
