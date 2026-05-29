#include "MainComponent.h"
#include "Fretboard.h"

MainComponent::MainComponent()
{
    setSize (600, 400);
}

MainComponent::~MainComponent() {}

void MainComponent::paint (juce::Graphics& g)
{
    GuitarFretboard test = GuitarFretboard();
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello, World!", getLocalBounds(), juce::Justification::centred, true);
    test.printScale("G", "Major", 12);
}

void MainComponent::resized() {}