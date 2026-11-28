#include "../Staff.hpp"

#include <cw.h>

#include <cue/DropdownNode.hpp>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

namespace cw::modcmmts {
    namespace impl {
        static constexpr uint8_t MAX_REASON_CHARS = 36;
    };
};

asp::Duration StaffBanPopup::getDuration() const {
    switch (m_durType) {
        default: [[fallthrough]];

        case DurationType::Day: return asp::Duration::fromDays(m_duration);
        case DurationType::Minute: return asp::Duration::fromMinutes(m_duration);
        case DurationType::Hour: return asp::Duration::fromHours(m_duration);
        case DurationType::Week: return asp::Duration::fromWeeks(m_duration);
    };
}

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

    auto durationSelect = cue::DropdownNode::create({ 0, 0, 0, 0 }, 125.0f, 23.0f, 65.0f);
    durationSelect->setID("duration-select");
    durationSelect->setAnchorPoint({ 1.0f, 1.0f });
    durationSelect->setZOrder(9);

    struct DurationCell final {
        std::string text;
        DurationType type;
        ccColor3B color = cw::colors::white;
    };

    auto cells = std::array{
        DurationCell{
            "Minutes",
            DurationType::Minute,
        },
        DurationCell{
            "Hours",
            DurationType::Hour,
            cw::colors::yellow,
        },
        DurationCell{
            "Days",
            DurationType::Day,
            cw::colors::orange,
        },
        DurationCell{
            "Weeks",
            DurationType::Week,
            cw::colors::red,
        },
    };

    for (auto& c : cells) {
        auto cell = BoxedLabel::create(std::move(c.text), cw::font::gold, durationSelect->getScaledContentWidth());
        cell->setTag((int)c.type);
        cell->setTextScale(0.5f);
        cell->setBoxColor(c.color);

        durationSelect->addCell(cell);
    };

    durationSelect->setCallback([this](auto, CCNode* cell) {
        m_durType = (DurationType)cell->getTag();
    });

    m_mainLayer->addChildAtPosition(durationSelect, Anchor::Center);

    auto submitBtn = Button::createWithNode(
        ButtonSprite::create(fmt::format("Ban {}", m_user.username).c_str(), cw::font::gold, "GJ_button_05.png", 0.75f),
        [this](auto) {
            cw::showAlert(
                "Confirm Ban",
                fmt::format("<cr>Ban</c> <co>{}</c> for <cc>{}</c>?", m_user.username, getDuration().toHumanString()),
                "Cancel",
                "OK",
                [this](auto, bool ok) {
                    if (ok) m_callback(m_inputBox->getString(), getDuration());
                }
            );
        }
    );
    submitBtn->setID("confirm-ban-btn");
    submitBtn->setScale(0.875f);

    m_mainLayer->addChildAtPosition(submitBtn, Anchor::Bottom);

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
                StaffBanPopup::create(
                    m_comment.author, [this](std::string reason, asp::Duration duration) {
                        m_callback(CommentAction::Ban, m_comment, std::move(reason), duration);
                    }
                )->show();
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