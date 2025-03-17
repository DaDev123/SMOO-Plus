#pragma once

#include "hk/diag/diag.h"
#include "hk/types.h"
#include "hk/util/Algorithm.h"
#include "hk/util/Math.h"
#include "hk/util/hash.h"
#include <type_traits>

namespace hk::sail {

#define _HK_SAIL_PRECALC_TEMPLATE template <bool IsPreCalc = false, typename T = typename std::conditional<IsPreCalc, u32, char>::type>

    namespace detail {

        class SymbolEntry;

        struct Symbol {
            enum Type
            {
                Type_Immediate,
                Type_Dynamic,
                Type_DataBlock,
                Type_ReadADRPGlobal,
                Type_Arithmetic,
                Type_MultipleCandidate,
            };

            const u32 destNameMurmur;
            const Type type;
            ptr symbolPtrCache;

            bool isCacheDisabled() const { return symbolPtrCache == 1; }
        };

#define _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC                     \
    void apply(bool abort, ptr* out, const char* destSymbol); \
    void apply(bool abort, ptr* out, const u32* destSymbolMurmur)

        struct SymbolVersioned : Symbol {
            const u64 versionsBitset;

            bool isVersion(u32 moduleIdx) const;
        };

        struct SymbolDataBlock : Symbol {
            struct DataBlock {
                size size;
                u8 data[];
            };
            const uintptr_t offsetToDataBlock;

            const u8 moduleIdx;
            const u8 versionBoundaryType;
            const u8 versionBoundary;
            const u8 sectionLimit;
            const s32 offsetToFoundBlock;

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        struct SymbolDynamic : Symbol {
            const u64 lookupNameRtldHash;
            const u32 lookupNameMurmur;

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        struct SymbolImmediate : SymbolVersioned {
            const u32 moduleIdx;
            const u32 offsetIntoModule;

            bool isVersionValid() const { return isVersion(moduleIdx); }

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        struct SymbolReadADRPGlobal : SymbolVersioned {
            const u32 symIdx;
            const s32 offsetToLoInstr;

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        struct SymbolArithmetic : SymbolVersioned {
            const u32 symIdx;
            const s32 addend;

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        struct SymbolMultipleCandidate : Symbol {
            const u64 offsetToCandidates;
            const u64 numCandidates;

            _HK_SAIL_DETAIL_SYMBOL_APPLY_FUNC;
        };

        constexpr size cSymbolEntrySize = util::max(sizeof(SymbolDataBlock), sizeof(SymbolDynamic), sizeof(SymbolImmediate), sizeof(SymbolReadADRPGlobal), sizeof(SymbolArithmetic), sizeof(SymbolMultipleCandidate));

        static_assert(cSymbolEntrySize == 32);
        static_assert(__builtin_offsetof(Symbol, destNameMurmur) == 0);
        static_assert(__builtin_offsetof(Symbol, type) == 4);
        static_assert(__builtin_offsetof(Symbol, symbolPtrCache) == 8);
        static_assert(__builtin_offsetof(SymbolMultipleCandidate, offsetToCandidates) == 16);
        static_assert(__builtin_offsetof(SymbolMultipleCandidate, numCandidates) == 24);
        static_assert(__builtin_offsetof(SymbolArithmetic, symIdx) == 24);

        class SymbolEntry {
            union {
                SymbolDataBlock mDataBlock;
                SymbolDynamic mDynamic;
                SymbolImmediate mImmediate;
                SymbolReadADRPGlobal mReadADRPGlobal;
                SymbolArithmetic mArithmetic;
                SymbolMultipleCandidate mMultiple;
                Symbol mBase;
            };

        public:
            _HK_SAIL_PRECALC_TEMPLATE
            void apply(bool abort, ptr* out, const T* destSymbol) {
                if (!mBase.isCacheDisabled() && mBase.symbolPtrCache != 0) { // > 1
                    *out = mBase.symbolPtrCache;
                    return;
                }

                switch (mBase.type) {
                case Symbol::Type_DataBlock:
                    mDataBlock.apply(abort, out, destSymbol);
                    break;
                case Symbol::Type_Dynamic:
                    mDynamic.apply(abort, out, destSymbol);
                    break;
                case Symbol::Type_Immediate:
                    mImmediate.apply(abort, out, destSymbol);
                    break;
                case Symbol::Type_ReadADRPGlobal:
                    mReadADRPGlobal.apply(abort, out, destSymbol);
                    break;
                case Symbol::Type_Arithmetic:
                    mArithmetic.apply(abort, out, destSymbol);
                    break;
                case Symbol::Type_MultipleCandidate:
                    mMultiple.apply(abort, out, destSymbol);
                    break;
                }

                if (!mBase.isCacheDisabled())
                    mBase.symbolPtrCache = *out;
            }

            u32 getNameMurmur32() const { return mBase.destNameMurmur; }
        };

        void loadVersions();

    } // namespace detail

    extern size gNumSymbols;
    extern detail::SymbolEntry gSymbols[];
    extern uint32_t gVersions[];

    _HK_SAIL_PRECALC_TEMPLATE
    ptr lookupSymbolFromDb(const T* symbol) {
        u32 destHash;
        if constexpr (IsPreCalc)
            destHash = *symbol;
        else
            destHash = util::hashMurmur(symbol);

        s32 idx = -1;

        if (gNumSymbols != 0)
            idx = util::binarySearch([](u32 idx) -> u32 {
                return gSymbols[idx].getNameMurmur32();
            },
                0, gNumSymbols - 1, destHash);

        if constexpr (IsPreCalc) {
            HK_ABORT_UNLESS(idx != -1, "UnresolvedSymbol: %08x\nTo use dynamic linking, add the symbols you intend to access to the symbol database.", *symbol);
        } else {
            HK_ABORT_UNLESS(idx != -1, "UnresolvedSymbol: %s\nTo use dynamic linking, add the symbols you intend to access to the symbol database.", symbol);
        }

        ptr out = 0;
        auto& entry = gSymbols[idx];
        entry.apply(true, &out, symbol);
        return out;
    }

    constexpr bool sUsePrecalcHashes =
#ifdef HK_USE_PRECALCULATED_SYMBOL_DB_HASHES
        true
#else
        false
#endif
        ;

} // namespace hk::sail
