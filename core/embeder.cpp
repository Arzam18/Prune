#include "embeder.hpp"

// Android embeds the binary data through embedded_data.S, which uses .incbin.
// This avoids the C++23 #embed directive unsupported by the NDK Clang version
// used by the Android build workflow. Keep the original path for desktop builds.
#if !defined(__ANDROID__)
#ifndef HCE
alignas(64) constexpr unsigned char baseModel[] = {
#embed "model.bin"
};
#endif
alignas(64) constexpr unsigned char magicsData[] = {
#embed "magics.out"
};
#endif
