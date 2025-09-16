#include <Geode/Geode.hpp>
#include <Geode/modify/LevelPage.hpp>

#include "RatingsManager.h"
#include "Utils.h"
#include "layers/GDDLLevelInfoPopup.h"
#include "settings/ExcludeRangeSettingV3.h"
#include "layers/GDDLAdvancedLevelInfoPopup.h"

class $modify(GDDLLevelPage, LevelPage) {
    struct Fields {
        CCMenuItemSpriteExtra* tierButton = nullptr;
        GDDLAdvancedLevelInfoPopup* advancedLevelInfoPopup = nullptr;
    };

    void updateDynamicPage(GJGameLevel* level) {
        LevelPage::updateDynamicPage(level);

        const int levelID = level->m_levelID;
        const bool isDemon = levelID == 14 || levelID == 18 || levelID == 20;

        const bool isLocked = m_levelDisplay->getChildByID("lock-sprite") != nullptr;
        const bool isOvercharged = isOverchargedEnabled();

        if (!isDemon || isLocked || !Utils::notExcluded(level->m_levelID)) {
            if (m_fields->tierButton) {
                m_fields->tierButton->removeFromParent();
                m_fields->tierButton = nullptr;

                if (!isOvercharged) {
                    m_starsSprite->setPositionX(m_starsSprite->getPositionX() + 22.0f);
                    m_starsLabel->setPositionX(m_starsLabel->getPositionX() + 20.0f);
                }
            }

            return;
        }

        if (m_fields->tierButton) return;

        const auto menu = CCMenu::create();
        menu->setContentSize({35.2f, 35.2f});
        menu->setScale(isOvercharged ? 0.7f : 0.5f);
        menu->setID("rating-menu"_spr);
        addChild(menu);

        if (isOvercharged) {
            // could look nicer but like,,,,it's the official levels. and overcharged menu. who gaf

            menu->setPosition(
                convertToNodeSpace(m_levelDisplay->convertToWorldSpace(m_difficultySprite->getPosition()))
                + ccp(32.0f, 5.0f)
            );
        } else {
            menu->setPosition(
                convertToNodeSpace(m_levelDisplay->convertToWorldSpace(m_starsSprite->getPosition()))
            );
            m_starsSprite->setPositionX(m_starsSprite->getPositionX() - 22.0f);
            m_starsLabel->setPositionX(m_starsLabel->getPositionX() - 20.0f);
        }

        // just so it displays a bit earlier
        const auto button = CCMenuItemSpriteExtra::create(
            Utils::getSpriteFromTier(RatingsManager::getCachedTier(levelID)),
            this, menu_selector(GDDLLevelPage::onGDDLInfo
        ));
        button->setID("rating"_spr);
        menu->addChildAtPosition(button, Anchor::Center);
        m_fields->tierButton = button;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void onGDDLInfo(CCObject *sender) {
        const int levelID = officialToGDDLID(m_level);

        if (Mod::get()->getSettingValue<bool>("use-old-info-popup")) {
            GDDLLevelInfoPopup::create(levelID)->show();
        } else {
            m_fields->advancedLevelInfoPopup = GDDLAdvancedLevelInfoPopup::create(m_level, levelID);
            m_fields->advancedLevelInfoPopup->show();
        }
    }

    bool isOverchargedEnabled() {
        const auto mod = Loader::get()->getLoadedMod("firee.overchargedlevels");
        if (!mod) return false;

        return (
            mod->getSettingValue<bool>("enabled") &&
            mod->getSettingValue<bool>("overcharge-menu")
        );
    }

    int officialToGDDLID(GJGameLevel* level) {
        switch (level->m_levelID) {
            case 14: return 1; // clubstep
            case 18: return 2; // toe 2
            case 20: return 3; // deadlocked
            default: return -1;
        }
    }
};
