#ifndef UNHOARDER_LABEL_VISIBILITY_SERVICE_V1_H
#define UNHOARDER_LABEL_VISIBILITY_SERVICE_V1_H
#include <cstdint>
#include <type_traits>
// Synchronous observation of the item borrowed from the qualified label caller.
// No native pointer is retained. A broker lease owns the callback's lifetime.
namespace UnhoarderLabelVisibility {
inline constexpr char Name[]="unhoarder-label-visibility";
inline constexpr uint32_t Version=1;
// This preflight can only avoid a query. It cannot supply a Hidden proof.
enum class Decision:uint32_t {RequireQuery,CannotHide};
struct Service {
    uint32_t size,version;
    Decision(__cdecl* query)(const void* unit,uint32_t guid) noexcept;
};
inline bool Valid(const Service* service) noexcept {
    return service && service->size>=sizeof(Service) && service->version==Version && service->query;
}
static_assert(sizeof(Service)==16 && std::is_standard_layout_v<Service>);
}

#endif
