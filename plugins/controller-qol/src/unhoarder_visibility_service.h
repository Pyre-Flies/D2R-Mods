#pragma once
#include <cstdint>
#include <type_traits>
// Read-only presentation decision; never authorizes or blocks native pickup.
namespace UnhoarderVisibility {
inline constexpr char Name[]="unhoarder-ground-visibility";
inline constexpr uint32_t Version=1;
enum class Decision:uint32_t { Unknown,Visible,Hidden };
struct Service {
 uint32_t size,version;
 Decision(__cdecl* query)(const void* unit,uint32_t guid,uint32_t code) noexcept;
};
inline bool Valid(const Service* s) noexcept {
 return s && s->size>=sizeof(Service) && s->version==Version && s->query;
}
static_assert(sizeof(Service)==16 && std::is_standard_layout_v<Service>);
}
