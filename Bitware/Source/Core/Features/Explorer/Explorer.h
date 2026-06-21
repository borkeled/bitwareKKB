#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <Engine/Engine.h>

namespace Explorer {

struct Node {
    Node* parent;
    SDK::Instance instance;
    std::vector<Node*> children;
    std::string name;
    std::string class_name;

    std::string GetPath() const {
        if (!parent)
            return instance.Name();
        return parent->GetPath() + "." + instance.Name();
    }
};

class Explorer {
public:
    void BuildTree();
    void FreeTree(Node* node);
    void RenderNode(Node* node);
    void RenderProperties();
    void RenderSettings();
    void RenderPath();
    void RenderAddress();
    void RenderPartProperties();
    void RenderModelTeleport();
    void RenderScriptViewer();

    Node* root = nullptr;
    Node* selected_node = nullptr;
    std::atomic<bool> is_refreshing = false;

    // Cached script data
    std::string cached_script_bytes;
    std::string cached_script_class;
    uintptr_t cached_script_address = 0;
    bool script_cache_valid = false;
};

}