#pragma once

namespace BackendSelector
{
    enum Mode
    {
        Usermode,
        KernelMode
    };

    Mode Show();
}
