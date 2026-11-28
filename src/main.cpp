#include <cw.h>

#include <rhatt/prelude.h>

#include <rhatt/base/Singleton.hpp>

#include <Geode/Geode.hpp>

#include <Geode/ui/Style.hpp>

#include <alphalaneous.alphas_geode_utils/ObjectModify.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

namespace cw::modcmmts {
    class IndexTaskDelegate final : public cw::base::Singleton<IndexTaskDelegate> {
    private:
        Ref<ProcessPopup> m_popup = nullptr;

    public:
        TaskHolder<WebRes> m_checkIndexTask;

        Ref<ProcessPopup> createProgressPopup() {
            cue::resetNode(m_popup);

            m_popup = ProcessPopup::create("Checking Geode index...");
            m_popup->setCloseCallback([this](auto, auto) {
                m_checkIndexTask.cancel();
                cue::resetNode(m_popup);
            });

            m_popup->show();

            return m_popup;
        }

        arc::Future<WebRes> checkModIndex(std::string modId) {
            co_return cw::request::parse(co_await cw::request::base().get(fmt::format("https://api.geode-sdk.org/v1/mods/{}", modId)));
        }
    };

    namespace main {
        static constexpr std::string_view GEODE_URL = "https://geode-sdk.org/mods/";
        static StringSet VALID_MODS;

        static bool mustAgreeToRules(std::string_view id) {
            return argon::signedIn() && Loader::get()->isModInstalled(id) && !Mod::get()->getSavedValue(rules::saved, false);
        }

        static void createPopup(std::string id, bool geodeTheme) {
            if (!mustAgreeToRules(id)) return CommentsPopup::create(std::move(id), geodeTheme)->show();

            RulesPopup::create(
                [id = std::move(id), geodeTheme](RulesPopup* sender, bool agreed) {
                    sender->removeFromParent();

                    if (!agreed) return;

                    Mod::get()->setSavedValue(rules::saved, true);
                    CommentsPopup::create(id, geodeTheme)->show();
                },
                geodeTheme
            )
                ->show();
        }
    };
};

$on_game(Loaded) {
    log::debug("Using web API url: {}", cw::url::apiBase);
#ifdef CW_TESTING
    log::warn("{} is in test mode!", Mod::get()->getID());
#endif

    auto sd = SelfDirector::get();

    async::spawn(
        sd->authorize(),
        [sd](cw::WebRes res) {
            if (res.isErr()) return log::error("{}: {}", res.getCode(), res.getError());

            auto userRes = res.getPayload<CommentUser>();
            if (userRes.isErr()) return log::error("Failed to parse payload: {}", userRes.unwrapErr());

            sd->setCurrentUser(std::move(userRes).unwrap());
        }
    );

    ButtonSettingPressedEvent(
        Mod::get(),
        "btn"
    )
        .listen([](std::string_view buttonKey) {
            SelfDirector::get()->startAuth();
        })
        .leak();
}

class $nodeModify(CommentsModPopup, ModPopup) {
    struct Fields final {
        std::string id;
        bool geodeTheme = false;
    };

    void modify() {
        auto f = m_fields.self();

        f->geodeTheme = Loader::get()->getLoadedMod(CW_GEODE_ID)->getSettingValue<std::string>("used-theme") != "Geometry Dash";

        if (auto res = getThisID(); res.isOk()) f->id = std::move(res).unwrap();
        if (f->id.empty()) return;

        if (auto self = reinterpret_cast<FLAlertLayer*>(this)) {
#ifdef CW_TESTING
            log::debug("Reading popup data");
#endif

            if (auto tabsMenu = self->m_mainLayer->querySelector("main-container > right-column > tabs-menu")) {
#ifdef CW_TESTING
                log::trace("Found tabs menu");
#endif

                tabsMenu->setPosition({ 0.0f, tabsMenu->getPositionY() - 2.0f });
                tabsMenu->setAnchorPoint({ 0, 1 });
                tabsMenu->setContentWidth(tabsMenu->getContentWidth() - 21.25f);

                auto tabSprite = GeodeTabSprite::create("particle_206_001.png", "Comments", 140.0f);
                tabSprite->select(false);

                auto tab = Button::createWithNode(
                    tabSprite,
                    [f](auto) {
                        if (auto const it = main::VALID_MODS.find(f->id); it != main::VALID_MODS.end()) return main::createPopup(f->id, f->geodeTheme);

                        auto delegate = IndexTaskDelegate::get();

                        auto popup = delegate->createProgressPopup();

                        delegate->m_checkIndexTask.spawn(
                            delegate->checkModIndex(f->id),
                            [id = f->id, geodeTheme = f->geodeTheme, popup](cw::WebRes res) mutable {
                                if (res.isOk()) {
                                    auto metaRes = res.getPayload<GeodeMod>();
                                    if (metaRes.isErr()) {
                                        log::error("Failed to parse Geode index response: {}", metaRes.unwrapErr());
                                        popup->setFailure("Unknown error");

                                        return;
                                    };

                                    auto const meta = std::move(metaRes).unwrap();
                                    if (meta.versions.empty()) {
                                        popup->setFailure("Mod is delisted");
                                        return;
                                    };

                                    popup->removeFromParent();
                                    main::VALID_MODS.insert(id);

                                    main::createPopup(std::move(id), geodeTheme);

                                    return;
                                };

                                popup->setFailure("Mod is unavailable");
                            }
                        );
                    }
                );
                tab->setID("comments-btn"_spr);
                tab->setTag(tabsMenu->getChildrenCount());

                tabsMenu->addChild(tab);
                tabsMenu->updateLayout();
            } else {
                log::error("Cannot find tabs menu");
            };
        } else {
            log::error("Cannot read popup data");
        };
    }

    Result<std::string> getThisID() {
        if (auto id = typeinfo_cast<CCString*>(getUserObject("mod"))) return Ok(id->getCString());
        if (auto self = typeinfo_cast<FLAlertLayer*>(this)) {
            if (auto btn = self->m_mainLayer->getChildByID("mod-online-page-button")) {
                if (auto url = typeinfo_cast<CCString*>(btn->getUserObject("url"))) {
                    log::trace("URL string object found in mod page button");

                    std::string urlStr = url->getCString();

                    if (utils::string::startsWith(urlStr, main::GEODE_URL)) return Ok(urlStr.erase(0, main::GEODE_URL.size()));
                    return Err("Mod ID not found");
                };

                return Err("URL string object not found");
            };

            return Err("URL button not found");
        };

        return Err("String object not found");
    }
};

void popups::showRules() {
    MDPopup::create(
        "Comment Rules",
        rules::fullText,
        "OK"
    )
        ->show();
}

Button* popups::addRulesBtn(CCNode* to) {
    auto btn = Button::createWithNode(
        EditorButtonSprite::createWithSpriteFrameName(
            "particle_218_001.png",
            0.75f,
            EditorBaseColor::DarkGray
        ),
        [](Button* sender) {
            popups::showRules();
        }
    );
    btn->setID("comment-rules-btn");
    btn->setScale(0.75f);

    to->addChildAtPosition(btn, Anchor::BottomLeft, btn->getScaledContentSize() * 0.825f, false);
    return btn;
}