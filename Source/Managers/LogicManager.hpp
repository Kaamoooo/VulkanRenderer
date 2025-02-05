#include <utility>


namespace Kaamoo {
    class LogicManager {
    public:
        LogicManager(std::shared_ptr<ResourceManager> resourceManager) {
            m_resourceManager = resourceManager;
        };

        ~LogicManager() {};

        LogicManager(const LogicManager &) = delete;

        LogicManager &operator=(const LogicManager &) = delete;

        void UpdateComponents(FrameInfo &frameInfo) {
            UpdateUbo(frameInfo);
            ComponentUpdateInfo updateInfo{};
            auto &_renderer = m_resourceManager->GetRenderer();
            auto &_gameObjects = m_resourceManager->GetGameObjects();
            RendererInfo rendererInfo{_renderer.getAspectRatio(), _renderer.FOV_Y, _renderer.NEAR_CLIP, _renderer.FAR_CLIP};
            updateInfo.frameInfo = &frameInfo;
            updateInfo.rendererInfo = &rendererInfo;


            static bool firstFrame = true;
            if (firstFrame) {
                for (auto &pair: _gameObjects) {
                    if (!pair.second.IsActive()) continue;
                    updateInfo.gameObject = &pair.second;
                    pair.second.Start(updateInfo);
                }
                firstFrame = false;
            }

            for (auto &pair: _gameObjects) {
                auto& _gameObject = pair.second;
                updateInfo.gameObject = &_gameObject;
                if (_gameObject.IsOnDisabled()) {
                    _gameObject.OnDisable(updateInfo);
                }
                if (_gameObject.IsOnEnabled()){
                    _gameObject.OnEnable(updateInfo);
                }
            }

            for (auto &pair: _gameObjects) {
                if (!pair.second.IsActive()) continue;
                updateInfo.gameObject = &pair.second;
                pair.second.Update(updateInfo);
            }

            for (auto &pair: _gameObjects) {
                if (!pair.second.IsActive()) continue;
                updateInfo.gameObject = &pair.second;
                pair.second.LateUpdate(updateInfo);
            }

            FixedUpdateComponents(frameInfo);
        }

        void FixedUpdateComponents(FrameInfo &frameInfo) {
            auto &_renderer = m_resourceManager->GetRenderer();
            auto &_gameObjects = m_resourceManager->GetGameObjects();
            static float reservedFrameTime = 0;
            float frameTime = frameInfo.frameTime + reservedFrameTime;

            while (frameTime >= FIXED_UPDATE_INTERVAL) {
                frameTime -= FIXED_UPDATE_INTERVAL;

                ComponentUpdateInfo updateInfo{};
                RendererInfo rendererInfo{_renderer.getAspectRatio()};
                updateInfo.frameInfo = &frameInfo;
                updateInfo.rendererInfo = &rendererInfo;
                for (auto &pair: _gameObjects) {
                    if (!pair.second.IsActive()) continue;
                    updateInfo.gameObject = &pair.second;
                    pair.second.FixedUpdate(updateInfo);
                }
                for (auto &pair: _gameObjects) {
                    if (!pair.second.IsActive()) continue;
                    updateInfo.gameObject = &pair.second;
                    pair.second.LateFixedUpdate(updateInfo);
                }
            }
            reservedFrameTime = frameTime;
        }

//Todo: Light is not active but it still contributes lighting
        void UpdateUbo(FrameInfo &frameInfo) {
            frameInfo.globalUbo.lightNum = LightComponent::GetLightNum();
            frameInfo.globalUbo.curTime = frameInfo.totalTime;
        }

    private:
        std::shared_ptr<ResourceManager> m_resourceManager;
    };
}