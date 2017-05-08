#include "memory.hpp"

void memory::Init(int argc, char** argv) {
    std::stringstream ss;
    for (int i = 1; i < argc; ++i) ss << argv[i] << " ";
    for (int i = argc - 1; i < 10; ++i) ss << argDefault_[i] << " ";
    ss >> IMemorySize_ >> DMemorySize_ >> IPageSize_ >> DPageSize_
        >> ICacheSize_ >> IBlockSize_ >> IAssociativity_
        >> DCacheSize_ >> DBlockSize_ >> DAssociativity_;
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

const uint32_t memory::getInstr() {
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
