// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppHidingFunction
#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include "Geode/modify/LevelInfoLayer.hpp"
#include <Geode/utils/web.hpp>

#include "RatingsManager.h"
#include "Utils.h"
#include "layers/GDDLLevelInfoPopup.h"
#include "settings/ExcludeRangeSettingV3.h"
#include "layers/GDDLAdvancedLevelInfoPopup.h"

using namespace geode::prelude;

class $modify(GDDLInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> infoLayerGetRatingListener;
        bool gddlTierUpdated = false;
        CCMenuItemSpriteExtra* tierButton = nullptr;
        GDDLAdvancedLevelInfoPopup* advancedLevelInfoPopup = nullptr;
    };

    // ReSharper disable once CppParameterMayBeConst
    bool init(GJGameLevel *p0, bool p1) {
        if (!LevelInfoLayer::init(p0, p1))
            return false;

        // setup web req
        m_fields->infoLayerGetRatingListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                const std::string response = res->string().unwrapOrDefault();
                if (response.empty()) {
                    updateButton(-1);
                } else {
                    const int levelID = m_level->m_levelID;
                    int tierAfterFetch = -1;
                    if(RatingsManager::addRatingFromResponse(levelID, response)) {
                        tierAfterFetch = RatingsManager::getDemonTier(levelID);
                    }
                    updateButton(tierAfterFetch);
                    if (m_fields->advancedLevelInfoPopup != nullptr) {
                        m_fields->advancedLevelInfoPopup->addRatingInfo();
                    }
                }
            } else if (e->isCancelled()) {
                updateButton(-1);
            }
        });

        if (m_level->m_stars != 10 || !Utils::notExcluded(m_level->m_levelID)) return true;

        m_fields->gddlTierUpdated = false;

        const auto menu = CCMenu::create();
        menu->setContentSize({35.2f, 35.2f});
        menu->setID("rating-menu"_spr);
        addChild(menu);

        const std::string buttonPosition = Mod::get()->getSettingValue<std::string>("button-position");

        if (buttonPosition == "Left of Difficulty") {
            menu->setPosition(m_difficultySprite->getPosition() + ccp(-45.0f, 6.0f));
        } else if (buttonPosition == "Stars/Moons Label") {
            m_starsLabel->setOpacity(0);

            menu->setPosition(m_starsLabel->getPosition());
            menu->setAnchorPoint(m_starsLabel->getAnchorPoint());
            menu->setScale(0.4f);
        } else {
            const auto titleLabel = getChildByID("title-label");

            const bool isLeft = buttonPosition == "Left of Title";
            const auto offset = ccp(
                (isLeft ? -1 : 1) * (titleLabel->getScaledContentWidth() / 2.0f + 10.0f),
                -1.5f
            );

            menu->setPosition(titleLabel->getPosition() + offset);

            // move the weekly/event label
            // weekly: 100001 - 200000, event: 200000+, thanks prevter <3
            if (!isLeft && m_level->m_dailyID > 100000) {
                if (const auto weeklyLabel = getChildByID("daily-label")) {
                    weeklyLabel->setPositionX(weeklyLabel->getPositionX() + 23.0f);
                }
            }

            menu->setScale(0.5f);
        }

        const int levelID = m_level->m_levelID;

        // just so it displays a bit earlier
        const auto button = CCMenuItemSpriteExtra::create(
            Utils::getSpriteFromTier(RatingsManager::getCachedTier(levelID)),
            this, menu_selector(GDDLInfoLayer::onGDDLInfo
        ));
        button->setID("rating"_spr);
        menu->addChildAtPosition(button, Anchor::Center);
        m_fields->tierButton = button;

        const int tier = RatingsManager::getDemonTier(levelID);

        if (tier != -1) {
            updateButton(tier);
            m_fields->gddlTierUpdated = true;
        }

        return true;
    }

    void updateLabelValues() {
        LevelInfoLayer::updateLabelValues();
        const bool isDemon = m_level->m_stars == 10;
        if (!isDemon || m_fields->gddlTierUpdated) return;

        // fetch information
        const int levelID = m_level->m_levelID;
        const int tier = RatingsManager::getDemonTier(levelID);

        if (tier == -1) {
            // web request 2.0 yaaay
            auto req = web::WebRequest();
            req.header("User-Agent", Utils::getUserAgent());
            m_fields->infoLayerGetRatingListener.setFilter(req.get(RatingsManager::getRequestUrl(levelID)));
        }
    }

    void updateButton(const int tier) {
        if (!m_fields->tierButton) return;

        m_fields->tierButton->removeAllChildren();
        m_fields->tierButton->addChild(Utils::getSpriteFromTier(tier));
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void onGDDLInfo(CCObject *sender) {
        if (Mod::get()->getSettingValue<bool>("use-old-info-popup")) {
            GDDLLevelInfoPopup::create(m_level->m_levelID)->show();
        } else {
            m_fields->advancedLevelInfoPopup = GDDLAdvancedLevelInfoPopup::create(m_level, m_level->m_levelID);
            m_fields->advancedLevelInfoPopup->show();
        }
    }
};
