
cmake_minimum_required (VERSION 3.18) 

project (Imogen VERSION 0.0.1 LANGUAGES CXX) 

set (Imogen_sourceDir ${CMAKE_CURRENT_LIST_DIR}/../Source)  # This could conceivably be in a different place, but the source tree must be intact 

set (bv_juceGitRepoToUse https://github.com/elk-audio/JUCE.git)
set (bv_juceGitRepo_TagToUse mind/headless_plugin_client_v5.2)

set (IMOGEN_BUILD_FOR_ELK TRUE)

include (${Imogen_sourceDir}/imogenConfig.cmake)

#

set (Imogen_sourceFiles
    ${Imogen_Common_SourceFiles}
    ${Imogen_Processor_SourceFiles})
#

set (bv_formats Standalone VST3)

#

set (thingsToLinkTo "")

juce_add_module ("${Imogen_sourceDir}/DSP_modules/bv_Harmonizer")
list (APPEND thingsToLinkTo bv_Harmonizer)

juce_add_module ("${Imogen_sourceDir}/DSP_modules/bv_ImogenEngine")
list (APPEND thingsToLinkTo bv_ImogenEngine)

juce_add_module ("${Imogen_sourceDir}/Shared-code/bv_SharedCode")
list (APPEND thingsToLinkTo "bv_SharedCode")

juce_add_module ("${Imogen_sourceDir}/DSP_modules/SynthBase/bv_SynthBase")
list (APPEND thingsToLinkTo "bv_SynthBase")

#

juce_add_plugin (ImogenHeadless
    PRODUCT_NAME                ImogenHeadless
    PLUGIN_CODE                 Imnh
    VERSION                     ${CMAKE_PROJECT_VERSION}
    BUNDLE_ID                   com.BenViningMusicSoftware.Imogen
    PLUGIN_NAME                 ImogenHeadless
    PLUGIN_MANUFACTURER_CODE    Benv
    DESCRIPTION                 "Real-time vocal harmonizer instrument"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    FORMATS                     ${bv_formats}
    VST3_CATEGORIES             "Pitch Shift"
    AU_MAIN_TYPE                "kAudioUnitType_MusicEffect"
    ICON_BIG                    ${ImogenIconPath}
    COMPANY_NAME                BenViningMusicSoftware
    COMPANY_WEBSITE             www.benvining.com
    COMPANY_EMAIL               ben.the.vining@gmail.com
    COPY_PLUGIN_AFTER_BUILD     FALSE
    COMPANY_COPYRIGHT           "This software is provided as-is, with no guarantee of completion or fitness for any particular purpose, by Ben Vining, under the terms and conditions of the GNU Public License."
    MICROPHONE_PERMISSION_ENABLED        TRUE
    MICROPHONE_PERMISSION_TEXT           "Imogen requires audio input to be able to produce its output. Please enable the microphone, or you won't hear anything when you press the keys."
    SEND_APPLE_EVENTS_PERMISSION_ENABLED FALSE
    DOCUMENT_BROWSER_ENABLED             FALSE
    VST_NUM_MIDI_INS 1
    VST_NUM_MIDI_OUTS 1
    DOCUMENT_EXTENSIONS xml
    )

#

target_sources (ImogenHeadless PRIVATE ${Imogen_sourceFiles})

source_group (TREE ${Imogen_sourceDir} PREFIX "" FILES ${Imogen_sourceFiles})

target_include_directories (ImogenHeadless PRIVATE ${Imogen_sourceDir})

set_target_properties (ImogenHeadless PROPERTIES FOLDER "")  # prevets IDE from generating an extra nested folder

#

list (APPEND bv_formats "All")

foreach (target ${bv_formats}) 
    set (thisTargetName "Imogen_${target}")  # this is how JUCE automatically names the build targets created for each format

    if (NOT TARGET ${thisTargetName})
        continue()
    endif()
	
	message (STATUS "Configuring ${thisTargetName}...")
    set_target_properties (${thisTargetName} PROPERTIES FOLDER "Build Targets" XCODE_GENERATE_SCHEME ON)
endforeach()

#

target_link_libraries (ImogenHeadless PRIVATE ${thingsToLinkTo})

#

target_include_directories (ImogenHeadless PUBLIC "third-party/MTS-ESP/Client" "MTS-ESP")
target_compile_definitions (ImogenHeadless PUBLIC bvsb_USE_MTS_ESP=1)

#

imogenConfig_step2()

#

target_compile_definitions (ImogenHeadless PUBLIC 
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_HEADLESS_PLUGIN_CLIENT=1
    IMOGEN_REMOTE_APP=0
    IMOGEN_HEADLESS=1
    IMOGEN_USE_ABLETON_LINK=0
    IMOGEN_USE_OSC=0
    bvsb_USE_MTS_ESP=0
    )

#
