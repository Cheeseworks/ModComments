#include "../Staff.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

bool StaffBanPopup::init(CommentUser user, Callback&& cb) {
    m_user = std::move(user);
    m_callback = std::move(cb);

    if (!GeodePopup::init(225.0f, 175.0f)) return false;

    setID("staff-ban-popup"_spr);
    setTitle(fmt::format("Ban {}?", m_user.username));

    return true;
}

StaffBanPopup* StaffBanPopup::create(CommentUser user, Callback&& cb) {
    auto ret = new StaffBanPopup();
    if (ret->init(std::move(user), std::move(cb))) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}

bool StaffPopup::init(Comment cmmt, Callback&& cb) {
    m_comment = std::move(cmmt);
    m_callback = std::move(cb);

    if (!GeodePopup::init(265.0f, 190.0f)) return false;

    setID("staff-popup"_spr);
    setTitle("Mod Comments Staff Panel");

    return true;
}

StaffPopup* StaffPopup::create(Comment cmmt, Callback&& cb) {
    auto ret = new StaffPopup();
    if (ret->init(std::move(cmmt), std::move(cb))) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}