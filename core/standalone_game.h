// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <array>

namespace vr {
// Evaluate loaded media metadata, never a path or emulated RAM. Ordinary Flycast and
// regression builds keep their normal game selection when the option is absent/OFF.
inline const char *standaloneGameError(std::string_view gameId)
{
#if defined(VR_STANDALONE_DEATHCRIMSONOX) && VR_STANDALONE_DEATHCRIMSONOX
    if (gameId != "DEATH CRIMSON OX")
        return "Death Crimson OX VR supports the verified Death Crimson OX Japan Rev A cartridge set (deathcoxj.zip).";
#endif
    return nullptr;
}
// Cartridge gates are separate from the shared boot-title check above. The raw MAME
// cartridge bytes establish the measured revision before any emulated code can run.
inline constexpr std::array<std::uint32_t, 11> standaloneArcadeChipCrcs = {
    0xb60e113e, 0x1f2b090e, 0xdc8557eb, 0x94494cbb, 0x69ba6a41, 0x49882766,
    0xcdc82805, 0x1a268360, 0xcf8674b8, 0x7ae6716e, 0xc91efb67
};

inline const char *standaloneArcadeMediaError(bool archive, bool slave)
{
#if defined(VR_STANDALONE_DEATHCRIMSONOX) && VR_STANDALONE_DEATHCRIMSONOX
    if (!archive || slave)
        return standaloneGameError("");
#endif
    return nullptr;
}

inline const char *standaloneArcadeSetError(std::string_view set, bool m2, std::size_t size, unsigned chips)
{
#if defined(VR_STANDALONE_DEATHCRIMSONOX) && VR_STANDALONE_DEATHCRIMSONOX
    if (set != "deathcoxj" || !m2 || size != 0x5800000 || chips != standaloneArcadeChipCrcs.size())
        return standaloneGameError("");
#endif
    return nullptr;
}

inline const char *standaloneArcadeChipMetadataError(unsigned chip, std::size_t offset,
        std::size_t length, std::uint32_t crc, bool normal)
{
#if defined(VR_STANDALONE_DEATHCRIMSONOX) && VR_STANDALONE_DEATHCRIMSONOX
    if (chip >= standaloneArcadeChipCrcs.size() || !normal || offset != chip * 0x800000ULL
            || length != (chip == 0 ? 0x400000U : 0x800000U) || crc != standaloneArcadeChipCrcs[chip])
        return standaloneGameError("");
#endif
    return nullptr;
}

inline std::uint32_t standaloneArcadeCrc32(const void *data, std::size_t length)
{
    static const auto table = [] {
        std::array<std::uint32_t, 256> result{};
        for (unsigned n = 0; n < result.size(); ++n) {
            std::uint32_t crc = n;
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
            result[n] = crc;
        }
        return result;
    }();
    const auto *bytes = static_cast<const unsigned char *>(data);
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t n = 0; n < length; ++n)
        crc = table[(crc ^ bytes[n]) & 0xff] ^ (crc >> 8);
    return ~crc;
}

inline const char *standaloneArcadeChipError(unsigned chip, std::size_t archiveLength,
        std::size_t bytesRead, const void *data)
{
#if defined(VR_STANDALONE_DEATHCRIMSONOX) && VR_STANDALONE_DEATHCRIMSONOX
    const std::size_t expectedLength = chip == 0 ? 0x400000U : 0x800000U;
    if (chip >= standaloneArcadeChipCrcs.size() || archiveLength != expectedLength
            || bytesRead != expectedLength || data == nullptr
            || standaloneArcadeCrc32(data, bytesRead) != standaloneArcadeChipCrcs[chip])
        return standaloneGameError("");
#endif
    return nullptr;
}
}