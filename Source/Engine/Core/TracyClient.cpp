//
//          Tracy profiler
//         ----------------
//
// For fast integration, compile and
// link with this source file (and none
// other) in your executable (or in the
// main DLL / shared object on multi-DLL
// projects).
//

// Define TRACY_ENABLE to enable profiler.

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS

#include "tracy\common\TracySystem.cpp"
#include "tracy\tracy\Tracy.hpp"

#ifdef TRACY_ENABLE

#ifdef _MSC_VER
#  pragma warning(push, 0)
#endif

#include "tracy\common/tracy_lz4.cpp"
#include "tracy\client/TracyProfiler.cpp"
#include "tracy\client/TracyCallstack.cpp"
#include "tracy\client/TracySysPower.cpp"
#include "tracy\client/TracySysTime.cpp"
#include "tracy\client/TracySysTrace.cpp"
#include "tracy\common/TracySocket.cpp"
#include "tracy\client/tracy_rpmalloc.cpp"
#include "tracy\client/TracyDxt1.cpp"
#include "tracy\client/TracyAlloc.cpp"
#include "tracy\client/TracyOverride.cpp"
#include "tracy\client/TracyKCore.cpp"

#ifdef TRACY_ROCPROF
#  include "client/TracyRocprof.cpp"
#endif
#ifdef _MSC_VER
#  pragma comment(lib, "ws2_32.lib")
#  pragma comment(lib, "advapi32.lib")
#  pragma comment(lib, "user32.lib")
#  pragma warning(pop)
#endif

#endif
