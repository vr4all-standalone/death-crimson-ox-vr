// SPDX-License-Identifier: GPL-2.0-or-later
#include "standalone_game.h"
#include <cstdio>
#include <cstring>

#ifndef EXPECT_STANDALONE_LUPIN
#error The test runner must declare which build policy it expects.
#endif

int main()
{
    int passed = 0, failed = 0;
    auto check = [&](bool ok, const char *name) {
        std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
        (ok ? passed : failed)++;
    };
    check(vr::standaloneGameError("LUPIN THE THIRD  -THE SHOOTING-") == nullptr,
            "loaded Lupin boot ID is accepted independently of media routing");
    const char *others[] = {
        " CONFIDENTIAL MISSION ---------", "THE HOUSE OF THE DEAD 2", "THE MAZE OF THE KINGS",
        "", "lupinshoo.zip", "LUPIN THE THIRD -THE SHOOTING-", "LUPIN THE THIRD  -THE SHOOTING- other"
    };
    const char *names[] = {
        "CM", "HOTD2", "MOK", "unidentified media", "a matching filename without loaded ID",
        "an ID with the wrong spacing", "an ID with a matching prefix"
    };
    for (unsigned i = 0; i < sizeof(others) / sizeof(*others); ++i)
        check((vr::standaloneGameError(others[i]) != nullptr) == (EXPECT_STANDALONE_LUPIN != 0), names[i]);
    const char *error = vr::standaloneGameError(" CONFIDENTIAL MISSION ---------");
    check(EXPECT_STANDALONE_LUPIN == 0 ? error == nullptr
            : error != nullptr && std::strstr(error, "Lupin the Third VR") != nullptr
                && std::strstr(error, "GDS-0018") != nullptr,
            "standalone rejection tells the player which game is supported");
    std::printf("standalone_game_test: %d passed, %d failed (policy %s)\n", passed, failed,
            EXPECT_STANDALONE_LUPIN ? "ON" : "OFF");
    return failed ? 1 : 0;
}
