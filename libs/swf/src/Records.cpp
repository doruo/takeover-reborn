#include "Records.h"

#include <string>

namespace swf::detail {

Rect readRect(BitReader& in) {
    in.align();
    const unsigned bits = in.ub(5);
    Rect r;
    r.xMin = in.sb(bits);
    r.xMax = in.sb(bits);
    r.yMin = in.sb(bits);
    r.yMax = in.sb(bits);
    in.align();
    return r;
}

Rgba readRgb(BitReader& in) {
    Rgba c;
    c.r = in.u8();
    c.g = in.u8();
    c.b = in.u8();
    c.a = 255;
    return c;
}

Rgba readRgba(BitReader& in) {
    Rgba c;
    c.r = in.u8();
    c.g = in.u8();
    c.b = in.u8();
    c.a = in.u8();
    return c;
}

Matrix readMatrix(BitReader& in) {
    in.align();
    Matrix m;
    if (in.flag()) { // HasScale
        const unsigned bits = in.ub(5);
        m.a = in.fb(bits);
        m.d = in.fb(bits);
    }
    if (in.flag()) { // HasRotate
        const unsigned bits = in.ub(5);
        m.b = in.fb(bits);
        m.c = in.fb(bits);
    }
    const unsigned bits = in.ub(5);
    m.tx = in.sb(bits);
    m.ty = in.sb(bits);
    in.align();
    return m;
}

namespace {

ColorTransform readCxformBody(BitReader& in, bool withAlpha) {
    in.align();
    const bool hasAdd = in.flag();
    const bool hasMul = in.flag();
    const unsigned bits = in.ub(4);
    ColorTransform cx;
    if (hasMul) {
        cx.rMul = static_cast<std::int16_t>(in.sb(bits));
        cx.gMul = static_cast<std::int16_t>(in.sb(bits));
        cx.bMul = static_cast<std::int16_t>(in.sb(bits));
        if (withAlpha) cx.aMul = static_cast<std::int16_t>(in.sb(bits));
    }
    if (hasAdd) {
        cx.rAdd = static_cast<std::int16_t>(in.sb(bits));
        cx.gAdd = static_cast<std::int16_t>(in.sb(bits));
        cx.bAdd = static_cast<std::int16_t>(in.sb(bits));
        if (withAlpha) cx.aAdd = static_cast<std::int16_t>(in.sb(bits));
    }
    in.align();
    return cx;
}

/// Trailing flags shared by the shadow/glow/bevel family:
/// Inner, Knockout, CompositeSource, [OnTop], Passes.
struct FilterFlags {
    bool inner, knockout, compositeSource, onTop;
    int passes;
};

FilterFlags readFilterFlags(BitReader& in, bool hasOnTop) {
    FilterFlags f{};
    f.inner = in.flag();
    f.knockout = in.flag();
    f.compositeSource = in.flag();
    f.onTop = hasOnTop ? in.flag() : false;
    f.passes = static_cast<int>(in.ub(hasOnTop ? 4 : 5));
    in.align();
    return f;
}

} // namespace

ColorTransform readColorTransformWithAlpha(BitReader& in) { return readCxformBody(in, true); }
ColorTransform readColorTransform(BitReader& in) { return readCxformBody(in, false); }

std::vector<Filter> readFilterList(BitReader& in) {
    const int count = in.u8();
    std::vector<Filter> filters;
    filters.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int filterId = in.u8();
        switch (filterId) {
        case 0: {
            DropShadowFilter f;
            f.color = readRgba(in);
            f.blurX = static_cast<float>(in.fixed16());
            f.blurY = static_cast<float>(in.fixed16());
            f.angle = static_cast<float>(in.fixed16());
            f.distance = static_cast<float>(in.fixed16());
            f.strength = in.fixed8();
            const FilterFlags flags = readFilterFlags(in, false);
            f.inner = flags.inner;
            f.knockout = flags.knockout;
            f.compositeSource = flags.compositeSource;
            f.passes = flags.passes;
            filters.emplace_back(f);
            break;
        }
        case 1: {
            BlurFilter f;
            f.blurX = static_cast<float>(in.fixed16());
            f.blurY = static_cast<float>(in.fixed16());
            f.passes = static_cast<int>(in.ub(5));
            in.ub(3); // reserved
            filters.emplace_back(f);
            break;
        }
        case 2: {
            GlowFilter f;
            f.color = readRgba(in);
            f.blurX = static_cast<float>(in.fixed16());
            f.blurY = static_cast<float>(in.fixed16());
            f.strength = in.fixed8();
            const FilterFlags flags = readFilterFlags(in, false);
            f.inner = flags.inner;
            f.knockout = flags.knockout;
            f.compositeSource = flags.compositeSource;
            f.passes = flags.passes;
            filters.emplace_back(f);
            break;
        }
        case 3: {
            BevelFilter f;
            f.shadowColor = readRgba(in);
            f.highlightColor = readRgba(in);
            f.blurX = static_cast<float>(in.fixed16());
            f.blurY = static_cast<float>(in.fixed16());
            f.angle = static_cast<float>(in.fixed16());
            f.distance = static_cast<float>(in.fixed16());
            f.strength = in.fixed8();
            const FilterFlags flags = readFilterFlags(in, true);
            f.inner = flags.inner;
            f.knockout = flags.knockout;
            f.compositeSource = flags.compositeSource;
            f.onTop = flags.onTop;
            f.passes = flags.passes;
            filters.emplace_back(f);
            break;
        }
        case 4:
        case 7: {
            GradientFilter f;
            f.isBevel = (filterId == 7);
            const int stopCount = in.u8();
            f.stops.resize(stopCount);
            for (auto& stop : f.stops) stop.color = readRgba(in);
            for (auto& stop : f.stops) stop.ratio = in.u8();
            f.blurX = static_cast<float>(in.fixed16());
            f.blurY = static_cast<float>(in.fixed16());
            f.angle = static_cast<float>(in.fixed16());
            f.distance = static_cast<float>(in.fixed16());
            f.strength = in.fixed8();
            const FilterFlags flags = readFilterFlags(in, true);
            f.inner = flags.inner;
            f.knockout = flags.knockout;
            f.compositeSource = flags.compositeSource;
            f.onTop = flags.onTop;
            f.passes = flags.passes;
            filters.emplace_back(f);
            break;
        }
        case 5: {
            ConvolutionFilter f;
            f.matrixX = in.u8();
            f.matrixY = in.u8();
            f.divisor = in.f32();
            f.bias = in.f32();
            f.matrix.resize(static_cast<std::size_t>(f.matrixX) * f.matrixY);
            for (float& v : f.matrix) v = in.f32();
            f.defaultColor = readRgba(in);
            in.ub(6); // reserved
            f.clamp = in.flag();
            f.preserveAlpha = in.flag();
            filters.emplace_back(f);
            break;
        }
        case 6: {
            ColorMatrixFilter f;
            for (float& v : f.matrix) v = in.f32();
            filters.emplace_back(f);
            break;
        }
        default:
            throw FormatError("unknown filter id " + std::to_string(filterId));
        }
    }
    return filters;
}

SoundInfo readSoundInfo(BitReader& in) {
    SoundInfo info;
    in.ub(2); // reserved
    info.syncStop = in.flag();
    info.syncNoMultiple = in.flag();
    const bool hasEnvelope = in.flag();
    const bool hasLoops = in.flag();
    const bool hasOutPoint = in.flag();
    const bool hasInPoint = in.flag();
    if (hasInPoint) info.inPoint = in.u32();
    if (hasOutPoint) info.outPoint = in.u32();
    if (hasLoops) info.loopCount = in.u16();
    if (hasEnvelope) {
        const int points = in.u8();
        info.envelope.resize(points);
        for (auto& p : info.envelope) {
            p.position44 = in.u32();
            p.leftLevel = in.u16();
            p.rightLevel = in.u16();
        }
    }
    return info;
}

PlaceObject readPlaceObject(BitReader& in, int version) {
    // Flag bits of the first byte (shared by PlaceObject2 and 3).
    constexpr std::uint8_t kHasClipActions = 0x80;
    constexpr std::uint8_t kHasClipDepth = 0x40;
    constexpr std::uint8_t kHasName = 0x20;
    constexpr std::uint8_t kHasRatio = 0x10;
    constexpr std::uint8_t kHasColorTransform = 0x08;
    constexpr std::uint8_t kHasMatrix = 0x04;
    constexpr std::uint8_t kHasCharacter = 0x02;
    constexpr std::uint8_t kMove = 0x01;
    // Second byte, PlaceObject3 only.
    constexpr std::uint8_t kHasOpaqueBackground = 0x40;
    constexpr std::uint8_t kHasVisible = 0x20;
    constexpr std::uint8_t kHasImage = 0x10;
    constexpr std::uint8_t kHasClassName = 0x08;
    constexpr std::uint8_t kHasCacheAsBitmap = 0x04;
    constexpr std::uint8_t kHasBlendMode = 0x02;
    constexpr std::uint8_t kHasFilterList = 0x01;

    const std::uint8_t flags = in.u8();
    const std::uint8_t flags3 = (version >= 3) ? in.u8() : 0;

    PlaceObject p;
    p.depth = in.u16();
    p.isMove = (flags & kMove) != 0;

    if ((flags3 & kHasClassName) || ((flags3 & kHasImage) && (flags & kHasCharacter))) {
        p.className = in.string();
    }
    if (flags & kHasCharacter) p.characterId = in.u16();
    if (flags & kHasMatrix) p.matrix = readMatrix(in);
    if (flags & kHasColorTransform) p.colorTransform = readColorTransformWithAlpha(in);
    if (flags & kHasRatio) p.ratio = in.u16();
    if (flags & kHasName) p.name = in.string();
    if (flags & kHasClipDepth) p.clipDepth = in.u16();
    if (flags3 & kHasFilterList) p.filters = readFilterList(in);
    if (flags3 & kHasBlendMode) {
        const std::uint8_t mode = in.u8();
        p.blendMode = static_cast<BlendMode>(mode == 0 ? 1 : mode);
    }
    if (flags3 & kHasCacheAsBitmap) {
        // Flash authoring tools sometimes set the flag but omit the byte when
        // it is the last field; the player then treats it as "enabled".
        p.cacheAsBitmap = in.remaining() > 0 ? in.u8() != 0 : true;
    }
    if (flags3 & kHasVisible) p.visible = in.u8() != 0;
    if (flags3 & kHasOpaqueBackground) readRgba(in); // never used by this game
    if (flags & kHasClipActions) {
        // AVM1 clip actions cannot appear in an AS3 movie; refuse rather than
        // silently mis-parse the rest of the timeline.
        throw FormatError("PlaceObject with AVM1 clip actions is not supported");
    }
    return p;
}

} // namespace swf::detail
