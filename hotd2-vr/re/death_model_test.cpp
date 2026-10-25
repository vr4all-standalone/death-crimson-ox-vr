// Reads the user's private IC2 fixture at runtime; no ROM bytes belong in this test.
#include "death_model.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace vr::hands;
static void check(bool value, const char *why)
{
    if (!value)
        throw std::runtime_error(why);
}
static uint32_t word(const std::vector<uint8_t> &d, size_t p)
{
    return d.at(p) | uint32_t(d.at(p + 1)) << 8 | uint32_t(d.at(p + 2)) << 16 | uint32_t(d.at(p + 3)) << 24;
}
static void put(std::vector<uint8_t> &d, size_t p, uint32_t x)
{
    for (int i = 0; i < 4; i++)
        d.at(p + i) = uint8_t(x >> (8 * i));
}
static float number(const std::vector<uint8_t> &d, size_t p)
{
    auto x = word(d, p);
    float f;
    std::memcpy(&f, &x, 4);
    return f;
}
static uint32_t crc(const uint8_t *d, size_t n)
{
    uint32_t c = ~0u;
    while (n--)
    {
        c ^= *d++;
        for (int i = 0; i < 8; i++)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}
// Test-only CRC collision patch: let malformed objects pass the content gate so the
// parser's independent bounds checks are exercised, without any production bypass.
static void repairCrc(std::vector<uint8_t> &d, size_t first, size_t count, size_t patch, uint32_t target)
{
    uint32_t base = crc(d.data() + first, count), basis[32]{}, mask[32]{};
    for (int i = 0; i < 32; i++)
    {
        d[patch + i / 8] ^= uint8_t(1u << (i % 8));
        uint32_t v = crc(d.data() + first, count) ^ base, m = 1u << i;
        d[patch + i / 8] ^= uint8_t(1u << (i % 8));
        for (int bit = 31; bit >= 0; bit--)
            if (v & (1u << bit))
            {
                if (basis[bit])
                {
                    v ^= basis[bit];
                    m ^= mask[bit];
                }
                else
                {
                    basis[bit] = v;
                    mask[bit] = m;
                    break;
                }
            }
    }
    uint32_t delta = target ^ base, result = 0;
    for (int bit = 31; bit >= 0; bit--)
        if (delta & (1u << bit))
        {
            check(basis[bit] != 0, "CRC patch matrix");
            delta ^= basis[bit];
            result ^= mask[bit];
        }
    for (int i = 0; i < 32; i++)
        if (result & (1u << i))
            d[patch + i / 8] ^= uint8_t(1u << (i % 8));
    check(crc(d.data() + first, count) == target, "CRC patch result");
}
static DeathGunReader reader(const std::vector<uint8_t> &d)
{
    return [&d](uint32_t offset, size_t size, uint8_t *out) {
        if (offset < 0x1000000)
            return false;
        size_t p = offset - 0x1000000;
        if (p > d.size() || size > d.size() - p)
            return false;
        std::memcpy(out, d.data() + p, size);
        return true;
    };
}
int main(int argc, char **argv)
{
    int failed = 0, total = 0;
    auto run = [&](const char *name, const std::function<void()> &f) {
        ++total;
        try
        {
            f();
            std::cout << "PASS " << name << '\n';
        }
        catch (const std::exception &e)
        {
            ++failed;
            std::cout << "FAIL " << name << ": " << e.what() << '\n';
        }
    };
    check(argc >= 2, "Expected private mpr-23515.ic2 fixture path");
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> source((std::istreambuf_iterator<char>(in)), {});
    check(source.size() == 0x800000, "Owned IC2 must be 8 MiB");
    std::vector<uint8_t> cache;
    std::string error;
    run("owned cartridge produces complete correctly oriented soldier pistol", [&] {
        check(buildDeathGun(reader(source), cache, error), error.c_str());
        check(cache.size() == 22152, "HND size");
        check(word(cache, 4) == 2 && word(cache, 100) == 3 && word(cache, 104) == 10,
              "complete gun flags / source strips");
        check(word(cache, 108) == 1 && word(cache, 112) == 64 && word(cache, 116) == 64, "one 64px texture");
        check(word(cache, 16508) == 4 && word(cache, 16512) == 1 && word(cache, 16516) == 0 &&
                  word(cache, 16520) == 156,
              "52 triangles / body only");
        check(std::abs(number(cache, 8)) < 0.0001f && number(cache, 12) > 0.04f && number(cache, 16) < -0.16f,
              "muzzle faces forward above grip");
        float lo = 1, hi = -1;
        for (size_t p = 16524; p < 22140; p += 36)
        {
            lo = std::min(lo, number(cache, p + 8));
            hi = std::max(hi, number(cache, p + 8));
        }
        check(std::abs((hi - lo) - 0.24f) < 0.00001f, "24cm longitudinal extent");
        float minY = 1;
        for (size_t p = 16524; p < 22140; p += 36)
            minY = std::min(minY, number(cache, p + 4));
        float sectionLo = 1, sectionHi = -1, ys = minY * .30f;
        for (size_t p = 16524; p < 22140; p += 108)
            for (int a = 0; a < 3; a++)
            {
                size_t pa = p + size_t(a) * 36, pb = p + size_t((a + 1) % 3) * 36;
                float ya = number(cache, pa + 4), yb = number(cache, pb + 4);
                if ((ya - ys) * (yb - ys) < 0)
                {
                    float t = (ys - ya) / (yb - ya),
                          z = number(cache, pa + 8) + t * (number(cache, pb + 8) - number(cache, pa + 8));
                    sectionLo = std::min(sectionLo, z);
                    sectionHi = std::max(sectionHi, z);
                }
            }
        check(sectionHi > sectionLo && std::abs((sectionLo + sectionHi) / 2) / (sectionHi - sectionLo) < .40f,
              "hold origin must sit inside the grip");
        check(validDeathGunCache(cache, error), error.c_str());
    });
    run("deterministic extraction", [&] {
        std::vector<uint8_t> again;
        check(buildDeathGun(reader(source), again, error), error.c_str());
        check(again == cache, "same source differs");
    });
    run("short read leaves no partial model", [&] {
        std::vector<uint8_t> out{1, 2, 3};
        check(!buildDeathGun([](uint32_t, size_t, uint8_t *) { return false; }, out, error), "read failure accepted");
        check(out.empty() && !error.empty(), "partial result / missing reason");
    });
    run("wrong game / source checksum rejected", [&] {
        auto d = source;
        d[0x201800] ^= 1;
        std::vector<uint8_t> out;
        check(!buildDeathGun(reader(d), out, error), "wrong ARI accepted");
    });
    run("corrupt source geometry rejected", [&] {
        auto d = source;
        d[0x20202c + 0xdc + 0x2b34 + 8] ^= 1;
        std::vector<uint8_t> out;
        check(!buildDeathGun(reader(d), out, error), "bad ARX CRC accepted");
    });
    const size_t nj = 0x20202c, cm = nj + 0xdc;
    auto badObject = [&](size_t where, uint32_t value) {
        auto d = source;
        put(d, where, value);
        repairCrc(d, nj, 27268, nj + 27264, 0x0773ccf0);
        std::vector<uint8_t> out;
        check(!buildDeathGun(reader(d), out, error), "malformed CRC-preserving model accepted");
        check(out.empty(), "partial malformed output");
    };
    run("out-of-bounds model pointer rejected after checksum gate", [&] { badObject(cm + 0x291c + 4, 0xfffffff0); });
    run("invalid vertex count rejected after checksum gate", [&] { badObject(cm + 0x2b34 + 4, 0xffff0000); });
    run("invalid strip vertex index rejected after checksum gate", [&] { badObject(cm + 0x296c, 0x00beffff); });
    run("invalid chunk length rejected after checksum gate", [&] { badObject(nj + 0xd4 + 4, 0xfffffff0); });
    run("nonfinite source coordinate rejected after checksum gate", [&] { badObject(cm + 0x2b34 + 8, 0x7f800000); });
    auto badCache = [&](size_t p, uint32_t v) {
        auto d = cache;
        if (d.empty())
            throw std::runtime_error("no baseline cache");
        put(d, p, v);
        check(!validDeathGunCache(d, error), "damaged cache accepted");
    };
    run("partial / empty cache rejected", [&] {
        check(!validDeathGunCache({}, error), "empty accepted");
        auto d = cache;
        d.resize(111);
        check(!validDeathGunCache(d, error), "truncated accepted");
    });
    run("complete flag required", [&] { badCache(100, 1); });
    run("finite bounded cache geometry required", [&] {
        badCache(16524, 0x7f800000);
        badCache(16524, 0x3f800000);
    });
    run("texture integrity required", [&] { badCache(124, 0); });
    run("bounded mesh counts required", [&] { badCache(16520, 0xffffffff); });
    run("correct muzzle orientation required", [&] { badCache(16, 0x3e800000); });
    run("trailing cache payload rejected", [&] {
        auto d = cache;
        d.push_back(0);
        check(!validDeathGunCache(d, error), "trailing accepted");
    });
    run("absent reader is rejected", [&] {
        std::vector<uint8_t> out;
        check(!buildDeathGun({}, out, error), "empty reader accepted");
    });
    run("throwing reader does not escape", [&] {
        std::vector<uint8_t> out{1};
        check(!buildDeathGun([](uint32_t, size_t, uint8_t *) -> bool { throw std::runtime_error("read failed"); }, out,
                             error),
              "throw accepted");
        check(out.empty() && !error.empty(), "failed reader exposed output");
    });
    run("invalid normal and UV are rejected", [&] {
        badCache(16524 + 12, 0);
        badCache(16524 + 24, 0x40000000);
    });
    run("native cache file reload and file bounds", [&] {
        check(argc >= 3 && !cache.empty(), "expected cache output path");
        auto save = [&](const std::string &path, const std::vector<uint8_t> &bytes) {
            std::ofstream out(path, std::ios::binary);
            out.write((const char *)bytes.data(), std::streamsize(bytes.size()));
            check(bool(out), "cache fixture write");
        };
        std::string path = argv[2];
        save(path, cache);
        check(loadDeathGunCache(path), "complete native cache did not reload");
        save(path + ".part", cache);
        check(!loadDeathGunCache(path + ".part"), "unfinished cache file accepted");
        std::remove((path + ".part").c_str());
        auto shortFile = cache;
        shortFile.resize(111);
        save(path, shortFile);
        check(!loadDeathGunCache(path), "short cache file accepted");
        auto longFile = cache;
        longFile.push_back(0);
        save(path, longFile);
        check(!loadDeathGunCache(path), "oversized cache file accepted");
        auto bad = cache;
        bad[124] ^= 1;
        save(path, bad);
        check(!loadDeathGunCache(path), "damaged cache file accepted");
        save(path, cache);
        check(loadDeathGunCache(path), "restored native cache did not reload");
    });
    if (argc >= 3 && !cache.empty())
    {
        std::ofstream out(argv[2], std::ios::binary);
        out.write((const char *)cache.data(), cache.size());
    }
    std::cout << (total - failed) << '/' << total << " death model tests passed\n";
    return failed ? 1 : 0;
}
