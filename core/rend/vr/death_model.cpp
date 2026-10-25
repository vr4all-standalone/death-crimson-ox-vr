/* Native E410 soldier pistol from the user's cartridge. No game assets included. GPL v2 or later. */
#include "death_model.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
namespace vr::hands
{
namespace
{
constexpr uint32_t AriOffset = 0x01201800, ArxOffset = 0x01202000;
constexpr size_t AriSize = 480, ArxSize = 144336, NjOffset = 0x2c, NjSize = 27268, PvrOffset = 0x10310, PvrSize = 2080;
constexpr size_t BodyVertices = 16524, BodyEnd = 22140;
void require(bool ok, const char *why)
{
    if (!ok)
        throw std::runtime_error(why);
}
uint32_t crc32(const uint8_t *p, size_t n)
{
    uint32_t c = ~0u;
    while (n--)
    {
        c ^= *p++;
        for (int i = 0; i < 8; i++)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}
struct Span
{
    const uint8_t *p;
    size_t size;
    void bounds(size_t at, size_t n) const
    {
        require(at <= size && n <= size - at, "native source offset is out of bounds");
    }
    uint16_t u16(size_t at) const
    {
        bounds(at, 2);
        return uint16_t(p[at] | uint16_t(p[at + 1]) << 8);
    }
    uint32_t u32(size_t at) const
    {
        bounds(at, 4);
        return uint32_t(p[at]) | uint32_t(p[at + 1]) << 8 | uint32_t(p[at + 2]) << 16 | uint32_t(p[at + 3]) << 24;
    }
    double f32(size_t at) const
    {
        uint32_t u = u32(at);
        require((u & 0x7f800000u) != 0x7f800000u, "native model contains a nonfinite number");
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }
    Span sub(size_t at, size_t n) const
    {
        bounds(at, n);
        return {p + at, n};
    }
    bool equal(size_t at, const char *text, size_t n) const
    {
        bounds(at, n);
        return std::memcmp(p + at, text, n) == 0;
    }
};
using Vec = std::array<double, 3>;
Vec sub(const Vec &a, const Vec &b)
{
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
Vec add(const Vec &a, const Vec &b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}
Vec mul(const Vec &a, double s)
{
    return {a[0] * s, a[1] * s, a[2] * s};
}
double dot(const Vec &a, const Vec &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vec cross(const Vec &a, const Vec &b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
Vec normal(const Vec &a)
{
    double n = std::sqrt(dot(a, a));
    require(n > 1e-8 && n < 1e5, "invalid native gun axis or normal");
    return mul(a, 1 / n);
}
Vec axes(const Vec &v, const std::array<Vec, 3> &r)
{
    return {dot(v, r[0]), dot(v, r[1]), dot(v, r[2])};
}
void word(std::vector<uint8_t> &out, uint32_t x)
{
    for (int i = 0; i < 4; i++)
        out.push_back(uint8_t(x >> (8 * i)));
}
void number(std::vector<uint8_t> &out, double x)
{
    float f = float(x);
    uint32_t u;
    std::memcpy(&u, &f, 4);
    word(out, u);
}
uint32_t twiddle(uint32_t x, uint32_t y)
{
    uint32_t v = 0;
    for (uint32_t bit = 0; bit < 5; bit++)
        v |= ((y >> bit) & 1) << (bit * 2) | ((x >> bit) & 1) << (bit * 2 + 1);
    return v;
}
std::vector<uint8_t> texture(Span pvr)
{
    require(pvr.size == PvrSize && pvr.equal(0, "GBIX", 4) && pvr.u32(4) == 8 && pvr.u32(8) == 3 &&
                pvr.equal(16, "PVRT", 4) && pvr.u32(20) == 2056 && pvr.u32(24) == 0x1001 && pvr.u16(28) == 64 &&
                pvr.u16(30) == 64,
            "unexpected soldier pistol texture descriptor");
    // Small VQ RGB565: 128 four-pixel codebook entries (1024 B), 32x32 indices.
    std::vector<uint8_t> out(64 * 64 * 4);
    for (uint32_t y = 0; y < 64; y++)
        for (uint32_t x = 0; x < 64; x++)
        {
            uint32_t index = pvr.p[32 + 1024 + twiddle(x >> 1, y >> 1)];
            require(index < 128, "small VQ codebook index is out of bounds");
            uint32_t c = pvr.u16(32 + index * 8 + ((x & 1) * 2 + (y & 1)) * 2);
            size_t at = (y * 64 + x) * 4;
            out[at] = uint8_t(((c >> 11) & 31) * 255 / 31);
            out[at + 1] = uint8_t(((c >> 5) & 63) * 255 / 63);
            out[at + 2] = uint8_t((c & 31) * 255 / 31);
            out[at + 3] = 255;
        }
    require(crc32(out.data(), out.size()) == 0x703049d6, "soldier pistol decoded texture integrity failed");
    return out;
}
struct Corner
{
    uint16_t index;
    int16_t u, v;
};
using Triangle = std::array<Corner, 3>;
bool validate(Span d)
{
    require(d.size == DeathGunCacheBytes && d.equal(0, "HND1", 4) && d.u32(4) == 2,
            "invalid Death Crimson pistol cache size/version");
    for (size_t p = 8; p < 100; p += 4)
        (void)d.f32(p);
    require(d.u32(100) == 3 && d.u32(104) == 10 && d.u32(108) == 1,
            "pistol cache must contain one complete native gun");
    require(std::abs(d.f32(8)) < 0.0001 && std::abs(d.f32(12) - 0.05427991) < 0.0001 &&
                std::abs(d.f32(16) + 0.17932251) < 0.0001,
            "pistol cache muzzle has the wrong orientation");
    for (size_t p = 20; p <= 32; p += 4)
        require(d.f32(p) == 0, "pistol cache grip/slide is invalid");
    for (int i = 0; i < 16; i++)
        require(d.f32(36 + size_t(i) * 4) == (i % 5 == 0 ? 1. : 0.), "pistol cache slide transform is invalid");
    require(d.u32(112) == 64 && d.u32(116) == 64 && d.u32(120) == 3, "pistol cache texture layout is invalid");
    require(crc32(d.p + 124, 16384) == 0x703049d6, "pistol cache texture checksum failed");
    require(d.u32(16508) == 4 && d.u32(16512) == 1 && d.u32(16516) == 0 && d.u32(16520) == 156,
            "pistol cache mesh counts are invalid");
    for (size_t p = BodyEnd; p < d.size; p += 4)
        require(d.u32(p) == 0, "pistol cache has an unexpected hand or slide mesh");
    Vec lo{1, 1, 1}, hi{-1, -1, -1};
    std::map<std::array<int, 3>, uint32_t> ids;
    std::vector<uint32_t> corners;
    for (size_t p = BodyVertices; p < BodyEnd; p += 36)
    {
        Vec v{d.f32(p), d.f32(p + 4), d.f32(p + 8)}, n{d.f32(p + 12), d.f32(p + 16), d.f32(p + 20)};
        require(std::abs(v[0]) < 0.015 && v[1] > -0.09 && v[1] < 0.09 && v[2] > -0.20 && v[2] < 0.06,
                "pistol cache geometry is outside its bounds");
        require(std::abs(dot(n, n) - 1) < 0.01, "pistol cache vertex normal is invalid");
        require(d.f32(p + 24) >= 0 && d.f32(p + 24) <= 1 && d.f32(p + 28) >= 0 && d.f32(p + 28) <= 1 &&
                    d.u32(p + 32) == 0xffffffff,
                "pistol cache UV or color is invalid");
        std::array<int, 3> key;
        for (int a = 0; a < 3; a++)
        {
            lo[a] = std::min(lo[a], v[a]);
            hi[a] = std::max(hi[a], v[a]);
            key[a] = int(std::round(v[a] * 1e6));
        }
        auto entry = ids.emplace(key, uint32_t(ids.size()));
        corners.push_back(entry.first->second);
    }
    constexpr double expectedLo[3] = {-0.01370691, -0.08072187, -0.18747137},
                     expectedHi[3] = {0.01370691, 0.08272818, 0.05252863};
    for (int a = 0; a < 3; a++)
        require(std::abs(lo[a] - expectedLo[a]) < 0.0001 && std::abs(hi[a] - expectedHi[a]) < 0.0001,
                "pistol cache shape/scale is incomplete");
    require(ids.size() == 27, "pistol cache source vertices are incomplete");
    std::map<std::pair<uint32_t, uint32_t>, unsigned> edges;
    for (size_t t = 0; t < corners.size(); t += 3)
    {
        require(corners[t] != corners[t + 1] && corners[t] != corners[t + 2] && corners[t + 1] != corners[t + 2],
                "pistol cache contains a degenerate triangle");
        for (int c = 0; c < 3; c++)
        {
            auto a = corners[t + c], b = corners[t + (c + 1) % 3];
            edges[{std::min(a, b), std::max(a, b)}]++;
        }
    }
    unsigned two = 0, four = 0;
    for (const auto &e : edges)
    {
        two += e.second == 2;
        four += e.second == 4;
    }
    require(two == 76 && four == 1 && edges.size() == 77, "pistol cache topology is incomplete");
    return true;
}
} // namespace
bool validDeathGunCache(const std::vector<uint8_t> &data, std::string &error)
{
    error.clear();
    try
    {
        return validate({data.data(), data.size()});
    }
    catch (const std::exception &e)
    {
        error = e.what();
        return false;
    }
}
bool loadDeathGunCache(const std::string &path)
{
    try
    {
        if (path.empty() || std::filesystem::u8path(path).extension() == ".part")
            return false;
        std::ifstream in(std::filesystem::u8path(path), std::ios::binary | std::ios::ate);
        if (!in || in.tellg() != std::streamoff(DeathGunCacheBytes))
            return false;
        std::vector<uint8_t> data(DeathGunCacheBytes);
        in.seekg(0);
        std::string error;
        return bool(in.read(reinterpret_cast<char *>(data.data()), std::streamsize(data.size()))) &&
               validDeathGunCache(data, error);
    }
    catch (const std::exception &)
    {
        return false;
    }
}
bool buildDeathGun(const DeathGunReader &read, std::vector<uint8_t> &out, std::string &error)
{
    out.clear();
    error.clear();
    try
    {
        require(bool(read), "native cartridge reader is unavailable");
        std::vector<uint8_t> ari(AriSize), arx(ArxSize);
        require(read(AriOffset, ari.size(), ari.data()), "could not read the native E410 archive index");
        require(crc32(ari.data(), ari.size()) == 0xd18c824c, "owned cartridge does not match the E410 archive index");
        require(read(ArxOffset, arx.size(), arx.data()), "could not read the native E410 archive");
        require(crc32(arx.data(), arx.size()) == 0xee33a987, "owned cartridge E410 archive integrity failed");
        Span index{ari.data(), ari.size()}, archive{arx.data(), arx.size()};
        require(index.u32(32) == 11 && index.equal(0x50, "E410A_ATOP.NJ\0", 14) && index.u32(0x70) == NjSize &&
                    index.u32(0x74) == NjOffset && index.equal(0xf0, "MSGUN01.PVR\0", 12) &&
                    index.u32(0x110) == PvrSize && index.u32(0x114) == PvrOffset,
                "unexpected E410 archive entries");
        Span nj = archive.sub(NjOffset, NjSize), pvr = archive.sub(PvrOffset, PvrSize);
        require(crc32(nj.p, nj.size) == 0x0773ccf0 && crc32(pvr.p, pvr.size) == 0x7713c0f4,
                "native pistol file integrity failed");
        Span tl{nullptr, 0}, cm{nullptr, 0};
        unsigned chunks = 0;
        for (size_t p = 0; p < nj.size;)
        {
            uint32_t size = nj.u32(p + 4);
            Span payload = nj.sub(p + 8, size);
            ++chunks;
            if (nj.equal(p, "NJTL", 4))
            {
                require(tl.size == 0, "duplicate native texture list");
                tl = payload;
            }
            else if (nj.equal(p, "NJCM", 4))
            {
                require(cm.size == 0, "duplicate native model");
                cm = payload;
            }
            else
                require(nj.equal(p, "POF0", 4), "unknown native model chunk");
            p += 8 + size;
        }
        require(chunks == 4 && tl.size == 184 && cm.size == 26904, "native pistol container is incomplete");
        require(tl.u32(0) == 8 && tl.u32(4) == 9, "native texture list is invalid");
        uint32_t name = tl.u32(8 + 3 * 12);
        require(tl.equal(name, "msgun01\0", 8), "wrong native pistol texture");
        // A rigid object owning all vertices/strips: no hand, animated body or parent pose.
        require(cm.u32(0x291c) == 0x54 && cm.u32(0x2920) == 0x2dc8 && cm.u32(0x2948) == 0 && cm.u32(0x294c) == 0,
                "unexpected native pistol object");
        require(cm.u32(0x2dc8) == 0x2b34 && cm.u32(0x2dcc) == 0x2950, "unexpected native pistol mesh pointers");
        require(cm.u32(0x2b34) == 0x00a30029 && cm.u32(0x2b38) == 0x001b0000 && cm.u32(0x2dc4) == 255,
                "native pistol vertices are incomplete");
        std::array<Vec, 27> vertices, normals;
        for (size_t i = 0; i < vertices.size(); i++)
            for (size_t a = 0; a < 3; a++)
            {
                vertices[i][a] = cm.f32(0x2b3c + i * 24 + a * 4);
                normals[i][a] = cm.f32(0x2b48 + i * 24 + a * 4);
                require(std::abs(vertices[i][a]) < 100, "native pistol vertex is outside its bounds");
            }
        require(cm.u32(0x2950) == 0x00062517 && cm.u32(0x2954) == 0xffffffff && cm.u32(0x2960) == 0x40030408 &&
                    cm.u16(0x2964) == 65 && cm.u16(0x2966) == 228 && cm.u16(0x2968) == 10 && cm.u32(0x2b30) == 255,
                "native pistol strips/material are incomplete");
        std::vector<Triangle> triangles;
        size_t p = 0x296a;
        unsigned stripCorners = 0;
        for (int strip = 0; strip < 10; strip++)
        {
            int count = int16_t(cm.u16(p));
            p += 2;
            bool reverse = count < 0;
            if (reverse)
                count = -count;
            require(count >= 3 && count <= 30, "invalid native pistol strip length");
            stripCorners += unsigned(count);
            std::vector<Corner> corners;
            for (int i = 0; i < count; i++)
            {
                require(p <= 0x2b30 && 6 <= 0x2b30 - p, "native pistol strip extends out of bounds");
                Corner c{cm.u16(p), int16_t(cm.u16(p + 2)), int16_t(cm.u16(p + 4))};
                p += 6;
                require(c.index < 27 && c.u >= 0 && c.u <= 255 && c.v >= 0 && c.v <= 255,
                        "native pistol strip index/UV is invalid");
                corners.push_back(c);
            }
            for (int i = 0; i < count - 2; i++)
            {
                Triangle t{corners[size_t(i)], corners[size_t(i + 1)], corners[size_t(i + 2)]};
                if ((i & 1) != int(reverse))
                    std::swap(t[0], t[1]);
                triangles.push_back(t);
            }
        }
        require(stripCorners == 72 && triangles.size() == 52 && p == 0x2b2e && cm.u16(p) == 0,
                "native pistol strip payload is incomplete");
        Vec z = normal(sub(vertices[7], vertices[4])), x = normal(sub(vertices[8], vertices[0]));
        x = normal(sub(x, mul(z, dot(z, x))));
        std::array<Vec, 3> frame{x, cross(z, x), z};
        Vec origin = mul(add(add(vertices[16], vertices[19]), add(vertices[17], vertices[18])), 0.25);
        double lo = 1e9, hi = -1e9;
        for (const Vec &v : vertices)
        {
            double zc = dot(v, z);
            lo = std::min(lo, zc);
            hi = std::max(hi, zc);
        }
        require(hi - lo > 24 && hi - lo < 25, "native pistol barrel extent is invalid");
        double scale = 0.24 / (hi - lo);
        Vec muzzle{};
        for (unsigned i : {0u, 4u, 8u, 12u})
            muzzle = add(muzzle, vertices[i]);
        muzzle = mul(axes(sub(mul(muzzle, 0.25), origin), frame), scale);
        auto rgba = texture(pvr);
        std::vector<uint8_t> result;
        result.reserve(DeathGunCacheBytes);
        result.insert(result.end(), {'H', 'N', 'D', '1'});
        word(result, 2);
        for (double f : muzzle)
            number(result, f);
        for (int i = 0; i < 4; i++)
            number(result, 0);
        for (int i = 0; i < 16; i++)
            number(result, i % 5 == 0 ? 1 : 0);
        word(result, 3);
        word(result, 10);
        word(result, 1);
        word(result, 64);
        word(result, 64);
        word(result, 3);
        result.insert(result.end(), rgba.begin(), rgba.end());
        word(result, 4);
        word(result, 1);
        word(result, 0);
        word(result, 156);
        for (const Triangle &t : triangles)
            for (const Corner &c : t)
            {
                Vec pos = mul(axes(sub(vertices[c.index], origin), frame), scale),
                    n = normal(axes(normals[c.index], frame));
                for (double f : pos)
                    number(result, f);
                for (double f : n)
                    number(result, f);
                number(result, double(c.u) / 255);
                number(result, double(c.v) / 255);
                word(result, 0xffffffff);
            }
        word(result, 0);
        word(result, 0);
        word(result, 0);
        if (!validDeathGunCache(result, error))
            return false;
        out = std::move(result);
        return true;
    }
    catch (const std::exception &e)
    {
        error = e.what();
        out.clear();
        return false;
    }
}
} // namespace vr::hands
