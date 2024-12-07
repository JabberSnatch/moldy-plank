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
    bstk::PlatformData EngineReloadModule(bstk::EngineModule& _module) override;
    void EngineReleasePlatformData(bstk::PlatformData _data) override;

    std::unordered_map<HWND, bstk::OSWindow> windows = {};
};
