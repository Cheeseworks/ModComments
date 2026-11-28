#pragma once

#include <util/Macros.h>

#include <util/base/InputLimiter.hpp>

#include <util/Comments.hpp>
#include <util/GeodeMod.hpp>

#include <Geode/Geode.hpp>

#include <Geode/ui/Style.hpp>

namespace cw::modcmmts {
    struct CommentRequest final {
        std::string modId;
        std::string content;
        UserIcons icons;
    };

    enum class CommentAction : uint8_t {
        Delete,
        Report,
        Ban,
    };

    enum class CommentVote : int8_t {
        Dislike = -1,
        None = 0,
        Like = 1,
    };

    namespace ui {
        class CommentModNode final : public cocos2d::CCNode {
        private:
            std::string m_id;

            GeodeMod m_data;
            bool m_dataOk = false;

            std::string getModName() const;
            std::vector<std::string> getModDevs() const;

        protected:
            bool init(std::string id, std::optional<GeodeMod> mod);

        public:
            static CommentModNode* create(std::string id, std::optional<GeodeMod> mod = std::nullopt);
        };

        class CommentsPopup final : public geode::GeodePopup, private base::InputLimiter {
        private:
            std::string m_modId;
            bool m_geodeTheme = false;

            geode::LoadingSpinner* m_geodeLoading = nullptr;

            static geode::utils::StringMap<GeodeMod> s_indexedMods;

            uint64_t m_page = 1;
            uint64_t m_maxPage = 1;
            uint64_t m_maxComments = 0;

            geode::Button* m_pageNextBtn = nullptr;
            geode::Button* m_pagePrevBtn = nullptr;
            geode::Label* m_pageLabel = nullptr;

            geode::Ref<ButtonSprite> m_pageBtnSprite = nullptr;

            geode::ScrollLayer* m_commentList = nullptr;
            cocos2d::CCNode* m_commentMenu = nullptr;

            geode::Label* m_errLabel = nullptr;

            geode::Button* m_refreshBtn = nullptr;

            geode::LoadingSpinner* m_loading = nullptr;

            static asp::Instant s_lastComment;
            static asp::Instant s_lastRefresh;

            geode::async::TaskHolder<WebRes> m_commentTask;
            geode::async::TaskHolder<WebRes> m_commentActionTask;

            arc::Future<WebRes> getComments();
            arc::Future<WebRes> sendComment(std::string content);

            arc::Future<WebRes> deleteComment(uint64_t id);
            arc::Future<WebRes> reportComment(uint64_t id, std::string reason);
            arc::Future<WebRes> banCommentUser(int id, std::string reason, asp::Duration duration);

            geode::async::TaskHolder<WebRes> m_geodeTask;

            arc::Future<WebRes> getGeodeData();

            void updatePageLabel();

            void refreshComments(bool skipCooldown = false);
            bool showInput() const;

        protected:
            void onDelete(Comment const& cmmt);
            void onReport(Comment const& cmmt);
            void onBan(Comment const& cmmt, std::string reason, asp::Duration duration);

            void onSend(geode::Button* sender);

            void onExit() override;

            bool init(std::string modId, bool geodeTheme);

        public:
            static CommentsPopup* create(std::string modId, bool geodeTheme = true);
        };
    };
};

template <>
struct matjson::Serialize<cw::modcmmts::CommentRequest> final {
    static geode::Result<cw::modcmmts::CommentRequest> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::CommentRequest const& value);
};