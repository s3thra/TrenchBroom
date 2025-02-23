/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "GameFactory.h"

#include "Exceptions.h"
#include "IO/CompilationConfigParser.h"
#include "IO/CompilationConfigWriter.h"
#include "IO/DiskFileSystem.h"
#include "IO/File.h"
#include "IO/FileMatcher.h"
#include "IO/GameConfigParser.h"
#include "IO/GameEngineConfigParser.h"
#include "IO/GameEngineConfigWriter.h"
#include "IO/IOUtils.h"
#include "IO/SystemPaths.h"
#include "Logger.h"
#include "Model/Game.h"
#include "Model/GameConfig.h"
#include "Model/GameImpl.h"
#include "PreferenceManager.h"
#include "RecoverableExceptions.h"

#include <kdl/collection_utils.h>
#include <kdl/string_compare.h>
#include <kdl/string_utils.h>
#include <kdl/vector_utils.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace TrenchBroom {
namespace Model {
GameFactory& GameFactory::instance() {
  static auto instance = GameFactory{};
  return instance;
}

void GameFactory::initialize(const GamePathConfig& gamePathConfig) {
  initializeFileSystem(gamePathConfig);
  loadGameConfigs();
}

void GameFactory::saveGameEngineConfig(
  const std::string& gameName, const GameEngineConfig& gameEngineConfig) {
  auto& config = gameConfig(gameName);
  writeGameEngineConfig(config, gameEngineConfig);
}

void GameFactory::saveCompilationConfig(
  const std::string& gameName, const CompilationConfig& compilationConfig, Logger& logger) {
  auto& config = gameConfig(gameName);
  writeCompilationConfig(config, compilationConfig, logger);
}

const std::vector<std::string>& GameFactory::gameList() const {
  return m_names;
}

size_t GameFactory::gameCount() const {
  return m_configs.size();
}

std::shared_ptr<Game> GameFactory::createGame(const std::string& gameName, Logger& logger) {
  return std::make_shared<GameImpl>(gameConfig(gameName), gamePath(gameName), logger);
}

std::vector<std::string> GameFactory::fileFormats(const std::string& gameName) const {
  return kdl::vec_transform(gameConfig(gameName).fileFormats, [](const auto& format) {
    return format.format;
  });
}

IO::Path GameFactory::iconPath(const std::string& gameName) const {
  const auto& config = gameConfig(gameName);
  return config.findConfigFile(config.icon);
}

IO::Path GameFactory::gamePath(const std::string& gameName) const {
  const auto it = m_gamePaths.find(gameName);
  if (it == std::end(m_gamePaths)) {
    throw GameException{"Unknown game: " + gameName};
  }
  auto& pref = it->second;
  return PreferenceManager::instance().get(pref);
}

bool GameFactory::setGamePath(const std::string& gameName, const IO::Path& gamePath) {
  const auto it = m_gamePaths.find(gameName);
  if (it == std::end(m_gamePaths)) {
    throw GameException{"Unknown game: " + gameName};
  }
  auto& pref = it->second;
  return PreferenceManager::instance().set(pref, gamePath);
}

bool GameFactory::isGamePathPreference(
  const std::string& gameName, const IO::Path& prefPath) const {
  const auto it = m_gamePaths.find(gameName);
  if (it == std::end(m_gamePaths)) {
    throw GameException{"Unknown game: " + gameName};
  }
  auto& pref = it->second;
  return pref.path() == prefPath;
}

static Preference<IO::Path>& compilationToolPathPref(
  const std::string& gameName, const std::string& toolName) {
  auto& prefs = PreferenceManager::instance();
  auto& pref = prefs.dynamicPreference(
    IO::Path{"Games"} + IO::Path{gameName} + IO::Path{"Tool Path"} + IO::Path{toolName},
    IO::Path{});
  return pref;
}

IO::Path GameFactory::compilationToolPath(
  const std::string& gameName, const std::string& toolName) const {
  return PreferenceManager::instance().get(compilationToolPathPref(gameName, toolName));
}

bool GameFactory::setCompilationToolPath(
  const std::string& gameName, const std::string& toolName, const IO::Path& gamePath) {
  return PreferenceManager::instance().set(compilationToolPathPref(gameName, toolName), gamePath);
}

GameConfig& GameFactory::gameConfig(const std::string& name) {
  const auto cIt = m_configs.find(name);
  if (cIt == std::end(m_configs)) {
    throw GameException{"Unknown game: " + name};
  }
  return cIt->second;
}

const GameConfig& GameFactory::gameConfig(const std::string& name) const {
  const auto cIt = m_configs.find(name);
  if (cIt == std::end(m_configs)) {
    throw GameException{"Unknown game: " + name};
  }
  return cIt->second;
}

std::pair<std::string, MapFormat> GameFactory::detectGame(const IO::Path& path) const {
  auto stream = openPathAsInputStream(path);
  if (!stream.is_open()) {
    throw FileSystemException{"Cannot open file: " + path.asString()};
  }

  auto gameName = IO::readGameComment(stream);
  if (m_configs.find(gameName) == std::end(m_configs)) {
    gameName = "";
  }

  const auto formatName = IO::readFormatComment(stream);
  const auto format = formatFromName(formatName);

  return {gameName, format};
}

const IO::Path& GameFactory::userGameConfigsPath() const {
  return m_userGameDir;
}

GameFactory::GameFactory() = default;

void GameFactory::initializeFileSystem(const GamePathConfig& gamePathConfig) {
  // Gather the search paths we're going to use.
  // The rest of this function will be chaining together TB filesystem objects for these search
  // paths.
  const auto& userGameDir = gamePathConfig.userGameDir;
  const auto& gameConfigSearchDirs = gamePathConfig.gameConfigSearchDirs;

  // All of the current search paths from highest to lowest priority
  auto chain = std::unique_ptr<IO::DiskFileSystem>{};
  for (auto it = gameConfigSearchDirs.rbegin(); it != gameConfigSearchDirs.rend(); ++it) {
    const auto path = *it;

    if (chain != nullptr) {
      chain = std::make_unique<IO::DiskFileSystem>(std::move(chain), path, false);
    } else {
      chain = std::make_unique<IO::DiskFileSystem>(path, false);
    }
  }

  // This is where we write configs
  if (chain != nullptr) {
    m_configFS = std::make_unique<IO::WritableDiskFileSystem>(std::move(chain), userGameDir, true);
  } else {
    m_configFS = std::make_unique<IO::WritableDiskFileSystem>(userGameDir, true);
  }

  m_userGameDir = userGameDir;
}

void GameFactory::loadGameConfigs() {
  auto errors = std::vector<std::string>{};

  const auto configFiles =
    m_configFS->findItemsRecursively(IO::Path(""), IO::FileNameMatcher("GameConfig.cfg"));
  for (const auto& configFilePath : configFiles) {
    try {
      loadGameConfig(configFilePath);
    } catch (const std::exception& e) {
      errors.push_back(kdl::str_to_string(
        "Could not load game configuration file ", configFilePath, ": ", e.what()));
    }
  }

  m_names = kdl::col_sort(std::move(m_names), kdl::cs::string_less());

  if (!errors.empty()) {
    throw errors;
  }
}

void GameFactory::loadGameConfig(const IO::Path& path) {
  try {
    doLoadGameConfig(path);
  } catch (const RecoverableException& e) {
    e.recover();
    doLoadGameConfig(path);
  }
}

void GameFactory::doLoadGameConfig(const IO::Path& path) {
  const auto configFile = m_configFS->openFile(path);
  const auto absolutePath = m_configFS->makeAbsolute(path);

  auto reader = configFile->reader().buffer();
  auto parser = IO::GameConfigParser{reader.stringView(), absolutePath};
  auto config = parser.parse();

  loadCompilationConfig(config);
  loadGameEngineConfig(config);

  const auto configName = config.name;
  m_configs.emplace(configName, std::move(config));
  m_names.push_back(configName);

  const auto gamePathPrefPath = IO::Path{"Games"} + IO::Path{configName} + IO::Path{"Path"};
  m_gamePaths.emplace(configName, Preference<IO::Path>{gamePathPrefPath, IO::Path{}});

  const auto defaultEnginePrefPath =
    IO::Path{"Games"} + IO::Path{configName} + IO::Path{"Default Engine"};
  m_defaultEngines.emplace(configName, Preference<IO::Path>{defaultEnginePrefPath, IO::Path{}});
}

void GameFactory::loadCompilationConfig(GameConfig& gameConfig) {
  const auto path = IO::Path{gameConfig.name} + IO::Path{"CompilationProfiles.cfg"};
  try {
    if (m_configFS->fileExists(path)) {
      const auto profilesFile = m_configFS->openFile(path);
      auto reader = profilesFile->reader().buffer();
      auto parser =
        IO::CompilationConfigParser{reader.stringView(), m_configFS->makeAbsolute(path)};
      gameConfig.compilationConfig = parser.parse();
      gameConfig.compilationConfigParseFailed = false;
    }
  } catch (const Exception& e) {
    std::cerr << "Could not load compilation configuration '" << path << "': " << e.what() << "\n";
    gameConfig.compilationConfigParseFailed = true;
  }
}

void GameFactory::loadGameEngineConfig(GameConfig& gameConfig) {
  const auto path = IO::Path{gameConfig.name} + IO::Path{"GameEngineProfiles.cfg"};
  try {
    if (m_configFS->fileExists(path)) {
      const auto profilesFile = m_configFS->openFile(path);
      auto reader = profilesFile->reader().buffer();
      auto parser = IO::GameEngineConfigParser{reader.stringView(), m_configFS->makeAbsolute(path)};
      gameConfig.gameEngineConfig = parser.parse();
      gameConfig.gameEngineConfigParseFailed = false;
    }
  } catch (const Exception& e) {
    std::cerr << "Could not load game engine configuration '" << path << "': " << e.what() << "\n";
    gameConfig.gameEngineConfigParseFailed = true;
  }
}

static IO::Path backupFile(IO::WritableDiskFileSystem& fs, const IO::Path& path) {
  const auto backupPath = path.addExtension("bak");
  fs.copyFile(path, backupPath, true);
  return backupPath;
}

void GameFactory::writeCompilationConfig(
  GameConfig& gameConfig, const CompilationConfig& compilationConfig, Logger& logger) {
  if (
    !gameConfig.compilationConfigParseFailed &&
    gameConfig.compilationConfig == std::move(compilationConfig)) {
    // NOTE: this is not just an optimization, but important for ensuring that
    // we don't clobber data saved by a newer version of TB, unless we actually make changes
    // to the config in this version of TB (see:
    // https://github.com/TrenchBroom/TrenchBroom/issues/3424)
    logger.debug() << "Skipping writing unchanged compilation config for " << gameConfig.name;
    return;
  }

  auto stream = std::stringstream{};
  auto writer = IO::CompilationConfigWriter{compilationConfig, stream};
  writer.writeConfig();

  const auto profilesPath = IO::Path{gameConfig.name} + IO::Path{"CompilationProfiles.cfg"};
  if (gameConfig.compilationConfigParseFailed) {
    const auto backupPath = backupFile(*m_configFS, profilesPath);

    logger.warn() << "Backed up malformed compilation config "
                  << m_configFS->makeAbsolute(profilesPath) << " to "
                  << m_configFS->makeAbsolute(backupPath);

    gameConfig.compilationConfigParseFailed = false;
  }

  m_configFS->createFileAtomic(profilesPath, stream.str());
  gameConfig.compilationConfig = std::move(compilationConfig);
  logger.debug() << "Wrote compilation config to " << m_configFS->makeAbsolute(profilesPath);
}

void GameFactory::writeGameEngineConfig(
  GameConfig& gameConfig, const GameEngineConfig& gameEngineConfig) {
  if (
    !gameConfig.gameEngineConfigParseFailed &&
    gameConfig.gameEngineConfig == std::move(gameEngineConfig)) {
    std::cout << "Skipping writing unchanged game engine config for " << gameConfig.name;
    return;
  }

  auto stream = std::stringstream{};
  auto writer = IO::GameEngineConfigWriter{gameEngineConfig, stream};
  writer.writeConfig();

  const auto profilesPath = IO::Path{gameConfig.name} + IO::Path{"GameEngineProfiles.cfg"};
  if (gameConfig.gameEngineConfigParseFailed) {
    const auto backupPath = backupFile(*m_configFS, profilesPath);

    std::cerr << "Backed up malformed game engine config " << m_configFS->makeAbsolute(profilesPath)
              << " to " << m_configFS->makeAbsolute(backupPath) << std::endl;

    gameConfig.gameEngineConfigParseFailed = false;
  }
  m_configFS->createFileAtomic(profilesPath, stream.str());
  gameConfig.gameEngineConfig = std::move(gameEngineConfig);
  std::cout << "Wrote game engine config to " << m_configFS->makeAbsolute(profilesPath)
            << std::endl;
}
} // namespace Model
} // namespace TrenchBroom
