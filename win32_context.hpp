#pragma once

#include "bstk.hpp"

#include <windows.h>
#undef CreateWindow

struct Win32Context : public bstk::OSContext
{
    bstk::OSWindow CreateWindow() override;
    bool PumpEvents(bstk::OSWindow& _window) override;

    bstk::EngineModule EngineLoad(char const* _path) override;
    bool EngineReloadRequired(bstk::EngineModule const& _module) override;
    void EngineReloadModule(bstk::EngineModule& _module) override;
};
