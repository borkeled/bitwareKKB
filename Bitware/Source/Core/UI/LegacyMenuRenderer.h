#pragma once
#include "IMenuRenderer.h"

class LegacyMenuRenderer : public IMenuRenderer {
public:
    LegacyMenuRenderer() = default;
    virtual const char* Name() const override { return "Legacy"; }
    virtual void Render() override;
};
