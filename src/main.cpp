#include "main.hpp"

#include <iostream>
#include <string>
#include <stdlib.h>

#include "VRUIControls/VRGraphicRaycaster.hpp"

#include "custom-types/shared/coroutine.hpp"

#include "System/Collections/IEnumerator.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/ColorBlock.hpp"
#include "UnityEngine/UI/Selectable_SelectionState.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/AnimationClip.hpp"
#include "UnityEngine/AnimationCurve.hpp"
#include "UnityEngine/Keyframe.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/Screen.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/ColorGradientSlider.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"

#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/FontStyles.hpp"

#include "GlobalNamespace/ColorsOverrideSettingsPanelController.hpp"
#include "GlobalNamespace/ColorSchemesSettings.hpp"
#include "GlobalNamespace/EditColorSchemeController.hpp"
#include "GlobalNamespace/ColorSchemeColorsToggleGroup.hpp"
#include "GlobalNamespace/EditColorSchemeController.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"


static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

HMUI::InputFieldView* redInput;
HMUI::InputFieldView* greenInput;
HMUI::InputFieldView* blueInput;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// This is a VERY shity way to fix a weird highlighting bug
custom_types::Helpers::Coroutine UpdateBGNextFrame() {
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForEndOfFrame::New_ctor()));
    UpdateBGStates();

    co_return;
}

bool isNumber(const std::string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

bool isValidSaberColor(std::string strValue) {
    if (!strValue.empty() || isNumber(strValue)) {
        float value = atof(strValue.c_str());
        getLogger().info("Converted string to float");

        if (!(value < 0 || value > 255)) {
            return true;
        }
    }

    return false;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

void LogHierarchy(UnityEngine::Transform* trans, int level = 0) {
    if (level == 0) getLogger().info("Logging Hierarchy for %s:", to_utf8(csstrtostr(trans->get_name())).c_str());

    std::string start;
    for (int i = 0; i < level; i++) {
        start += "\t";
    }
    start += "- ";

    for (int i = 0; i < trans->get_childCount(); i++) {
        getLogger().info("%s[%i] %s", start.c_str(), i, to_utf8(csstrtostr(trans->GetChild(i)->get_name())).c_str());
        LogHierarchy(trans->GetChild(i), level + 1);
    }
}

void UpdateSliderColor(SliderChangeColor sliderColor, GlobalNamespace::RGBPanelController* rgbPanelController, std::string_view stringViewValue) {
    std::string stringValue = (std::string)stringViewValue;

    if (isValidSaberColor(stringValue)) {
        float value = atof(stringValue.c_str()) / 255;
        getLogger().info("Color Value is %.2f", value);

        UnityEngine::Color newCol;
        HMUI::ColorGradientSlider* slider;

        switch(sliderColor) {
            case(SliderChangeColor::Red):
                newCol = UnityEngine::Color(value, rgbPanelController->color.g, rgbPanelController->color.b, 1);
                slider = rgbPanelController->redSlider;

                break;

            case(SliderChangeColor::Green):
                newCol = UnityEngine::Color(rgbPanelController->color.r, value, rgbPanelController->color.b, 1);
                slider = rgbPanelController->greenSlider;

                break;

            case(SliderChangeColor::Blue):
                newCol = UnityEngine::Color(rgbPanelController->color.r, rgbPanelController->color.g, value, 1);
                slider = rgbPanelController->blueSlider;

                break;
        }

        rgbPanelController->set_color(newCol);
        rgbPanelController->HandleSliderColorDidChange(slider, newCol, GlobalNamespace::ColorChangeUIEventType::PointerUp);

        getLogger().info("Changed Color to %s!", stringValue.c_str());
        return;
    }

    getLogger().info("Color \"%s\" is invalid!", stringValue.c_str());
    RefreshTextValues(rgbPanelController); // This fixes the length of the text input
}

void SetBGColor(HMUI::InputFieldView* inputFieldView) {
    std::string nameStr = to_utf8(csstrtostr(inputFieldView->get_name()));
    float inputValue = atof(to_utf8(csstrtostr(inputFieldView->GetComponent<HMUI::InputFieldView*>()->text)).c_str()) / 255;

    UnityEngine::Color newCol;
    
    if (nameStr == "RedInputField") {
        newCol = {inputValue, 0, 0, 1};
    } else if (nameStr == "GreenInputField") {
        newCol = {0, inputValue, 0, 1};
    } else if (nameStr == "BlueInputField") {
        newCol = {0, 0, inputValue, 1};
    } else {
        return;
    }

    HMUI::ImageView* bgImageView = inputFieldView->get_transform()->FindChild(il2cpp_utils::newcsstr("BG"))->GetComponent<HMUI::ImageView*>();

    bgImageView->set_color(newCol);
}

void InitSlider(HMUI::InputFieldView*& input, SliderChangeColor sliderColor, HMUI::ColorGradientSlider* slider, std::string name, GlobalNamespace::RGBPanelController* rgbPanelController) {
    input = QuestUI::BeatSaberUI::CreateStringSetting(slider->get_transform(), std::string_view(name), std::string_view("ur ma :)"), [&, rgbPanelController, sliderColor, name](std::string_view stringViewValue) {
        getLogger().info("Starting Color Update For %s", name.c_str());

        UpdateSliderColor(sliderColor, rgbPanelController, stringViewValue);
        input->GetComponent<HMUI::InputFieldViewStaticAnimations*>()->HandleInputFieldViewSelectionStateDidChange(HMUI::InputFieldView::SelectionState::Normal);
    });
    input->set_name(il2cpp_utils::newcsstr(name + "InputField"));

    UnityEngine::RectTransform* trans = input->GetComponent<UnityEngine::RectTransform*>();

    trans->set_position(trans->get_position() + UnityEngine::Vector3(0.92f, 0, 0));

    trans->set_anchorMin({0.95f, 1});
    trans->set_anchorMax({-0.1f, 1});

    slider->DoStateTransition(UnityEngine::UI::Selectable::SelectionState::Normal, true);
}

void InitSliderInputs(GlobalNamespace::RGBPanelController* rgbPanelController) {
    if (redInput != nullptr) return;
    getLogger().info("Init Inputs :)");

    // -- Red Input --

    HMUI::ColorGradientSlider* redSlider = rgbPanelController->redSlider;
    redSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.2f, 1});
    
    InitSlider(redInput, SliderChangeColor::Red, redSlider, "Red", rgbPanelController);

    // -- Green Input --

    HMUI::ColorGradientSlider* greenSlider = rgbPanelController->greenSlider;
    greenSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.3f, 1});

    InitSlider(greenInput, SliderChangeColor::Green, greenSlider, "Green", rgbPanelController);

    // -- Blue Input --

    HMUI::ColorGradientSlider* blueSlider = rgbPanelController->blueSlider;
    blueSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.39f, 1});

    InitSlider(blueInput, SliderChangeColor::Blue, blueSlider, "Blue", rgbPanelController);
}

void UpdateBGStates() {
    redInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(redInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
    greenInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(greenInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
    blueInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(blueInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
}

void RefreshTextValues(GlobalNamespace::RGBPanelController* rgbPanelController) {
    bool didInit = false;
    if (redInput == nullptr) {
        InitSliderInputs(rgbPanelController);
        didInit = true;
    }

    Il2CppString* redText = rgbPanelController->redSlider->valueText->m_text;
    Il2CppString* greenText = rgbPanelController->greenSlider->valueText->m_text;
    Il2CppString* blueText = rgbPanelController->blueSlider->valueText->m_text;

    // The Substrings remove the "R: ", "G: " and "B: " from the text strings, and also caps the length of the text

    redInput->SetText(redText->Substring(3, std::min(redText->get_Length() - 3, 3)));
    greenInput->SetText(greenText->Substring(3, std::min(greenText->get_Length() - 3, 3)));
    blueInput->SetText(blueText->Substring(3, std::min(blueText->get_Length() - 3, 3)));

    LogHierarchy(redInput->get_transform());

    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(UpdateBGNextFrame())));

    return;
}

MAKE_HOOK_MATCH(SetCustomBGHighlightColors, &HMUI::InputFieldViewStaticAnimations::HandleInputFieldViewSelectionStateDidChange, void, HMUI::InputFieldViewStaticAnimations* self, HMUI::InputFieldView::SelectionState state) {    
    SetCustomBGHighlightColors(self, state);
    SetBGColor(self->GetComponent<HMUI::InputFieldView*>());
}

MAKE_HOOK_MATCH(RefreshTextValues_HSVColorChange, &GlobalNamespace::HSVPanelController::RefreshSlidersColors, void, GlobalNamespace::HSVPanelController* self) {
    RefreshTextValues_HSVColorChange(self);

    GlobalNamespace::RGBPanelController* rgbPanelController = self->GetComponentInParent<GlobalNamespace::EditColorSchemeController*>()->rgbPanelController;
    RefreshTextValues(rgbPanelController);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), SetCustomBGHighlightColors);
    INSTALL_HOOK(getLogger(), RefreshTextValues_HSVColorChange);
    getLogger().info("Installed all hooks!");
}