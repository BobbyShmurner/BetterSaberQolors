#include "main.hpp"

#include "include/Utils.hpp"
#include "include/ColorUtility.hpp"

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
HMUI::InputFieldView* hexInput;

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

bool isValidSaberColor(std::string strValue) {
    if (!strValue.empty() || Utils::IsNumber(strValue)) {
        float value = atof(strValue.c_str());

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

void UpdateSliderColor(SliderChangeColor sliderColor, GlobalNamespace::RGBPanelController* rgbPanelController, std::string_view stringViewValue) {
    std::string stringValue = (std::string)stringViewValue;
    UnityEngine::Color newCol;

    if (sliderColor == SliderChangeColor::Hex) {
        int stringlength = static_cast<int>(stringValue.length());

        if (stringlength == 0 || stringlength > 7) { RefreshTextValues(rgbPanelController); }
        if (stringlength != 7) {
            getLogger().info("Didnt Update Color (Not Full Hex Code)");
            return;
        }

        newCol = ColorUtility::ParseHtmlString(stringValue.substr(1));

        rgbPanelController->set_color(newCol);
        rgbPanelController->HandleSliderColorDidChange(rgbPanelController->redSlider, newCol, GlobalNamespace::ColorChangeUIEventType::PointerUp);

        getLogger().info("Changed Color to %s!", Utils::ToString(newCol).c_str());

        return;
    } else {
        if (isValidSaberColor(stringValue)) {
            float value = atof(stringValue.c_str()) / 255;
            HMUI::ColorGradientSlider* slider;

            switch(sliderColor) {
                case (SliderChangeColor::Red):
                    newCol = UnityEngine::Color(value, rgbPanelController->color.g, rgbPanelController->color.b, 1);
                    slider = rgbPanelController->redSlider;

                    break;

                case (SliderChangeColor::Green):
                    newCol = UnityEngine::Color(rgbPanelController->color.r, value, rgbPanelController->color.b, 1);
                    slider = rgbPanelController->greenSlider;

                    break;

                case (SliderChangeColor::Blue):
                    newCol = UnityEngine::Color(rgbPanelController->color.r, rgbPanelController->color.g, value, 1);
                    slider = rgbPanelController->blueSlider;

                    break;
                default:
                    getLogger().info("Failed To Change Color, Invalid SliderChangeColor");

                    return;
            }

            rgbPanelController->set_color(newCol);
            rgbPanelController->HandleSliderColorDidChange(slider, newCol, GlobalNamespace::ColorChangeUIEventType::PointerUp);

            getLogger().info("Changed Color to %s!", Utils::ToString(newCol).c_str());
            return;
        }

        getLogger().info("Color \"%s\" is invalid!", stringValue.c_str());
        RefreshTextValues(rgbPanelController); // This fixes the length of the text input
    }
}

void RefreshHexBG() {
    if (hexInput == nullptr) return;
    UnityEngine::Color newCol = {1, 1, 1, 1};

    newCol.r = atof(Utils::Il2cppStrToStr(redInput->GetComponent<HMUI::InputFieldView*>()->text).c_str()) / 255;
    newCol.g = atof(Utils::Il2cppStrToStr(greenInput->GetComponent<HMUI::InputFieldView*>()->text).c_str()) / 255;
    newCol.b = atof(Utils::Il2cppStrToStr(blueInput->GetComponent<HMUI::InputFieldView*>()->text).c_str()) / 255;

    hexInput->get_transform()->FindChild(il2cpp_utils::newcsstr("BG"))->GetComponent<HMUI::ImageView*>()->set_color(newCol);

    if (newCol.get_grayscale() > 0.7f) {
        hexInput->get_transform()->Find(il2cpp_utils::newcsstr("Text"))->GetComponent<HMUI::CurvedTextMeshPro*>()->set_color({0.5f, 0.5f, 0.5f, 1});
    } else {
        hexInput->get_transform()->Find(il2cpp_utils::newcsstr("Text"))->GetComponent<HMUI::CurvedTextMeshPro*>()->set_color({1, 1, 1, 1});
    }
}

void SetBGColor(HMUI::InputFieldView* inputFieldView) {
    std::string nameStr = Utils::Il2cppStrToStr(inputFieldView->get_name());
    float inputValue = atof(Utils::Il2cppStrToStr(inputFieldView->GetComponent<HMUI::InputFieldView*>()->text).c_str()) / 255;

    UnityEngine::Color newCol;
    
    if (nameStr == "RedInputField") {
        newCol = {inputValue, 0, 0, 1};
    } else if (nameStr == "GreenInputField") {
        newCol = {0, inputValue, 0, 1};
    } else if (nameStr == "BlueInputField") {
        newCol = {0, 0, inputValue, 1};
    } else if (nameStr == "HexInputField") {
        RefreshHexBG();
        return;
    } else {
        newCol = {0, 0, 0, 0.5f}; // This is the default bg colour
    }

    HMUI::ImageView* bgImageView = inputFieldView->get_transform()->FindChild(il2cpp_utils::newcsstr("BG"))->GetComponent<HMUI::ImageView*>();

    bgImageView->set_color(newCol);
    RefreshHexBG();
}

void InitSliderInit(HMUI::InputFieldView*& input, SliderChangeColor sliderColor, std::string name, GlobalNamespace::RGBPanelController* rgbPanelController, HMUI::ColorGradientSlider* slider = nullptr) {
    UnityEngine::Transform* parentTrans;
    if (slider != nullptr) parentTrans = slider->get_transform();

    UnityEngine::Vector2 anchorMin = {0.95f, 1.0f};
    UnityEngine::Vector2 anchorMax = {-0.1f, 1.0f};
    UnityEngine::Vector3 posOffset = {0.92, 0, 0};

    if (sliderColor == SliderChangeColor::Hex) {
        parentTrans = rgbPanelController->get_transform()->get_parent();

        anchorMin = {0.5f, 1.0f};
        anchorMax = {0.23f, 1.0f};
        posOffset = {-0.38f, 0.043f, 0};
    }

    input = QuestUI::BeatSaberUI::CreateStringSetting(parentTrans, std::string_view(name), std::string_view("ooga booga monki :)"), [&, rgbPanelController, sliderColor, name](std::string_view stringViewValue) {
        getLogger().info("Starting Color Update For %s", name.c_str());

        UpdateSliderColor(sliderColor, rgbPanelController, stringViewValue);
        input->GetComponent<HMUI::InputFieldViewStaticAnimations*>()->HandleInputFieldViewSelectionStateDidChange(HMUI::InputFieldView::SelectionState::Normal);
    });
    input->set_name(il2cpp_utils::newcsstr(name + "InputField"));
    input->get_transform()->FindChild(il2cpp_utils::newcsstr("Text"))->GetComponent<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::UpperCase);

    UnityEngine::RectTransform* trans = input->GetComponent<UnityEngine::RectTransform*>();

    UnityEngine::RectTransform* baseLocTrans = trans;
    if (sliderColor == SliderChangeColor::Hex) {
        baseLocTrans = rgbPanelController->get_transform()->get_parent()->FindChild(il2cpp_utils::newcsstr("OkButton"))->GetComponent<UnityEngine::RectTransform*>();
    }

    trans->set_position(baseLocTrans->get_position() + posOffset);

    trans->set_anchorMin(anchorMin);
    trans->set_anchorMax(anchorMax);
}

void InitSliderInputs(GlobalNamespace::RGBPanelController* rgbPanelController) {
    if (redInput != nullptr) return;
    getLogger().info("Init Inputs :)");

    // -- Red Input --

    HMUI::ColorGradientSlider* redSlider = rgbPanelController->redSlider;
    redSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.2f, 1});
    
    InitSliderInit(redInput, SliderChangeColor::Red, "Red", rgbPanelController, redSlider);

    // -- Green Input --

    HMUI::ColorGradientSlider* greenSlider = rgbPanelController->greenSlider;
    greenSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.3f, 1});

    InitSliderInit(greenInput, SliderChangeColor::Green, "Green", rgbPanelController, greenSlider);

    // -- Blue Input --

    HMUI::ColorGradientSlider* blueSlider = rgbPanelController->blueSlider;
    blueSlider->GetComponent<UnityEngine::RectTransform*>()->set_anchorMax({-0.39f, 1});

    InitSliderInit(blueInput, SliderChangeColor::Blue, "Blue", rgbPanelController, blueSlider);

    // -- Hex Input --

    InitSliderInit(hexInput, SliderChangeColor::Hex, "Hex", rgbPanelController);
}

void UpdateBGStates() {
    redInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(redInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
    greenInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(greenInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
    blueInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(blueInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
    hexInput->GetComponent<HMUI::InputFieldView*>()->DoStateTransition(hexInput->GetComponent<HMUI::InputFieldView*>()->selectionState.value, true);
}

void RefreshTextValues(GlobalNamespace::RGBPanelController* rgbPanelController) {
    if (rgbPanelController == nullptr) return;
    if (redInput == nullptr) {
        InitSliderInputs(rgbPanelController);
    }

    Il2CppString* redText = rgbPanelController->redSlider->valueText->m_text;
    Il2CppString* greenText = rgbPanelController->greenSlider->valueText->m_text;
    Il2CppString* blueText = rgbPanelController->blueSlider->valueText->m_text;

    // The Substrings remove the "R: ", "G: " and "B: " from the text strings, and also caps the length of the text

    redInput->SetText(redText->Substring(3, std::min(redText->get_Length() - 3, 3)));
    greenInput->SetText(greenText->Substring(3, std::min(greenText->get_Length() - 3, 3)));
    blueInput->SetText(blueText->Substring(3, std::min(blueText->get_Length() - 3, 3)));

    RefreshHexBG();

    if (static_cast<int>(Utils::Il2cppStrToStr(hexInput->get_text()).length()) <= 1) {
        hexInput->SetText(il2cpp_utils::newcsstr("#"));
    } else {
        UnityEngine::Color col = hexInput->get_transform()->FindChild(il2cpp_utils::newcsstr("BG"))->GetComponent<HMUI::ImageView*>()->get_color();
        hexInput->SetText(il2cpp_utils::newcsstr(("#" + ColorUtility::ToHtmlStringRGB(col)).c_str()));
    }

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