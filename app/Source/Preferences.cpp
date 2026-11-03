#include "Preferences.h"

#include "algo/Json.h"

namespace
{
    juce::PropertiesFile::Options options ()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "Algo-Rythm";
        o.filenameSuffix  = ".settings";
#if JUCE_LINUX || JUCE_BSD
        o.folderName = ".config/Algo-Rythm"; // Not ~/Algo-Rythm, which holds the library
#else
        o.folderName = "Algo-Rythm";
#endif
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat       = juce::PropertiesFile::storeAsXML;
        return o;
    }
}

Preferences::Preferences ()
    : m_file (options ())
{
}

algo::GenerationSettings Preferences::settings () const
{
    const auto text = m_file.getValue ("settings");
    if (text.isEmpty ())
        return {};
    try
    {
        auto settings = algo::Json::parse (text.toStdString ()).get<algo::GenerationSettings> ();
        if (algo::validate (settings).empty ())
            return settings;
    }
    catch (const std::exception&)
    {
    }
    return {};
}

void Preferences::setSettings (const algo::GenerationSettings& settings)
{
    auto copy = settings;
    copy.previous.reset ();
    copy.rerolls = {};
    m_file.setValue ("settings", juce::String (algo::Json (copy).dump ()));
}

juce::String Preferences::windowBounds () const
{
    return m_file.getValue ("windowBounds");
}

void Preferences::setWindowBounds (const juce::String& bounds)
{
    m_file.setValue ("windowBounds", bounds);
}

bool Preferences::clickOn () const
{
    return m_file.getBoolValue ("click", true);
}

void Preferences::setClickOn (bool on)
{
    m_file.setValue ("click", on);
}

juce::File Preferences::lastExportFolder () const
{
    const auto path = m_file.getValue ("lastExportFolder");
    if (path.isNotEmpty () && juce::File::isAbsolutePath (path))
        return juce::File (path);
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
}

void Preferences::setLastExportFolder (const juce::File& folder)
{
    m_file.setValue ("lastExportFolder", folder.getFullPathName ());
}

std::unique_ptr<juce::XmlElement> Preferences::audioState () const
{
    return m_file.getXmlValue ("audioState");
}

void Preferences::setAudioState (const juce::XmlElement* xml)
{
    m_file.setValue ("audioState", xml);
}

bool Preferences::abTestMode () const
{
    return m_file.getBoolValue ("abTest", false);
}

void Preferences::setAbTestMode (bool on)
{
    m_file.setValue ("abTest", on);
}

bool Preferences::leftHanded () const
{
    return m_file.getBoolValue ("leftHanded", false);
}

void Preferences::setLeftHanded (bool on)
{
    m_file.setValue ("leftHanded", on);
}

void Preferences::save ()
{
    m_file.saveIfNeeded ();
}
