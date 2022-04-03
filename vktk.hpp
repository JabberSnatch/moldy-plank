#pragma once

#include <cstdint>

namespace vktk
{

struct Context;

Context* CreateContext(std::uint64_t _hinstance, std::uint64_t _hwindow);
void DeleteContext(Context* _context);

} // namespace vktk
