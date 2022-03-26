
#include <cstdint>
#include <functional>

namespace asset_manager {
enum class AssetType {
  Texture,
  Model,
  Count_,
};

struct AssetHash {
  uint64_t hash;

  AssetHash(uint64_t hash_, AssetType type)
      : hash((hash_ & (uint64_t(1) << 60) - 1) | (uint64_t(type) << 60)) {}
};

void *GetAsset(AssetHash hash);
void *NewAsset(AssetHash hash);
void FreeAsset(AssetHash hash);
using IterCallback = std::function<void(void *)>;
void IterateGroup(AssetType type, IterCallback cb);
void DestroyGroup(AssetType type);
void ClearUnused(AssetType type);

} // namespace asset_manager