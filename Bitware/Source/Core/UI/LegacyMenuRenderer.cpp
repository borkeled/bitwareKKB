#include "LegacyMenuRenderer.h"
#include <imgui/imgui.h>
#include <Miscellaneous/Config/ConfigManager.h>
#include <imgui/imgui_internal.h>
#include <imgui/addons/imgui_addons.h>
#include <imgui/addons/colors/colors.h>
#include <Globals.hxx>
#include <Core/Graphics/Graphics.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include <Infrastructure/Obfuscation.h>

void LegacyMenuRenderer::Render()
{
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    ImGuiStyle& Style = ImGui::GetStyle();
    ImGuiIO& Io = ImGui::GetIO(); (void)Io;

    ImGui::SetNextWindowSize(ImVec2(500, 530), ImGuiCond_Once);
    ImGui::SetNextWindowPos(Io.DisplaySize / 2, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    float HeaderHeight = ImGui::GetFontSize() + 6.0f * 2.0f;
    static ImVec2 LogoSize(25, 25);

    bool MainWindow = ImGui::Begin(WRAPPER_MARCO("main"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(2);

    if (MainWindow)
    {
        ImVec2 WinPos = ImGui::GetWindowPos();
        ImVec2 WinSize = ImGui::GetWindowSize();
        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
        DrawList->AddRectFilled(WinPos, WinPos + WinSize, Menu::Bg);
        DrawList->AddRect(WinPos + ImVec2(1.0f, 1.0f), WinPos + WinSize - ImVec2(1.0f, 1.0f), Menu::Outline);
        DrawList->AddRect(WinPos + ImVec2(2.0f, 2.0f), WinPos + WinSize - ImVec2(2.0f, 2.0f), Menu::Accent);

        float Top = HeaderHeight + Style.WindowBorderSize - 8.0f;
        float Bottom = Style.WindowBorderSize + Style.WindowPadding.y + 1.0f;

        ImVec2 ContentMin = WinPos + ImVec2(Style.WindowBorderSize * 2.0f + Style.WindowPadding.x, Top + Style.WindowPadding.y + 6.0f);
        ImVec2 ContentMax = WinPos + ImVec2(WinSize.x - Style.WindowBorderSize * 2.0f - Style.WindowPadding.x, WinSize.y - Bottom);

        DrawList->AddRectFilled(ContentMin, ContentMax, Menu::InnerBg);
        DrawList->AddRect(ContentMin - ImVec2(1.0f, 1.0f), ContentMax + ImVec2(1.0f, 1.0f), Menu::Outline);
        DrawList->AddRect(ContentMin - ImVec2(2.0f, 2.0f), ContentMax + ImVec2(2.0f, 2.0f), Menu::DarkAccent);

        float TextX = WinPos.x + Style.WindowBorderSize + 8.0f;
        float TextY = WinPos.y + (Top - ImGui::CalcTextSize(WRAPPER_MARCO("Bitware")).y) * 0.5f + 8.0f;

        ImVec2 TextPos(TextX, TextY);

        Menu::DrawLabelShadow(DrawList, TextPos, Menu::Accent, WRAPPER_MARCO("Bitware"));
        TextPos.x += ImGui::CalcTextSize(WRAPPER_MARCO("Bitware")).x;

        static int Section = 0;
        const char* TabLabels[] = { WRAPPER_MARCO("Aimbot"), WRAPPER_MARCO("Visuals"), WRAPPER_MARCO("Players"), WRAPPER_MARCO("Settings"), WRAPPER_MARCO("Configs") };
        const int TabCount = IM_ARRAYSIZE(TabLabels);

        Menu::Tabs(DrawList, WinPos, TextY, Section, TabLabels, TabCount, 10.0f, 8.0f);

        ImGui::SetCursorScreenPos(ContentMin);
        ImGui::PushClipRect(ContentMin, ContentMax, true);
        ImGui::BeginGroup();

        if (Section == 0)
        {
            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;

            float LeftWidth = (AvailSize.x - Spacing) * 0.5f;
            float RightWidth = AvailSize.x - Spacing - LeftWidth;

            float SidePad = 4.0f;
            float HalfHeight = (AvailSize.y - SidePad * 2.0f) * 0.5f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

            if (Menu::BeginChild(WRAPPER_MARCO("Aimbot"), ImVec2(LeftWidth - SidePad, HalfHeight)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Enabled"), &Globals::Aimbot::Enabled);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x + 12.0f);
                Menu::KeyBindEx(WRAPPER_MARCO("Aimbot Key"), &Globals::Aimbot::Aimbot_Key, &Globals::Aimbot::Aimbot_Mode);

                Menu::CheckBox(WRAPPER_MARCO("Sticky aim"), &Globals::Aimbot::AimbotSticky);
                Menu::CheckBox(WRAPPER_MARCO("Knocked check"), &Globals::Aimbot::KnockedCheck);
                Menu::CheckBox(WRAPPER_MARCO("Wall check"), &Globals::Aimbot::WallCheck);

                Menu::Combo(WRAPPER_MARCO("Aimbot type"), &Globals::Aimbot::Aimbot_type, { WRAPPER_MARCO("Mouse"), WRAPPER_MARCO("Camera") });

                Menu::Combo(WRAPPER_MARCO("HitPart"), &Globals::Aimbot::HitPart, { WRAPPER_MARCO("Head"), WRAPPER_MARCO("Torso"), WRAPPER_MARCO("LowerTorso") });

                if (Globals::Aimbot::Aimbot_type == 0)
                {

                    Menu::SliderFloat(WRAPPER_MARCO("Mouse smoothing X"), &Globals::Aimbot::Mouse::Smoothing_X, 0.0f, 12.0f);
                    Menu::SliderFloat(WRAPPER_MARCO("Mouse smoothing Y"), &Globals::Aimbot::Mouse::Smoothing_Y, 0.0f, 12.0f);

                    Menu::SliderFloat(WRAPPER_MARCO("Mouse sensitivty"), &Globals::Aimbot::Mouse::Mouse_Sensitivty, 0.0f, 5.0f);
                }
                else if (Globals::Aimbot::Aimbot_type == 1)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Camera smoothing X"), &Globals::Aimbot::Camera::Smoothing_X, 0.0f, 12.0f);
                    Menu::SliderFloat(WRAPPER_MARCO("Camera smoothing Y"), &Globals::Aimbot::Camera::Smoothing_Y, 0.0f, 12.0f);
                }
                Menu::EndChild();
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);
            float Bottom = HalfHeight - 8;
            if (Menu::BeginChild(WRAPPER_MARCO("Aimbot FOV"), ImVec2(LeftWidth - SidePad, Bottom)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Draw FOV"), &Globals::Aimbot::DrawFov);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Color"), Globals::Aimbot::FovColor);

                {
                    float xBtnOffset = ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f;
                    float keyWidth = ImGui::GetContentRegionAvail().x * 0.35f;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.40f);
                    Menu::KeyBindEx(WRAPPER_MARCO("FOV Toggle"), &Globals::Aimbot::FovToggleKey, &Globals::Aimbot::FovToggleMode, ImVec2(keyWidth, 0));
                    ImGui::SameLine(xBtnOffset);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.0f));
                    if (ImGui::SmallButton(WRAPPER_MARCO("X##fov")))
                    {
                        Globals::Aimbot::FovToggleKey = ImGuiKey_None;
                    }
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip(WRAPPER_MARCO("Clear keybind"));
                }

                Menu::CheckBox(WRAPPER_MARCO("Fill FOV"), &Globals::Aimbot::FillFov);
                Menu::CheckBox(WRAPPER_MARCO("Use FOV"), &Globals::Aimbot::useFov);

                Menu::SliderFloat(WRAPPER_MARCO("Size"), &Globals::Aimbot::FovSize, 1.0f, 500.0f);

                Menu::CheckBox(WRAPPER_MARCO("Spin"), &Globals::Aimbot::FovSpin);
                if (Globals::Aimbot::FovSpin)
                {
                    Menu::Combo(WRAPPER_MARCO("Direction"), &Globals::Aimbot::FovSpinDirection, { WRAPPER_MARCO("Clockwise"), WRAPPER_MARCO("Counter-Clockwise") });
                    Menu::SliderInt(WRAPPER_MARCO("Speed"), &Globals::Aimbot::FovSpinSpeed, 1, 5);
                }

                Menu::EndChild();
            }

            ImGui::SameLine(0.0f, Spacing);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 248);
            if (Menu::BeginChild(WRAPPER_MARCO("Triggerbot"), ImVec2(RightWidth - SidePad, HalfHeight)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Enabled"), &Globals::Triggerbot::Enabled);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x + 12.0f);
                Menu::KeyBindEx(WRAPPER_MARCO("Trigger Key"), &Globals::Triggerbot::Triggerbot_Key, &Globals::Triggerbot::Triggerbot_Mode);

                Menu::CheckBox(WRAPPER_MARCO("Knocked check"), &Globals::Triggerbot::KnockedCheck);
                Menu::CheckBox(WRAPPER_MARCO("Wall check"), &Globals::Triggerbot::WallCheck);

                Menu::Combo(WRAPPER_MARCO("HitPart"), &Globals::Triggerbot::HitPart, { WRAPPER_MARCO("Head"), WRAPPER_MARCO("Torso"), WRAPPER_MARCO("LowerTorso"), WRAPPER_MARCO("All") });

                Menu::Combo(WRAPPER_MARCO("Fire Mode"), &Globals::Triggerbot::FireMode, { WRAPPER_MARCO("Tap"), WRAPPER_MARCO("Full Auto") });

                Menu::SliderInt(WRAPPER_MARCO("Delay (ms)"), &Globals::Triggerbot::Delay, 0, 500);
                Menu::SliderInt(WRAPPER_MARCO("Randomize (ms)"), &Globals::Triggerbot::Randomize, 0, 250);

                Menu::EndChild();
            }

        }

        if (Section == 1)
        {
            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;

            float LeftWidth = (AvailSize.x - Spacing) * 0.5f;
            float RightWidth = AvailSize.x - Spacing - LeftWidth;

            float SidePad = 4.0f;
            float HalfHeight = (AvailSize.y - SidePad * 2.0f) * 0.5f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

            if (Menu::BeginChild(WRAPPER_MARCO("Visuals"), ImVec2(LeftWidth - SidePad, HalfHeight)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Enabled"), &Globals::Visuals::Enabled);
                {
                    float xBtnOffset = ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f;
                    float keyWidth = ImGui::GetContentRegionAvail().x * 0.35f;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.40f);
                    Menu::KeyBindEx(WRAPPER_MARCO("ESP Toggle"), &Globals::Visuals::ToggleKey, &Globals::Visuals::ToggleMode, ImVec2(keyWidth, 0));
                    ImGui::SameLine(xBtnOffset);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.0f));
                    if (ImGui::SmallButton(WRAPPER_MARCO("X##esp")))
                    {
                        Globals::Visuals::ToggleKey = ImGuiKey_None;
                    }
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip(WRAPPER_MARCO("Clear keybind"));
                }
                Menu::CheckBox(WRAPPER_MARCO("Box"), &Globals::Visuals::Box);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Box Color"), Globals::Visuals::Colors::Box);

                if (Globals::Visuals::Box)
                {
                    Menu::CheckBox(WRAPPER_MARCO("Box Fill"), &Globals::Visuals::Box_Fill);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                    Menu::ColorEdit4(WRAPPER_MARCO("Box Fill Top"), Globals::Visuals::Colors::BoxFill_Top);
                    if (Globals::Visuals::Box_Fill_Gradient)
                    {
                        ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() - 24);
                        Menu::ColorEdit4(WRAPPER_MARCO("Box Fill Bottom"), Globals::Visuals::Colors::BoxFill_Bottom);
                    }
                }

                Menu::CheckBox(WRAPPER_MARCO("Healthbar"), &Globals::Visuals::Healthbar);
                if (Globals::Visuals::Healthbar_Type == 0)
                {
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                    Menu::ColorEdit4(WRAPPER_MARCO("Healthbar Color"), Globals::Visuals::Colors::Healthbar);
                }
                else if (Globals::Visuals::Healthbar_Type == 1)
                {
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                    Menu::ColorEdit4(WRAPPER_MARCO("Healthbar Top"), Globals::Visuals::Colors::Healthbar_Top);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() - 24);
                    Menu::ColorEdit4(WRAPPER_MARCO("Healthbar Middle"), Globals::Visuals::Colors::Healthbar_Middle);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() - 51);
                    Menu::ColorEdit4(WRAPPER_MARCO("Healthbar Bottom"), Globals::Visuals::Colors::Healthbar_Bottom);
                }

                Menu::CheckBox(WRAPPER_MARCO("Health"), &Globals::Visuals::Health);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Health Color"), Globals::Visuals::Colors::Health);

                Menu::CheckBox(WRAPPER_MARCO("Name"), &Globals::Visuals::Name);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Name Color"), Globals::Visuals::Colors::Name);

                Menu::CheckBox(WRAPPER_MARCO("Distance"), &Globals::Visuals::Distance);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Distance Color"), Globals::Visuals::Colors::Distance);

                Menu::CheckBox(WRAPPER_MARCO("Rig Type"), &Globals::Visuals::Rig_Type);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Rig Type Color"), Globals::Visuals::Colors::Rig_Type);

                Menu::CheckBox(WRAPPER_MARCO("Tool"), &Globals::Visuals::Tool);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Tool Color"), Globals::Visuals::Colors::Tool);

                Menu::CheckBox(WRAPPER_MARCO("Skeleton"), &Globals::Visuals::Skeleton);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Skeleton Color"), Globals::Visuals::Colors::Skeleton);

                Menu::CheckBox(WRAPPER_MARCO("Chams"), &Globals::Visuals::Chams);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Chams Color"), Globals::Visuals::Colors::Chams);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() - 24);
                Menu::ColorEdit4(WRAPPER_MARCO("Chams Outline"), Globals::Visuals::Colors::ChamsOutline);

                Menu::CheckBox(WRAPPER_MARCO("Wall Check"), &Globals::Visuals::WallCheck);
                if (Globals::Visuals::WallCheck)
                {
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                    Menu::ColorEdit4(WRAPPER_MARCO("Occluded Color"), Globals::Visuals::Colors::Occluded);
                }

                Menu::EndChild();
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);
            float Bottom = HalfHeight - 8;
            if (Menu::BeginChild(WRAPPER_MARCO("World"), ImVec2(LeftWidth - SidePad, Bottom)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Skybox Changer"), &Globals::World::Skybox);
                if (Globals::World::Skybox)
                {
                    Menu::Combo(WRAPPER_MARCO("Skybox Type"), &Globals::World::Skybox_Type, {
                        WRAPPER_MARCO("Bitware.fun"), WRAPPER_MARCO("Space"), WRAPPER_MARCO("Pink Sky"), WRAPPER_MARCO("Minecraft"), WRAPPER_MARCO("Night Cloudy"),
                        WRAPPER_MARCO("Sparkling Night"), WRAPPER_MARCO("Winterness"), WRAPPER_MARCO("Dark Crimson"), WRAPPER_MARCO("Nebula"),
                        WRAPPER_MARCO("Tropical"), WRAPPER_MARCO("Green Sky")
                        });

                    Menu::CheckBox(WRAPPER_MARCO("Skybox Rotation"), &Globals::World::Rotate);
                    if (Globals::World::Rotate)
                    {
                        Menu::SliderFloat(WRAPPER_MARCO("Rotation Speed"), &Globals::World::Skybox_Rotate_Speed, 0.0f, 5.0f);
                    }
                }

                Menu::CheckBox(WRAPPER_MARCO("Atmosphere"), &Globals::World::Ambience);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Atmosphere Color"), Globals::World::Colors::Ambience);

                Menu::CheckBox(WRAPPER_MARCO("Fog"), &Globals::World::Fog);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - Menu::GetColorPickerWidth() + Style.ChildPadding.x - 1.0f);
                Menu::ColorEdit4(WRAPPER_MARCO("Fog Color"), Globals::World::Colors::Fog);
                if (Globals::World::Fog)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Fog Distance"), &Globals::World::Fog_Distance, 0.0f, 1000.0f);
                }

                Menu::CheckBox(WRAPPER_MARCO("Brightness"), &Globals::World::Brightness);
                if (Globals::World::Brightness)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Brightness Value"), &Globals::World::BrightnessI, 0.0f, 10.0f);
                }

                Menu::CheckBox(WRAPPER_MARCO("Exposure"), &Globals::World::Exposure);
                if (Globals::World::Exposure)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Exposure Value"), &Globals::World::ExposureI, -3.0f, 3.0f);
                }

                Menu::CheckBox(WRAPPER_MARCO("FOV"), &Globals::World::FOV);
                if (Globals::World::FOV)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("FOV Value"), &Globals::World::FOV_Distance, 70.0f, 120.0f);
                }

                Menu::EndChild();
            }

            ImGui::SameLine(0.0f, Spacing);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 248);
            if (Menu::BeginChild(WRAPPER_MARCO("Options"), ImVec2(RightWidth - SidePad, AvailSize.y - SidePad * 2.0f)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Exclude Client"), &Globals::Settings::Client_Check);
                Menu::CheckBox(WRAPPER_MARCO("Exclude Team"), &Globals::Settings::Team_Check);

                Menu::SliderFloat(WRAPPER_MARCO("Render Distance"), &Globals::Visuals::Render_Distance, 0.0f, 500.0f);

                if (Globals::Visuals::Healthbar)
                {
                    Menu::Combo(WRAPPER_MARCO("Healthbar Style"), &Globals::Visuals::Healthbar_Type, { WRAPPER_MARCO("Static"), WRAPPER_MARCO("Gradient") });
                    Menu::SliderInt(WRAPPER_MARCO("Healthbar Gap"), &Globals::Visuals::Gap, 1, 5);
                    Menu::SliderInt(WRAPPER_MARCO("Healthbar Thickness"), &Globals::Visuals::Thickness, 1, 5);
                }

                if (Globals::Visuals::Box)
                {
                    Menu::Combo(WRAPPER_MARCO("Box Style"), &Globals::Visuals::Box_Type, { WRAPPER_MARCO("Bounding"), WRAPPER_MARCO("Corner") });
                }

                if (Globals::Visuals::Box_Fill)
                {
                    Menu::CheckBox(WRAPPER_MARCO("Fill Gradient"), &Globals::Visuals::Box_Fill_Gradient);

                    if (Globals::Visuals::Box_Fill_Gradient)
                    {
                        Menu::CheckBox(WRAPPER_MARCO("Fill Rotation"), &Globals::Visuals::Box_Fill_Gradient_Rotate);
                    }

                    if (Globals::Visuals::Box_Fill_Gradient_Rotate)
                    {
                        Menu::Combo(WRAPPER_MARCO("Rotation Type"), &Globals::Visuals::Box_Fill_Type, { WRAPPER_MARCO("Side"), WRAPPER_MARCO("Bottom"), WRAPPER_MARCO("Spin") });
                        Menu::SliderInt(WRAPPER_MARCO("Rotation Speed"), &Globals::Visuals::BoxFillSpeed, 1, 5);
                    }
                }

                if (Globals::Visuals::Name)
                {
                    Menu::Combo(WRAPPER_MARCO("Name Display"), &Globals::Visuals::Name_Type, { WRAPPER_MARCO("Name"), WRAPPER_MARCO("Display Name"), WRAPPER_MARCO("Name & Display Name") });
                }

                if (Globals::Visuals::Chams)
                {
                    Menu::CheckBox(WRAPPER_MARCO("Chams Fade"), &Globals::Visuals::ChamsFade);

                    if (Globals::Visuals::ChamsFade)
                    {
                        Menu::SliderInt(WRAPPER_MARCO("Fade Speed"), &Globals::Visuals::ChamsFadeSpeed, 1, 5);
                    }
                }

                Menu::EndChild();
            }
        }
        if (Section == 2)
        {
            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;

            float LeftWidth = (AvailSize.x - Spacing) * 0.6f;
            float RightWidth = AvailSize.x - Spacing - LeftWidth;

            float SidePad = 4.0f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

            if (Menu::BeginChild(WRAPPER_MARCO("PlayerList"), ImVec2(LeftWidth - SidePad, AvailSize.y - SidePad * 2)))
            {
                if (!Globals::Player_Cache) return;
                for (auto& player : *Globals::Player_Cache)
                {
                    if (!player.Character.Address) continue;

                    ImGui::PushID((int)player.UserID);

                    bool whitelisted = Globals::Whitelist::UserIDs.count(player.UserID) > 0;
                    Menu::CheckBox(WRAPPER_MARCO("wl"), &whitelisted);
                    if (whitelisted != (Globals::Whitelist::UserIDs.count(player.UserID) > 0))
                    {
                        if (whitelisted)
                            Globals::Whitelist::UserIDs.insert(player.UserID);
                        else
                            Globals::Whitelist::UserIDs.erase(player.UserID);
                    }

                    ImGui::SameLine();

                    if (player.Local_Player)
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), WRAPPER_MARCO("[You] "));
                    else
                        ImGui::Text(WRAPPER_MARCO("      "));

                    ImGui::SameLine();

                    std::string info = std::to_string((int)player.Distance) + std::string(WRAPPER_MARCO("m ")) + std::to_string((int)player.Health) + std::string(WRAPPER_MARCO("/")) + std::to_string((int)player.MaxHealth);
                    float infoW = ImGui::CalcTextSize(info.c_str()).x + 4;
                    float nameW = ImGui::GetContentRegionAvail().x - infoW;
                    if (nameW < 10) nameW = 10;

                    float nameStartX = ImGui::GetCursorPosX();
                    {
                        ImVec2 nameSize = ImGui::CalcTextSize(player.Name.c_str());
                        if (nameSize.x > nameW)
                        {
                            std::string displayName = player.Name;
                            while (!displayName.empty() && ImGui::CalcTextSize((displayName + "...").c_str()).x > nameW)
                                displayName.pop_back();
                            ImGui::Text("%s", (displayName + "...").c_str());
                        }
                        else
                        {
                            ImGui::Text("%s", player.Name.c_str());
                        }
                    }
                    ImGui::SameLine(nameStartX + nameW);
                    float hp = player.MaxHealth > 0.f ? (player.Health / player.MaxHealth) : 0.f;
                    ImVec4 hpColor = hp > 0.5f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                                 : (hp > 0.25f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                                                : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, hpColor);
                    ImGui::TextDisabled("%s", info.c_str());
                    ImGui::PopStyleColor();

                    ImGui::PopID();
                }
                Menu::EndChild();
            }

            ImGui::SameLine(0.0f, Spacing);
            ImGui::SetCursorScreenPos(ImVec2(ContentMin.x + LeftWidth + Spacing + SidePad, ContentMin.y + SidePad));

            if (Menu::BeginChild(WRAPPER_MARCO("WhitelistSettings"), ImVec2(RightWidth - SidePad * 2, AvailSize.y - SidePad * 2)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Enable Whitelist"), &Globals::Whitelist::Enabled);
                float colorPickerW = Menu::GetColorPickerWidth();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - colorPickerW) * 0.5f);
                Menu::ColorEdit4(WRAPPER_MARCO("Whitelist Color"), Globals::Whitelist::Color);
                ImGui::Dummy(ImVec2(0.0f, 8.0f));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0.0f, 8.0f));
                ImGui::TextDisabled(WRAPPER_MARCO("%zu whitelisted"), Globals::Whitelist::UserIDs.size());
                Menu::EndChild();
            }
        }
        if (Section == 3)
        {
            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;

            float LeftWidth = (AvailSize.x - Spacing) * 0.5f;
            float RightWidth = AvailSize.x - Spacing - LeftWidth;

            float SidePad = 4.0f;
            float HalfHeight = (AvailSize.y - SidePad * 2.0f) * 0.5f;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

            if (Menu::BeginChild(WRAPPER_MARCO("Misc"), ImVec2(LeftWidth - SidePad, HalfHeight)))
            {
                Menu::CheckBox(WRAPPER_MARCO("Explorer"), &Globals::Explorer::Open);

                Menu::CheckBox(WRAPPER_MARCO("Speedhack"), &Globals::Misc::Speed);
                if (Globals::Misc::Speed)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Speed Value"), &Globals::Misc::Speed_Value, 10.0f, 500.0f);
                }

                Menu::CheckBox(WRAPPER_MARCO("Jump Power"), &Globals::Misc::Jump);
                if (Globals::Misc::Jump)
                {
                    Menu::SliderFloat(WRAPPER_MARCO("Jump Value"), &Globals::Misc::Jump_Value, 10.0f, 200.0f);
                }

                Menu::EndChild();
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad + LeftWidth + Spacing);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - HalfHeight);

            if (Menu::BeginChild(WRAPPER_MARCO("Settings"), ImVec2(RightWidth - SidePad, HalfHeight)))
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 37.0f);
                Menu::KeyBind(WRAPPER_MARCO("Menu Key"), &Globals::Settings::Menu_Keybind);
                Menu::CheckBox(WRAPPER_MARCO("Streamproof"), &Globals::Settings::Streamproof);
                Menu::Combo(WRAPPER_MARCO("Performance Type"), &Globals::Settings::Performance_Mode, { WRAPPER_MARCO("Low"), WRAPPER_MARCO("Medium"), WRAPPER_MARCO("High") });
                Menu::Combo(WRAPPER_MARCO("Wall Check Method"), &Globals::Settings::WallCheck_Method, { WRAPPER_MARCO("OBB"), WRAPPER_MARCO("Raycast") });

                Menu::EndChild();
            }
        }
        if (Section == 4)
        {
            ImVec2 AvailSize(ContentMax.x - ContentMin.x, ContentMax.y - ContentMin.y);
            float Spacing = Style.ItemSpacing.x;

            float LeftWidth = (AvailSize.x - Spacing) * 0.45f;
            float RightWidth2 = AvailSize.x - Spacing - LeftWidth;

            float SidePad = 4.0f;

            static int SelectedConfig = -1;
            static char ConfigName[64] = "";

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SidePad);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SidePad);

            if (Menu::BeginChild(WRAPPER_MARCO("ConfigList"), ImVec2(LeftWidth - SidePad, AvailSize.y - SidePad * 2.0f)))
            {
                auto Configs = ConfigManager::Get().ListNames();
                for (int i = 0; i < (int)Configs.size(); i++)
                {
                    bool IsSelected = (SelectedConfig == i);
                    if (Menu::SelectableLabel(Configs[i].c_str(), IsSelected))
                    {
                        SelectedConfig = i;
                        strncpy_s(ConfigName, Configs[i].c_str(), _TRUNCATE);
                    }
                }

                if (Configs.empty())
                {
                    ImGui::TextDisabled(WRAPPER_MARCO("No configs found"));
                }

                Menu::EndChild();
            }

            ImGui::SameLine(0.0f, Spacing);
            ImGui::SetCursorScreenPos(ImVec2(ContentMin.x + LeftWidth + Spacing + SidePad, ContentMin.y + SidePad));

            if (Menu::BeginChild(WRAPPER_MARCO("ConfigActions"), ImVec2(RightWidth2 - SidePad * 2, AvailSize.y - SidePad * 2)))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));

                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(WRAPPER_MARCO("##configname"), ConfigName, sizeof(ConfigName));
                ImGui::PopItemWidth();

                ImGui::PopStyleColor(5);

                ImGui::Dummy(ImVec2(0.0f, 8.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                if (ImGui::Button(WRAPPER_MARCO("Save"), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    if (strlen(ConfigName) > 0)
                    {
                        ConfigManager::Get().Save(ConfigName);
                        SelectedConfig = -1;
                        ConfigName[0] = '\0';
                    }
                }

                ImGui::PopStyleColor(4);

                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                if (ImGui::Button(WRAPPER_MARCO("Load"), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    if (SelectedConfig >= 0)
                    {
                        auto Configs = ConfigManager::Get().ListNames();
                        if (SelectedConfig < (int)Configs.size())
                        {
                            ConfigManager::Get().Load(Configs[SelectedConfig].c_str());
                        }
                    }
                }

                ImGui::PopStyleColor(4);

                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                if (ImGui::Button(WRAPPER_MARCO("Save (Overwrite)"), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    if (SelectedConfig >= 0)
                    {
                        auto Configs = ConfigManager::Get().ListNames();
                        if (SelectedConfig < (int)Configs.size())
                        {
                            ConfigManager::Get().Save(Configs[SelectedConfig].c_str());
                        }
                    }
                }

                ImGui::PopStyleColor(4);

                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.78f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                if (ImGui::Button(WRAPPER_MARCO("Delete"), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    if (SelectedConfig >= 0)
                    {
                        auto Configs = ConfigManager::Get().ListNames();
                        if (SelectedConfig < (int)Configs.size())
                        {
                            ConfigManager::Get().Delete(Configs[SelectedConfig].c_str());
                            SelectedConfig = -1;
                            ConfigName[0] = '\0';
                        }
                    }
                }

                ImGui::PopStyleColor(4);

                Menu::EndChild();
            }
        }

        ImGui::EndGroup();
        ImGui::PopClipRect();
    }

    ImGui::End();

    // Explorer window
    if (Globals::Explorer::Open)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
        ImGui::SetNextWindowPos(Io.DisplaySize / 2, ImGuiCond_Once, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        bool explorer_open = true;
        bool explorer_window = ImGui::Begin(WRAPPER_MARCO("Explorer"), &explorer_open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize);
        ImGui::PopStyleVar(2);

        if (explorer_window)
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImRect window_bb(window->Rect());

            window->DrawList->AddRectFilled(window_bb.Min, window_bb.Max, ImGui::GetColorU32(ImGuiCol_WindowBg));

            if (Style.WindowBorderSize > 0.0f)
            {
                window->DrawList->AddRect(window_bb.Min, window_bb.Max, ImGui::GetColorU32(ImGuiCol_Border), Style.WindowRounding, ImDrawFlags_None, Style.WindowBorderSize);
                window->DrawList->AddRect(window_bb.Min + ImVec2(Style.WindowBorderSize, Style.WindowBorderSize), window_bb.Max - ImVec2(Style.WindowBorderSize, Style.WindowBorderSize), ImGui::GetColorU32(ImGuiCol_Border), Style.WindowRounding, ImDrawFlags_None, Style.WindowBorderSize);

                window->DrawList->AddLine(window_bb.Min + ImVec2(Style.WindowBorderSize * 2.0f, Style.WindowBorderSize * 2.0f), ImVec2(window_bb.Max.x - Style.WindowBorderSize, window_bb.Min.y + Style.WindowBorderSize * 2.0f), IM_COL32(0, 107, 0, 255), Style.WindowBorderSize);
                window->DrawList->AddLine(window_bb.Min + ImVec2(Style.WindowBorderSize * 2.0f, Style.WindowBorderSize * 3.0f), ImVec2(window_bb.Max.x - Style.WindowBorderSize, window_bb.Min.y + Style.WindowBorderSize * 3.0f), IM_COL32(0, 199, 0, 255), Style.WindowBorderSize);
                window->DrawList->AddLine(window_bb.Min + ImVec2(Style.WindowBorderSize * 2.0f, Style.WindowBorderSize * 4.0f), ImVec2(window_bb.Max.x - Style.WindowBorderSize, window_bb.Min.y + Style.WindowBorderSize * 4.0f), ImGui::GetColorU32(ImGuiCol_Border), Style.WindowBorderSize);
            }

            window->DrawList->AddText(window_bb.Min + ImVec2(Style.FramePadding.x + Style.WindowBorderSize * 3.0f, Style.FramePadding.y + Style.WindowBorderSize * 4.0f), ImGui::GetColorU32(ImGuiCol_Text), WRAPPER_MARCO("Explorer"));

            ImGui::SetCursorScreenPos(window_bb.Min + ImVec2(Style.WindowPadding.x, ImGui::GetFrameHeight() + Style.WindowBorderSize * 3.0f + 3.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Style.WindowPadding);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildPadding, ImVec2(Style.WindowPadding.x, 4.6f));
            if (ImGui::BeginChild("explorer_body", ImGui::GetContentRegionAvail() - Style.WindowPadding, ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground))
            {
                float bottom_height = ImGui::GetFontSize() * 12.0f;
                const float bottom_gap = 3.0f;
                float available_height = ImGui::GetContentRegionAvail().y - bottom_gap;
                float explorer_height = available_height - bottom_height - Style.ItemSpacing.y;

                if (explorer_height > 0 && available_height > 0 && bottom_height > 0)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildPadding, ImVec2(Style.WindowPadding.x, 3.0f));
                    if (ImGui::BeginChild("Explorer", ImVec2(0, explorer_height), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar))
                    {
                        if (!g_Explorer.is_refreshing.load())
                        {
                            if (g_Explorer.root)
                            {
                                try
                                {
                                    g_Explorer.RenderNode(g_Explorer.root);
                                }
                                catch (...)
                                {
                                    ImGui::TextDisabled("Error rendering explorer tree");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("No game instance found");
                            }
                        }
                        else
                        {
                            ImGui::TextDisabled("Refreshing...");
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ChildPadding, ImVec2(Style.WindowPadding.x, 3.0f));
                if (ImGui::BeginChild("Settings", ImVec2(0, bottom_height), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    if (!g_Explorer.is_refreshing.load())
                    {
                        try
                        {
                            g_Explorer.RenderSettings();
                            g_Explorer.RenderProperties();
                        }
                        catch (...)
                        {
                            ImGui::TextDisabled("Error rendering explorer settings/properties");
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("Refreshing...");
                    }
                    ImGui::EndChild();
                }
                ImGui::PopStyleVar();
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
        }
        ImGui::End();
        if (!explorer_open) Globals::Explorer::Open = false;
    }
}


