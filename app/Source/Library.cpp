#include "Library.h"

#include "algo/Files.h"
#include "algo/Pitch.h"
#include "algo/Ratings.h"

#include <algorithm>
#include <stdexcept>

Library::Library (juce::File root)
    : m_root (std::move (root))
{
    folder ().createDirectory ();
}

juce::File Library::defaultRoot ()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Algo-Rythm");
}

juce::File Library::save (const algo::SongStarter& starter) const
{
    const auto file = folder ().getChildFile (juce::String (algo::defaultFileName (starter)) + ".json");
    algo::saveStarter (starter, file.getFullPathName ().toStdString ());
    return file;
}

std::vector<Library::Entry> Library::list () const
{
    std::vector<Entry> entries;
    for (const auto& file : folder ().findChildFiles (juce::File::findFiles, false, "*.json"))
    {
        try
        {
            const auto starter = load (file);
            Entry      entry;
            entry.file   = file;
            entry.key    = juce::String (algo::pitchClassName (starter.key.root, starter.key) + " " + starter.key.scale);
            entry.bpm    = starter.bpm;
            entry.date   = file.getLastModificationTime ();
            entry.rating = starter.rating.value_or (0);
            entries.push_back (entry);
        }
        catch (const std::exception&)
        {
        }
    }
    std::sort (entries.begin (), entries.end (), [] (const Entry& a, const Entry& b)
    {
        return a.date > b.date;
    });
    return entries;
}

algo::SongStarter Library::load (const juce::File& file) const
{
    return algo::loadStarter (file.getFullPathName ().toStdString ());
}

bool Library::remove (const juce::File& file) const
{
    return file.moveToTrash ();
}

void Library::appendRating (const algo::SongStarter& starter, const std::string& tables) const
{
    const auto   file = ratingsFile ();
    juce::String text;
    if (!file.existsAsFile ())
        text << algo::kRatingsHeader << "\n";

    const auto date = juce::Time::getCurrentTime ().formatted ("%Y-%m-%d");
    text << juce::String (algo::ratingCsvLine (starter, date.toStdString (), tables)) << "\n";
    if (!file.appendText (text, false, false, "\n"))
        throw std::runtime_error ("ratings.csv cannot be written");
}
