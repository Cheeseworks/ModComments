#include "../Staff.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

namespace cw::modcmmts {
    struct CommentUserStatusData final {
        std::string name;
        std::string description;
        std::string badgeSprite;
    };

    namespace impl {
        static constexpr uint8_t MAX_REASON_CHARS = 36;
    };
};

bool StaffBanPopup::init(CommentUser user, Callback&& cb) {
    m_user = std::move(user);
    m_callback = std::move(cb);

    if (!GeodePopup::init(265.0f, 195.0f, GeodePopupStyle::Alt2)) return false;

    setID("staff-ban-popup"_spr);
    setTitle(fmt::format("Ban {}?", m_user.username));

    m_inputBox = TextInput::create(m_mainLayer->getContentWidth() - 37.5f, "Tell us why...", cw::ui::font::chat);
    m_inputBox->setID("reason-input");
    m_inputBox->setMaxCharCount(impl::MAX_REASON_CHARS);
    m_inputBox->setContentHeight(m_inputBox->getScaledContentHeight() * 1.5f);

    createInputLimitLabel(impl::MAX_REASON_CHARS);

    setMildLimitWarning(20);
    setModerateLimitWarning(32);

    m_mainLayer->addChildAtPosition(m_inputBox, Anchor::Top, { 0.0f, -50.0f });

    cw::addHelpButton(
        m_mainLayer,
        fmt::format("If you'd like to ban <co>{}</c>, use the tools in this pop-up. Handle responsibly.", m_user.username)
    );

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

    if (!GeodePopup::init(295.0f, 170.0f, GeodePopupStyle::Alt)) return false;

    setID("staff-popup"_spr);
    setTitle("Mod Comments Staff Panel");

    auto cell = CommentItem::create(m_comment, m_mainLayer->getContentWidth() - 37.5f, false);
    cell->setID("comment-cell");

    m_mainLayer->addChildAtPosition(cell, Anchor::Center, { 0.0f, 3.75f });

    auto label = Label::createRich("You have tools at your disposal to enforce <cr>the rules</c>. Please make sure to <co>use these powers responsibly</c>, abuse will most likely result in the <ca>revoking of staff privileges</c>.", cw::font::geode::bold);
    label->setScale(0.425f);
    label->setAnchorPoint({ 0.5f, 1.0f });
    label->setMaxWidth((m_mainLayer->getContentWidth() - 42.5f) / label->getScale());
    label->setAlignment(Label::Alignment::Center);

    m_mainLayer->addChildAtPosition(label, Anchor::Center, { 0.0f, -5.0f });

    auto layout = RowLayout::create()
                      ->setGap(5.0f)
                      ->setDefaultScaleLimits(0.125f, 0.75f)
                      ->setAutoGrowAxis(0.0f)
                      ->setGrowCrossAxis(true);

    auto menu = CCNode::create();
    menu->setID("actions-menu");
    menu->setAnchorPoint(cw::anchor::center);
    menu->setZOrder(9);
    menu->setLayout(layout);

    m_mainLayer->addChildAtPosition(menu, Anchor::Bottom, { 0.0f, 25.0f });

    struct ActionButton final {
        std::string id;
        ZStringView spr;
        Button::ButtonCallback cb = nullptr;
    };

    auto btns = std::array{
        ActionButton{
            "delete-comment-btn",
            "particle_193_001.png",
            [this](auto) {
                m_callback(CommentAction::Delete, m_comment, std::nullopt, std::nullopt);
                onClose(m_closeBtn);
            },
        },
        ActionButton{
            "report-comment-btn",
            "info-warning.png"_gode,
            [this](auto) {
                m_callback(CommentAction::Report, m_comment, std::nullopt, std::nullopt);
                onClose(m_closeBtn);
            },
        },
        ActionButton{
            "ban-user-btn",
            "particle_194_001.png",
            [this](auto) {
                StaffBanPopup::create(m_comment.author, nullptr)->show();
            },
        },
    };

    for (auto& b : btns) {
        auto btn = Button::createWithNode(
            EditorButtonSprite::createWithSpriteFrameName(
                b.spr.c_str(),
                0.75f,
                EditorBaseColor::DarkGray
            ),
            std::move(b.cb)
        );
        btn->setID(std::move(b.id));

        menu->addChild(btn);
    };

    menu->updateLayout();

    auto bg = cue::createBackground(
        menu->getScaledContentSize() * 1.25f,
        {
            .zOrder = 1,
            .id = "",
        }
    );

    m_mainLayer->addChildAtPosition(bg, Anchor::Bottom, { 0.0f, 25.0f });

    cw::addHelpButton(
        m_mainLayer,
        fmt::format("This is the <cg>staff panel</c> for the comment <cc>{}</c> by <co>{}</c>. You can <cr>take action on it through here</c> if necessary.", m_comment.id, m_comment.author.username)
    );

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