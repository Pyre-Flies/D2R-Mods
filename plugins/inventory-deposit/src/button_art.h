#pragma once

#include <cstddef>

namespace InventoryDepositButtonArt {
struct Blob {
    const void* bytes{};
    std::size_t byteCount{};
};
inline constexpr std::size_t MaximumByteCount=64u*1024u*1024u;

// Views this DLL's embedded RCDATA. The DLL owns the returned memory; the
// ResourceService copies it synchronously before registration returns.
// On failure output is unchanged. No game resource file is opened or changed.
bool Load(unsigned resourceId,Blob& output) noexcept;
}
