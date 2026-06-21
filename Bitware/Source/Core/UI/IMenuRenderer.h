#pragma once

class IMenuRenderer {
public:
    virtual ~IMenuRenderer() = default;
    virtual const char* Name() const = 0;
    virtual void Render() = 0;
};
