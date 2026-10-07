#include "owned_payloads.h"
#include <cstdio>
#include <cstdlib>
struct Payload {static inline int alive{};int value;Payload(int n):value(n){++alive;}~Payload(){--alive;}};
void Check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
    QolTasks::Payloads<Payload,2> q;
    auto first=q.Put(std::make_unique<Payload>(1));
    auto second=q.Put(std::make_unique<Payload>(2));
    Check(first && second && Payload::alive==2,"queue owns accepted payloads");
    Check(!q.Put(std::make_unique<Payload>(3)) && Payload::alive==2,"full queue destroys rejected payload");
    auto executing=q.Take(first);
    Check(executing && executing->value==1 && !q.Take(first),"callback owns payload exactly once");
    q.Reset();Check(Payload::alive==1 && !q.Take(second),"session reset frees discarded work without deleting in-flight payload");
    auto newer=q.Put(std::make_unique<Payload>(4));
    Check(newer!=first && newer!=second && !q.Take(second),"old session token cannot claim replacement");
    (void)q.Take(newer);Check(Payload::alive==1,"immediate scheduling failure releases payload");
    executing.reset();Check(Payload::alive==0,"completed callback releases final owner");
    auto switching=q.Put(std::make_unique<Payload>(6));
    auto fallback=q.Take(switching);q.Reset();
    Check(!q.Put(std::move(fallback),switching) && Payload::alive==0,"session change between scheduler failure and fallback cannot requeue stale work");
    {QolTasks::Payloads<Payload> unload;unload.Put(std::make_unique<Payload>(5));}
    Check(Payload::alive==0,"unload frees pending work");
    std::puts("Queued payload ownership, cancellation and stale tokens passed.");
}
