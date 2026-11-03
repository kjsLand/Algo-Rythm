#pragma once

#include "algo/Types.h"

#include <juce_core/juce_core.h>

#include <string>
#include <vector>

// The favourites library in ~/Algo-Rythm/library/ (T14-04) and the ratings file ~/Algo-Rythm/ratings.csv (T14-05).
class Library
{
public:
    struct Entry
    {
        juce::File   file;
        juce::String key;
        int          bpm { 0 };
        juce::Time   date;
        int          rating { 0 }; // 0 = no rating
    };

    // Makes the folders if they do not exist.
    explicit Library (juce::File root = defaultRoot ());

    static juce::File defaultRoot ();

    juce::File folder () const
    {
        return m_root.getChildFile ("library");
    }
    juce::File ratingsFile () const
    {
        return m_root.getChildFile ("ratings.csv");
    }

    // Writes the song starter as JSON with its default file name. Returns the file.
    // Throws std::runtime_error if the file cannot be written.
    juce::File save (const algo::SongStarter& starter) const;

    // The saved files, the newest first. A file that does not load is not in the list.
    std::vector<Entry> list () const;

    // Throws std::runtime_error if the file does not load.
    algo::SongStarter load (const juce::File& file) const;

    // Moves the file to the system trash. Returns false if that is not possible.
    bool remove (const juce::File& file) const;

    // Adds one line to ratings.csv. If the file does not exist, writes the header line first.
    void appendRating (const algo::SongStarter& starter, const std::string& tables) const;

private:
    juce::File m_root;
};
