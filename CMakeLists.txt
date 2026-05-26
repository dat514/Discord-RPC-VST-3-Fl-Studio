cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

project(DiscordRichPresenceVST VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(JUCE)
if(NOT juce_POPULATED)
    FetchContent_Populate(JUCE)

    # JUCE 7 still contains this plugin-hosting constructor, but Clang/MinGW
    # cannot compile it. This plugin does not host other plugins, so the
    # constructor can be excluded for portable MinGW builds.
    set(JUCE_PLUGIN_INSTANCE_HEADER
        "${juce_SOURCE_DIR}/modules/juce_audio_processors/processors/juce_AudioPluginInstance.h")
    file(READ "${JUCE_PLUGIN_INSTANCE_HEADER}" JUCE_PLUGIN_INSTANCE_TEXT)
    string(FIND "${JUCE_PLUGIN_INSTANCE_TEXT}" "#if ! JUCE_MINGW" JUCE_MINGW_PATCH_FOUND)

    if(JUCE_MINGW_PATCH_FOUND EQUAL -1)
        string(REPLACE
            "    template <size_t numLayouts>\n    AudioPluginInstance (const short channelLayoutList[numLayouts][2]) : AudioProcessor (channelLayoutList) {}"
            "   #if ! JUCE_MINGW\n    template <size_t numLayouts>\n    AudioPluginInstance (const short channelLayoutList[numLayouts][2]) : AudioProcessor (channelLayoutList) {}\n   #endif"
            JUCE_PLUGIN_INSTANCE_TEXT
            "${JUCE_PLUGIN_INSTANCE_TEXT}")
        file(WRITE "${JUCE_PLUGIN_INSTANCE_HEADER}" "${JUCE_PLUGIN_INSTANCE_TEXT}")
    endif()

    add_subdirectory("${juce_SOURCE_DIR}" "${juce_BINARY_DIR}")
endif()

juce_add_plugin(DiscordRichPresence
    COMPANY_NAME "Local"
    BUNDLE_ID "com.local.discordrichpresence"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dprs
    PLUGIN_CODE Drp3
    FORMATS VST3
    PRODUCT_NAME "Discord Rich Presence"
)

juce_generate_juce_header(DiscordRichPresence)

target_sources(DiscordRichPresence
    PRIVATE
        Source/DawUiStateDetector.cpp
        Source/DawUiStateDetector.h
        Source/DiscordIpcClient.cpp
        Source/DiscordIpcClient.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

target_compile_definitions(DiscordRichPresence
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DIRECT2D=0
        JUCE_USE_ACCESSIBILITY=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        WIN32_LEAN_AND_MEAN
)

target_link_libraries(DiscordRichPresence
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MINGW)
    target_link_options(DiscordRichPresence_VST3 PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()
