#pragma once

#include "Data.h"
#include "Types.h"

#include <string>

namespace algo
{
    // Writes a MusicXML 4.0 score. Part 1 is the guitar with a notation staff and a 6-line tab staff in the
    // tuning of the song starter. Each note has <string> and <fret> elements and each chord has a <harmony>
    // element. If the song starter has a hook, part 2 has the hook.
    std::string writeMusicXml (const SongStarter& starter, const DataSet& data = bundledData ());
}
