#pragma once
#include <juce_core/juce_core.h>

// Session log for host-interaction diagnostics (state save/restore, slot
// loads, editor lifetime). Message thread only — never call from the audio
// thread. Active only inside a real plugin host or Standalone; unit tests
// (wrapper type Undefined) write nothing.
//
// File: <temp>/Grano/grano-debug.log, trimmed to the last 512 KB on open.
namespace GranoLog
{
    void write(const juce::String& message);

    // Short stable id for an object, e.g. "P3f2a" for a processor.
    juce::String id(const void* p, char prefix);
}
