#pragma once

#ifdef TRACY_ENABLE

#include "tracy\tracy\Tracy.hpp"
#include <cstring>

#if defined(_DEBUG) || defined(BS_DEVELOPMENT_BUILD)
#define BS_PROFILE_DETAILED_ENABLED 1
#else
#define BS_PROFILE_DETAILED_ENABLED 0
#endif

#define BS_PROFILE_SCOPE(name_) ZoneScopedN(name_)
#define BS_PROFILE_FUNCTION() ZoneScoped
#define BS_PROFILE_FRAME_MARK() FrameMark
#define BS_PROFILE_PLOT(name_, ...) TracyPlot(name_, static_cast<double>((__VA_ARGS__)))
#define BS_PROFILE_TEXT(text_, size_) TracyMessage(text_, size_)

#define BS_PROFILE_SCOPE_DYNAMIC(name_) \
    ZoneScoped;                         \
    ZoneName(name_, std::strlen(name_))

#else

#define BS_PROFILE_SCOPE(name_)
#define BS_PROFILE_FUNCTION()
#define BS_PROFILE_FRAME_MARK()
#define BS_PROFILE_PLOT(name_, ...)
#define BS_PROFILE_TEXT(text_, size_)

#define BS_PROFILE_SCOPE_DYNAMIC(name_)
#define BS_PROFILE_DETAILED_ENABLED 0

#endif
