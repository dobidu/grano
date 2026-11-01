#include "GranoTheme.h"
#include "BinaryData.h"

namespace
{
    juce::Typeface::Ptr load(const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor(data, (size_t)size);
    }

    juce::Typeface::Ptr typefaceFor(Theme::Face face)
    {
        // Message thread only; loaded once per process.
        static const juce::Typeface::Ptr silk     = load(BinaryData::ArchivoCondSemiBold_ttf,   BinaryData::ArchivoCondSemiBold_ttfSize);
        static const juce::Typeface::Ptr silkBold = load(BinaryData::ArchivoCondBold_ttf,       BinaryData::ArchivoCondBold_ttfSize);
        static const juce::Typeface::Ptr text     = load(BinaryData::ArchivoSemiCondMedium_ttf, BinaryData::ArchivoSemiCondMedium_ttfSize);
        static const juce::Typeface::Ptr wordmark = load(BinaryData::ArchivoExpExtraBold_ttf,   BinaryData::ArchivoExpExtraBold_ttfSize);
        static const juce::Typeface::Ptr lcd      = load(BinaryData::MartianMonoCondMedium_ttf, BinaryData::MartianMonoCondMedium_ttfSize);

        switch (face)
        {
            case Theme::Face::silk:     return silk;
            case Theme::Face::silkBold: return silkBold;
            case Theme::Face::text:     return text;
            case Theme::Face::wordmark: return wordmark;
            case Theme::Face::lcd:      return lcd;
        }
        return silk;
    }
}

juce::Font Theme::font(Face face, float pointHeight, float tracking)
{
    return juce::Font(juce::FontOptions{ typefaceFor(face) }
                          .withPointHeight(pointHeight)
                          .withKerningFactor(tracking));
}
