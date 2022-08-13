#pragma once

#include "loader/bstk.hpp"
#include "loader/iotk.hpp"

#include <windows.h>
#undef CreateWindow

#include <unordered_map>

struct Win32Context : public bstk::OSContext
{
    bstk::OSWindow CreateWindow() override;
    bool PumpEvents(bstk::OSWindow& _window, iotk::input_t& _state) override;

    bstk::EngineModule EngineLoad(std::string const& _path, std::string const& _lockfile) override;
    void EngineRelease(bstk::EngineModule& _module) override;
    bool EngineReloadRequired(bstk::EngineModule const& _module) override;
    void EngineReloadModule(bstk::EngineModule& _module) override;

    std::unordered_map<HWND, bstk::OSWindow> windows = {};
};
