#pragma once
#include "Console.h"

// Module banks of the console. Each bank lays its controls out at fixed
// positions in body coordinates (the editor canvas is a fixed 1200 x 780 base
// scaled as a whole), matching design/mock/grano-ui.html.

namespace Banks
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // Base: owns created child components.
    class OwningBank : public Console::Bank
    {
    public:
        using Console::Bank::Bank;
    protected:
        template <class T, class... Args>
        T& add(juce::Component& parent, juce::Rectangle<int> bounds, Args&&... args)
        {
            auto c = std::make_unique<T>(std::forward<Args>(args)...);
            auto& ref = *c;
            parent.addAndMakeVisible(ref);
            ref.setBounds(bounds);
            owned_.push_back(std::move(c));
            return ref;
        }
        juce::Component& seam(juce::Component& parent, juce::Rectangle<int> bounds);
    private:
        std::vector<std::unique_ptr<juce::Component>> owned_;
    };

    class GrainBank    : public OwningBank { public: explicit GrainBank(APVTS&); };
    class EngineBank   : public OwningBank
    {
    public:
        explicit EngineBank(APVTS&);
    private:
        std::unique_ptr<juce::ParameterAttachment> modeAttach_;
    };
    class EnvelopeBank : public OwningBank { public: explicit EnvelopeBank(APVTS&); };
    class MotionBank   : public OwningBank { public: explicit MotionBank(APVTS&); };
    class ColorBank    : public OwningBank { public: explicit ColorBank(APVTS&); };

    class PatternBank : public OwningBank
    {
    public:
        explicit PatternBank(APVTS&);
    private:
        void layoutForMode();
        APVTS& apvts_;
        juce::Component *divLabel_{}, *div_{}, *scaleLabel_{}, *scale_{}, *ring_{}, *sens_{};
        Console::Silk* info_{};
        std::array<juce::Component*, 3> euclid_{};
        std::unique_ptr<juce::ParameterAttachment> modeAttach_, divAttach_;
    };

    class LfoBank : public OwningBank { public: LfoBank(APVTS&, int lfoNumber); };

    class MatrixBank : public OwningBank
    {
    public:
        explicit MatrixBank(APVTS&);
        void refresh();   // dim rows without source or destination
    private:
        class Row;
        std::vector<Row*> rows_;
    };
}
