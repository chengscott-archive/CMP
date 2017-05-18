#pragma once
#include <fstream>
#include <sstream>
#define ToBig(x) (__builtin_bswap32(x))

class memory {
public:
    void Init(int, char**);
    void LoadInstr();
    uint32_t LoadData();
    void DumpReport() const;
    const uint32_t& getPC() const { return PC_; }
    void setPC(const uint32_t& rhs) { PC_ = rhs; }
    const uint32_t getInstr();
    const uint32_t loadWord(const size_t);
    const uint32_t loadHalfWord(const size_t);
    const uint32_t loadByte(const size_t);
    void saveWord(const size_t, const uint32_t);
    void saveHalfWord(const size_t, const uint32_t);
    void saveByte(const size_t, const uint32_t);

private:
    uint32_t cycle_ = 0, PC_ = 0, PC0_ = 0, IDisk_[1024] = {}, DDisk_[1024] = {};
    size_t icount_ = 0, dcount_ = 0,
        sizeDefault_[10] = { 64, 32, 8, 16, 16, 4, 4, 16, 4, 1 };
    struct SizeInfo {
        size_t page, Memory, PT, TLB, cacheBlock, cacheIndex, cacheTag;
        uint32_t cacheNWay, cachePerWay;
    } ISize_, DSize_;
    struct Info {
        uint32_t PT = 0, TLB = 0, Cache = 0;
    } IHit_, IMiss_, DHit_, DMiss_;
    struct Memory_entry {
        uint32_t LRU = 0;
        bool valid = false;
    } IMemory_[1024], DMemory_[1024];
    struct PTE {
        uint32_t PPN = 0;
        bool valid = false;
    } IPageTable_[1024], DPageTable_[1024];
    struct TLB_entry {
        uint32_t VPN = 0, PPN = 0, LRU = 0;
        bool valid = false;
    } ITLB_[1024], DTLB_[1024];
    struct Cache_entry {
        uint32_t tag = 0;
        bool valid = false, MRU = false;
    } ICache_[1024][1024], DCache_[1024][1024];
    void HitMiss(const bool, const uint32_t);
};
