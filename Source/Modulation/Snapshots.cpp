#include "Snapshots.h"

void Snapshots::save(int slot, const juce::ValueTree& state) noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    slots_[i] = state.createCopy();
}

void Snapshots::recall(int slot, juce::AudioProcessorValueTreeState& apvts) const noexcept
{
    const int i = juce::jlimit(0, kNumSlots - 1, slot);
    if (slots_[i].isValid())
        apvts.replaceState(slots_[i].createCopy());
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
