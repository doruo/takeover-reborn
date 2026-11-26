#include "swf/Movie.h"

#include "Records.h"
#include "swf/BitReader.h"
#include "swf/TagCode.h"

#include <zlib.h>

#include <fstream>
#include <iterator>
#include <string>

namespace swf {

    // /----- TAG HELPERS -----/

    namespace {

        using detail::readColorTransformWithAlpha;
        using detail::readFilterList;
        using detail::readMatrix;
        using detail::readPlaceObject;
        using detail::readRect;
        using detail::readRgb;
        using detail::readRgba;
        using detail::readSoundInfo;

        RawTag readTagHeader(BitReader& in) {
            const std::uint16_t codeAndLength = in.u16();
            const std::uint16_t code = codeAndLength >> 6;
            std::uint32_t length = codeAndLength & 0x3F;
            if (length == 0x3F)
                length = in.u32();

            return {code, in.bytes(length)};
        }

        std::vector<std::uint8_t> inflate(Bytes compressed, std::size_t expectedSize) {
            std::vector<std::uint8_t> out(expectedSize);
            uLongf outSize = static_cast<uLongf>(out.size());
            const int rc = ::uncompress(out.data(), &outSize, compressed.data(), static_cast<uLong>(compressed.size()));
            if (rc != Z_OK && rc != Z_BUF_ERROR)
                throw FormatError("zlib error " + std::to_string(rc));

            out.resize(outSize);
            return out;
        }

        int decodeSoundRate(unsigned code) {
            switch (code) {
            case 0: return 5512;
            case 1: return 11025;
            case 2: return 22050;
            default: return 44100;
            }
        }

        /// Handles the tags that may appear inside a timeline (root or sprite).
        /// Returns false for tags that are not timeline-related.
        bool parseTimelineTag(const RawTag& tag, Timeline& timeline) {
            if (timeline.frames.empty())
                timeline.frames.emplace_back();

            Frame& frame = timeline.frames.back();
            BitReader in(tag.body);

            switch (static_cast<TagCode>(tag.code)) {
            case TagCode::ShowFrame:
                timeline.frames.emplace_back();
                return true;
            case TagCode::PlaceObject2:
                frame.commands.emplace_back(readPlaceObject(in, 2));
                return true;
            case TagCode::PlaceObject3:
                frame.commands.emplace_back(readPlaceObject(in, 3));
                return true;
            case TagCode::RemoveObject: {
                in.u16(); // character id, redundant with the depth
                frame.commands.emplace_back(RemoveObject{in.u16()});

                return true;
            }
            case TagCode::RemoveObject2:
                frame.commands.emplace_back(RemoveObject{in.u16()});
                return true;
            case TagCode::FrameLabel:
                frame.labels.push_back(in.string());
                return true;
            case TagCode::StartSound: {
                StartSound s;
                s.soundId = in.u16();
                s.info = readSoundInfo(in);
                frame.sounds.push_back(std::move(s));

                return true;
            }
            case TagCode::SoundStreamHead:
            case TagCode::SoundStreamHead2:
            case TagCode::SoundStreamBlock:
                // Stream heads exist in a few clips but no stream block carries data
                // in this game, so there is nothing to play.
                return true;
            case TagCode::End:
                return true;
            default:
                return false;
            }
        }

        /// The SWF timeline always ends with a ShowFrame followed by End, which leaves
        /// one trailing empty frame in our representation; drop it.
        void finalizeTimeline(Timeline& timeline, int declaredFrameCount) {
            while (static_cast<int>(timeline.frames.size()) > declaredFrameCount && !timeline.frames.empty()) {
                const Frame& last = timeline.frames.back();
                if (!last.commands.empty() || !last.sounds.empty() || !last.labels.empty())
                    break;

                timeline.frames.pop_back();
            }

            // A sprite may declare more frames than it has ShowFrame tags; pad so the
            // runtime's totalFrames matches Flash.
            while (static_cast<int>(timeline.frames.size()) < declaredFrameCount)
                timeline.frames.emplace_back();
        }

        // /----- DEFINITION PARSERS -----/

        ShapeDef parseShape(BitReader& in, int version) {
            ShapeDef s;
            s.id = in.u16();
            s.version = version;
            s.bounds = readRect(in);
            s.edgeBounds = s.bounds;

            if (version == 4) {
                s.edgeBounds = readRect(in);
                in.ub(5);
                s.usesFillWindingRule = in.flag();
                in.ub(2); // non-scaling / scaling strokes hints
                in.align();
            }

            s.records = in.rest();
            return s;
        }

        MorphShapeDef parseMorphShape(BitReader& in, int version) {
            MorphShapeDef m;
            m.id = in.u16();
            m.version = version;
            m.startBounds = readRect(in);
            m.endBounds = readRect(in);

            if (version == 2) {
                readRect(in); // start edge bounds
                readRect(in); // end edge bounds
                in.u8();      // flags
            }

            m.records = in.rest();
            return m;
        }

        ButtonDef parseButton2(BitReader& in) {
            ButtonDef b;
            b.id = in.u16();
            b.trackAsMenu = (in.u8() & 1) != 0;
            in.u16(); // action offset; AS3 buttons carry no actions

            for (;;) {
                const std::uint8_t flags = in.u8();
                if (flags == 0)
                    break;

                ButtonRecord r;
                const bool hasBlendMode = flags & 0x20;
                const bool hasFilters = flags & 0x10;
                r.stateHitTest = flags & 0x08;
                r.stateDown = flags & 0x04;
                r.stateOver = flags & 0x02;
                r.stateUp = flags & 0x01;
                r.characterId = in.u16();
                r.depth = in.u16();
                r.matrix = readMatrix(in);
                r.colorTransform = readColorTransformWithAlpha(in);

                if (hasFilters)
                    r.filters = readFilterList(in);

                if (hasBlendMode) {
                    const std::uint8_t mode = in.u8();
                    r.blendMode = static_cast<BlendMode>(mode == 0 ? 1 : mode);
                }

                b.records.push_back(std::move(r));
            }

            return b;
        }

        EditTextDef parseEditText(BitReader& in) {
            EditTextDef t;
            t.id = in.u16();
            t.bounds = readRect(in);
            const std::uint8_t f1 = in.u8();
            const std::uint8_t f2 = in.u8();
            const bool hasText = f1 & 0x80;
            t.wordWrap = f1 & 0x40;
            t.multiline = f1 & 0x20;
            t.password = f1 & 0x10;
            t.readOnly = f1 & 0x08;
            const bool hasTextColor = f1 & 0x04;
            const bool hasMaxLength = f1 & 0x02;
            const bool hasFont = f1 & 0x01;
            const bool hasFontClass = f2 & 0x80;
            t.autoSize = f2 & 0x40;
            t.hasLayout = f2 & 0x20;
            t.noSelect = f2 & 0x10;
            t.border = f2 & 0x08;
            t.wasStatic = f2 & 0x04;
            t.html = f2 & 0x02;
            t.useOutlines = f2 & 0x01;

            if (hasFont)
                t.fontId = in.u16();

            if (hasFontClass)
                t.fontClass = in.string();

            if (hasFont || hasFontClass)
                t.fontHeight = in.u16();

            if (hasTextColor)
                t.textColor = readRgba(in);

            if (hasMaxLength)
                t.maxLength = in.u16();

            if (t.hasLayout) {
                t.align = static_cast<EditTextDef::Align>(in.u8());
                t.leftMargin = in.u16();
                t.rightMargin = in.u16();
                t.indent = in.u16();
                t.leading = in.s16();
            }

            t.variableName = in.string();

            if (hasText)
                t.initialText = in.string();

            return t;
        }

        TextDef parseText(BitReader& in, bool withAlpha) {
            TextDef t;
            t.id = in.u16();
            t.bounds = readRect(in);
            t.matrix = readMatrix(in);
            const unsigned glyphBits = in.u8();
            const unsigned advanceBits = in.u8();

            for (;;) {
                const std::uint8_t flags = in.u8();
                if (flags == 0)
                    break;

                TextRecord r;

                if (flags & 0x08)
                    r.fontId = in.u16();

                if (flags & 0x04)
                    r.color = withAlpha ? readRgba(in) : readRgb(in);

                if (flags & 0x01)
                    r.xOffset = in.s16();

                if (flags & 0x02)
                    r.yOffset = in.s16();

                if (flags & 0x08)
                    r.height = in.u16();

                const int glyphCount = in.u8();
                r.glyphs.resize(glyphCount);

                for (auto& g : r.glyphs) {
                    g.index = in.ub(glyphBits);
                    g.advance = in.sb(advanceBits);
                }

                in.align();
                t.records.push_back(std::move(r));
            }

            return t;
        }

        FontDef parseFont3(BitReader& in) {
            FontDef f;
            f.id = in.u16();
            const std::uint8_t flags = in.u8();
            f.hasLayout = flags & 0x80;
            f.smallText = flags & 0x20;
            const bool wideOffsets = flags & 0x08;
            const bool wideCodes = flags & 0x04;
            f.italic = flags & 0x02;
            f.bold = flags & 0x01;
            f.languageCode = in.u8();
            const int nameLength = in.u8();
            const Bytes nameBytes = in.bytes(nameLength);
            f.name.assign(nameBytes.begin(), nameBytes.end());

            while (!f.name.empty() && f.name.back() == '\0')
                f.name.pop_back();

            const int glyphCount = in.u16();
            if (glyphCount > 0) {
                // Offset table: one entry per glyph plus the CodeTableOffset, all
                // relative to the start of the table. Fonts embedded only for their
                // name ("device font" references) have no glyphs and no tables.
                const std::size_t tableStart = in.bytePosition();
                std::vector<std::uint32_t> offsets(glyphCount + 1);
                for (auto& offset : offsets)
                    offset = wideOffsets ? in.u32() : in.u16();

                f.glyphShapes.reserve(glyphCount);

                for (int i = 0; i < glyphCount; ++i) {
                    in.seek(tableStart + offsets[i]);
                    f.glyphShapes.push_back(in.bytes(offsets[i + 1] - offsets[i]));
                }

                in.seek(tableStart + offsets[glyphCount]);
                f.codeTable.resize(glyphCount);

                for (auto& code : f.codeTable)
                    code = wideCodes ? in.u16() : in.u8();
            }

            if (in.remaining() == 0)
                f.hasLayout = false; // flag set without a layout block

            if (f.hasLayout) {
                f.ascent = in.u16();
                f.descent = in.u16();
                f.leading = in.s16();
                f.advances.resize(glyphCount);

                for (auto& a : f.advances)
                    a = in.s16();

                f.glyphBounds.resize(glyphCount);

                for (auto& r : f.glyphBounds)
                    r = readRect(in);

                const int kerningCount = in.u16();
                f.kerning.resize(kerningCount);

                for (auto& k : f.kerning) {
                    k.code1 = wideCodes ? in.u16() : in.u8();
                    k.code2 = wideCodes ? in.u16() : in.u8();
                    k.adjustment = in.s16();
                }
            }

            return f;
        }

        SoundDef parseSound(BitReader& in) {
            SoundDef s;
            s.id = in.u16();
            s.format = static_cast<SoundDef::Format>(in.ub(4));
            s.sampleRate = decodeSoundRate(in.ub(2));
            s.is16Bit = in.flag();
            s.stereo = in.flag();
            s.sampleCount = in.u32();
            s.data = in.rest();

            return s;
        }

        BitmapDef parseLossless(BitReader& in, bool withAlpha) {
            BitmapDef b;
            b.id = in.u16();
            b.kind = withAlpha ? BitmapDef::Kind::LosslessAlpha : BitmapDef::Kind::Lossless;
            b.losslessFormat = in.u8();
            b.width = in.u16();
            b.height = in.u16();

            if (b.losslessFormat == 3)
                b.colorTableSize = in.u8();

            b.imageData = in.rest();
            return b;
        }

    } // namespace

    // /----- LOADING -----/

    Movie Movie::loadFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw FormatError("cannot open " + path.string());

        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return loadBytes(std::move(bytes));
    }

    Movie Movie::loadBytes(std::vector<std::uint8_t> fileBytes) {
        if (fileBytes.size() < 8)
            throw FormatError("file too small to be a SWF");

        Movie movie;
        movie.header_.compression = static_cast<char>(fileBytes[0]);

        if (fileBytes[1] != 'W' || fileBytes[2] != 'S')
            throw FormatError("missing SWF signature");

        movie.header_.version = fileBytes[3];
        movie.header_.fileLength = static_cast<std::uint32_t>(fileBytes[4]) | (fileBytes[5] << 8) | (fileBytes[6] << 16) |
                                   (static_cast<std::uint32_t>(fileBytes[7]) << 24);

        const Bytes payload(fileBytes.data() + 8, fileBytes.size() - 8);

        switch (movie.header_.compression) {
        case 'F':
            movie.data_.assign(payload.begin(), payload.end());
            break;
        case 'C':
            movie.data_ = inflate(payload, movie.header_.fileLength - 8);
            break;
        default:
            throw FormatError("LZMA-compressed SWF files are not supported");
        }

        movie.parse();
        return movie;
    }

    const Character* Movie::find(CharacterId id) const {
        const auto it = characters_.find(id);
        return it == characters_.end() ? nullptr : &it->second;
    }

    std::optional<CharacterId> Movie::findClass(const std::string& className) const {
        const auto it = classToId_.find(className);
        if (it == classToId_.end())
            return std::nullopt;

        return it->second;
    }

    // /----- TAG DISPATCH -----/

    void Movie::parse() {
        BitReader in(data_);
        header_.frameSize = readRect(in);
        header_.frameRate = in.u16() / 256.0; // 8.8 fixed point
        header_.frameCount = in.u16();
        headerBytes_ = Bytes(data_.data(), in.bytePosition());

        while (!in.atEnd()) {
            const std::size_t tagOffset = in.bytePosition();
            const RawTag tag = readTagHeader(in);
            ++tagStats_[tag.code];

            if (tag.code == static_cast<std::uint16_t>(TagCode::End))
                break;

            rawTags_.push_back(tag);

            try {
                parseDefinitionTag(tag);
            } catch (const FormatError& e) {
                throw FormatError(std::string(tagName(tag.code)) + " tag at offset " + std::to_string(tagOffset + 8) +
                                  ": " + e.what());
            }
        }

        finalizeTimeline(root_, header_.frameCount);
    }

    void Movie::parseDefinitionTag(const RawTag& tag) {
        {
            if (parseTimelineTag(tag, root_))
                return;

            BitReader body(tag.body);

            switch (static_cast<TagCode>(tag.code)) {
            case TagCode::SetBackgroundColor:
                background_ = readRgb(body);
                break;
            case TagCode::DefineShape: { auto s = parseShape(body, 1); characters_.emplace(s.id, std::move(s)); break; }
            case TagCode::DefineShape2: { auto s = parseShape(body, 2); characters_.emplace(s.id, std::move(s)); break; }
            case TagCode::DefineShape3: { auto s = parseShape(body, 3); characters_.emplace(s.id, std::move(s)); break; }
            case TagCode::DefineShape4: { auto s = parseShape(body, 4); characters_.emplace(s.id, std::move(s)); break; }
            case TagCode::DefineMorphShape: { auto m = parseMorphShape(body, 1); characters_.emplace(m.id, std::move(m)); break; }
            case TagCode::DefineMorphShape2: { auto m = parseMorphShape(body, 2); characters_.emplace(m.id, std::move(m)); break; }
            case TagCode::JpegTables:
                jpegTables_ = tag.body;
                break;
            case TagCode::DefineBits: {
                BitmapDef b;
                b.id = body.u16();
                b.kind = BitmapDef::Kind::Jpeg;
                b.imageData = body.rest();
                b.jpegTables = jpegTables_; // movies merged from several FLAs carry several tables
                characters_.emplace(b.id, std::move(b));
                break;
            }
            case TagCode::DefineBitsJpeg2: {
                BitmapDef b;
                b.id = body.u16();
                b.kind = BitmapDef::Kind::EncodedImage;
                b.imageData = body.rest();
                characters_.emplace(b.id, std::move(b));
                break;
            }
            case TagCode::DefineBitsJpeg3: {
                BitmapDef b;
                b.id = body.u16();
                b.kind = BitmapDef::Kind::JpegWithAlpha;
                const std::uint32_t alphaOffset = body.u32();
                b.imageData = body.bytes(alphaOffset);
                b.alphaData = body.rest();
                characters_.emplace(b.id, std::move(b));
                break;
            }
            case TagCode::DefineBitsLossless: { auto b = parseLossless(body, false); characters_.emplace(b.id, std::move(b)); break; }
            case TagCode::DefineBitsLossless2: { auto b = parseLossless(body, true); characters_.emplace(b.id, std::move(b)); break; }
            case TagCode::DefineSprite: {
                SpriteDef sprite;
                sprite.id = body.u16();
                const int declaredFrames = body.u16();

                while (!body.atEnd()) {
                    const RawTag inner = readTagHeader(body);
                    ++tagStats_[inner.code];

                    if (inner.code == static_cast<std::uint16_t>(TagCode::End))
                        break;

                    bool handled = false;

                    try {
                        handled = parseTimelineTag(inner, sprite.timeline);
                    } catch (const FormatError& e) {
                        throw FormatError("sprite " + std::to_string(sprite.id) + ", frame " +
                                          std::to_string(sprite.timeline.frames.size()) + ", " +
                                          std::string(tagName(inner.code)) + ": " + e.what());
                    }

                    if (!handled) {
                        throw FormatError("unexpected tag " + std::string(tagName(inner.code)) + " inside sprite " +
                                          std::to_string(sprite.id));
                    }
                }

                finalizeTimeline(sprite.timeline, declaredFrames);
                characters_.emplace(sprite.id, std::move(sprite));
                break;
            }
            case TagCode::DefineButton2: { auto b = parseButton2(body); characters_.emplace(b.id, std::move(b)); break; }
            case TagCode::DefineEditText: { auto t = parseEditText(body); characters_.emplace(t.id, std::move(t)); break; }
            case TagCode::DefineText: { auto t = parseText(body, false); characters_.emplace(t.id, std::move(t)); break; }
            case TagCode::DefineText2: { auto t = parseText(body, true); characters_.emplace(t.id, std::move(t)); break; }
            case TagCode::DefineFont3: { auto f = parseFont3(body); characters_.emplace(f.id, std::move(f)); break; }
            case TagCode::DefineFontName: {
                const CharacterId fontId = body.u16();
                if (auto it = characters_.find(fontId); it != characters_.end()) {
                    if (auto* font = std::get_if<FontDef>(&it->second))
                        font->displayName = body.string();
                }
                break;
            }
            case TagCode::DefineSound: { auto s = parseSound(body); characters_.emplace(s.id, std::move(s)); break; }
            case TagCode::CsmTextSettings: {
                const CharacterId textId = body.u16();
                TextRenderSettings settings;
                settings.useAdvancedAntiAliasing = body.ub(2) == 1;
                settings.gridFit = static_cast<int>(body.ub(3));
                body.ub(3);
                settings.thickness = body.f32();
                settings.sharpness = body.f32();
                textSettings_[textId] = settings;
                break;
            }
            case TagCode::SymbolClass:
            case TagCode::ExportAssets: {
                const int count = body.u16();
                for (int i = 0; i < count; ++i) {
                    const CharacterId id = body.u16();
                    std::string name = body.string();
                    classToId_[name] = id;
                    symbolClasses_[id] = std::move(name);
                }
                break;
            }
            default:
                // Code (DoABC), metadata and debugging tags carry nothing the
                // runtime needs.
                break;
            }
        }
    }

    // /----- SWF WRITING -----/

    std::vector<std::uint8_t> writeSwf(int version, Bytes headerBytes, const std::vector<RawTag>& tags) {
        std::vector<std::uint8_t> body(headerBytes.begin(), headerBytes.end());
        auto put16 = [&body](std::uint32_t v) {
            body.push_back(static_cast<std::uint8_t>(v));
            body.push_back(static_cast<std::uint8_t>(v >> 8));
        };

        auto put32 = [&](std::uint32_t v) {
            put16(v & 0xFFFF);
            put16(v >> 16);
        };

        auto putTag = [&](std::uint16_t code, Bytes payload) {
            // Always use the long form: valid for every tag, and some tags
            // (bitmaps) require it.
            put16(static_cast<std::uint32_t>(code << 6) | 0x3F);
            put32(static_cast<std::uint32_t>(payload.size()));
            body.insert(body.end(), payload.begin(), payload.end());
        };

        for (const RawTag& tag : tags)
            putTag(tag.code, tag.body);

        putTag(static_cast<std::uint16_t>(TagCode::End), {});

        uLongf compressedSize = compressBound(static_cast<uLong>(body.size()));
        std::vector<std::uint8_t> file(8 + compressedSize);
        const int rc = ::compress2(file.data() + 8, &compressedSize, body.data(), static_cast<uLong>(body.size()),
                                   Z_BEST_COMPRESSION);
        if (rc != Z_OK)
            throw FormatError("zlib compression failed");

        file.resize(8 + compressedSize);
        const std::uint32_t totalLength = static_cast<std::uint32_t>(body.size() + 8);
        file[0] = 'C';
        file[1] = 'W';
        file[2] = 'S';
        file[3] = static_cast<std::uint8_t>(version);

        for (int i = 0; i < 4; ++i)
            file[4 + i] = static_cast<std::uint8_t>(totalLength >> (8 * i));

        return file;
    }

} // namespace swf
