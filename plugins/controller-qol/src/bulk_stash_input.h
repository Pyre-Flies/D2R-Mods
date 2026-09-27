#pragma once
namespace QolBulkStash {
struct Gesture {
    bool latched{},consumed{};
    template<class Request> bool Update(bool modifier,bool stick,Request request) noexcept {
        if(!modifier || !stick)latched=false;
        if(!stick)consumed=false;
        if(modifier && stick && !latched){latched=true;if(request())consumed=true;}
        return consumed;
    }
};
}
