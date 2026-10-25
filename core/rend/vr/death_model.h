/* Death Crimson OX's soldier pistol, extracted from the user's own cartridge.
 * No game geometry or textures are distributed with this code. GPL v2 or later. */
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace vr::hands
{
// Absolute NAOMI cartridge offsets (not the concatenated GD/IC1 filesystem).
// The callback must either copy exactly size bytes or return false.
using DeathGunReader = std::function<bool(uint32_t offset, size_t size, uint8_t *destination)>;
constexpr size_t DeathGunCacheBytes = 22152;
// Complete E410 soldier pistol, not the protagonists' mythical Crimson.
// HND1 v2, gun-only + complete; 10 source strips / 52 triangles, 0.24 m barrel-axis extent.
bool buildDeathGun(const DeathGunReader &read, std::vector<uint8_t> &out, std::string &error);
// Exact layout/counts, texture integrity, finite shape/orientation/topology checks.
bool validDeathGunCache(const std::vector<uint8_t> &data, std::string &error);
// Bounded read. Never accepts a .part file by implication or allocates from file fields.
bool loadDeathGunCache(const std::string &path);
} // namespace vr::hands
