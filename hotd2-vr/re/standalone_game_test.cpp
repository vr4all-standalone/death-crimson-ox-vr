// SPDX-License-Identifier: GPL-2.0-or-later
#include "standalone_game.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>

#ifndef EXPECT_STANDALONE_DEATHCRIMSONOX
#error The test runner must declare which build policy it expects.
#endif

int main(int argc, char **argv)
{
    int passed = 0, failed = 0;
    auto check = [&](bool ok, const char *name) {
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
        (ok ? passed : failed)++;
    };
    check(vr::standaloneGameError("DEATH CRIMSON OX") == nullptr,
            "loaded Death Crimson boot ID is accepted independently of media routing");
    const char *others[] = {
        " CONFIDENTIAL MISSION ---------", "THE HOUSE OF THE DEAD 2", "THE MAZE OF THE KINGS",
        "", "deathcoxj.zip", "Death Crimson OX", "DEATH CRIMSON OX other"
    };
    const char *names[] = {
        "CM", "HOTD2", "MOK", "unidentified media", "a matching filename without loaded ID",
        "an ID with the wrong spacing", "an ID with a matching prefix"
    };
    for (unsigned i = 0; i < sizeof(others) / sizeof(*others); ++i)
        check((vr::standaloneGameError(others[i]) != nullptr) == (EXPECT_STANDALONE_DEATHCRIMSONOX != 0), names[i]);
    const char *error = vr::standaloneGameError(" CONFIDENTIAL MISSION ---------");
    check(EXPECT_STANDALONE_DEATHCRIMSONOX == 0 ? error == nullptr
            : error != nullptr && std::strstr(error, "Death Crimson OX VR") != nullptr
                && std::strstr(error, "Japan Rev A") != nullptr,
            "standalone rejection tells the player which game is supported");
    const bool enforced = EXPECT_STANDALONE_DEATHCRIMSONOX != 0;
    check(vr::standaloneArcadeMediaError(true, false) == nullptr, "normal archive route remains available");
    check((vr::standaloneArcadeMediaError(false, false) != nullptr) == enforced, "raw BIN/LST cannot bypass revision validation");
    check((vr::standaloneArcadeMediaError(true, true) != nullptr) == enforced, "slave boot cannot bypass revision validation");
    check(vr::standaloneArcadeSetError("deathcoxj", true, 0x5800000, 11) == nullptr, "verified Japan Rev A set metadata accepted");
    for (const char *set : {"deathcox", "deathcoxo", "lupinshoo", "", "deathcoxj other"})
        check((vr::standaloneArcadeSetError(set, true, 0x5800000, 11) != nullptr) == enforced, "unverified set rejected before loading");
    check((vr::standaloneArcadeSetError("deathcoxj", false, 0x5800000, 11) != nullptr) == enforced, "wrong cartridge type rejected");
    check((vr::standaloneArcadeSetError("deathcoxj", true, 0x5800001, 11) != nullptr) == enforced, "wrong cartridge size rejected");
    check((vr::standaloneArcadeSetError("deathcoxj", true, 0x5800000, 10) != nullptr) == enforced, "incomplete chip definition rejected");
    const unsigned crcs[] = {0xb60e113e,0x1f2b090e,0xdc8557eb,0x94494cbb,0x69ba6a41,0x49882766,0xcdc82805,0x1a268360,0xcf8674b8,0x7ae6716e,0xc91efb67};
    const char *chips[] = {"epr-23524a.ic22","mpr-23514.ic1","mpr-23515.ic2","mpr-23516.ic3","mpr-23517.ic4","mpr-23518.ic5","mpr-23519.ic6","mpr-23520.ic7","mpr-23521.ic8","mpr-23522.ic9","mpr-23523.ic10"};
    for(unsigned i=0;i<11;i++) {
        const unsigned length=i==0?0x400000:0x800000, offset=i*0x800000;
        check(vr::standaloneArcadeChipMetadataError(i, offset, length, crcs[i], true) == nullptr, "known chip metadata accepted");
        check((vr::standaloneArcadeChipMetadataError(i, offset, length, crcs[i]^1, true) != nullptr) == enforced, "wrong chip CRC definition rejected");
        check((vr::standaloneArcadeChipMetadataError(i, offset+1, length, crcs[i], true) != nullptr) == enforced, "wrong chip mapping rejected");
        check((vr::standaloneArcadeChipMetadataError(i, offset, length, crcs[i], false) != nullptr) == enforced, "non-normal chip definition rejected");
    }
    check((vr::standaloneArcadeChipMetadataError(11, 0, 0, 0, true) != nullptr) == enforced, "extra chip definition rejected");
    check(vr::standaloneArcadeCrc32("123456789",9)==0xcbf43926, "CRC matches independent IEEE check vector");
    check(vr::standaloneArcadeCrc32(nullptr,0)==0, "CRC of empty input is zero");
    std::vector<unsigned char> data(0x400000, 0);
    // A same-size fake retaining the game's common boot title must not pass content validation.
    std::memcpy(data.data()+0x30,"DEATH CRIMSON OX",15);
    check((vr::standaloneArcadeChipError(0, data.size(), data.size(), data.data()) != nullptr) == enforced, "same title and length cannot substitute for measured program bytes");
    check((vr::standaloneArcadeChipError(0, data.size()-1, data.size(), data.data()) != nullptr) == enforced, "short archive entry rejected");
    check((vr::standaloneArcadeChipError(0, data.size()+1, data.size(), data.data()) != nullptr) == enforced, "oversize archive entry rejected");
    check((vr::standaloneArcadeChipError(0, data.size(), data.size()-1, data.data()) != nullptr) == enforced, "short actual read rejected");
    check((vr::standaloneArcadeChipError(0, data.size(), data.size(), nullptr) != nullptr) == enforced, "absent payload rejected");
    if(argc==2) {
        for(unsigned i=0;i<11;i++) {
            std::ifstream in(std::string(argv[1])+"/"+chips[i],std::ios::binary);
            std::vector<unsigned char> chip((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            check(in.good()||in.eof(), "private fixture can be read");
            check(!chip.empty() && vr::standaloneArcadeChipError(i,chip.size(),chip.size(),chip.data())==nullptr, "actual private chip accepted");
            if(!chip.empty()) {
                chip.back()^=1;
                check((vr::standaloneArcadeChipError(i,chip.size(),chip.size(),chip.data())!=nullptr)==enforced, "one changed private chip byte rejected");
            }
        }
        std::ifstream originalIn(std::string(argv[1])+"/epr-23524.ic22",std::ios::binary);
        std::vector<unsigned char> original((std::istreambuf_iterator<char>(originalIn)), std::istreambuf_iterator<char>());
        const std::string title="DEATH CRIMSON OX";
        check(original.size()==0x400000, "original Japan program fixture has the same 4 MiB length");
        check(std::search(original.begin(),original.end(),title.begin(),title.end())!=original.end(), "original Japan program retains the common boot ID");
        check((vr::standaloneArcadeChipError(0,original.size(),original.size(),original.data())!=nullptr)==enforced,
                "actual original Japan program rejected despite common boot ID");    }    std::printf("standalone_game_test: %d passed, %d failed (policy %s)\n", passed, failed,
            EXPECT_STANDALONE_DEATHCRIMSONOX ? "ON" : "OFF");
    return failed ? 1 : 0;
}
