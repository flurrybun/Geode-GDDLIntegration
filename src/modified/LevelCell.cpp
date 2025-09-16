#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/web.hpp>

#include "RatingsManager.h"
#include "Utils.h"
#include "settings/ExcludeRangeSettingV3.h"

using namespace geode::prelude;

class $modify(LevelCell) {
    struct Fields {
        EventListener<web::WebTask> infoLayerGetRatingListener;
        CCSprite* tierSpr = nullptr;
        int gddlTier = -1;
    };

    bool init() {
        if (!LevelCell::init()) return false;

        if (m_level->m_stars != 10 || !Utils::notExcluded(m_level->m_levelID)) return true;
        if (!Mod::get()->getSettingValue<bool>("show-in-level-cell")) return true;
        log::info("init level cell");

        // setup web req
        m_fields->infoLayerGetRatingListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                const std::string response = res->string().unwrapOrDefault();
                if (response.empty()) {
                    updateTier(-1);
                } else {
                    const int levelID = m_level->m_levelID;
                    int tierAfterFetch = -1;
                    if(RatingsManager::addRatingFromResponse(levelID, response)) {
                        tierAfterFetch = RatingsManager::getDemonTier(levelID);
                    }
                    updateTier(tierAfterFetch);
                }
            } else if (e->isCancelled()) {
                updateTier(-1);
            }
        });

        return true;
    }

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        log::info("aaah");

        if (m_level->m_stars != 10 || !Utils::notExcluded(m_level->m_levelID)) return;
        if (!Mod::get()->getSettingValue<bool>("show-in-level-cell")) return;

        const int levelID = m_level->m_levelID;
        m_fields->gddlTier = RatingsManager::getDemonTier(levelID);

        if (m_fields->gddlTier == -1) {
            // web request 2.0 yaaay
            auto req = web::WebRequest();
            req.header("User-Agent", Utils::getUserAgent());
            m_fields->infoLayerGetRatingListener.setFilter(req.get(RatingsManager::getRequestUrl(levelID)));
        }

        const auto tierSpr = Utils::getSpriteFromTier(RatingsManager::getCachedTier(levelID));
        tierSpr->setID("rating"_spr);
        m_fields->tierSpr = tierSpr;

        const std::string buttonPosition = Mod::get()->getSettingValue<std::string>("button-position");

        if (buttonPosition == "Left of Difficulty" || buttonPosition == "Stars/Moons Label") {
            auto difficultyContainer = m_mainLayer->getChildByID("difficulty-container");
            auto starsLabel = difficultyContainer->getChildByID("stars-label");
            starsLabel->setVisible(false);

            tierSpr->setPosition(starsLabel->getPosition());
            tierSpr->setAnchorPoint(starsLabel->getAnchorPoint());
            tierSpr->setScale(0.11f);

            difficultyContainer->addChild(tierSpr);
        } else {
            const auto titleLabel = m_mainLayer->getChildByID("level-name");

            tierSpr->setAnchorPoint({0.0f, 0.5f});
            tierSpr->setScale(0.138f);

            const bool isLeft = buttonPosition == "Left of Title";
            const auto offset = ccp(
                isLeft ? 2.0f : (titleLabel->getScaledContentWidth() + 2.0f),
                -1.5f
            );

            tierSpr->setPosition(titleLabel->getPosition() + offset);
            if (isLeft) titleLabel->setPositionX(titleLabel->getPositionX() + 22.0f);

            // move the checkmark/percentage
            if (const auto checkmark = m_mainLayer->getChildByID("completed-icon")) {
                checkmark->setPositionX(checkmark->getPositionX() + 17.0f);
            } else if (const auto percentLabel = m_mainLayer->getChildByID("percentage-label")) {
                percentLabel->setPositionX(percentLabel->getPositionX() + 17.0f);
            }

            m_mainLayer->addChild(tierSpr);
        }
    }

    void updateTier(const int tier) {
        if (!m_fields->tierSpr) return;

        log::info("updating tier to {}", tier);
        m_fields->gddlTier = tier;

        auto newTierSpr = Utils::getSpriteFromTier(tier);
        newTierSpr->setPosition(m_fields->tierSpr->getPosition());
        newTierSpr->setScale(m_fields->tierSpr->getScale());
        newTierSpr->setAnchorPoint(m_fields->tierSpr->getAnchorPoint());

        addChild(newTierSpr);
        m_fields->tierSpr->removeFromParent();
    }
};
