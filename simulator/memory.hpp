#pragma once
#include <fstream>
#include <sstream>
#define ToBig(x) (__builtin_bswap32(x))

class memory {
public:
    void Init(int, char**);
    void LoadInstr();
    uint32_t LoadData();
    const uint32_t& getPC() const { return PC_; }
    void setPC(const uint32_t& rhs) { PC_ = rhs; }
    const uint32_t getInstr();
    const uint32_t loadWord(const size_t) const;
    const uint32_t loadHalfWord(const size_t) const;
    const uint32_t loadByte(const size_t) const;
    void saveWord(const size_t, const uint32_t);
    void saveHalfWord(const size_t, const uint32_t);
    void saveByte(const size_t, const uint32_t);

private:
    uint32_t PC_ = 0, PC0_ = 0;
    size_t icount_ = 0, dcount_ = 0,
        IMemorySize_, DMemorySize_, IPageSize_, DPageSize_,
        ICacheSize_, IBlockSize_, IAssociativity_,
        DCacheSize_, DBlockSize_, DAssociativity_;
    uint32_t IDisk_[1024] = {}, DDisk_[1024] = {}, argDefault_[10] = {
        64, 32, 8, 16, 16, 4, 4, 16, 4, 0
    };
    uint32_t ICacheHit_ = 0, ICacheMiss_ = 0, DCacheHit_ = 0, DCacheMiss_ = 0,
        ITLBHit_ = 0, ITLBMiss_ = 0, DTLBHit_ = 0, DTLBMiss_ = 0,
        IPageTableHit_ = 0, IPageTableMiss_ = 0,
        DPageTableHit_ = 0, DPageTableMiss_ = 0;
    struct TLB_entry {
        uint32_t VPN = 0, PPN = 0, LRU = 0;
        bool valid = false;
    } ITLB[1024], DTLB[1024];
    struct PTE {
        uint32_t PPN = 0;
        bool valid = false;
    } IPageTable_[1024], DPageTable_[1024];
    struct Cache_entry {
        uint32_t tag = 0;
        bool valid = false, MRU = false;
    } ICache_[1024], DCache_[1024];
    struct Memory_entry {
        uint32_t LRU = 0;
        bool valid = false;
    } IMemory_[1024], DMemory_[1024];
};
