#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// GranoTheme — the single source of design tokens for the editor.
//
// Visual world: a stage lighting desk. Powder-coat desk, raised bank plates
// with console tape labels, recessed wells for faders and the monitor,
// amber scribble strips for names and values. See DESIGN_SPEC.md.
//
// Every colour used by Source/UI and PluginEditor lives here; no other file
// may construct a juce::Colour from a literal.

namespace Theme
{
    // ── Canvas ────────────────────────────────────────────────────────────────
    inline constexpr int kBaseWidth  = 1200;
    inline constexpr int kBaseHeight = 780;
    inline constexpr float kMinScale = 0.75f;
    inline constexpr float kMaxScale = 1.6f;

    // ── Colours ───────────────────────────────────────────────────────────────
    namespace C
    {
        inline const juce::Colour desk     { 0xff17181bu }; // powder-coat desk
        inline const juce::Colour plate    { 0xff1d1f23u }; // bank plate, raised
        inline const juce::Colour well     { 0xff0b0c0du }; // recessed wells
        inline const juce::Colour line     { 0xff2c2f35u }; // panel seams
        inline const juce::Colour line2    { 0xff3a3e46u }; // hover / touched seams
        inline const juce::Colour silk     { 0xffcfcbc1u }; // silkscreen text
        inline const juce::Colour silkDim  { 0xff8c887fu }; // secondary silkscreen
        inline const juce::Colour lcdBg    { 0xff0e0d09u }; // scribble strip glass
        inline const juce::Colour lcd      { 0xffffb547u }; // scribble strip value
        inline const juce::Colour lcdName  { 0xffe9e2d0u }; // scribble strip name
        inline const juce::Colour lcdDim   { 0xffa38d61u }; // unlit strip text
        inline const juce::Colour tape     { 0xffece7dau }; // console tape
        inline const juce::Colour tapeInk  { 0xff18181au }; // marker on tape
        inline const juce::Colour cap      { 0xff5b5f67u }; // fader cap
        inline const juce::Colour capHi    { 0xff7a7f88u }; // fader cap, hover
        inline const juce::Colour key      { 0xff2a2d33u }; // unlit key
        inline const juce::Colour keyHover { 0xff2d3036u };
        inline const juce::Colour edge     { 0xff0a0a0bu }; // key / knob outline
        inline const juce::Colour knob     { 0xff33373eu }; // encoder body
        inline const juce::Colour groove   { 0xff000000u }; // fader slot
        inline const juce::Colour grooveHi { 0xff22252au }; // fader slot highlight
        inline const juce::Colour tick     { 0xff3a3d44u }; // fader scale ticks
        inline const juce::Colour grid     { 0xff17191cu }; // monitor grid
        inline const juce::Colour lfoAxis  { 0xff1b1d21u }; // LFO preview zero line
        inline const juce::Colour waveform { 0xff4c5058u }; // sample waveform
        inline const juce::Colour menu     { 0xff22252au }; // popup menu
        inline const juce::Colour disabled { 0xff5d6068u }; // disabled menu text
        inline const juce::Colour meterOff { 0xff1a1c20u }; // unlit meter segment
        inline const juce::Colour meterLo  { 0xff8fd08au };
        inline const juce::Colour meterMid { 0xffe8c55au };
        inline const juce::Colour clipOff  { 0xff3a1512u };
        inline const juce::Colour ringOff  { 0xff4a4e56u }; // Euclid empty step
        inline const juce::Colour midTick  { 0xff6a6e76u }; // bipolar centre mark
        inline const juce::Colour glint    { 0x08ffffffu }; // strip glass top edge
        inline const juce::Colour shadow   { 0x8c000000u }; // plate / cap drop shadow
        inline const juce::Colour live     { 0xffffffffu }; // live / now
        inline const juce::Colour clip     { 0xffff4438u }; // clipping only

        // Grain head colour ramps from warm amber (quiet) to hot orange (peak).
        inline const juce::Colour grainLo  { 0xffffcd6eu };
        inline const juce::Colour grainHi  { 0xffff963cu };

        // Module edge colours (console tape edge, fills, lit keys).
        inline const juce::Colour mGrain   { 0xfff5d547u };
        inline const juce::Colour mEngine  { 0xff9db0c4u };
        inline const juce::Colour mMotion  { 0xffff8a4cu };
        inline const juce::Colour mColor   { 0xffef6aa8u };
        inline const juce::Colour mPattern { 0xff55cc92u };
        inline const juce::Colour mMod     { 0xff4fb0f7u };
    }

    // ── Type ──────────────────────────────────────────────────────────────────
    // Archivo (OFL) cuts for silkscreen, Martian Mono (OFL) for values.
    // Sizes are CSS-like point heights at 1× scale; floor is 13.
    enum class Face
    {
        silk,      // Archivo condensed semibold — labels, strip names
        silkBold,  // Archivo condensed bold — tape, keys
        text,      // Archivo semi-condensed medium — hints, info copy
        wordmark,  // Archivo expanded extra-bold — GRANO
        lcd        // Martian Mono condensed medium — values
    };

    juce::Font font(Face face, float pointHeight, float tracking = 0.0f);

    // ── Metrics ───────────────────────────────────────────────────────────────
    inline constexpr float kPlateRadius = 4.0f;
    inline constexpr float kKeyRadius   = 3.0f;
    inline constexpr float kStripRadius = 2.0f;
    inline constexpr int   kTapeH       = 22;
    inline constexpr int   kBodyTop     = 28;   // bank body starts below the tape
    inline constexpr float kOffAlpha    = 0.38f; // bypassed module body
}
