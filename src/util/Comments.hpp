#pragma once

#include <Geode/Geode.hpp>

namespace cw::modcmmts {
    enum class ModLevel : uint8_t {
        None,
        Mod,
        Elder,
        Leaderboard,
    };

    struct UserIcons {
        uint16_t icon = 1;
        IconType iconType = IconType::Cube;
        uint8_t color1 = 1;
        uint8_t color2 = 1;
        uint8_t colorGlow = 1;
        bool useGlow = false;
    };

    struct CommentUser final : UserIcons {
        int id = 0;
        std::string username;
        bool staff;
        ModLevel gdMod;
        asp::SystemTime created;
    };

    struct Comment final {
        uint64_t id = 0;
        CommentUser author;
        std::string modId;
        std::string content;
        asp::SystemTime created;
        uint64_t likes = 0;
        uint64_t dislikes = 0;
        int8_t myVote = 0;
    };

    struct CommentReport final {
        uint64_t id = 0;
        CommentUser author;
        Comment comment;
        std::string reason;
        asp::SystemTime created;
    };

    struct CommentBannedUser final {
        CommentUser user;
        std::string reason;
        asp::SystemTime since;
        asp::Duration until;
    };
};

template <>
struct matjson::Serialize<cw::modcmmts::UserIcons> final {
    static geode::Result<cw::modcmmts::UserIcons> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::UserIcons const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::CommentUser> final {
    static geode::Result<cw::modcmmts::CommentUser> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::CommentUser const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::Comment> final {
    static geode::Result<cw::modcmmts::Comment> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::Comment const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::CommentReport> final {
    static geode::Result<cw::modcmmts::CommentReport> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::CommentReport const& value);
};

template <>
struct matjson::Serialize<cw::modcmmts::CommentBannedUser> final {
    static geode::Result<cw::modcmmts::CommentBannedUser> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::CommentBannedUser const& value);
};