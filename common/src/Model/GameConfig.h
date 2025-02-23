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

#pragma once

#include "Color.h"
#include "EL/Expression.h"
#include "FloatType.h"
#include "IO/Path.h"
#include "Model/BrushFaceAttributes.h"
#include "Model/CompilationConfig.h"
#include "Model/GameEngineConfig.h"
#include "Model/Tag.h"

#include <vecmath/bbox.h>

#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace TrenchBroom {
namespace Model {
struct MapFormatConfig {
  std::string format;
  IO::Path initialMap;
};

bool operator==(const MapFormatConfig& lhs, const MapFormatConfig& rhs);
bool operator!=(const MapFormatConfig& lhs, const MapFormatConfig& rhs);
std::ostream& operator<<(std::ostream& str, const MapFormatConfig& config);

struct PackageFormatConfig {
  std::vector<std::string> extensions;
  std::string format;
};

bool operator==(const PackageFormatConfig& lhs, const PackageFormatConfig& rhs);
bool operator!=(const PackageFormatConfig& lhs, const PackageFormatConfig& rhs);
std::ostream& operator<<(std::ostream& str, const PackageFormatConfig& config);

struct FileSystemConfig {
  IO::Path searchPath;
  PackageFormatConfig packageFormat;
};

bool operator==(const FileSystemConfig& lhs, const FileSystemConfig& rhs);
bool operator!=(const FileSystemConfig& lhs, const FileSystemConfig& rhs);
std::ostream& operator<<(std::ostream& str, const FileSystemConfig& config);

struct TextureFilePackageConfig {
  PackageFormatConfig fileFormat;
};

bool operator==(const TextureFilePackageConfig& lhs, const TextureFilePackageConfig& rhs);
bool operator!=(const TextureFilePackageConfig& lhs, const TextureFilePackageConfig& rhs);
std::ostream& operator<<(std::ostream& str, const TextureFilePackageConfig& config);

struct TextureDirectoryPackageConfig {
  IO::Path rootDirectory;
};

bool operator==(const TextureDirectoryPackageConfig& lhs, const TextureDirectoryPackageConfig& rhs);
bool operator!=(const TextureDirectoryPackageConfig& lhs, const TextureDirectoryPackageConfig& rhs);
std::ostream& operator<<(std::ostream& str, const TextureDirectoryPackageConfig& config);

using TexturePackageConfig = std::variant<TextureFilePackageConfig, TextureDirectoryPackageConfig>;
std::ostream& operator<<(std::ostream& str, const TexturePackageConfig& config);

IO::Path getRootDirectory(const TexturePackageConfig& texturePackageConfig);

struct TextureConfig {
  TexturePackageConfig package;
  PackageFormatConfig format;
  IO::Path palette;
  std::string property;
  IO::Path shaderSearchPath;
  std::vector<std::string> excludes; // Glob patterns used to match texture names for exclusion
};

bool operator==(const TextureConfig& lhs, const TextureConfig& rhs);
bool operator!=(const TextureConfig& lhs, const TextureConfig& rhs);
std::ostream& operator<<(std::ostream& str, const TextureConfig& config);

struct EntityConfig {
  std::vector<IO::Path> defFilePaths;
  std::vector<std::string> modelFormats;
  Color defaultColor;
  std::optional<EL::Expression> scaleExpression;
};

bool operator==(const EntityConfig& lhs, const EntityConfig& rhs);
bool operator!=(const EntityConfig& lhs, const EntityConfig& rhs);
std::ostream& operator<<(std::ostream& str, const EntityConfig& config);

struct FlagConfig {
  std::string name;
  std::string description;
  int value;
};

bool operator==(const FlagConfig& lhs, const FlagConfig& rhs);
bool operator!=(const FlagConfig& lhs, const FlagConfig& rhs);
std::ostream& operator<<(std::ostream& str, const FlagConfig& config);

struct FlagsConfig {
  std::vector<FlagConfig> flags;

  int flagValue(const std::string& flagName) const;
  std::string flagName(size_t index) const;
  std::vector<std::string> flagNames(int mask = ~0) const;
};

bool operator==(const FlagsConfig& lhs, const FlagsConfig& rhs);
bool operator!=(const FlagsConfig& lhs, const FlagsConfig& rhs);
std::ostream& operator<<(std::ostream& str, const FlagsConfig& config);

struct FaceAttribsConfig {
  FlagsConfig surfaceFlags;
  FlagsConfig contentFlags;
  BrushFaceAttributes defaults{BrushFaceAttributes::NoTextureName};
};

bool operator==(const FaceAttribsConfig& lhs, const FaceAttribsConfig& rhs);
bool operator!=(const FaceAttribsConfig& lhs, const FaceAttribsConfig& rhs);
std::ostream& operator<<(std::ostream& str, const FaceAttribsConfig& config);

struct CompilationTool {
  std::string name;
  std::optional<std::string> description;
};

bool operator==(const CompilationTool& lhs, const CompilationTool& rhs);
bool operator!=(const CompilationTool& lhs, const CompilationTool& rhs);
std::ostream& operator<<(std::ostream& str, const CompilationTool& tool);

struct GameConfig {
  std::string name;
  IO::Path path;
  IO::Path icon;
  bool experimental;
  std::vector<MapFormatConfig> fileFormats;
  FileSystemConfig fileSystemConfig;
  TextureConfig textureConfig;
  EntityConfig entityConfig;
  FaceAttribsConfig faceAttribsConfig;
  std::vector<SmartTag> smartTags;
  std::optional<vm::bbox3> softMapBounds;
  std::vector<CompilationTool> compilationTools;

  CompilationConfig compilationConfig{};
  GameEngineConfig gameEngineConfig{};
  bool compilationConfigParseFailed{false};
  bool gameEngineConfigParseFailed{false};

  size_t maxPropertyLength{1023};

  IO::Path findInitialMap(const std::string& formatName) const;
  IO::Path findConfigFile(const IO::Path& filePath) const;
};

bool operator==(const GameConfig& lhs, const GameConfig& rhs);
bool operator!=(const GameConfig& lhs, const GameConfig& rhs);
std::ostream& operator<<(std::ostream& str, const GameConfig& config);
} // namespace Model
} // namespace TrenchBroom
