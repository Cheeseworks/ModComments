#include "../CommentsPopup.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <cue/PlayerIcon.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

namespace cw::modcmmts {
    namespace impl {
#ifdef CW_TESTING
#define MC_COMMENT_WAIT 5
#define MC_REFRESH_WAIT 1
#else
#define MC_COMMENT_WAIT 30
#define MC_REFRESH_WAIT 2
#endif

        static constexpr uint8_t COMMENT_WAIT = MC_COMMENT_WAIT;
        static constexpr uint8_t REFRESH_WAIT = MC_REFRESH_WAIT;

        static constexpr uint8_t MAX_COMMENT_CHARS = 192;

        static bool isBanned() {
            return SelfDirector::get()->isBanned();
        }
    };
};

Result<CommentRequest> matjson::Serialize<CommentRequest>::fromJson(matjson::Value const& value) {
    CommentRequest out;

    GEODE_UNWRAP_INTO(out.modId, value["mod"].asString());
    GEODE_UNWRAP_INTO(out.content, value["content"].asString());
    GEODE_UNWRAP_INTO(out.icons, value["icons"].as<UserIcons>());

    return Ok(std::move(out));
}

matjson::Value matjson::Serialize<CommentRequest>::toJson(CommentRequest const& value) {
    Value out;
    out["mod"] = value.modId;
    out["content"] = value.content;
    out["icons"] = value.icons;

    return out;
}

std::string CommentModNode::getModName() const {
    return m_dataOk ? m_data.versions[0].name : Loader::get()->getInstalledMod(m_id)->getName().c_str();
}

std::vector<std::string> CommentModNode::getModDevs() const {
    std::vector<std::string> out;

    if (m_dataOk) {
        out.reserve(m_data.developers.size());
        for (auto const& dev : m_data.developers)
            out.push_back(dev.displayName);
    } else {
        out = Loader::get()->getInstalledMod(m_id)->getDevelopers();
    };

    return out;
}

bool CommentModNode::init(std::string id, std::optional<GeodeMod> mod) {
    m_id = std::move(id);

    if (mod.has_value()) {
        m_dataOk = true;
        m_data = std::move(mod).value();
    };

    if (!CCNode::init()) return false;

    auto layout = RowLayout::create()
                      ->setGap(5.0f)
                      ->setAutoScale(false)
                      ->setAutoGrowAxis(0.0f)
                      ->setGrowCrossAxis(true);

    setAnchorPoint({ 0.5, 1 });
    setLayout(layout);

    if (m_dataOk) {
        auto icon = LazySprite::create({ 18.75f, 18.75f });
        icon->setID("logo");
        icon->setAutoResize(true);
        icon->setAnchorPoint({ 0.5, 0.5 });

        icon->setLoadCallback([this, icon](Result<> res) {
            if (res.isOk()) cue::rescaleToMatch(icon, 18.75f);
            updateLayout();
        });

        addChild(icon);

        icon->loadFromUrl(fmt::format("https://api.geode-sdk.org/v1/mods/{}/logo", m_id));
    } else {
        auto icon = createModLogo(Loader::get()->getInstalledMod(m_id));
        icon->setID("logo");

        cue::rescaleToMatch(icon, 18.75f);

        addChild(icon);
    };

    auto name = Label::create(getModName(), cw::ui::font::big);
    name->setID("name");
    name->setAnchorPoint({ 0, 0.5 });
    name->setLimitLabelWidth(165.0f, 0.525f);

    addChild(name);

    std::string devsText;
    auto const devList = getModDevs();

    if (devList.size() > 2) {
        devsText = fmt::format("{} + {} more", devList[0], devList.size() - 1);
    } else if (devList.size() > 1) {
        devsText = fmt::format("{} & {}", devList[0], devList[1]);
    } else {
        devsText = devList[0];
    };

    auto devs = Label::create(std::move(devsText), cw::ui::font::gold);
    devs->setID("developers");
    devs->setAnchorPoint({ 0, 0.5 });
    devs->setLimitLabelWidth(100.0f, 0.425f);

    addChild(devs);

    updateLayout();

    return true;
}

CommentModNode* CommentModNode::create(std::string id, std::optional<GeodeMod> mod) {
    auto ret = new CommentModNode();
    if (ret->init(std::move(id), std::move(mod))) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}

asp::Instant CommentsPopup::s_lastComment;
asp::Instant CommentsPopup::s_lastRefresh;

StringMap<GeodeMod> CommentsPopup::s_indexedMods;

bool CommentsPopup::showInput() const {
    return argon::signedIn() && Loader::get()->isModInstalled(m_modId);
}

arc::Future<cw::WebRes> CommentsPopup::getGeodeData() {
    co_return cw::request::parse(co_await cw::request::base().get(fmt::format("https://api.geode-sdk.org/v1/mods/{}", m_modId)));
}

arc::Future<cw::WebRes> CommentsPopup::deleteComment(uint64_t id) {
    CW_ARGON_UNWRAP(auto token);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .param("comment", id);

    co_return cw::request::parse(co_await req.send("DELETE", "/v2/comments/delete"_api));
}

arc::Future<cw::WebRes> CommentsPopup::reportComment(uint64_t id, std::string reason) {
    CW_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["comment"] = id;
    body["reason"] = std::move(reason);

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return cw::request::parse(co_await req.post("/v2/reports/send"_api));
}

arc::Future<cw::WebRes> CommentsPopup::banCommentUser(int id, std::string reason, asp::Duration duration) {
    CW_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["account_id"] = id;
    body["reason"] = std::move(reason);
    body["duration"] = duration.seconds();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return cw::request::parse(co_await req.post("/v2/bans/ban"_api));
}

arc::Future<cw::WebRes> CommentsPopup::getComments() {
    auto req = cw::request::base()
                   .param("mod", m_modId)
                   .param("page", m_page);

    co_return cw::request::parse(co_await req.get("/v2/comments/get"_api));
}

arc::Future<cw::WebRes> CommentsPopup::sendComment(std::string content) {
    CW_ARGON_UNWRAP(auto token);

    matjson::Value body;
    body["mod"] = m_modId;
    body["content"] = std::move(content);
    body["icons"] = co_await players::getUserIconsCo();

    auto req = (co_await request::withAuthCo(std::move(token)))
                   .bodyJSON(body);

    co_return cw::request::parse(co_await req.post("/v2/comments/send"_api));
}

void CommentsPopup::updatePageLabel() {
    m_pageLabel->setText(fmt::format("Page {}/{} ({} Total)", m_page, m_maxPage, m_maxComments));
    m_pageBtnSprite->setString(cw::fmtNum(m_page).c_str());
}

void CommentsPopup::refreshComments(bool skipCooldown) {
    if (!skipCooldown) {
        auto elapsed = s_lastRefresh.elapsed().seconds();
        if (elapsed < impl::REFRESH_WAIT) return;
    };

    s_lastRefresh = asp::Instant::now();

    m_commentList->setVisible(false);
    m_commentMenu->setVisible(false);

    m_pageNextBtn->setEnabled(false);
    m_pagePrevBtn->setEnabled(false);

    m_refreshBtn->setVisible(false);

    m_loading->setVisible(true);

    m_errLabel->setVisible(false);

    m_commentList->m_contentLayer->removeAllChildren();

    m_commentTask.spawn(
        getComments(),
        [this](cw::WebRes res) {
            if (res.isOk()) {
                auto const fallback = [code = res.getCode()](std::string_view err) {
                    Notification::create(fmt::format("Comments failed to load ({})", code), NotificationIcon::Error);
                    log::error("{}: {}", code, err);
                };

                auto cmmtRes = std::move(res).getPayload<srv::CommentListResponse>();
                if (cmmtRes.isErr()) return fallback(cmmtRes.unwrapErr());

                auto cmmt = std::move(cmmtRes).unwrap();
                m_page = cmmt.pagination.page;
                m_maxPage = cmmt.pagination.totalPages;
                m_maxComments = cmmt.pagination.totalItems;

                for (auto& comment : cmmt.items) {
                    auto cell = CommentItem::create(
                        std::move(comment),
                        m_commentList->getScaledContentWidth(),
                        true,
                        m_geodeTheme
                    );
                    cell->setActionCallback([this](CommentAction act, Comment const& cmmt, std::optional<std::string> reason, std::optional<asp::Duration> duration) {
                        switch (act) {
                            default: return;

                            case CommentAction::Delete: return onDelete(cmmt);
                            case CommentAction::Report: return onReport(cmmt);
                        };
                    });

                    m_commentList->m_contentLayer->addChild(cell);
                };

                m_pageNextBtn->setVisible(m_page < m_maxPage);
                m_pagePrevBtn->setVisible(m_page > 1);

                m_pageNextBtn->setEnabled(true);
                m_pagePrevBtn->setEnabled(true);

                m_commentList->m_contentLayer->updateLayout();
                m_commentList->scrollToTop();

                m_commentList->setVisible(true);
                m_commentMenu->setVisible(showInput());
            } else {
                m_errLabel->setVisible(true);
            };

            updatePageLabel();

            m_refreshBtn->setVisible(true);

            m_loading->setVisible(false);
        }
    );
}

bool CommentsPopup::init(std::string modId, bool geodeTheme) {
    m_modId = std::move(modId);
    m_geodeTheme = geodeTheme;

    if (!GeodePopup::init(430.0f, 270.0f)) return false;

    setID(fmt::format("popup-{}", modId));

    setCloseButtonSpr(
        CircleButtonSprite::createWithSpriteFrameName(
            "close.png"_gode,
            0.875f,
            m_geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green
        ),
        0.825f
    );

    m_geodeLoading = LoadingSpinner::create(20.0f);
    m_mainLayer->addChildAtPosition(m_geodeLoading, Anchor::Top, { 0.0f, -18.75f });

    auto addModNode = [this](CommentModNode* modNode) {
        m_mainLayer->addChildAtPosition(modNode, Anchor::Top, { 0.0f, -8.25f });
        m_geodeLoading->setVisible(false);
    };

    if (Loader::get()->isModInstalled(m_modId)) {
        auto modNode = CommentModNode::create(m_modId);
        addModNode(modNode);
    } else {
        if (auto const it = s_indexedMods.find(m_modId); it != s_indexedMods.end()) {
            auto modNode = CommentModNode::create(m_modId, it->second);
            addModNode(modNode);
        };

        m_geodeTask.spawn(
            getGeodeData(),
            [this, addModNode = std::move(addModNode)](cw::WebRes res) {
                if (res.isErr()) return log::error("{}: {}", res.getCode(), res.getError());

                auto dataRes = res.getPayload<GeodeMod>();
                if (dataRes.isErr()) return log::error("Failed to serialize response: {}", dataRes.unwrapErr());

                s_indexedMods[m_modId] = std::move(dataRes).unwrap();

                auto modNode = CommentModNode::create(m_modId, s_indexedMods[m_modId]);
                addModNode(modNode);
            }
        );
    };

    auto cmmtBorder = cue::createBackground(
        { m_mainLayer->getScaledContentWidth() - 75.0f, m_mainLayer->getScaledContentHeight() - 40.0f },
        {
            .opacity = 255,
            .texture = "black-square.png"_gode,
            .zOrder = 1,
            .id = "",
        }
    );
    cmmtBorder->setAnchorPoint({ 0.5, 0 });

    m_mainLayer->addChildAtPosition(cmmtBorder, Anchor::Bottom, { 0.0f, 8.75f });

    m_loading = LoadingSpinner::create(50.0f);
    m_loading->setID("loading-circle");
    m_loading->setVisible(false);

    cmmtBorder->addChildAtPosition(m_loading, Anchor::Center);

    auto isShowInput = showInput();

    m_commentList = ScrollLayer::create({ cmmtBorder->getScaledContentWidth() - 12.5f, cmmtBorder->getScaledContentHeight() - (isShowInput ? 45.0f : 25.0f) });
    m_commentList->setID("comment-list");
    m_commentList->setAnchorPoint({ 0.5, 1 });
    m_commentList->ignoreAnchorPointForPosition(false);

    m_commentList->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(3.25f));

    cmmtBorder->addChildAtPosition(m_commentList, Anchor::Top, { 0.0f, -5.0f });

    m_commentList->m_contentLayer->updateLayout();
    m_commentList->scrollToTop();

    m_errLabel = Label::create("Something went wrong...", cw::ui::font::gold);
    m_errLabel->setID("error-label");
    m_errLabel->setZOrder(9);
    m_errLabel->setScale(0.475f);
    m_errLabel->setAlignment(Label::Alignment::Center);
    m_errLabel->setVisible(false);

    cmmtBorder->addChildAtPosition(m_errLabel, Anchor::Center);

    static constexpr auto pageBtnSprName = "GJ_arrow_02_001.png";

    m_pageNextBtn = Button::createWithSpriteFrameName(
        pageBtnSprName,
        [this](Button* sender) {
            if (m_page < m_maxPage) m_page++;
            if (m_page > m_maxPage) m_page = m_maxPage;

            sender->setVisible(m_page < m_maxPage);

            refreshComments(true);
        }
    );
    m_pageNextBtn->setID("next-page-btn");
    m_pageNextBtn->setScale(0.875f);
    m_pageNextBtn->setVisible(false);

    if (auto spr = typeinfo_cast<CCSprite*>(m_pageNextBtn->getDisplayNode())) spr->setFlipX(true);

    m_pagePrevBtn = Button::createWithSpriteFrameName(
        pageBtnSprName,
        [this](Button* sender) {
            if (m_page > 1) m_page--;

            sender->setVisible(m_page > 1);

            refreshComments(true);
        }
    );
    m_pagePrevBtn->setID("previous-page-btn");
    m_pagePrevBtn->setScale(0.875f);
    m_pagePrevBtn->setVisible(false);

    m_mainLayer->addChildAtPosition(m_pageNextBtn, Anchor::Right, { -18.75f, -3.75f });
    m_mainLayer->addChildAtPosition(m_pagePrevBtn, Anchor::Left, { 18.75f, -3.75f });

    m_pageLabel = Label::create("Loading...", cw::ui::font::gold);
    m_pageLabel->setID("page-label");
    m_pageLabel->setScale(0.5f);
    m_pageLabel->setAnchorPoint({ 1, 1 });
    m_pageLabel->setAlignment(Label::Alignment::Right);

    m_mainLayer->addChildAtPosition(m_pageLabel, Anchor::BottomRight, { -15.0f, -1.25f });

    auto sendMenuLayout = RowLayout::create()
                              ->setAutoScale(false)
                              ->setAxisAlignment(AxisAlignment::Between);

    m_commentMenu = CCNode::create();
    m_commentMenu->setID("send-container");
    m_commentMenu->setAnchorPoint({ 0.5, 0 });
    m_commentMenu->setContentSize({ m_commentList->getScaledContentWidth(), 25.0f });
    m_commentMenu->setLayout(sendMenuLayout);

    cmmtBorder->addChildAtPosition(m_commentMenu, Anchor::Bottom, { 0.0f, 5.0f });

    m_inputBox = TextInput::create(m_commentMenu->getScaledContentWidth() - 2.5f, "Share your thoughts...", cw::ui::font::chat);
    m_inputBox->setID("comment-text-input");
    m_inputBox->setScale(0.925f);
    m_inputBox->setMaxCharCount(impl::MAX_COMMENT_CHARS);
    m_inputBox->setAlignment(Label::Alignment::Left);
    m_inputBox->setCommonFilter(CommonFilter::Any);
    m_inputBox->setContentHeight(m_inputBox->getScaledContentHeight() * 2.0f);
    m_inputBox->setDisplayEmojis(true);

    createInputLimitLabel(impl::MAX_COMMENT_CHARS);

    setMildLimitWarning(96);
    setModerateLimitWarning(156);

    m_commentMenu->addChild(m_inputBox);

    m_commentMenu->setVisible(isShowInput);

    if (!isShowInput) {
        auto install = Label::createRich(
            Loader::get()->isModInstalled(m_modId) ? (argon::signedIn() ? "<cr>Unknown error</c>"  // idk let me have fun with my ternaries >:C
                                                                          :
                                                                          (impl::isBanned() ? "You've been <cr>banned</c> from commenting." : "<cy>Log in</c> to <cg>post comments</c>!")) :
                                                     "<cy>Install this mod</c> if you'd like to <cg>post comments</c> here!",
            cw::ui::font::chat
        );
        install->setScale(0.675f);
        install->setAnchorPoint({ 0.5, 0 });
        install->setAlignment(Label::Alignment::Center);

        cmmtBorder->addChildAtPosition(install, Anchor::Bottom, { 0.0f, 4.25f });
    };

    auto sendBtnSpr = EditorButtonSprite::createWithSpriteFrameName(
        "particle_12_001.png",
        0.875f,
        m_geodeTheme ? EditorBaseColor::DarkGray : EditorBaseColor::Green
    );

    auto sendBtn = Button::createWithNode(
        sendBtnSpr,
        [this, isShowInput](Button* sender) {
            if (isShowInput) onSend(sender);
        }
    );
    sendBtn->setID("send-comment-btn");
    sendBtn->setEnabled(isShowInput);

    cue::rescaleToMatch(sendBtn, 26.25f);

    m_commentMenu->addChild(sendBtn);
    m_commentMenu->updateLayout();

    m_inputBox->setEnterCallback([this, sendBtn](auto) {
        onSend(sendBtn);
    });

    m_pageBtnSprite = ButtonSprite::create(
        cw::fmtNum(m_page).c_str(),
        cw::font::big,
        geodeTheme ? "GE_button_05.png"_gode : "GJ_button_01.png",
        0.75f
    );

    auto pageBtn = Button::createWithNode(
        m_pageBtnSprite
    );
    pageBtn->setID("jump-to-page-btn");
    pageBtn->setScale(0.625f);

    m_mainLayer->addChildAtPosition(pageBtn, Anchor::TopLeft, { 20.0f, -50.0f });

    auto linkBtnMenuLayout = ColumnLayout::create()
                                 ->setGap(2.0f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(0.0f);

    auto linkBtnMenu = CCNode::create();
    linkBtnMenu->setID("link-container");
    linkBtnMenu->setContentSize({ 12.5f, 1.25f });
    linkBtnMenu->setZOrder(1);
    linkBtnMenu->setLayout(linkBtnMenuLayout);

    m_mainLayer->addChildAtPosition(linkBtnMenu, Anchor::BottomLeft, { 7.5f, 7.5f });

    auto linkBtns = std::array{
        cw::LinkButton{
            "comment-rules-btn",
            "accountBtn_myLists_001.png",
            [](auto) {
                popups::showRules();
            },
        },
        cw::LinkButton{
            "discord-btn",
            "gj_discordIcon_001.png",
            [](auto) {
                cw::popups::showDiscord();
            },
        },
        cw::LinkButton{
            "support-me-btn",
            "gift.png"_gode,
            [](auto) {
                cw::popups::showSupport();
            },
        },
    };

    for (auto& linkBtn : linkBtns) {
        auto b = Button::createWithSpriteFrameName(
            linkBtn.sprite,
            std::move(linkBtn.callback)
        );
        b->setID(std::move(linkBtn.id));

        cue::rescaleToMatch(b, 22.5f);

        linkBtnMenu->addChild(b);
    };

    linkBtnMenu->updateLayout();

    m_refreshBtn = Button::createWithNode(
        CircleButtonSprite::createWithSpriteFrameName(
            "reload.png"_gode,
            0.975f,
            m_geodeTheme ? CircleBaseColor::DarkPurple : CircleBaseColor::Green
        ),
        [this](Button* sender) {
            refreshComments();
        }
    );
    m_refreshBtn->setID("refresh-comments-btn");
    m_refreshBtn->setScale(0.625f);

    m_mainLayer->addChildAtPosition(m_refreshBtn, Anchor::BottomRight, {}, false);

    cw::addHelpButton(
        m_mainLayer,
        "This is the <cg>Mod Comment Menu</c>. You can <cy>read and send comments about this mod</c> here. Remember to <cr>read and follow the rules</c>."
    );

    refreshComments();

    return true;
}

void CommentsPopup::onSend(Button* sender) {
    if (!showInput()) return;

    m_inputBox->unfocus();

    auto elapsed = s_lastComment.elapsed().seconds();
    if (elapsed < impl::COMMENT_WAIT) {
        auto remaining = impl::COMMENT_WAIT - elapsed;

        createQuickPopup(
            "Slow Down!",
            fmt::format("You must <co>wait {} {} before sending your next comment</c>!", remaining, str::plurals::appendS("second", remaining)),
            "OK",
            nullptr,
            nullptr
        );

        return;
    };

    auto inputStr = str::trim(m_inputBox->getString());

    if (inputStr.size() > impl::MAX_COMMENT_CHARS) return Notification::create(fmt::format("Comment exceeds {} characters", impl::MAX_COMMENT_CHARS), NotificationIcon::Warning)->show();
    if (inputStr.empty()) return Notification::create("Comment cannot be empty.", NotificationIcon::Error)->show();

    sender->setEnabled(false);
    if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({ 65, 65, 65 });

    m_refreshBtn->setVisible(false);

    m_commentTask.spawn(
        sendComment(std::move(inputStr)),
        [this, sender](cw::WebRes res) {
            if (res.isOk()) {
                s_lastComment = asp::Instant::now();

                m_page = 1;
                m_inputBox->setString("", true);

                refreshComments();
            } else if (res.isErr()) {
                m_refreshBtn->setVisible(true);
                Notification::create(fmt::format("Error {}", res.getCode()), NotificationIcon::Error)->show();
            };

            if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->setColor({ 255, 255, 255 });
            sender->setEnabled(true);
        }
    );
}

void CommentsPopup::onDelete(Comment const& cmmt) {
    if (m_commentActionTask.isPending()) return;

    createQuickPopup(
        "Delete Comment",
        "<cr>Delete</c> this comment?",
        "Cancel",
        "Yes",
        [this, &cmmt](auto, bool ok) {
            if (ok) m_commentActionTask.spawn(
                deleteComment(cmmt.id),
                [this](cw::WebRes res) {
                    if (res.isOk()) return refreshComments(true);
                    Notification::create(fmt::format("Failed to delete comment ({})", res.getCode()), NotificationIcon::Error)->show();
                }
            );
        }
    );
}

void CommentsPopup::onReport(Comment const& cmmt) {
    if (m_commentActionTask.isPending()) return;

    if (SelfDirector::get()->isReported(cmmt.id)) return Notification::create("You already reported this comment", NotificationIcon::Warning)->show();

    CommentReportPopup::create(
        cmmt,
        [this](Comment const& cmmt, std::string reason) {
            if (auto popup = CCScene::get()->getChildByType<CommentReportPopup>()) cue::resetNode(popup);
            auto popup = ProcessPopup::create("Sending report...");
            popup->show();

            popup->setCloseCallback([this](ProcessPopup* sender, ProcessPopup::State state) {
                if (state == ProcessPopup::State::Ongoing) {
                    m_commentActionTask.cancel();
                    Notification::create("Canceled report", NotificationIcon::Error)->show();
                };

                sender->removeFromParent();
            });

            m_commentActionTask.spawn(
                reportComment(cmmt.id, std::move(reason)),
                [&cmmt, popup = Ref(popup)](cw::WebRes res) {
                    if (res.isErr()) return popup->setFailure(fmt::format("Failed to report comment ({})", res.getCode()));

                    SelfDirector::get()->setReport(cmmt.id);
                    popup->setSuccess("Reported successfully");
                }
            );
        },
        m_geodeTheme
    )
        ->show();
}

void CommentsPopup::onExit() {
    if (auto popup = CCScene::get()->getChildByType<RulesPopup>()) popup->removeFromParent();
    GeodePopup::onExit();
}

CommentsPopup* CommentsPopup::create(std::string modId, bool geodeTheme) {
    auto ret = new CommentsPopup();
    if (ret->init(std::move(modId), geodeTheme)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}