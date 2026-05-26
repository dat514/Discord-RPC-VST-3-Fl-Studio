#include "DawUiStateDetector.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <algorithm>
#include <vector>
#endif

namespace
{
#if JUCE_WINDOWS
    struct WindowInfo
    {
        HWND hwnd = nullptr;
        juce::String title;
        juce::String className;
        int priority = 100;
    };

    struct WindowCollector
    {
        std::vector<WindowInfo> windows;
        int maxWindows = 300;
    };

    juce::String readWindowText(HWND hwnd)
    {
        wchar_t buffer[512] {};
        const auto length = GetWindowTextW(hwnd, buffer, static_cast<int>(std::size(buffer)));

        if (length <= 0)
            return {};

        return juce::String(buffer).substring(0, length).trim();
    }

    juce::String readClassName(HWND hwnd)
    {
        wchar_t buffer[256] {};
        const auto length = GetClassNameW(hwnd, buffer, static_cast<int>(std::size(buffer)));

        if (length <= 0)
            return {};

        return juce::String(buffer).substring(0, length).trim();
    }

    bool belongsToCurrentProcess(HWND hwnd)
    {
        if (hwnd == nullptr || ! IsWindow(hwnd))
            return false;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        return pid == GetCurrentProcessId();
    }

    bool alreadyHasWindow(const std::vector<WindowInfo>& windows, HWND hwnd)
    {
        for (const auto& window : windows)
            if (window.hwnd == hwnd)
                return true;

        return false;
    }

    void addWindow(WindowCollector& collector, HWND hwnd, int priority)
    {
        if (static_cast<int>(collector.windows.size()) >= collector.maxWindows
            || hwnd == nullptr
            || ! belongsToCurrentProcess(hwnd)
            || alreadyHasWindow(collector.windows, hwnd))
        {
            return;
        }

        WindowInfo info;
        info.hwnd = hwnd;
        info.title = readWindowText(hwnd);
        info.className = readClassName(hwnd);
        info.priority = priority;

        if (info.title.isNotEmpty() || info.className.isNotEmpty())
            collector.windows.push_back(std::move(info));
    }

    void addWindowAndParents(WindowCollector& collector, HWND hwnd, int priority)
    {
        for (auto* current = hwnd; current != nullptr; current = GetParent(current))
        {
            addWindow(collector, current, priority);

            if (GetParent(current) == nullptr)
                break;

            ++priority;
        }
    }

    BOOL CALLBACK enumChildProc(HWND hwnd, LPARAM userData)
    {
        auto& collector = *reinterpret_cast<WindowCollector*>(userData);

        if (static_cast<int>(collector.windows.size()) >= collector.maxWindows)
            return FALSE;

        if (IsWindowVisible(hwnd))
            addWindow(collector, hwnd, 80);

        return TRUE;
    }

    bool containsIgnoreCase(const juce::String& text, const juce::String& needle)
    {
        return text.indexOfIgnoreCase(needle) >= 0;
    }

    juce::String mergedText(const WindowInfo& info)
    {
        return (info.title + " " + info.className).trim();
    }

    juce::String surfaceFromText(const juce::String& text)
    {
        if (containsIgnoreCase(text, "piano roll"))
            return "Piano roll";

        if (containsIgnoreCase(text, "playlist"))
            return "Playlist";

        if (containsIgnoreCase(text, "mixer"))
            return "Mixer";

        if (containsIgnoreCase(text, "channel rack") || containsIgnoreCase(text, "step sequencer"))
            return "Channel rack";

        if (containsIgnoreCase(text, "browser"))
            return "Browser";

        if (containsIgnoreCase(text, "plugin picker"))
            return "Plugin picker";

        return {};
    }

    juce::String parseMixerInsert(const juce::String& text)
    {
        if (containsIgnoreCase(text, "master") && containsIgnoreCase(text, "mixer"))
            return "Master";

        const auto lower = text.toLowerCase();

        auto parseNumberAfter = [&lower] (const juce::String& token) -> juce::String
        {
            auto index = lower.indexOf(token);

            while (index >= 0)
            {
                auto cursor = index + token.length();

                while (cursor < lower.length() && ! juce::CharacterFunctions::isDigit(lower[cursor]))
                    ++cursor;

                juce::String number;

                while (cursor < lower.length() && juce::CharacterFunctions::isDigit(lower[cursor]))
                    number += lower[cursor++];

                if (number.isNotEmpty())
                    return token.substring(0, 1).toUpperCase() + token.substring(1) + " " + number;

                index = lower.indexOf(cursor, token);
            }

            return {};
        };

        auto insert = parseNumberAfter("insert");
        if (insert.isNotEmpty())
            return insert;

        auto track = parseNumberAfter("track");
        if (track.isNotEmpty())
            return track;

        return {};
    }

    juce::String parseProjectName(juce::String title)
    {
        title = title.trim();

        const auto flpIndex = title.indexOfIgnoreCase(".flp");
        if (flpIndex > 0)
        {
            auto project = title.substring(0, flpIndex).trim();
            project = project.fromLastOccurrenceOf("\\", false, false)
                             .fromLastOccurrenceOf("/", false, false)
                             .trim();

            return project;
        }

        const auto flStudioIndex = title.indexOfIgnoreCase(" - FL Studio");
        if (flStudioIndex > 0)
        {
            const auto project = title.substring(0, flStudioIndex).trim();

            if (project.isNotEmpty()
                && ! project.equalsIgnoreCase("FL Studio")
                && ! project.equalsIgnoreCase("Untitled"))
            {
                return project;
            }
        }

        return {};
    }

    bool isHostOrPanelTitle(const juce::String& title)
    {
        static const char* blocked[] =
        {
            "FL Studio",
            "Playlist",
            "Piano roll",
            "Channel rack",
            "Step sequencer",
            "Mixer",
            "Browser",
            "Plugin picker",
            "Toolbar",
            "Hint"
        };

        for (const auto* token : blocked)
            if (containsIgnoreCase(title, token))
                return true;

        return false;
    }

    juce::String cleanPluginTitle(juce::String title)
    {
        title = title.trim();

        const auto dash = title.indexOf(" - ");
        if (dash > 0)
        {
            const auto tail = title.substring(dash + 3);
            if (containsIgnoreCase(tail, "FL Studio")
                || containsIgnoreCase(tail, "wrapper")
                || containsIgnoreCase(tail, "insert")
                || containsIgnoreCase(tail, "track"))
            {
                title = title.substring(0, dash).trim();
            }
        }

        return title;
    }

    bool looksLikePluginWindow(const WindowInfo& info)
    {
        const auto title = cleanPluginTitle(info.title);

        if (title.length() < 2 || isHostOrPanelTitle(title))
            return false;

        if (containsIgnoreCase(info.className, "tooltips"))
            return false;

        if (info.priority <= 20)
            return true;

        const auto all = mergedText(info);
        return containsIgnoreCase(all, "wrapper")
            || containsIgnoreCase(all, "vst")
            || containsIgnoreCase(all, "plugin");
    }

    DawUiStateDetector::Snapshot classifyWindows(const std::vector<WindowInfo>& windows)
    {
        DawUiStateDetector::Snapshot result;
        result.hostActive = true;

        for (const auto& info : windows)
        {
            if (result.rawWindowTitle.isEmpty() && info.title.isNotEmpty())
                result.rawWindowTitle = info.title;

            if (result.projectName.isEmpty())
                result.projectName = parseProjectName(info.title);

            if (result.surface.isEmpty())
                result.surface = surfaceFromText(mergedText(info));

            if (result.mixerInsert.isEmpty())
                result.mixerInsert = parseMixerInsert(info.title);

            if (result.focusedPlugin.isEmpty() && looksLikePluginWindow(info))
                result.focusedPlugin = cleanPluginTitle(info.title);
        }

        if (result.surface.isEmpty() && result.focusedPlugin.isNotEmpty())
            result.surface = "Plugin editor";

        return result;
    }

    std::vector<WindowInfo> collectWindowsFor(HWND foregroundWindow, HWND cursorWindow)
    {
        WindowCollector collector;

        DWORD foregroundPid = 0;
        const auto foregroundThread = GetWindowThreadProcessId(foregroundWindow, &foregroundPid);

        GUITHREADINFO guiInfo {};
        guiInfo.cbSize = sizeof(guiInfo);

        if (foregroundThread != 0 && GetGUIThreadInfo(foregroundThread, &guiInfo))
        {
            addWindowAndParents(collector, guiInfo.hwndFocus, 0);
            addWindowAndParents(collector, guiInfo.hwndActive, 3);
            addWindowAndParents(collector, guiInfo.hwndCapture, 8);
        }

        addWindowAndParents(collector, cursorWindow, 12);
        addWindowAndParents(collector, foregroundWindow, 20);

        if (foregroundWindow != nullptr)
            EnumChildWindows(foregroundWindow, enumChildProc, reinterpret_cast<LPARAM>(&collector));

        std::stable_sort(collector.windows.begin(), collector.windows.end(),
                         [] (const WindowInfo& a, const WindowInfo& b)
                         {
                             return a.priority < b.priority;
                         });

        return collector.windows;
    }
#endif
}

DawUiStateDetector::Snapshot DawUiStateDetector::scan()
{
#if JUCE_WINDOWS
    auto* foregroundWindow = GetForegroundWindow();

    if (! belongsToCurrentProcess(foregroundWindow))
        return {};

    POINT cursorPosition {};
    HWND cursorWindow = nullptr;

    if (GetCursorPos(&cursorPosition))
        cursorWindow = WindowFromPoint(cursorPosition);

    auto windows = collectWindowsFor(foregroundWindow, cursorWindow);
    auto result = classifyWindows(windows);

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x0001) != 0 && belongsToCurrentProcess(cursorWindow))
    {
        WindowCollector clickedCollector;
        addWindowAndParents(clickedCollector, cursorWindow, 0);
        lastClickedSnapshot = classifyWindows(clickedCollector.windows);
        lastClickMs = juce::Time::getMillisecondCounter();
        hasLastClickedSnapshot = true;
    }

    if (hasLastClickedSnapshot && juce::Time::getMillisecondCounter() - lastClickMs < 10 * 60 * 1000)
    {
        if (lastClickedSnapshot.surface.isNotEmpty())
            result.surface = lastClickedSnapshot.surface;

        if (lastClickedSnapshot.projectName.isNotEmpty())
            result.projectName = lastClickedSnapshot.projectName;

        result.focusedPlugin = lastClickedSnapshot.focusedPlugin;

        if (lastClickedSnapshot.mixerInsert.isNotEmpty())
            result.mixerInsert = lastClickedSnapshot.mixerInsert;
    }

    return result;
#else
    return {};
#endif
}
