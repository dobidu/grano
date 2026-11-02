#include "Snapshots.h"
#include "../Parameters.h"

namespace
{
// Instance settings, not sound: a recall keeps the current value.
constexpr const char* kNotRecalled[] = { ParamIDs::noSourceMode };

juce::ValueTree findParam(const juce::ValueTree& state, const char* id)
{
    return state.getChildWithProperty("id", juce::String(id));
}
} // namespace

void Snapshots::save(int slot, const juce::ValueTree& state) noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    slots_[i] = state.createCopy();
    live_ = i;
}

void Snapshots::clearSlot(int slot) noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    slots_[i] = {};
    if (live_ == i)
        live_ = -1;
}

void Snapshots::recall(int slot, juce::AudioProcessorValueTreeState& apvts) noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    if (slots_[i].isValid())
    {
        auto next = slots_[i].createCopy();
        const auto live = apvts.copyState(); // flushes pending parameter values
        for (const char* id : kNotRecalled)
        {
            const auto current = findParam(live, id);
            if (!current.isValid())
                continue;
            if (auto stored = findParam(next, id); stored.isValid())
                stored.copyPropertiesFrom(current, nullptr);
            else
                next.appendChild(current.createCopy(), nullptr);
        }
        apvts.replaceState(next);
        live_ = i;
    }
}

bool Snapshots::isOccupied(int slot) const noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    return slots_[i].isValid();
}

juce::ValueTree Snapshots::serialise() const
{
    juce::ValueTree root("Snapshots");
    for (int i = 0; i < kNumSlots; ++i)
    {
        if (!slots_[i].isValid())
            continue;
        const juce::Identifier slotID("Slot" + juce::String(i));
        juce::ValueTree slotTree(slotID);
        slotTree.addChild(slots_[i].createCopy(), -1, nullptr);
        root.addChild(slotTree, -1, nullptr);
    }
    return root;
}

void Snapshots::deserialise(const juce::ValueTree& data)
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        const juce::Identifier slotID("Slot" + juce::String(i));
        const juce::ValueTree slotTree = data.getChildWithName(slotID);
        if (slotTree.isValid() && slotTree.getNumChildren() > 0)
            slots_[i] = slotTree.getChild(0).createCopy();
    }
}
