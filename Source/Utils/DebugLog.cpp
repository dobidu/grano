#include "DebugLog.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
    juce::FileLogger* logger()
    {
        static std::unique_ptr<juce::FileLogger> instance = []() -> std::unique_ptr<juce::FileLogger>
        {
            if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Undefined)
                return nullptr;
            const auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("Grano").getChildFile("grano-debug.log");
            file.getParentDirectory().createDirectory();
            juce::String welcome;
            welcome << "---- Grano "
           #ifdef JucePlugin_VersionString
                    << JucePlugin_VersionString
           #endif
                    << " | host: " << juce::PluginHostType().getHostDescription()
                    << " | format: " << juce::AudioProcessor::getWrapperTypeDescription(
                                            juce::PluginHostType::getPluginLoadedAs())
                    << " | " << juce::SystemStats::getOperatingSystemName();
            return std::make_unique<juce::FileLogger>(file, welcome, 512 * 1024);
        }();
        return instance.get();
    }
}

void GranoLog::write(const juce::String& message)
{
    if (auto* l = logger())
        l->logMessage(juce::Time::getCurrentTime().formatted("%H:%M:%S") + "."
                      + juce::String(juce::Time::getCurrentTime().getMilliseconds()).paddedLeft('0', 3)
                      + "  " + message);
}

juce::String GranoLog::id(const void* p, char prefix)
{
    const auto v = (juce::uint64)(juce::pointer_sized_uint)p;
    return juce::String::charToString((juce::juce_wchar)prefix)
         + juce::String::toHexString((int)((v >> 4) & 0xffff)).paddedLeft('0', 4);
}
