#include "GoodMenuRenderer.h"
#include <Core/UI/GOOD/headers/includes.h>

GoodMenuRenderer::GoodMenuRenderer()
{
}

void GoodMenuRenderer::Initialize()
{
    if (m_Initialized)
        return;

    m_Initialized = true;

    var->gui.strict_license = false;
    var->gui.tab = 1;
    var->gui.tab_stored = 1;

    elements->window.name = "Bitware";
}

void GoodMenuRenderer::Render()
{
    Initialize();
    gui->render();
}
