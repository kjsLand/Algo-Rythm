#include "AppState.h"

#include "Library.h"

#include "AbTest.h"

#include "algo/Ratings.h"

#include <algorithm>

AppState::AppState ()
{
    m_rules               = std::make_shared<DataContext> ();
    m_rules->data         = algo::bundledData ();
    m_settings.masterSeed = static_cast<std::uint64_t> (m_random.nextInt (1000000000));
}

AppState::~AppState ()
{
    m_pool.removeAllJobs (true, 10000);
}

void AppState::settingsChanged ()
{
    sendChangeMessage ();
}

void AppState::select (int section, int bar)
{
    if (section == m_section && bar == m_bar)
        return;
    m_section = section;
    m_bar     = bar;
    sendChangeMessage ();
}

juce::File AppState::userFolder ()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Algo-Rythm");
}

std::shared_ptr<AppState::DataContext> AppState::contextFor (const std::string& tables)
{
    if (tables != "learned")
        return m_rules;

    if (m_learned == nullptr)
    {
        std::vector<std::string> warnings;
        auto                     context = std::make_shared<DataContext> ();
        try
        {
            context->data = algo::dataForTables (algo::bundledData (), "learned", userFolder ().getChildFile ("learned").getFullPathName ().toStdString (), &warnings);
        }
        catch (const std::exception& error)
        {
            warnings.push_back (std::string ("The learned tables are not valid: ") + error.what ());
            context->data = algo::bundledData ();
        }
        if (!warnings.empty () && !m_abTest)
            setStatus (juce::String (warnings.front ()));
        m_learned = context;
    }
    return m_learned;
}

const algo::DataSet& AppState::data () const
{
    return m_rules->data;
}

void AppState::setTables (const std::string& tables)
{
    m_settings.tables = tables;
    m_learned.reset (); // Load the files again, because the user can change them.
    contextFor (tables);
    sendChangeMessage ();
}

void AppState::setAbTestMode (bool on)
{
    m_abTest = on;
    sendChangeMessage ();
}

void AppState::setLeftHanded (bool on)
{
    m_leftHanded = on;
    sendChangeMessage ();
}

void AppState::setStatus (const juce::String& text)
{
    m_status = text;
    sendChangeMessage ();
}

bool AppState::sectionLock (int section, int item) const
{
    const auto& locks = m_settings.locks.sections;
    if (section < 0 || section >= static_cast<int> (locks.size ()))
        return false;
    const auto& lock = locks[static_cast<std::size_t> (section)];
    return item == 0 ? lock.chords : (item == 1 ? lock.voicings : lock.strum);
}

void AppState::setSectionLock (int section, int item, bool locked)
{
    if (section < 0)
        return;
    auto& locks = m_settings.locks.sections;
    if (static_cast<int> (locks.size ()) <= section)
        locks.resize (static_cast<std::size_t> (section) + 1);
    auto& lock                                                           = locks[static_cast<std::size_t> (section)];
    (item == 0 ? lock.chords : (item == 1 ? lock.voicings : lock.strum)) = locked;
    sendChangeMessage ();
}

void AppState::runJob (std::function<algo::SongStarter ()> job)
{
    const int id = ++m_latest;
    ++m_busy;
    setStatus ("Generating...");

    juce::WeakReference<AppState> weak (this);
    m_pool.addJob ([weak, id, job = std::move (job)]
    {
        std::optional<algo::SongStarter> result;
        juce::String                     error;
        try
        {
            result = job ();
        }
        catch (const std::exception& e)
        {
            error = e.what ();
        }

        juce::MessageManager::callAsync ([weak, id, result = std::move (result), error]
        {
            auto* self = weak.get ();
            if (self == nullptr)
                return;
            --self->m_busy;

            // A new request started before this job ended, so this result is old (T11-02).
            if (id != self->m_latest.load ())
                return;

            if (!result)
            {
                self->setStatus ("Error: " + error);
                return;
            }
            self->setStarter (*result);
            self->setStatus ("Seed " + juce::String (static_cast<juce::int64> (result->seed)) + ". Press G to generate again.");
        });
    });
}

void AppState::requestGenerate ()
{
    requestGenerate (static_cast<std::uint64_t> (m_random.nextInt (1000000000)));
}

void AppState::requestGenerate (std::uint64_t seed)
{
    auto settings       = m_settings;
    settings.masterSeed = seed;
    settings.previous   = m_starter;
    settings.rerolls    = {};
    if (m_abTest)
        settings.tables = AbTest::chooseTables (m_random);

    m_settings.masterSeed = seed;
    auto context          = contextFor (settings.tables);
    runJob ([settings, context]
    {
        return algo::generate (settings, context->data, nullptr, &context->cache);
    });
}

void AppState::requestReroll (algo::Stage stage, int section)
{
    if (m_starter == nullptr)
        return;
    auto starter = m_starter;
    auto context = contextFor (starter->tables);
    runJob ([starter, context, stage, section]
    {
        return algo::reroll (*starter, stage, section, context->data);
    });
}

void AppState::setStarter (algo::SongStarter starter)
{
    m_starter = std::make_shared<const algo::SongStarter> (std::move (starter));
    m_section = std::clamp (m_section, 0, std::max (0, static_cast<int> (m_starter->sections.size ()) - 1));
    m_bar     = 0;
    sendChangeMessage ();
}

void AppState::rate (int rating)
{
    if (m_starter == nullptr || rating < algo::kMinRating || rating > algo::kMaxRating)
        return;

    auto rated   = *m_starter;
    rated.rating = rating;
    m_starter    = std::make_shared<const algo::SongStarter> (rated);

    try
    {
        Library library;
        library.save (rated);
        library.appendRating (rated, m_abTest ? rated.tables : std::string ());
        setStatus ("Rated " + juce::String (rating) + " of 5 and saved to the library.");
    }
    catch (const std::exception& error)
    {
        setStatus ("Error: " + juce::String (error.what ()));
    }
}
