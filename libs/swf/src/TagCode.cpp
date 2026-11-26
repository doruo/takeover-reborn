#include "swf/TagCode.h"

namespace swf {

std::string_view tagName(std::uint16_t code) {
    switch (code) {
    case 0: return "End";
    case 1: return "ShowFrame";
    case 2: return "DefineShape";
    case 4: return "PlaceObject";
    case 5: return "RemoveObject";
    case 6: return "DefineBits";
    case 7: return "DefineButton";
    case 8: return "JpegTables";
    case 9: return "SetBackgroundColor";
    case 10: return "DefineFont";
    case 11: return "DefineText";
    case 12: return "DoAction";
    case 13: return "DefineFontInfo";
    case 14: return "DefineSound";
    case 15: return "StartSound";
    case 17: return "DefineButtonSound";
    case 18: return "SoundStreamHead";
    case 19: return "SoundStreamBlock";
    case 20: return "DefineBitsLossless";
    case 21: return "DefineBitsJpeg2";
    case 22: return "DefineShape2";
    case 23: return "DefineButtonCxform";
    case 24: return "Protect";
    case 26: return "PlaceObject2";
    case 28: return "RemoveObject2";
    case 32: return "DefineShape3";
    case 33: return "DefineText2";
    case 34: return "DefineButton2";
    case 35: return "DefineBitsJpeg3";
    case 36: return "DefineBitsLossless2";
    case 37: return "DefineEditText";
    case 39: return "DefineSprite";
    case 41: return "ProductInfo";
    case 43: return "FrameLabel";
    case 45: return "SoundStreamHead2";
    case 46: return "DefineMorphShape";
    case 48: return "DefineFont2";
    case 56: return "ExportAssets";
    case 57: return "ImportAssets";
    case 58: return "EnableDebugger";
    case 59: return "DoInitAction";
    case 60: return "DefineVideoStream";
    case 61: return "VideoFrame";
    case 62: return "DefineFontInfo2";
    case 63: return "DebugId";
    case 64: return "EnableDebugger2";
    case 65: return "ScriptLimits";
    case 66: return "SetTabIndex";
    case 69: return "FileAttributes";
    case 70: return "PlaceObject3";
    case 71: return "ImportAssets2";
    case 73: return "DefineFontAlignZones";
    case 74: return "CsmTextSettings";
    case 75: return "DefineFont3";
    case 76: return "SymbolClass";
    case 77: return "Metadata";
    case 78: return "DefineScalingGrid";
    case 82: return "DoAbc";
    case 83: return "DefineShape4";
    case 84: return "DefineMorphShape2";
    case 86: return "DefineSceneAndFrameLabelData";
    case 87: return "DefineBinaryData";
    case 88: return "DefineFontName";
    case 89: return "StartSound2";
    case 90: return "DefineBitsJpeg4";
    case 91: return "DefineFont4";
    case 93: return "EnableTelemetry";
    default: return "Unknown";
    }
}

} // namespace swf
