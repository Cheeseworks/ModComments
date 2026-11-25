#include "../InputLimiter.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

std::string cw::modcmmts::base::InputLimiter::getCharCountText(std::string_view input) const {
    return fmt::format("{} / {}", input.size(), m_inputMaxChars);
}

Label* cw::modcmmts::base::InputLimiter::createInputLimitLabel(uint8_t limit) {
    m_inputMaxChars = limit;

    m_inputBox->setMaxCharCount(m_inputMaxChars);

    m_inputBoxCharLabel = Label::create(getCharCountText(m_inputBox->getString()), cw::ui::font::chat);
    m_inputBoxCharLabel->setScale(0.5f);
    m_inputBoxCharLabel->setOpacity(125);
    m_inputBoxCharLabel->setAnchorPoint({ 1, 0 });
    m_inputBoxCharLabel->setAlignment(Label::Alignment::Right);

    m_inputBox->addChildAtPosition(m_inputBoxCharLabel, Anchor::BottomRight, { -2.0f, 2.0f }, false);

    m_inputBox->setCallback([this](std::string const& input) {
        m_inputBoxCharLabel->setText(getCharCountText(input));

        auto size = input.size();
        if (size > m_inputMaxChars) {
            m_inputBoxCharLabel->setColor({ 225, 75, 75 });
        } else if (size > m_inputModerateWarn) {
            m_inputBoxCharLabel->setColor({ 225, 150, 75 });
        } else if (size > m_inputMildWarn) {
            m_inputBoxCharLabel->setColor({ 225, 200, 100 });
        } else {
            m_inputBoxCharLabel->setColor({ 225, 225, 225 });
        };
    });

    return m_inputBoxCharLabel;
}

void cw::modcmmts::base::InputLimiter::setModerateLimitWarning(uint8_t threshold) {
    m_inputModerateWarn = threshold;
}

void cw::modcmmts::base::InputLimiter::setMildLimitWarning(uint8_t threshold) {
    m_inputMildWarn = threshold;
}