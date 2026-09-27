#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <vector>

using namespace geode::prelude;

enum class BotState { IDLE, RECORDING, PLAYING };

struct FrameInput {
    size_t frame;
    int button;
    bool player2;
    bool push;
};

class MNBot {
public:
    static MNBot& get() {
        static MNBot instance;
        return instance;
    }

    BotState state = BotState::IDLE;
    size_t currentFrame = 0;
    std::vector<FrameInput> inputs;
    size_t playIndex = 0;

    bool noclip = false;
    bool autoclicker = false;
    int autoclickInterval = 5;

    void reset() {
        currentFrame = 0;
        playIndex = 0;
    }

    void clearReplay() {
        inputs.clear();
        reset();
    }
};

class $modify(BotPlayLayer, PlayLayer) {
    struct Fields {
        cocos2d::CCLabelBMFont* m_hudLabel = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        MNBot::get().reset();

        auto label = cocos2d::CCLabelBMFont::create("MNbot: IDLE", "bigFont.fnt");
        if (label) {
            label->setScale(0.4f);
            label->setPosition({10.f, 20.f});
            label->setAnchorPoint({0.f, 0.f});
            label->setZOrder(999);
            this->addChild(label);
            m_fields->m_hudLabel = label;
        }

        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        auto& bot = MNBot::get();
        if (m_isPaused || !m_player1) return;

        bot.currentFrame++;

        // Autoclicker logic
        if (bot.autoclicker && m_player1 && (bot.currentFrame % bot.autoclickInterval == 0)) {
            m_player1->pushButton(PlayerButton::Jump);
            m_player1->releaseButton(PlayerButton::Jump);
        }

        // Replay Playback
        if (bot.state == BotState::PLAYING) {
            while (bot.playIndex < bot.inputs.size() && bot.inputs[bot.playIndex].frame <= bot.currentFrame) {
                const auto& in = bot.inputs[bot.playIndex];
                auto player = in.player2 ? m_player2 : m_player1;
                if (player) {
                    if (in.push) {
                        player->pushButton(static_cast<PlayerButton>(in.button));
                    } else {
                        player->releaseButton(static_cast<PlayerButton>(in.button));
                    }
                }
                bot.playIndex++;
            }
        }

        // Update HUD Label
        if (m_fields->m_hudLabel) {
            std::string stateStr = "IDLE";
            if (bot.state == BotState::RECORDING) stateStr = "[REC] RECORDING";
            else if (bot.state == BotState::PLAYING) stateStr = "[PLAY] PLAYING";

            std::string text = fmt::format("MNbot {} | Frame: {} | Inputs: {}", 
                stateStr, bot.currentFrame, bot.inputs.size());
            m_fields->m_hudLabel->setString(text.c_str());
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        MNBot::get().reset();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (MNBot::get().noclip) {
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }
};

class $modify(BotPlayerObject, PlayerObject) {
    void pushButton(PlayerButton button) {
        PlayerObject::pushButton(button);

        auto& bot = MNBot::get();
        if (bot.state == BotState::RECORDING) {
            bool isP2 = (this == PlayLayer::get()->m_player2);
            bot.inputs.push_back({bot.currentFrame, static_cast<int>(button), isP2, true});
        }
    }

    void releaseButton(PlayerButton button) {
        PlayerObject::releaseButton(button);

        auto& bot = MNBot::get();
        if (bot.state == BotState::RECORDING) {
            bool isP2 = (this == PlayLayer::get()->m_player2);
            bot.inputs.push_back({bot.currentFrame, static_cast<int>(button), isP2, false});
        }
    }
};
