#pragma once

#include "Data.h"
#include "Types.h"

#include <string>

namespace algo
{
    // Bars on one line of tab.
    constexpr int kTabBarsPerLine = 4;

    // The tab of one bar: 6 lines, with the high E string at the top. Each slot is one column.
    // A fret number with 2 digits makes all columns of the bar 2 characters wide. A Mute is "x".
    std::string tabBar (const Bar& bar, const StrumPattern& strum, const Tuning& tuning = Tuning {});

    // Returns text such as "x32010", from the low E string to the high E string. A fret above 9 is in brackets.
    std::string chordDiagram (const Voicing& voicing);

    // The whole song starter as ASCII tab. Each section has a header line with the section name, the BPM
    // and the key, a chord chart line, a strum line, the tab lines and a chord diagram for each new chord.
    std::string writeTab (const SongStarter& starter, const DataSet& data = bundledData ());
}
