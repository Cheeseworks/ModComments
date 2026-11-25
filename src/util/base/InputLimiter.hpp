#pragma once

#include <Geode/UI.hpp>

namespace cw::modcmmts {
    namespace base {
        class InputLimiter {  // idk :P
        private:
            geode::Label* m_inputBoxCharLabel = nullptr;

            uint8_t m_inputMaxChars = std::numeric_limits<uint8_t>::max();

            uint8_t m_inputModerateWarn = 225;
            uint8_t m_inputMildWarn = 180;

            std::string getCharCountText(std::string_view input) const;

        protected:
            geode::TextInput* m_inputBox;

            geode::Label* createInputLimitLabel(uint8_t limit = std::numeric_limits<uint8_t>::max());

            void setModerateLimitWarning(uint8_t threshold);
            void setMildLimitWarning(uint8_t threshold);
        };
    };
};