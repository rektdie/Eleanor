#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include "nnue.h"

namespace ACC {

struct BucketPair {
    int white;
    int black;
};

using Accumulator = std::array<int16_t, NNUE::HL_SIZE>;

struct AccumulatorPair {
    alignas(ALIGNMENT) Accumulator white;
    alignas(ALIGNMENT) Accumulator black;

    Accumulator& get(bool perspective) { return perspective ? black : white; }
    const Accumulator& get(bool perspective) const { return perspective ? black : white; }
};

enum class UpdateType : uint8_t {
    None,
    Refresh,
    Copy,
    Delta
};

struct Update {
    UpdateType type = UpdateType::None;
    uint8_t refreshMask = 0;
    bool stm = false;
    bool sub2Opp = false;
    uint8_t nAdd = 0, nSub = 0;
    int8_t add1 = 0, addPT1 = 0, add2 = 0, addPT2 = 0;
    int8_t sub1 = 0, subPT1 = 0, sub2 = 0, subPT2 = 0;

    void setAddSub(bool s, int add, int addPT, int sub, int subPT) {
        type = UpdateType::Delta;
        stm = s; sub2Opp = false; nAdd = 1; nSub = 1;
        add1 = add; addPT1 = addPT; sub1 = sub; subPT1 = subPT;
    }

    void setAddSubSub(bool s, int add, int addPT, int sub1_, int subPT1_, int sub2_, int subPT2_) {
        type = UpdateType::Delta;
        stm = s; sub2Opp = true; nAdd = 1; nSub = 2;
        add1 = add; addPT1 = addPT;
        sub1 = sub1_; subPT1 = subPT1_; sub2 = sub2_; subPT2 = subPT2_;
    }

    void setAddAddSubSub(bool s, int add1_, int addPT1_, int add2_, int addPT2_, int sub1_, int subPT1_, int sub2_, int subPT2_) {
        type = UpdateType::Delta;
        stm = s; sub2Opp = false; nAdd = 2; nSub = 2;
        add1 = add1_; addPT1 = addPT1_; add2 = add2_; addPT2 = addPT2_;
        sub1 = sub1_; subPT1 = subPT1_; sub2 = sub2_; subPT2 = subPT2_;
    }

    void setRefresh() {
        *this = Update();
        type = UpdateType::Refresh;
    }
};

struct FinnyEntry {
    alignas(ALIGNMENT) Accumulator acc;
    uint64_t bb[2][6];
};

struct FinnyTable {
    FinnyEntry entries[2][2][NNUE::INPUT_BUCKETS];

    void Reset();
};

struct AccStack {
    std::vector<AccumulatorPair> slots;
    std::vector<const AccumulatorPair*> cur;
    FinnyTable finny;

    explicit AccStack(size_t n) : slots(n), cur(n, nullptr) { finny.Reset(); }

    AccStack(const AccStack& other) : AccStack(other.slots.size()) {}
    AccStack& operator=(const AccStack&) { return *this; }
};

int CalculateIndex(bool perspective, bool side, int pieceType, int square, bool mirrored);

const int16_t* Row(int bucket, int index);

void Apply(Accumulator& dst, const Accumulator& src, const int16_t* a1, const int16_t* a2, const int16_t* s1, const int16_t* s2);

void ApplyInPlace(Accumulator& acc, const int16_t* const* adds, int nAdds, const int16_t* const* subs, int nSubs);

void AddRow(Accumulator& acc, const int16_t* row);

}
