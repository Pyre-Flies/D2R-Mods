#pragma once
#include <D2RLPlugin/item.h>
namespace QolShared {
inline bool SameItem(const D2RL::Items::ItemInfo& expected,const D2RL::Items::ItemInfo& actual) noexcept {
    using C=D2RL::Items::ItemContainer;
    return expected.container==C::SharedStash && actual.container==C::SharedStash &&
        expected.runtimeId==actual.runtimeId && expected.code==actual.code && expected.classId==actual.classId &&
        (expected.sharedStashPage==UINT32_MAX || actual.sharedStashPage==expected.sharedStashPage);
}
}
