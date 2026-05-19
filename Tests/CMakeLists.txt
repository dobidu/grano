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
    test_sample_io.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/PluginEditor.cpp
    # F1 engine sources compiled directly (same pattern as plugin sources)
    ${CMAKE_SOURCE_DIR}/Source/Engine/EnvelopeShapes.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/GrainPool.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/GranularEngine.cpp
    # F2 — Sample I/O
    ${CMAKE_SOURCE_DIR}/Source/Engine/SampleBuffer.cpp
    # F6b — Sub-grain + Stochastic timing
    ${CMAKE_SOURCE_DIR}/Source/Engine/SubGrain.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/StochasticTiming.cpp
    # F6c — Feedback path + Spectral processor
    ${CMAKE_SOURCE_DIR}/Source/Engine/FeedbackPath.cpp
    ${CMAKE_SOURCE_DIR}/Source/Engine/SpectralProcessor.cpp
    # F2 — UI
    ${CMAKE_SOURCE_DIR}/Source/UI/WaveformDisplay.cpp
    # F3 — UI components and LookAndFeel
    ${CMAKE_SOURCE_DIR}/Source/UI/LookAndFeel/GranoLAF.cpp
    ${CMAKE_SOURCE_DIR}/Source/UI/Knob.cpp
    ${CMAKE_SOURCE_DIR}/Source/UI/Slider.cpp
    # F3 — Parameters
    ${CMAKE_SOURCE_DIR}/Source/Parameters.cpp
    test_parameters.cpp
    # F4 — Processing modules
    ${CMAKE_SOURCE_DIR}/Source/Modules/Motion.cpp
    test_motion.cpp
    ${CMAKE_SOURCE_DIR}/Source/Modules/Color.cpp
    test_color.cpp
    ${CMAKE_SOURCE_DIR}/Source/Modules/Pattern.cpp
    test_pattern.cpp
    # F5 — Modulation
    ${CMAKE_SOURCE_DIR}/Source/Modulation/Lfo.cpp
    ${CMAKE_SOURCE_DIR}/Source/Modulation/ModMatrix.cpp
    ${CMAKE_SOURCE_DIR}/Source/Modulation/Snapshots.cpp
    # F5 — UI panels (needed because PluginEditor.cpp is compiled into GranoTests)
    ${CMAKE_SOURCE_DIR}/Source/UI/LfoPanel.cpp
    ${CMAKE_SOURCE_DIR}/Source/UI/ModulationMatrixView.cpp
    test_modulation.cpp
    test_subgrain.cpp
    test_stochastic.cpp
    test_feedback.cpp
    test_spectral.cpp
)

target_include_directories(GranoTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(GranoTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_formats
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
