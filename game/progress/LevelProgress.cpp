#include "game/progress/LevelProgress.h"

#include "game/data/LevelInfo.h"
#include "game/framework/core/As3.h"
#include "game/framework/storage/SerializableObject.h"
#include "game/sim/battle/MissionStats.h"

#include <cmath>

namespace takeover {

    LevelProgress LevelProgress::fromMission(const MissionResult& result, const LevelInfo& level) {
        LevelProgress progress;
        progress.completeStatus = level.rankFor(result);
        progress.score = as3::toInt(100000 * std::floor(60 * 20 / result.missionTime));

        return progress;
    }

    bool LevelProgress::compareStatus(const LevelProgress& other) const {
        if (other.completeStatus == completeStatus)
            return other.score > score;

        return other.completeStatus > completeStatus;
    }

    void LevelProgress::serialize(ByteArray& data) const {
        SerializableObject object;
        object.add("scrs", SerializableObject::SHORT, static_cast<double>(score));
        object.add("stts", SerializableObject::SHORT, static_cast<double>(completeStatus));
        object.save(data);
    }

    void LevelProgress::deserialize(ByteArray& data) {
        SerializableObject object;
        object.load(data);

        if (object.has("scrs"))
            score = static_cast<int>(object.getNumber("scrs"));

        if (object.has("stts"))
            completeStatus = static_cast<int>(object.getNumber("stts"));
    }

} // namespace takeover
