#include "memory.hpp"

void memory::Init(int argc, char** argv) {
    std::stringstream ss;
    for (int i = 1; i < argc; ++i) ss << argv[i] << " ";
    for (int i = argc - 1; i < 10; ++i) ss << sizeDefault_[i] << " ";
    ss >> ISize_.memory >> DSize_.memory >> ISize_.page >> DSize_.page
        >> ISize_.cache >> ISize_.cacheBlock >> ISize_.cacheNWay
        >> DSize_.cache >> DSize_.cacheBlock >> DSize_.cacheNWay;
    // Calculate Size
    ISize_.PT = 1024/ISize_.page;
    DSize_.PT = 1024/DSize_.page;
    ISize_.TLB = ISize_.PT/4;
    DSize_.TLB = DSize_.PT/4;
    uint32_t lg_[2048] = {};
    for (int i = 1; i <= 10; ++i) lg_[1 << i] = i;
    ISize_.cacheOffset = lg_[ISize_.cacheBlock];
    ISize_.cacheIndex = lg_[ISize_.cache];
    ISize_.cacheTag = 32 - ISize_.cacheIndex - ISize_.cacheOffset;
    ISize_.cachePerWay = ISize_.cache/ISize_.cacheBlock/ISize_.cacheNWay;
    DSize_.cacheOffset = lg_[DSize_.cacheBlock];
    DSize_.cacheIndex = lg_[DSize_.cache];
    DSize_.cacheTag = 32 - DSize_.cacheIndex - DSize_.cacheOffset;
    DSize_.cachePerWay = DSize_.cache/DSize_.cacheBlock/DSize_.cacheNWay;
}

void memory::LoadInstr() {
    FILE* image = fopen("iimage.bin", "rb");
    int v;
    fread(&v, sizeof(int), 1, image);
    PC_ = PC0_ = ToBig(v);
    fread(&v, sizeof(int), 1, image);
    ISize_.disk = ToBig(v);
    for (size_t i = 0; i < ISize_.disk; ++i) {
        fread(&v, sizeof(int), 1, image);
        IDisk_[i] = ToBig(v);
    }
    fclose(image);
}

uint32_t memory::LoadData() {
    FILE* image = fopen("dimage.bin", "rb");
    int v;
    fread(&v, sizeof(int), 1, image);
    uint32_t SP = ToBig(v);
    fread(&v, sizeof(int), 1, image);
    DSize_.disk = ToBig(v);
    for (size_t i = 0; i < 4 * DSize_.disk; ++i) {
        fread(&v, sizeof(char), 1, image);
        DDisk_[i] = ToBig(v) >> 24;
    }
    fclose(image);
    return SP;
}

void memory::HitMiss(const bool type, const uint32_t VA) {
    SizeInfo &size = type ? DSize_ : ISize_;
    Info &hit = type ? DHit_ : IHit_,
        &miss = type ? DMiss_ : IMiss_;
    //PTE *pagetable = type ? DPageTable_ : IPageTable_;
    TLB_entry *TLB = type ? DTLB_ : ITLB_;
    Cache_entry (*cache)[1024] = type ? DCache_ : ICache_;
    uint32_t VPN = VA/size.page, PPN;
    bool isTLBHit = false;
    for (size_t i = 0; i < size.TLB; ++i) {
        if (TLB[i].valid && TLB[i].VPN == VPN) {
            // TLB Hit
            ++hit.TLB;
            TLB[i].LRU = cycle_;
            PPN = TLB[i].PPN;
            isTLBHit = true;
            break;
        }
    }
    if (!isTLBHit) {
        // TLB Miss
        ++miss.TLB;
    }
    uint32_t PA = PPN * size.page + (VA % size.page),
        index = (PA/size.cacheBlock) % size.cache,
        tag = PA/size.cacheBlock/size.cache;
    bool isCacheHit = false;
    size_t MRU;
    for (size_t i = 0; i < size.cacheNWay; ++i) {
        if (cache[index][i].valid && cache[index][i].tag == tag) {
            // Cache Hit
            ++hit.Cache;
            cache[index][i].MRU = true;
            MRU = i;
            isCacheHit = true;
            break;
        }
    }
    // Cache MRU
    bool isMRUfull = true;
    for (size_t i = 0; i < size.cacheNWay && isMRUfull; ++i)
        if (!cache[index][i].MRU)
            isMRUfull = false;
    if (isMRUfull) {
        for (size_t i = 0; i < size.cacheNWay; ++i)
            if (i != MRU)
                cache[index][i].MRU = false;
    }
    if (!isCacheHit) {
        // Cache Miss
        ++miss.Cache;
        // update cache
    }
}

const uint32_t memory::getInstr() {
    ++cycle_;
    uint32_t ret = 0;
    if (PC_ >= PC0_) ret = IDisk_[(PC_ - PC0_) / 4];
    PC_ += 4;
    return ret;
}

const uint32_t memory::loadWord(const size_t rhs) const {
    return (DDisk_[rhs] & 0xff) << 24 |
            (DDisk_[rhs + 1] & 0xff) << 16 |
            (DDisk_[rhs + 2] & 0xff) << 8 |
            (DDisk_[rhs + 3] & 0xff);
}

const uint32_t memory::loadHalfWord(const size_t rhs) const {
    return (DDisk_[rhs] & 0xff) << 8 | (DDisk_[rhs + 1] & 0xff);
}

const uint32_t memory::loadByte(const size_t rhs) const {
    return DDisk_[rhs];
}

void memory::saveWord(const size_t lhs, const uint32_t rhs) {
    DDisk_[lhs] = (rhs >> 24) & 0xff;
    DDisk_[lhs + 1] = (rhs >> 16) & 0xff;
    DDisk_[lhs + 2] = (rhs >> 8) & 0xff;
    DDisk_[lhs + 3] = rhs & 0xff;
}

void memory::saveHalfWord(const size_t lhs, const uint32_t rhs) {
    DDisk_[lhs] = (rhs >> 8) & 0xff;
    DDisk_[lhs + 1] = rhs & 0xff;
}

void memory::saveByte(const size_t lhs, const uint32_t rhs) {
    DDisk_[lhs] = rhs & 0xff;
}
