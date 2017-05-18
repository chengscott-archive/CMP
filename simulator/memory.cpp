#include "memory.hpp"

void memory::Init(int argc, char** argv) {
    std::stringstream ss;
    for (int i = 1; i < argc; ++i) ss << argv[i] << " ";
    for (int i = argc - 1; i < 10; ++i) ss << sizeDefault_[i] << " ";
    size_t imemory, dmemory, icache, dcache, icacheOffset, dcacheOffset;
    ss >> imemory >> dmemory >> ISize_.page >> DSize_.page
        >> icache >> ISize_.cacheBlock >> ISize_.cacheNWay
        >> dcache >> DSize_.cacheBlock >> DSize_.cacheNWay;
    // Calculate Size
    ISize_.Memory = imemory/ISize_.page;
    DSize_.Memory = dmemory/DSize_.page;
    ISize_.PT = 1024/ISize_.page;
    DSize_.PT = 1024/DSize_.page;
    ISize_.TLB = ISize_.PT/4;
    DSize_.TLB = DSize_.PT/4;
    uint32_t lg_[2048] = {};
    for (int i = 1; i <= 10; ++i) lg_[1 << i] = i;
    ISize_.cachePerWay = (icache/ISize_.cacheBlock)/ISize_.cacheNWay;
    icacheOffset = lg_[ISize_.cacheBlock];
    ISize_.cacheIndex = lg_[ISize_.cachePerWay];
    ISize_.cacheTag = 32 - ISize_.cacheIndex - icacheOffset;
    DSize_.cachePerWay = (dcache/DSize_.cacheBlock)/DSize_.cacheNWay;
    dcacheOffset = lg_[DSize_.cacheBlock];
    DSize_.cacheIndex = lg_[DSize_.cachePerWay];
    DSize_.cacheTag = 32 - DSize_.cacheIndex - dcacheOffset;
}

void memory::LoadInstr() {
    FILE* image = fopen("iimage.bin", "rb");
    int v;
    fread(&v, sizeof(int), 1, image);
    PC_ = PC0_ = ToBig(v);
    fread(&v, sizeof(int), 1, image);
    icount_ = ToBig(v);
    for (size_t i = 0; i < icount_; ++i) {
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
    dcount_ = ToBig(v);
    for (size_t i = 0; i < 4 * dcount_; ++i) {
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
    /* TLB Access */
    uint32_t VPN = VA/size.page, PPN;
    TLB_entry *TLB = type ? DTLB_ : ITLB_;
    bool isTLBMiss = true;
    for (size_t i = 0; i < size.TLB; ++i) {
        if (TLB[i].valid && TLB[i].VPN == VPN) {
            // TLB Hit
            ++hit.TLB;
            TLB[i].LRU = cycle_;
            PPN = TLB[i].PPN;
            isTLBMiss = false;
            break;
        }
    }
    if (isTLBMiss) {
        // TLB Miss
        ++miss.TLB;
        /* PT Access */
        Memory_entry *memory = type ? DMemory_ : IMemory_;
        PTE *pagetable = type ? DPageTable_ : IPageTable_;
        Cache_entry (*cache)[1024] = type ? DCache_ : ICache_;
        if (pagetable[VPN].valid) {
            // PT Hit
            ++hit.PT;
            PPN = pagetable[VPN].PPN;
            memory[PPN].LRU = cycle_;
        } else {
            // PT Miss
            ++miss.PT;
            // update Memory
            bool isMemoryFull = true;
            for (size_t i = 0; i < size.Memory; ++i) {
                if (!memory[i].valid) {
                    memory[i].valid = true;
                    memory[i].LRU = cycle_;
                    PPN = i;
                    isMemoryFull = false;
                    break;
                }
            }
            if (isMemoryFull) {
                size_t cycle = 600000, LRU;
                for (size_t i = 0; i < size.Memory; ++i) {
                    if (memory[i].LRU < cycle)
                        cycle = memory[i].LRU, LRU = i;
                }
                memory[LRU].valid = true;
                memory[LRU].LRU = cycle_;
                PPN = LRU;
                // clear TLB
                for (size_t i = 0; i < size.TLB; ++i) {
                    if (TLB[i].valid && TLB[i].PPN == PPN)
                        TLB[i].valid = false;
                }
                // clear PT
                for (size_t i = 0; i < size.PT; ++i) {
                    if (pagetable[i].valid && pagetable[i].PPN == PPN)
                        pagetable[i].valid = false;
                }
                // clear Cache
                for (size_t i = 0; i < size.cachePerWay; ++i) {
                    for (size_t j = 0; j < size.cacheNWay; ++j) {
                        if (cache[i][j].valid) {
                            uint32_t tPA = (cache[i][j].tag << (32 - size.cacheTag)) |
                                (i << (32 - size.cacheTag - size.cacheIndex));
                            if (tPA / size.page == PPN) {
                                cache[i][j].valid = cache[i][j].MRU = false;
                            }
                        }
                    }
                }
            }
            // update PT
            pagetable[VPN].valid = true;
            pagetable[VPN].PPN = PPN;
        }
        // update TLB
        bool isTLBfull = true;
        for (size_t i = 0; i < size.TLB; ++i) {
            if (!TLB[i].valid) {
                TLB[i].valid = true;
                TLB[i].VPN = VPN;
                TLB[i].PPN = PPN;
                TLB[i].LRU = cycle_;
                isTLBfull = false;
                break;
            }
        }
        if (isTLBfull) {
            size_t cycle = 600000, LRU;
            for (size_t i = 0; i < size.TLB; ++i) {
                if (TLB[i].LRU < cycle)
                    cycle = TLB[i].LRU, LRU = i;
            }
            TLB[LRU].valid = true;
            TLB[LRU].VPN = VPN;
            TLB[LRU].PPN = PPN;
            TLB[LRU].LRU = cycle_;
        }
    }
    /* Cache Access */
    const uint32_t PA = PPN * size.page + (VA % size.page),
        index = (PA/size.cacheBlock) % size.cachePerWay,
        tag = PA/size.cacheBlock/size.cachePerWay,
        cacheNWay = size.cacheNWay;
    Cache_entry *cacheIndex = (type ? DCache_ : ICache_)[index];
    bool isCacheMiss = true;
    size_t MRU;
    for (size_t i = 0; i < cacheNWay; ++i) {
        if (cacheIndex[i].valid && cacheIndex[i].tag == tag) {
            // Cache Hit
            ++hit.Cache;
            cacheIndex[i].MRU = true;
            MRU = i;
            isCacheMiss = false;
            break;
        }
    }
    if (isCacheMiss) {
        // Cache Miss
        ++miss.Cache;
        MRU = tag;
        bool isAllValid = true;
        // find not valid
        for (size_t i = 0; i < cacheNWay; ++i) {
            if (!cacheIndex[i].valid) {
                cacheIndex[i].tag = tag;
                cacheIndex[i].valid = cacheIndex[i].MRU = true;
                isAllValid = false;
                break;
            }
        }
        if (isAllValid) {
            // use MRU
            for (size_t i = 0; i < cacheNWay; ++i) {
                if (!cacheIndex[i].MRU) {
                    cacheIndex[i].tag = tag;
                    cacheIndex[i].valid = cacheIndex[i].MRU = true;
                    MRU = i;
                    break;
                }
            }
        }
    }
    // update Cache MRU
    bool isMRUfull = true;
    for (size_t i = 0; i < cacheNWay; ++i) {
        if (!cacheIndex[i].MRU) {
            isMRUfull = false;
            break;
        }
    }
    if (isMRUfull) {
        for (size_t i = 0; i < cacheNWay; ++i)
            if (i != MRU || cacheNWay == 1)
                cacheIndex[i].MRU = false;
    }
}

void memory::DumpReport() const {
    FILE *report = fopen("report.rpt", "w");
    fprintf(report,
        "ICache :\n"
        "# hits: %u\n"
        "# misses: %u\n\n"
        "DCache :\n"
        "# hits: %u\n"
        "# misses: %u\n\n"
        "ITLB :\n"
        "# hits: %u\n"
        "# misses: %u\n\n"
        "DTLB :\n"
        "# hits: %u\n"
        "# misses: %u\n\n"
        "IPageTable :\n"
        "# hits: %u\n"
        "# misses: %u\n\n"
        "DPageTable :\n"
        "# hits: %u\n"
        "# misses: %u\n\n",
        IHit_.Cache, IMiss_.Cache,
        DHit_.Cache, DMiss_.Cache,
        IHit_.TLB, IMiss_.TLB,
        DHit_.TLB, DMiss_.TLB,
        IHit_.PT, IMiss_.PT,
        DHit_.PT, DMiss_.PT
    );
    fclose(report);
}

const uint32_t memory::getInstr() {
    ++cycle_;
    uint32_t ret = 0;
    if (PC_ >= PC0_) {
        HitMiss(false, PC_);
        ret = IDisk_[(PC_ - PC0_) / 4];
    }
    PC_ += 4;
    return ret;
}

const uint32_t memory::loadWord(const size_t rhs) {
    HitMiss(true, rhs);
    return (DDisk_[rhs] & 0xff) << 24 |
            (DDisk_[rhs + 1] & 0xff) << 16 |
            (DDisk_[rhs + 2] & 0xff) << 8 |
            (DDisk_[rhs + 3] & 0xff);
}

const uint32_t memory::loadHalfWord(const size_t rhs) {
    HitMiss(true, rhs);
    return (DDisk_[rhs] & 0xff) << 8 | (DDisk_[rhs + 1] & 0xff);
}

const uint32_t memory::loadByte(const size_t rhs) {
    HitMiss(true, rhs);
    return DDisk_[rhs];
}

void memory::saveWord(const size_t lhs, const uint32_t rhs) {
    HitMiss(true, lhs);
    DDisk_[lhs] = (rhs >> 24) & 0xff;
    DDisk_[lhs + 1] = (rhs >> 16) & 0xff;
    DDisk_[lhs + 2] = (rhs >> 8) & 0xff;
    DDisk_[lhs + 3] = rhs & 0xff;
}

void memory::saveHalfWord(const size_t lhs, const uint32_t rhs) {
    HitMiss(true, lhs);
    DDisk_[lhs] = (rhs >> 8) & 0xff;
    DDisk_[lhs + 1] = rhs & 0xff;
}

void memory::saveByte(const size_t lhs, const uint32_t rhs) {
    HitMiss(true, lhs);
    DDisk_[lhs] = rhs & 0xff;
}
