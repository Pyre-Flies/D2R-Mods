#pragma once
#include <cstdint>
namespace ControllerQoL {
struct PhysicalInput {
    bool valid = false;
    uint16_t buttons = 0;
    uint8_t leftTrigger = 0, rightTrigger = 0;
    unsigned providers = 0;
};
// Do not stop at a connected-but-idle provider: Steam Input may expose the
// actual controller through a different XInput version.
template<class Poll> PhysicalInput CollectPhysicalInput(Poll poll) noexcept {
    PhysicalInput result;
    for (unsigned provider = 0; provider < 3; ++provider) {
        for (unsigned user = 0; user < 4; ++user) {
            PhysicalInput sample;
            if (!poll(provider, user, sample)) continue;
            result.valid = true;
            result.providers |= 1u << provider;
            result.buttons |= sample.buttons;
            if (sample.leftTrigger > result.leftTrigger) result.leftTrigger = sample.leftTrigger;
            if (sample.rightTrigger > result.rightTrigger) result.rightTrigger = sample.rightTrigger;
        }
    }
    return result;
}
PhysicalInput ReadControllerInput() noexcept;
}
