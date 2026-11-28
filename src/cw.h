#pragma once

#include <util/Macros.h>

#include <cue/Util.hpp>

#include <dankmeme.argon/argon.hpp>

#include <ui/Include.h>

#include <util/Include.h>

#include <Geode/Geode.hpp>

#include <Geode/ui/Toggler.hpp>

namespace cw::modcmmts {
    namespace popups {
        void showRules();
        geode::Button* addRulesBtn(cocos2d::CCNode* to);
    };

    using namespace ui;
};