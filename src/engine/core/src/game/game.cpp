#include "halley/core/game/game.h"

#include "halley/ui/ui_factory.h"
using namespace Halley;

Game::~Game() = default;

void Game::init(const Environment&, const Vector<String>&)
{}

void Game::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{}

bool Game::shouldCreateSeparateConsole() const
{
	return isDevMode();
}

void Game::endGame()
{}

std::unique_ptr<Stage> Game::makeStage(StageID)
{
	return std::unique_ptr<Stage>();
}

int Game::getTargetFPS() const
{
	return 60;
}

String Game::getDevConAddress() const
{
	return "";
}

int Game::getDevConPort() const
{
	return 12500;
}

std::shared_ptr<GameConsole> Game::getGameConsole() const
{
	return {};
}

void Game::onUncaughtException(const Exception& exception, TimeLine timeLine)
{
	throw exception;
}

std::unique_ptr<ISceneEditor> Game::createSceneEditorInterface()
{
	return {};
}

std::unique_ptr<IEditorCustomTools> Game::createEditorCustomToolsInterface()
{
	return {};
}

std::unique_ptr<UIFactory> Game::createUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
{
	return std::make_unique<UIFactory>(api, resources, i18n);
}

void Game::attachToEditorDebugConsole(UIDebugConsoleCommands& commands, Resources& gameResources, IProject& project)
{
}

const HalleyAPI& Game::getAPI() const
{
	if (!api) {
		throw Exception("HalleyAPI is only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *api;
}

Resources& Game::getResources() const
{
	if (!resources) {
		throw Exception("Resources are only initialized on Game right before call to startGame()", HalleyExceptions::Core);
	}
	return *resources;
}
