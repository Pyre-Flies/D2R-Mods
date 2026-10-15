#pragma once
#include "ring.h"
#include <D2RLPlugin/api.h>
#include <array>

namespace Sockets {
struct RingResources {
    const D2RL::ResourceService* service{};
    std::array<D2RL::Resources::RegistrationHandle,4> handles{};
    void Stop(const D2RL::PluginContext* context) noexcept {
        if(service && service->unregisterResource) for(auto& handle:handles) {
            if(handle) (void)service->unregisterResource(context,handle);
            handle=0;
        }
        service=nullptr;
    }
    bool Start(const D2RL::PluginContext* context) noexcept {
        if(context->QueryService(&service)!=D2RL::ServiceQueryResult::Success ||
           !D2RL::HasResourceServiceField(service,D2RL::ResourceServiceRequiredSize) ||
           !service->registerResource || !service->unregisterResource) {service=nullptr;return false;}
        // Retain the established rim path; the center has its own owned asset.
        constexpr const char* paths[]={
            "data/hd/global/ui/d2rloader/always-show-sockets/empty-outline.sprite",
            "data/hd/global/ui/d2rloader/always-show-sockets/empty-outline.lowend.sprite",
            "data/hd/global/ui/d2rloader/always-show-sockets/socket-center.sprite",
            "data/hd/global/ui/d2rloader/always-show-sockets/socket-center.lowend.sprite"};
        try {
            for(unsigned i=0;i<4;++i) {
                const auto bytes=MakeRingSprite((i%2)?64:128,i<2?RingLayer::Rim:RingLayer::Center);
                const D2RL::Resources::ResourceRegistration registration{
                    .structSize=D2RL::Resources::ResourceRegistrationSize,.path=paths[i],
                    .bytes=bytes.data(),.byteCount=bytes.size()};
                if(service->registerResource(context,&registration,&handles[i])!=D2RL::Resources::Result::Success || !handles[i]) {
                    Stop(context);return false;
                }
            }
        } catch(...) {Stop(context);return false;}
        return true;
    }
};
}
