#pragma once
#include "data_fixture.h"
#include "recorded_deposits.h"
namespace TestData {
inline void AddDeposits(Fixture& f) {
    f.bytes[5].resize(10*364);
    std::memcpy(f.Row(5,8),RecordedGemDeposit.data(),364);
    std::memcpy(f.Row(5,9),RecordedClusterDeposit.data(),364);
    for(unsigned row:{8u,9u}) {
        f.Set(5,row,0x16,f.bag);f.Set(5,row,0x5a,row);f.Set(5,row,0x77,f.property);
    }
    f.Set(5,8,0x20,1u); // Type family containing the fixture amethyst.
    f.Set(5,9,0x20,900u);
    f.Set(0,900,0x80,Fixture::ItemsCode("1gc"));
    f.Row(0,900)[0x11e]=f.Row(0,900)[0x11f]=1;
}
}
