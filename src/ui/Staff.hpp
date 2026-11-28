#pragma once

#include "CommentsPopup.hpp"

#include <Geode/Geode.hpp>

#include <Geode/ui/Style.hpp>

namespace cw::modcmmts {
    namespace ui {
        class StaffBanPopup final : public geode::GeodePopup, private base::InputLimiter {
            using Callback = geode::Function<void(std::string, asp::Duration)>;

            enum class DurationType : uint8_t {
                Minute,
                Hour,
                Day,
                Week,
            };

        private:
            CommentUser m_user;
            Callback m_callback = nullptr;

            uint64_t m_duration = 3;
            DurationType m_durType;

            asp::Duration getDuration() const;

        protected:
            bool init(CommentUser user, Callback&& cb);

        public:
            static StaffBanPopup* create(CommentUser user, Callback&& cb);
        };

        class StaffPopup final : public geode::GeodePopup {
            using Callback = geode::Function<void(CommentAction, Comment const&, std::optional<std::string>, std::optional<asp::Duration>)>;

        private:
            Comment m_comment;
            Callback m_callback = nullptr;

        protected:
            bool init(Comment cmmt, Callback&& cb);

        public:
            static StaffPopup* create(Comment cmmt, Callback&& cb);
        };
    };
};