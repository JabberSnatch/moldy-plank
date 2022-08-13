#pragma once

#include "loader/bstk.hpp"
#include "loader/iotk.hpp"

struct XlibContext : public bstk::OSContext
{
    bstk::OSWindow CreateWindow() override;
    bool PumpEvents(bstk::OSWindow& _window, iotk::input_t& _state) override;

    bstk::EngineModule EngineLoad(std::string const& _path, std::string const& _lockfile) override;
    void EngineRelease(bstk::EngineModule& _module) override;
    bool EngineReloadRequired(bstk::EngineModule const& _module) override;
    void EngineReloadModule(bstk::EngineModule& _module) override;
};
