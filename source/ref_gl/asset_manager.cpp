#include "asset_manager.hpp"
#include <array>
#include <deque>
#include <map>
#include <smmintrin.h>
#include <variant>
#include <vector>

extern "C" {
#include "r_image.h"
}

void Active(image_t *image, bool onoff) {
  if (onoff != image->flags & if_active) {
    reinterpret_cast<std::underlying_type_t<imageflags_t> &>(image->flags) ^=
        if_active;
  }
}

bool Active(image_t *image) { return image->flags & if_active; }

using namespace asset_manager;

struct StackIndex {
  uint32_t stackIndex;
  uint32_t itemIndex;
  void *stackData;
};

static std::map<uint64_t, StackIndex> assets;

// Return nearest zero bit index within 8bits
uint32_t GetIndex(uint16_t bits) {
  size_t subIndex = 0;

  while (bits & (1 << subIndex)) {
    subIndex++;
  }

  return subIndex;
}

// Return 8bit bitmask where each 1bit is 0xff within SSE registry
uint16_t GetXMXMask(__m128i bits) {
  using HalfType = uint64_t;
  static const __m128i fullMask =
      _mm_set1_epi64x(std::numeric_limits<HalfType>::max());
  const __m128i masked = _mm_cmpeq_epi8(bits, fullMask);
  return _mm_movemask_epi8(masked);
}

// Return nearest 8bit number index with at least one zero bit and SSE mask
auto GetIndexAndXMXMask(__m128i bits) {
  const uint16_t mask = GetXMXMask(bits);
  return std::make_pair(GetIndex(mask), mask);
}

// Get nearest zero bit index wihin SSE registry
uint32_t GetIndex(__m128i bits) {
  auto [maskIndex, _] = GetIndexAndXMXMask(bits);
  const uint32_t maskIndexTile = maskIndex / 8;
  uint64_t extractedHalf = [&] {
    if (maskIndexTile) {
      return _mm_extract_epi64(bits, 1);
    } else {
      return _mm_extract_epi64(bits, 0);
    }
  }();
  const uint32_t maskIndexMod = maskIndex % 8;
  const uint32_t nearestBit = GetIndex(uint8_t(extractedHalf >> maskIndexMod));

  return (maskIndex * 8) + nearestBit;
}

// Invert single bit within SSE registry
// Since SSE cannot operate with full bitset, extract 64bit, modify it and
// reinsert back
void InvertBit(__m128i &bits, uint32_t bit) {
  const uint32_t half = bit / 64;
  const uint32_t bitMod = bit % 64;

  uint64_t extractedHalf = [&] {
    if (half) {
      return _mm_extract_epi64(bits, 1);
    } else {
      return _mm_extract_epi64(bits, 0);
    }
  }();

  extractedHalf ^= uint64_t(1) << bitMod;

  if (half) {
    bits = _mm_insert_epi64(bits, extractedHalf, 1);
  } else {
    bits = _mm_insert_epi64(bits, extractedHalf, 0);
  }
}

// Find zero bit, set it and return it's overall index and if whole register was
// filled
auto ReserveBit(__m128i &bits) {
  auto [byteIndex, mask] = GetIndexAndXMXMask(bits);
  const uint32_t longHalf = byteIndex / 8;

  uint64_t extractedHalf = [&] {
    if (longHalf) {
      return _mm_extract_epi64(bits, 1);
    } else {
      return _mm_extract_epi64(bits, 0);
    }
  }();

  const uint32_t byteHalfIndex = (byteIndex % 8) * 8;
  const uint32_t nearestBit = GetIndex(uint8_t(extractedHalf >> byteHalfIndex));
  extractedHalf |= uint64_t(1) << (nearestBit + byteHalfIndex);

  bool filled = extractedHalf == std::numeric_limits<uint64_t>::max();

  if (longHalf) {
    bits = _mm_insert_epi64(bits, extractedHalf, 1);
    filled = filled && (mask & 0xf) == 0xf;
  } else {
    bits = _mm_insert_epi64(bits, extractedHalf, 0);
    filled = filled && (mask & 0xf0) == 0xf0;
  }

  return std::make_pair((byteIndex * 8) + nearestBit, filled);
}

using MaskType = __m128i;

struct MaskStorage {
  __m128i data;
};

template <class C, size_t stackCount> struct AssetStorage {
  static constexpr size_t MAX_LEAVES = sizeof(MaskStorage) * 8;
  static constexpr size_t MAX_ASSETS = MAX_LEAVES * MAX_LEAVES;

  std::array<MaskStorage, MAX_LEAVES> leafMasks;
  MaskType rootMasks;
  uint32_t numUsedAssets = 0;
  std::array<C, stackCount> baseStack;
  std::deque<decltype(baseStack)> heapStacks; // how 2 defrag?

  StackIndex NewAsset(AssetHash hash) {
    if (numUsedAssets >= MAX_ASSETS) {
      throw std::runtime_error("Maximum number of used assets reached!");
    }

    const uint32_t rootIndex = GetIndex(rootMasks);
    auto [subIndex, filled] = ReserveBit(leafMasks.at(rootIndex).data);

    if (filled) {
      InvertBit(rootMasks, rootIndex);
    }

    const uint32_t totalIndex = (rootIndex * MAX_LEAVES) + subIndex;
    const uint32_t stackIndex = totalIndex / stackCount;
    const uint32_t indexInStack = totalIndex % stackCount;

    if (heapStacks.size() < stackIndex) {
      heapStacks.resize(stackIndex);
    }

    C *item = stackIndex ? &heapStacks.at(stackIndex - 1).at(indexInStack)
                         : &baseStack.at(indexInStack);
    item->assetHash = hash.hash;
    Active(item, true);
    item->index = numUsedAssets++;

    return {stackIndex, indexInStack, item};
  }

  void FreeAsset(StackIndex index) {
    numUsedAssets--;
    const uint32_t totalIndex =
        (index.stackIndex * stackCount) + index.itemIndex;
    const uint32_t leafIndex = totalIndex / MAX_LEAVES;
    const uint32_t bitIndex = totalIndex % MAX_LEAVES;
    auto &leaf = leafMasks.at(leafIndex);

    C *item = index.stackIndex
                  ? &heapStacks.at(index.stackIndex - 1).at(index.itemIndex)
                  : &baseStack.at(index.itemIndex);

    *item = {};

    if (GetXMXMask(leaf.data) == 0xffff) {
      InvertBit(rootMasks, leafIndex);
    }

    InvertBit(leaf.data, bitIndex);
  }

  void DestroyAll() {
    leafMasks.fill(MaskStorage{});
    rootMasks = MaskType{};
    numUsedAssets = 0;
    baseStack.fill({});
    heapStacks.clear();
  }

  void ClearUnused() {
    size_t stackIndex = 0;

    auto RemoveItem = [&](auto *item) {
      if (item->markAsUnused) {
        assets.erase(item->assetHash);
        const uint32_t leafIndex = stackIndex / MAX_LEAVES;
        const uint32_t bitIndex = stackIndex % MAX_LEAVES;
        auto &leaf = leafMasks.at(leafIndex);

        if (GetXMXMask(leaf.data) == 0xffff) {
          InvertBit(rootMasks, leafIndex);
        }

        InvertBit(leaf.data, bitIndex);

        *item = {};
      }
    };

    for (auto &r : baseStack) {
      RemoveItem(&r);
      stackIndex++;
    }

    for (auto &h : heapStacks) {
      for (auto &r : h) {
        RemoveItem(&r);
        stackIndex++;
      }
    }
  }
};

static AssetStorage<image_s, 1024> textures;

using AssetStorageVariant =
    std::variant<std::add_pointer_t<decltype(textures)>>;

static std::array<AssetStorageVariant, uint32_t(AssetType::Count_)> assetGroups{
    &textures,
    nullptr,
};

uint32_t GroupFromHash(AssetHash hash) { return hash.hash >> 60; }

namespace asset_manager {

void *GetAsset(AssetHash hash) {
  if (auto found = assets.find(hash.hash); found != assets.end()) {
    return found->second.stackData;
  }

  return nullptr;
}

void *NewAsset(AssetHash hash) {
  auto groupId = GroupFromHash(hash);

  StackIndex stackData =
      std::visit([&](auto group) { return group->NewAsset(hash); },
                 assetGroups.at(groupId));

  assets.emplace(hash.hash, stackData);

  return stackData.stackData;
}

void FreeAsset(AssetHash hash) {
  auto found = assets.find(hash.hash);

  if (found == assets.end()) {
    return;
  }

  auto groupId = GroupFromHash(hash);

  std::visit([&](auto group) { group->FreeAsset(found->second); },
             assetGroups.at(groupId));

  assets.erase(found);
}

void IterateGroup(AssetType type, IterCallback cb) {
  std::visit(
      [&](auto group) {
        for (auto &item : group->baseStack) {
          if (Active(&item)) {
            cb(&item);
          }
        }

        for (auto &stack : group->heapStacks) {
          for (auto &item : stack) {
            if (Active(&item)) {
              cb(&item);
            }
          }
        }
      },
      assetGroups.at(uint64_t(type)));
}

void DestroyGroup(AssetType type) {
  std::visit([&](auto group) { group->DestroyAll(); },
             assetGroups.at(uint64_t(type)));

  auto lbIter = assets.lower_bound(AssetHash(0, type).hash);

  if (lbIter == assets.end() || (lbIter->first >> 60) != uint64_t(type)) {
    return;
  }

  auto ubIter = assets.upper_bound(
      AssetHash(std::numeric_limits<uint64_t>::max(), type).hash);
  assets.erase(lbIter, ubIter);
}

void ClearUnused(AssetType type) {
  std::visit([&](auto group) { group->ClearUnused(); },
             assetGroups.at(uint64_t(type)));
}

} // namespace asset_manager
