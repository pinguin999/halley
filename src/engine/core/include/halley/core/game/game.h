#pragma once

#include "halley/core/stage/stage.h"

namespace Halley
{
	class SceneEditorContext;
	class IPluginRegistry;
	class ResourceLocator;
	class Stage;
	class Environment;
	class GameConsole;
	class SceneEditorInterface;
	
	class Game
	{
	public:
		virtual ~Game();

		virtual void init(const Environment&, const Vector<String>& /*args*/);
		virtual int initPlugins(IPluginRegistry &registry) = 0;
		virtual void initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator);

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
		virtual bool isDevMode() const = 0;
		virtual bool shouldCreateSeparateConsole() const;

		virtual std::unique_ptr<Stage> startGame(const HalleyAPI*) = 0;
		virtual void endGame();

		virtual std::unique_ptr<Stage> makeStage(StageID id);

		virtual int getTargetFPS() const;

		virtual String getDevConAddress() const;
		virtual int getDevConPort() const;

		virtual std::shared_ptr<GameConsole> getGameConsole() const;

		virtual void onUncaughtException(const Exception& exception, TimeLine timeLine);

		virtual std::unique_ptr<SceneEditorInterface> createSceneEditorInterface();
	};
}
