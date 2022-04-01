#pragma once

#include "bstk.hpp"

#include <windows.h>
#undef CreateWindow

struct Win32Context : public bstk::OSContext
{
    bstk::OSWindow CreateWindow() override;
    bool PumpEvents(bstk::OSWindow& _window) override;

    bstk::EngineModule EngineLoad(std::string const& _path, std::string const& _lockfile) override;
    void EngineRelease(bstk::EngineModule& _module) override;
    bool EngineReloadRequired(bstk::EngineModule const& _module) override;
    void EngineReloadModule(bstk::EngineModule& _module) override;
};
