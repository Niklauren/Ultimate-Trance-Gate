#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Presets.h"

namespace tg
{
//==============================================================================
// Per-user settings shared by every instance: favourite presets.
// macOS: ~/Library/Application Support/Ultimate Trance Gate/settings.xml
class Settings
{
public:
    Settings()
    {
        juce::PropertiesFile::Options o;
        o.applicationName     = "settings";
        o.filenameSuffix      = ".xml";
        o.folderName          = "Ultimate Trance Gate";
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat       = juce::PropertiesFile::storeAsXML;
        props = std::make_unique<juce::PropertiesFile> (o);
        favourites = juce::StringArray::fromTokens (props->getValue ("favourites"), "\n", "");
        favourites.removeEmptyStrings();
        if (! props->containsKey ("favourites"))
            favourites = { "Standard|Rage Chop", "Rage|Rage Chop", "Eurodance|Rhythm Stab 3-3-2", "Creative|Bouncing Ball" };
    }

    static juce::String key (int libraryIndex)
    {
        const auto& p = presetLibrary()[(size_t) libraryIndex];
        return p.category + "|" + p.name;
    }

    bool isFavourite (int libraryIndex) const { return favourites.contains (key (libraryIndex)); }

    void toggleFavourite (int libraryIndex)
    {
        const auto k = key (libraryIndex);
        if (favourites.contains (k)) favourites.removeString (k);
        else favourites.add (k);
        props->setValue ("favourites", favourites.joinIntoString ("\n"));
        props->saveIfNeeded();
    }

    juce::Array<int> favouriteIndices() const
    {
        juce::Array<int> out;
        for (auto& f : favourites)
        {
            const int idx = presetIndexByName (f.upToFirstOccurrenceOf ("|", false, false),
                                               f.fromFirstOccurrenceOf ("|", false, false));
            if (idx >= 0) out.add (idx);
        }
        return out;
    }

private:
    std::unique_ptr<juce::PropertiesFile> props;
    juce::StringArray favourites;
};

//==============================================================================
// Pattern -> standard MIDI file (one note per hit, C3 = 60, velocity = step level).
// The loop is repeated until it fills whole bars (max 4 bars).
inline juce::File writePatternMidi (const PatternData& p, float gate, const juce::String& name)
{
    constexpr int ppq = 960;
    const double stepBeats = resolutions()[(size_t) p.resolution].beats;
    const double loopBeats = stepBeats * p.length;
    int reps = 1;
    while (reps < 32 && std::fmod (loopBeats * reps, 4.0) > 1.0e-6 && loopBeats * reps < 16.0) ++reps;

    juce::MidiMessageSequence seq;
    for (int rep = 0; rep < reps; ++rep)
        for (int i = 0; i < p.length; ++i)
        {
            const auto& st = p.steps[(size_t) i];
            if (! p.isOn (i)) continue;
            if (i > 0 && p.steps[(size_t) i - 1].tie && p.isOn (i - 1)) continue;   // tied-in: already covered
            const int r = juce::jmax (1, st.ratchet);
            const double start = (rep * p.length + i) * stepBeats;
            const double sub = stepBeats / r;
            for (int k = 0; k < r; ++k)
            {
                double len = sub * gate;
                if (k == r - 1 && st.tie)
                {
                    int j = i;   // extend over the tied steps
                    while (j < p.length - 1 && p.steps[(size_t) j].tie && p.isOn (j + 1)) ++j;
                    len = (j - i + 1) * stepBeats - k * sub;
                }
                const auto vel = (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (st.level * 127.0f));
                const double t0 = (start + k * sub) * ppq;
                seq.addEvent (juce::MidiMessage::noteOn (1, 60, vel), t0);
                seq.addEvent (juce::MidiMessage::noteOff (1, 60), t0 + juce::jmax (1.0, len * ppq - 1.0));
            }
        }
    seq.updateMatchedPairs();
    seq.addEvent (juce::MidiMessage::endOfTrack(), loopBeats * reps * ppq);

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);

    auto safeName = juce::File::createLegalFileName (name.isEmpty() ? juce::String ("Gate Pattern") : name);
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("Ultimate Trance Gate").getChildFile ("UTG " + safeName + ".mid");
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    if (auto os = file.createOutputStream())
        mf.writeTo (*os);
    return file;
}

} // namespace tg
