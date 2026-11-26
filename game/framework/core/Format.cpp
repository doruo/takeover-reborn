#include "game/framework/core/Format.h"

#include <cmath>

namespace takeover::format {

std::string time(double totalSeconds, bool showHours, bool showMinutes, bool showSeconds, bool showMilliseconds) {
    std::string result;
    int seconds = static_cast<int>(std::floor(totalSeconds));
    const int milliseconds = static_cast<int>(std::floor(1000 * (totalSeconds - seconds)));
    int minutes = static_cast<int>(std::floor(seconds / 60.0));
    seconds -= minutes * 60;
    const int hours = static_cast<int>(std::floor(minutes / 60.0));
    minutes -= hours * 60;
    if (showHours) result += std::to_string(hours);
    if (showMinutes) {
        if (showHours) result += minutes < 10 ? ":0" : ":";
        result += std::to_string(minutes);
    }
    if (showSeconds) {
        if (showMinutes) result += seconds < 10 ? ":0" : ":";
        result += std::to_string(seconds);
    }
    if (showMilliseconds) {
        if (showSeconds) result += std::string(".") + (milliseconds < 100 ? "0" : "") + (milliseconds < 10 ? "0" : "");
        result += std::to_string(milliseconds);
    }
    return result;
}

} // namespace takeover::format
