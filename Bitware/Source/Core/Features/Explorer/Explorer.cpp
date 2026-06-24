#include "Explorer.h"
#include <Globals.hxx>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/addons/imgui_addons.h>
#include <Engine/Engine.h>
#include <Infrastructure/Obfuscation.h>
#include <Auth/skStr.h>
#include <Core/Features/Cache/Cache.h>

void Explorer::Explorer::FreeTree(Node* node)
{
    OBF_PROLOGUE;
    if (!node) return;

    for (Node* child : node->children)
    {
        if (child)
        {
            FreeTree(child);
            delete child;
        }
    }
    node->children.clear();
}

void Explorer::Explorer::BuildTree()
{
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;

    Node* old_root = root;
    Node* old_selected = selected_node;

    selected_node = nullptr;
    root = nullptr;

    if (old_root)
    {
        FreeTree(old_root);
        delete old_root;
    }

    if (!Globals::Datamodel.Address)
    {
        is_refreshing = false;
        return;
    }

    root = new Node{};
    root->parent = nullptr;
    root->instance = Globals::Datamodel;
    root->name = Globals::Datamodel.Name();
    root->class_name = Globals::Datamodel.Class();

    auto build_tree = [&](Node* parent, SDK::Instance& instance, auto& self_ref) -> void
    {
        if (!parent) return;

        Node* child_node = new Node{};
        child_node->parent = parent;
        child_node->instance = instance;
        child_node->name = instance.Name();
        child_node->class_name = instance.Class();

        parent->children.push_back(child_node);

        auto children = instance.Children();
        for (auto& child_instance : children)
        {
            self_ref(child_node, child_instance, self_ref);
        }
    };

    auto children = Globals::Datamodel.Children();
    for (auto& instance : children)
    {
        build_tree(root, instance, build_tree);
    }

    is_refreshing = false;
}

void Explorer::Explorer::RenderNode(Node* node)
{
    if (!node || !root) return;

    char buf[512];
    snprintf(buf, sizeof(buf), std::string(skCrypt("%s [%s]")).c_str(),
        node->name.c_str(),
        node->class_name.c_str()
    );

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    ImVec4 accent_color = ImVec4(0.0f, 0.42f, 0.0f, 1.0f);
    ImVec4 hover_color = ImVec4(0.0f, 0.42f, 0.0f, 0.3f);

    bool is_selected = (node == selected_node);
    int colors_pushed = 0;

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hover_color);
    colors_pushed++;

    if (node->children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (is_selected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
        colors_pushed += 2;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 5.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 1.0f));

    bool open = ImGui::TreeNodeEx((void*)node, flags, "");
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        selected_node = node;
    }

    ImGui::Text("%s", buf);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(colors_pushed);

    if (open)
    {
        for (Node* child : node->children)
        {
            RenderNode(child);
        }

        if (!node->children.empty())
        {
            ImGui::TreePop();
        }
    }
}

void Explorer::Explorer::RenderProperties()
{
    OBF_PROLOGUE;

    if (this->selected_node != nullptr)
    {
        this->RenderPath();
        this->RenderAddress();

        std::string class_name = this->selected_node->instance.Class();

        if (class_name.find(std::string(skCrypt("Part"))) != std::string::npos)
        {
            this->RenderPartProperties();
        }
        else if (class_name == std::string(skCrypt("Model")) || class_name == std::string(skCrypt("Actor")))
        {
            this->RenderModelTeleport();
        }
        else if (class_name == std::string(skCrypt("LocalScript")) || class_name == std::string(skCrypt("ModuleScript")) || class_name == std::string(skCrypt("Script")))
        {
            this->RenderScriptViewer();
        }
    }
    else
    {
        ImGui::TextDisabled(std::string(skCrypt("No node selected")).c_str());
    }
}

void Explorer::Explorer::RenderSettings()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

    bool refreshing = is_refreshing.load();
    if (refreshing)
    {
        ImGui::BeginDisabled();
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button(std::string(skCrypt("Refresh Explorer")).c_str()))
    {
        if (!is_refreshing.exchange(true))
        {
            std::thread([this]()
            {
                OBF_PROLOGUE;
                this->BuildTree();
            }).detach();
        }
    }

    ImGui::PopStyleColor(4);

    if (refreshing)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopStyleVar();
}

void Explorer::Explorer::RenderPath()
{
    std::string path = this->selected_node->GetPath();

    ImGuiStyle& style = ImGui::GetStyle();
    float available_width = ImGui::GetContentRegionAvail().x;

    const char* path_label = std::string(skCrypt("Path: ")).c_str();
    ImVec2 label_size = ImGui::CalcTextSize(path_label);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImVec2 button_size = {
            ImGui::CalcTextSize(std::string(skCrypt("copy")).c_str()).x + ImVec2(style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f).x,
            ImGui::CalcTextSize(std::string(skCrypt("copy")).c_str()).y + ImVec2(style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f).y
        };
    ImGui::PopStyleVar();

    float max_path_width = available_width - label_size.x - button_size.x - style.ItemSpacing.x;

    std::string display_path = path;
    ImVec2 path_text_size = ImGui::CalcTextSize(path.c_str());

    if (path_text_size.x > max_path_width)
    {
        display_path.clear();
        const char* path_cstr = path.c_str();
        int path_len = static_cast<int>(path.length());

        for (int i = path_len; i > 0; --i)
        {
            std::string test_path = std::string(skCrypt("...")) + std::string(path_cstr + (path_len - i));
            ImVec2 test_size = ImGui::CalcTextSize(test_path.c_str());

            if (test_size.x <= max_path_width)
            {
                display_path = test_path;
                break;
            }
        }

        if (display_path.empty())
        {
            display_path = std::string(skCrypt("..."));
        }
    }

    ImGui::Text("%s%s", path_label, display_path.c_str());
    ImGui::SameLine();

    if (ImGui::SmallButton(std::string(skCrypt("copy##path")).c_str()))
    {
        ImGui::SetClipboardText(path.c_str());
    }
}

void Explorer::Explorer::RenderAddress()
{
    std::uint64_t address = this->selected_node->instance.Address;

    ImGui::Text(std::string(skCrypt("Address: 0x%llx")).c_str(), address);
    ImGui::SameLine();

    if (ImGui::SmallButton(std::string(skCrypt("copy##address")).c_str()))
    {
        char buf[32];
        snprintf(buf, sizeof(buf), std::string(skCrypt("0x%llx")).c_str(), address);
        ImGui::SetClipboardText(buf);
    }
}

void Explorer::Explorer::RenderPartProperties()
{
    OBF_PROLOGUE;

    SDK::Instance instance = this->selected_node->instance;
    SDK::Part part(instance.Address);
    SDK::Part primitive = part.Get_Primitive();

    if (!primitive.Address)
    {
        ImGui::TextDisabled(std::string(skCrypt("No primitive found")).c_str());
        return;
    }

    SDK::Vector3 position = primitive.Get_Position();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button(std::string(skCrypt("TELEPORT")).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        for (auto& p : *Globals::Player_Cache)
        {
            if (p.Local_Player && p.HumanoidRootPart.Address)
            {
                SDK::Vector3 target_pos = position;
                target_pos.y += 3.0f;
                SDK::Part local_part(p.HumanoidRootPart.Address);
                for (int w = 0; w < 10000; w++)
                {
                    local_part.Set_PartPosition(target_pos);
                    local_part.Write_Velocity(SDK::Vector3(0, 0, 0));
                }
            }
        }
    }

    ImGui::PopStyleColor(4);
}

void Explorer::Explorer::RenderModelTeleport()
{
    OBF_PROLOGUE;

    SDK::Instance instance = this->selected_node->instance;

    uintptr_t primary_part = Driver->Read<uintptr_t>(instance.Address + Offsets::Model::PrimaryPart);
    uintptr_t prim_addr = 0;
    SDK::Vector3 position{};

    if (primary_part && primary_part > 0x10000)
    {
        prim_addr = Driver->Read<uintptr_t>(primary_part + Offsets::BasePart::Primitive);
        if (prim_addr)
            position = Driver->Read<SDK::Vector3>(prim_addr + Offsets::Primitive::Position);
    }

    if (!prim_addr)
    {
        try
        {
            auto children = instance.Children();
            for (auto& child : children)
            {
                if (!child.Address) continue;
                std::string cn;
                cn = child.Class();
                if (cn.find(std::string(skCrypt("Part"))) != std::string::npos)
                {
                    SDK::Part part(child.Address);
                    SDK::Part prim = part.Get_Primitive();
                    if (prim.Address)
                    {
                        prim_addr = prim.Address;
                        position = prim.Get_Position();
                        break;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    if (!prim_addr)
    {
        ImGui::TextDisabled(std::string(skCrypt("No positionable part found")).c_str());
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button(std::string(skCrypt("TELEPORT")).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        for (auto& p : *Globals::Player_Cache)
        {
            if (p.Local_Player && p.HumanoidRootPart.Address)
            {
                SDK::Vector3 target_pos = position;
                target_pos.y += 3.0f;
                SDK::Part local_part(p.HumanoidRootPart.Address);
                for (int w = 0; w < 10000; w++)
                {
                    local_part.Set_PartPosition(target_pos);
                    local_part.Write_Velocity(SDK::Vector3(0, 0, 0));
                }
            }
        }
    }

    ImGui::PopStyleColor(4);
}

void Explorer::Explorer::RenderScriptViewer()
{
    OBF_PROLOGUE;

    SDK::Instance instance = this->selected_node->instance;
    uintptr_t instance_addr = instance.Address;
    std::string class_name = instance.Class();

    // bytecode field offset based on script type
    uintptr_t bytecode_offset = 0;
    if (class_name == std::string(skCrypt("ModuleScript")))
        bytecode_offset = Offsets::ModuleScript::ByteCode;
    else if (class_name == std::string(skCrypt("LocalScript")) || class_name == std::string(skCrypt("Script")))
        bytecode_offset = Offsets::LocalScript::ByteCode; 

    if (!bytecode_offset)
    {
        ImGui::TextDisabled(std::string(skCrypt("Unknown script type")).c_str());
        return;
    }

    // invalidate cache if selection changed
    if (cached_script_address != instance_addr)
    {
        script_cache_valid = false;
    }

    ImGui::SeparatorText(std::string(skCrypt("Script Bytecode")).c_str());

    // bbbutton to read bytecode
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.42f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.78f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button(std::string(skCrypt("Read Bytecode")).c_str()))
    {
        // read the bytecode wrapper object
        uintptr_t bytecode_wrapper = Driver->Read<uintptr_t>(instance_addr + bytecode_offset);
        if (bytecode_wrapper && bytecode_wrapper > 0x10000)
        {
            // read pointer to bytecode data
            uintptr_t bytecode_ptr = Driver->Read<uintptr_t>(bytecode_wrapper + Offsets::ByteCode::Pointer);
            // read size
            uint64_t bytecode_size = Driver->Read<uint64_t>(bytecode_wrapper + Offsets::ByteCode::Size);

            if (bytecode_ptr && bytecode_ptr > 0x10000 && bytecode_size > 0 && bytecode_size < 0x1000000)
            {
                // read the raw bytecode data
                std::string raw_data;
                raw_data.resize(bytecode_size);
                if (DriverReadVirtualMemory) {
                    DriverReadVirtualMemory(Driver->Get_Handle(), reinterpret_cast<void*>(bytecode_ptr), raw_data.data(), static_cast<ULONG>(bytecode_size), nullptr);
                }

                cached_script_bytes = raw_data;
                cached_script_class = class_name;
                cached_script_address = instance_addr;
                script_cache_valid = true;
            }
            else
            {
                script_cache_valid = false;
            }
        }
        else
        {
            script_cache_valid = false;
        }
    }

    ImGui::PopStyleColor(4);

    ImGui::SameLine();

    if (ImGui::Button(std::string(skCrypt("Copy Bytecode")).c_str()))
    {
        if (script_cache_valid && !cached_script_bytes.empty())
        {
            ImGui::SetClipboardText(cached_script_bytes.c_str());
        }
    }

    if (script_cache_valid && !cached_script_bytes.empty())
    {
        ImGui::Spacing();
        ImGui::Text(std::string(skCrypt("Bytecode Size: %zu bytes")).c_str(), cached_script_bytes.size());

        static int bytecode_view_tab = 0;
        ImGui::Spacing();
        if (ImGui::SmallButton(std::string(skCrypt("Hex Dump")).c_str())) bytecode_view_tab = 0;
        ImGui::SameLine();
        if (ImGui::SmallButton(std::string(skCrypt("Extracted Strings")).c_str())) bytecode_view_tab = 1;

        ImGui::Spacing();

        if (ImGui::BeginChild("ScriptBytecodeView", ImVec2(0, 300), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const size_t bytes = cached_script_bytes.size();

            if (bytecode_view_tab == 0)
            {
                // Hex dump view
                const size_t bytes_per_line = 16;
                std::string hex_dump;

                for (size_t i = 0; i < bytes; i += bytes_per_line)
                {
                    char offset_str[16];
                    snprintf(offset_str, sizeof(offset_str), "%08zx  ", i);
                    hex_dump += offset_str;

                    size_t line_len = (std::min)(bytes_per_line, bytes - i);
                    for (size_t j = 0; j < bytes_per_line; j++)
                    {
                        if (j < line_len)
                        {
                            char byte_str[8];
                            snprintf(byte_str, sizeof(byte_str), "%02x ", (unsigned char)cached_script_bytes[i + j]);
                            hex_dump += byte_str;
                        }
                        else
                        {
                            hex_dump += "   ";
                        }
                    }

                    hex_dump += " |";

                    for (size_t j = 0; j < line_len; j++)
                    {
                        unsigned char c = (unsigned char)cached_script_bytes[i + j];
                        if (c >= 32 && c <= 126)
                            hex_dump += c;
                        else
                            hex_dump += '.';
                    }

                    hex_dump += "|\n";
                }

                ImGui::TextUnformatted(hex_dump.c_str());
            }
            else
            {
                std::string extracted;
                int string_count = 0;

                for (size_t i = 0; i < bytes; i++)
                {
                    if ((unsigned char)cached_script_bytes[i] >= 32 &&
                        (unsigned char)cached_script_bytes[i] <= 126)
                    {
                        std::string str;
                        size_t j = i;
                        while (j < bytes &&
                               (unsigned char)cached_script_bytes[j] >= 32 &&
                               (unsigned char)cached_script_bytes[j] <= 126)
                        {
                            str += cached_script_bytes[j];
                            j++;
                        }

                        if (str.length() >= 3 && str.find_first_of(" \t\n\r") == std::string::npos)
                        {
                            char line[512];
                            snprintf(line, sizeof(line), "[%04zx] %s\n", i, str.c_str());
                            extracted += line;
                            string_count++;
                        }

                        i = j; // skip past
                    }
                }

                if (string_count > 0)
                {
                    char header[64];
                    snprintf(header, sizeof(header), std::string(skCrypt("Found %d strings:\n\n")).c_str(), string_count);
                    ImGui::TextUnformatted(header);
                    ImGui::TextUnformatted(extracted.c_str());
                }
                else
                {
                    ImGui::TextDisabled(std::string(skCrypt("No recognizable strings found")).c_str());
                }
            }
        }
        ImGui::EndChild();
    }
    else
    {
        ImGui::Spacing();
        ImGui::TextDisabled(std::string(skCrypt("Click 'Read Bytecode' to load script data")).c_str());
    }
}