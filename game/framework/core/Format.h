// Format.h - text formatting helpers (original: _SafePkg_1882/Utils).
#pragma once

#include <string>

namespace takeover::format {

/// "m:ss", "h:mm:ss.mmm"... (original: Utils._SafeStr_2966).
std::string time(double seconds, bool showHours, bool showMinutes, bool showSeconds, bool showMilliseconds);

} // namespace takeover::format
