#pragma once
#include "IMenuRenderer.h"

class GoodMenuRenderer : public IMenuRenderer {
public:
    GoodMenuRenderer();
    virtual const char* Name() const override { return "GOOD"; }
    virtual void Render() override;

private:
    bool m_Initialized = false;
    void Initialize();
};
