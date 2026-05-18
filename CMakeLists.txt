cmake_minimum_required(VERSION 3.22)
project(Grano VERSION 0.1.0 LANGUAGES CXX C)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── JUCE location ─────────────────────────────────────────────────────────────
# Default to ~/JUCE (Linux/macOS) or %USERPROFILE%/JUCE (Windows).
# Override with -DJUCE_DIR=/path/to/JUCE at configure time.
if(WIN32)
    set(_default_juce_dir "$ENV{USERPROFILE}/JUCE")
else()
    set(_default_juce_dir "$ENV{HOME}/JUCE")
endif()
set(JUCE_DIR "${_default_juce_dir}" CACHE PATH "Path to JUCE 8 source clone")

add_subdirectory(${JUCE_DIR} JUCE-build EXCLUDE_FROM_ALL)

# ── Options ───────────────────────────────────────────────────────────────────
option(GRANO_BUILD_TESTS  "Build Catch2 test executable"           ON)
option(GRANO_ENABLE_TSAN  "Enable ThreadSanitizer (Linux/macOS)"   OFF)
option(GRANO_ENABLE_ASAN  "Enable AddressSanitizer"                OFF)
option(GRANO_USE_MP3      "Enable JUCE MP3 format support"         ON)

# ── Plugin target ─────────────────────────────────────────────────────────────
# AU is an Apple-only format; gate it to prevent CMake errors on other platforms.
if(APPLE)
    set(_formats VST3 AU Standalone)
else()
    set(_formats VST3 Standalone)
endif()

juce_add_plugin(Grano
    PRODUCT_NAME            "Grano"
    COMPANY_NAME            "Bidu"
    BUNDLE_ID               "br.com.bidu.grano"
    PLUGIN_MANUFACTURER_CODE "Bidu"
    PLUGIN_CODE             "Gran"
    FORMATS                 ${_formats}
    IS_SYNTH                FALSE        # sample-based effect instrument; revisit for MIDI triggering
    NEEDS_MIDI_INPUT        TRUE         # required for MIDI learn in F5; harmless for now
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES         "Instrument" "Sampler"
    AU_MAIN_TYPE            "kAudioUnitType_MusicDevice"
)

# ── Source files ──────────────────────────────────────────────────────────────
# Explicit listing — no globbing — so generated IDE projects stay honest.
target_sources(Grano
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        # F1 — Engine data structures and granular engine
        Source/Engine/EnvelopeShapes.cpp
        Source/Engine/GrainPool.cpp
        Source/Engine/GranularEngine.cpp
        # F2 — Sample I/O
        Source/Engine/SampleBuffer.cpp
        # F2 — UI
        Source/UI/WaveformDisplay.cpp
        # F3 — UI components and LookAndFeel
        Source/UI/LookAndFeel/GranoLAF.cpp
        Source/UI/Knob.cpp
        Source/UI/Slider.cpp
        # F3 — Parameters
        Source/Parameters.cpp
        # F4 — Processing modules
        Source/Modules/Motion.cpp
)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(Grano
    PUBLIC
        JUCE_WEB_BROWSER=0          # no embedded browser; reduces binary size
        JUCE_USE_CURL=0             # no cURL dependency
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_MP3AUDIOFORMAT=$<BOOL:${GRANO_USE_MP3}>
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(Grano
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Sanitizers ────────────────────────────────────────────────────────────────
if(GRANO_ENABLE_TSAN AND NOT WIN32)
    target_compile_options(Grano PRIVATE -fsanitize=thread)
    target_link_options(Grano PRIVATE -fsanitize=thread)
endif()

if(GRANO_ENABLE_ASAN)
    target_compile_options(Grano PRIVATE -fsanitize=address)
    target_link_options(Grano PRIVATE -fsanitize=address)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(GRANO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
