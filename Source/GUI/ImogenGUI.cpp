
/*===================================================================================================================================================
           _             _   _                _                _                 _               _
          /\ \          /\_\/\_\ _           /\ \             /\ \              /\ \            /\ \     _
          \ \ \        / / / / //\_\        /  \ \           /  \ \            /  \ \          /  \ \   /\_\
          /\ \_\      /\ \/ \ \/ / /       / /\ \ \         / /\ \_\          / /\ \ \        / /\ \ \_/ / /
         / /\/_/     /  \____\__/ /       / / /\ \ \       / / /\/_/         / / /\ \_\      / / /\ \___/ /
        / / /       / /\/________/       / / /  \ \_\     / / / ______      / /_/_ \/_/     / / /  \/____/
       / / /       / / /\/_// / /       / / /   / / /    / / / /\_____\    / /____/\       / / /    / / /
      / / /       / / /    / / /       / / /   / / /    / / /  \/____ /   / /\____\/      / / /    / / /
  ___/ / /__     / / /    / / /       / / /___/ / /    / / /_____/ / /   / / /______     / / /    / / /
 /\__\/_/___\    \/_/    / / /       / / /____\/ /    / / /______\/ /   / / /_______\   / / /    / / /
 \/_________/            \/_/        \/_________/     \/___________/    \/__________/   \/_/     \/_/
 
 
 This file is part of the Imogen codebase.
 
 @2021 by Ben Vining. All rights reserved.
 
 ImogenGUI.cpp: This file defines implementation details for Imogen's top-level GUI component.

===================================================================================================================================================*/


#include "bv_SharedCode/BinaryDataHelpers.h"

#include "Holders/ImogenGuiHolder.h"


ImogenGUI::ImogenGUI(): tooltipWindow(this, msBeforeTooltip)
{
    createParameters();
    jassert (parameters.size() == numParams);
           
    this->setBufferedToImage (true);
    
    makePresetMenu (selectPreset);
    // selectPreset.onChange = [this] { holder->sendLoadPreset (selectPreset.getText()); };
    
    //addAndMakeVisible(selectPreset);
    addAndMakeVisible (mainDial);
    
    setSize (940, 435);
  
#if JUCE_MAC
    darkMode.store (juce::Desktop::isOSXDarkModeActive());
#else
    darkMode.store (true);
#endif
    
    mainDial.showPitchCorrection();
    
    rescanPresetsFolder();
}


ImogenGUI::~ImogenGUI()
{
    this->setLookAndFeel (nullptr);
}


/*=========================================================================================================
 =========================================================================================================*/

void ImogenGUI::rescanPresetsFolder()
{
//    availablePresets.clearQuick();
//    const auto xtn = getPresetFileExtension();
//
//    for (auto entry  :   juce::RangedDirectoryIterator (getPresetsFolder(), false))
//    {
//        const auto filename = entry.getFile().getFileName();
//
//        if (filename.endsWith (xtn))
//            availablePresets.add (filename.dropLastCharacters (xtn.length()));
//    }
    
    // show spinning/progress graphic...?
    
    repaint();
}


void ImogenGUI::savePreset (const juce::String& presetName)
{
    const auto filename = bav::addFileExtensionIfMissing (presetName, getPresetFileExtension());

//    auto state = tree.copyState();
//
//    if (state.hasProperty ("editorSize"))
//        state.removeProperty ("editorSize", nullptr);
//
//    auto xml = state.createXml();
//
//    xml->setAttribute ("presetName", filename.dropLastCharacters (getPresetFileExtension().length()));
//
//    xml->writeTo (getPresetsFolder().getChildFile (filename));

    rescanPresetsFolder();
}


void ImogenGUI::loadPreset (const juce::String& presetName)
{
    if (presetName.isEmpty())
    {
        // display error message...
        return;
    }

    rescanPresetsFolder();

    const auto filename = bav::addFileExtensionIfMissing (presetName, getPresetFileExtension());
    const auto presetToLoad = presetsFolder().getChildFile (filename);

    if (! presetToLoad.existsAsFile())
    {
        // display error message...
        return;
    }

//    const auto result = isUsingDoublePrecision() ? updatePluginInternalState (*juce::parseXML (presetToLoad), doubleEngine, true)
//                                                 : updatePluginInternalState (*juce::parseXML (presetToLoad), floatEngine,  true);

//    if (! result)
//    {
//        // display error message...
//    }
}


void ImogenGUI::deletePreset (const juce::String& presetName)
{
    rescanPresetsFolder();
    
    auto presetToDelete = presetsFolder().getChildFile (bav::addFileExtensionIfMissing (presetName, getPresetFileExtension()));

    if (presetToDelete.existsAsFile())
    {
        if (! presetToDelete.moveToTrash())
            presetToDelete.deleteFile();

        rescanPresetsFolder();
    }
}


void ImogenGUI::renamePreset (const juce::String& previousName, const juce::String& newName)
{
    rescanPresetsFolder();
    
    const auto extension = getPresetFileExtension();

    const auto presetToLoad = presetsFolder().getChildFile (bav::addFileExtensionIfMissing (previousName, extension));

    if (! presetToLoad.existsAsFile())
    {
        // display error message...
        return;
    }

    auto xml = juce::parseXML (presetToLoad);

    if (! presetToLoad.moveToTrash())  // delete the old preset file
        presetToLoad.deleteFile();

    const auto name = bav::removeFileExtensionIfThere (newName, extension);

    xml->setAttribute ("presetName", name);
    xml->writeTo (presetsFolder().getChildFile (name + extension));

    rescanPresetsFolder();
}


/*=========================================================================================================
    juce::Component functions
 =========================================================================================================*/

void ImogenGUI::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    
    if (darkMode.load())
    {
        
    }
    else
    {
        
    }
}

void ImogenGUI::resized()
{
    //selectPreset.setBounds (x, y, w, h);
    //mainDial.setBounds (x, y, w, h);
}

bool ImogenGUI::keyPressed (const juce::KeyPress& key)
{
    juce::ignoreUnused (key);
    return false;
}

bool ImogenGUI::keyStateChanged (bool isKeyDown)
{
    juce::ignoreUnused (isKeyDown);
    return false;
}

void ImogenGUI::modifierKeysChanged (const juce::ModifierKeys& modifiers)
{
    juce::ignoreUnused (modifiers);
}

void ImogenGUI::focusLost (FocusChangeType cause)
{
    juce::ignoreUnused (cause);
}


/*=========================================================================================================
 =========================================================================================================*/

void ImogenGUI::setDarkMode (bool shouldUseDarkMode)
{
    darkMode.store (shouldUseDarkMode);
    // inform all child components of the change...
    this->repaint();
}

inline void ImogenGUI::makePresetMenu (juce::ComboBox& box)
{
    juce::ignoreUnused (box);
}

void ImogenGUI::createParameters()
{
    parameters.clear();
    parameters.reserve (numParams);
    
    for (int i = 0; i < numParams; ++i)
    {
        const auto id = ParameterID(i);
        parameters.emplace_back (std::make_unique<ImogenGUIParameter> (id, getParameterNameShort(id)));
    }
}

ImogenGUIParameter* ImogenGUI::getParameter (ParameterID paramID) const
{
    for (auto& parameter : parameters)
    {
        auto* param = parameter.get();
        if (param->getID() == paramID)
            return param;
    }
    
    return nullptr;
}
