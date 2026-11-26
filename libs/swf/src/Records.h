// Records.h - decoders for the small structures reused by many tags.
// Internal to the swf library.
#pragma once

#include "swf/BitReader.h"
#include "swf/Timeline.h"
#include "swf/Types.h"

#include <vector>

namespace swf::detail {

Rect readRect(BitReader& in);
Rgba readRgb(BitReader& in);
Rgba readRgba(BitReader& in);
Matrix readMatrix(BitReader& in);
ColorTransform readColorTransformWithAlpha(BitReader& in);
/// CXFORM without alpha (DefineButtonCxform, PlaceObject v1).
ColorTransform readColorTransform(BitReader& in);
std::vector<Filter> readFilterList(BitReader& in);
SoundInfo readSoundInfo(BitReader& in);

/// PlaceObject2 (version 2) and PlaceObject3 (version 3).
PlaceObject readPlaceObject(BitReader& in, int version);

} // namespace swf::detail
