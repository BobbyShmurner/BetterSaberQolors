#pragma once

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "modloader/shared/modloader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"

// Including these for the fucntion declerations below
#include "HMUI/InputFieldView.hpp"
#include "HMUI/InputFieldViewStaticAnimations.hpp"

#include "GlobalNamespace/RGBPanelController.hpp"

#include "SliderChangeColor.hpp"

// Define these functions here so that we can easily read configuration and log information from other files
Configuration& getConfig();
Logger& getLogger();

// Defining these fucntions here as there can be issues when making hooks
void InitSliderInputs(GlobalNamespace::RGBPanelController* rgbPanelController);
void RefreshTextValues(GlobalNamespace::RGBPanelController* rgbPanelController);
void UpdateSliderColor(SliderChangeColor sliderColor, GlobalNamespace::RGBPanelController* rgbPanelController, std::string_view stringViewValue);
void UpdateBGStates();