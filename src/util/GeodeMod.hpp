#pragma once

#include <Geode/Geode.hpp>

namespace cw::modcmmts {
    struct GeodeModDev final {
        std::string username;
        std::string displayName;
        bool owner = false;
    };

    struct GeodeModVersion final {
        std::string name;
        std::string version;
    };

    struct GeodeMod final {
        std::string id;
        std::vector<GeodeModDev> developers;
        asp::SmallVec<GeodeModVersion, 1> versions;
    };
};

template <>
struct matjson::Serialize<cw::modcmmts::GeodeModDev> final {
    static geode::Result<cw::modcmmts::GeodeModDev> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::GeodeModDev const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::GeodeModVersion> final {
    static geode::Result<cw::modcmmts::GeodeModVersion> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::GeodeModVersion const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::GeodeMod> final {
    static geode::Result<cw::modcmmts::GeodeMod> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::GeodeMod const& value);
};