#pragma once
#include <cstdint>
#include <windows.h>

namespace D2R {

enum class UnitType : uint32_t {
    Player   = 0,
    Monster  = 1,
    Object   = 2,
    Missile  = 3,
    Item     = 4,
    Tile     = 5
};

enum ItemFlags : uint32_t {
    ITEMFLAG_IDENTIFIED    = 0x00000010,
    ITEMFLAG_SOCKETED      = 0x00000800,
    ITEMFLAG_IN_STORE      = 0x00002000,
    ITEMFLAG_BROKEN        = 0x00020000,
    ITEMFLAG_ETHEREAL      = 0x00400000,
    ITEMFLAG_RUNEWORD      = 0x04000000,
};

enum ContainerType : uint8_t {
    CONTAINER_UNSPECIFIED  = 0,
    CONTAINER_INVENTORY    = 1,
    CONTAINER_TRADE        = 2,
    CONTAINER_CUBE         = 3,
    CONTAINER_STASH        = 4,
    CONTAINER_BELT         = 5
};

// 4-byte packed ASCII item codes (reversed due to little-endian)
// 'ibk ' -> 0x206B6269 (Tome of Identify)
// 'isc ' -> 0x20637369 (Scroll of Identify)
// 'tbk ' -> 0x206B6274 (Tome of Town Portal)
// 'tsc ' -> 0x20637374 (Scroll of Town Portal)
// 'box ' -> 0x20786F62 (Horadric Cube)
constexpr uint32_t ITEM_CODE_IBK = 0x206B6269;
constexpr uint32_t ITEM_CODE_ISC = 0x20637369;
constexpr uint32_t ITEM_CODE_TBK = 0x206B6274;
constexpr uint32_t ITEM_CODE_TSC = 0x20637374;
constexpr uint32_t ITEM_CODE_BOX = 0x20786F62;

#pragma pack(push, 1)
struct ItemData {
    uint32_t itemQuality;
    uint32_t itemFlags;
    uint32_t itemFlagsEx;
    uint32_t actionStamp;
    uint32_t fileIndex;
    uint32_t itemInitSeed;
    uint32_t commandFlag;
    uint8_t  itemLocation;
    uint8_t  nodePage;
    uint8_t  pad0[2];
    uint32_t itemFormat;
    uint32_t earLevel;
    uint32_t invPage;
    uint8_t  cellX;
    uint8_t  cellY;
    uint8_t  pad1[2];
};

struct UnitAny {
    UnitType dwType;
    uint32_t dwClassId;
    uint32_t dwUnitId;
    uint32_t dwAnimMode;
    union {
        void*     pPlayerData;
        ItemData* pItemData;
        void*     pMonsterData;
        void*     pObjectData;
    };
    uint8_t  nAct;
    uint8_t  pad0[3];
    void*    pAct;
    uint64_t dwSeed[2];
    void*    pInitSeed;
    void*    pPath;
    void*    pAnimSeq;
    uint32_t dwAnimFrame;
    uint32_t dwAnimSpeed;
    void*    pGfxSeq;
    void*    pGfxInfo;
    void*    pInventory;
    UnitAny* pPrevUnit;
    UnitAny* pNextUnit;
};

struct InventoryNode {
    UnitAny* pItem;
    InventoryNode* pNext;
};

struct Inventory {
    uint32_t dwSignature;
    void*    pMemPool;
    UnitAny* pOwner;
    UnitAny* pFirstItem;
    UnitAny* pLastItem;
    uint32_t dwItemCount;
    // ... additional container grids
};

#pragma pack(pop)

// D2RLoader Plugin ABI v2
#pragma pack(push, 8)
struct D2RLoaderPluginInfo {
    uint32_t structSize;      // sizeof(D2RLoaderPluginInfo)
    uint32_t abiVersion;      // 2
    const char* id;           // lowercase, digits, '.', '_', '-'
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    uint32_t role;            // 0x04 = Client, 0x08 = Server, 0x0C = Shared
    uint32_t flags;           // 0
};
#pragma pack(pop)

} // namespace D2R
