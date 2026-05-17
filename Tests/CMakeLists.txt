include(FetchContent)

# Catch2 v3 — downloaded at configure time, cached by CMake
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# Test executable — compile plugin sources directly so we get GranoAudioProcessor
# symbols without depending on JUCE's internal plugin shared-code target.
add_executable(GranoTests
    test_main.cpp
    test_engine.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    # F1 engine sources compiled directly (same pattern as plugin sources)
    ${CMAKE_SOURCE_DIR}/Source/Engine/EnvelopeShapes.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/GrainPool.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/GranularEngine.cpp
)

target_include_directories(GranoTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(GranoTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
)

target_compile_definitions(GranoTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_USE_MP3AUDIOFORMAT=1
        # Suppress JUCE's "JucePlugin_*" macros warning in test context
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
)

# Register tests with CTest via Catch2's CMake integration
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(GranoTests)

# Propagate TSAN flags to tests when the sanitizer option is enabled
if(GRANO_ENABLE_TSAN AND NOT WIN32)
    target_compile_options(GranoTests PRIVATE -fsanitize=thread)
    target_link_options(GranoTests PRIVATE -fsanitize=thread)
endif()
