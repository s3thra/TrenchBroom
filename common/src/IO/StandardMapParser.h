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

#include "FloatType.h"
#include "IO/MapParser.h"
#include "IO/Parser.h"
#include "IO/Tokenizer.h"
#include "Model/MapFormat.h"

#include <kdl/vector_set_forward.h>

#include <vecmath/forward.h>

#include <string_view>
#include <tuple>
#include <vector>

namespace TrenchBroom {
namespace IO {
class ParserStatus;

namespace QuakeMapToken {
using Type = unsigned int;
static const Type Integer = 1 << 0;      // integer number
static const Type Decimal = 1 << 1;      // decimal number
static const Type String = 1 << 2;       // string
static const Type OParenthesis = 1 << 3; // opening parenthesis: (
static const Type CParenthesis = 1 << 4; // closing parenthesis: )
static const Type OBrace = 1 << 5;       // opening brace: {
static const Type CBrace = 1 << 6;       // closing brace: }
static const Type OBracket = 1 << 7;     // opening bracket: [
static const Type CBracket = 1 << 8;     // closing bracket: ]
static const Type Comment = 1 << 9;      // line comment starting with ///
static const Type Eof = 1 << 10;         // end of file
static const Type Eol = 1 << 11;         // end of line
static const Type Number = Integer | Decimal;
} // namespace QuakeMapToken

class QuakeMapTokenizer : public Tokenizer<QuakeMapToken::Type> {
private:
  static const std::string& NumberDelim();
  bool m_skipEol;

public:
  explicit QuakeMapTokenizer(std::string_view str);

  void setSkipEol(bool skipEol);

private:
  Token emitToken() override;
};

class StandardMapParser : public MapParser, public Parser<QuakeMapToken::Type> {
private:
  using Token = QuakeMapTokenizer::Token;
  using EntityPropertyKeys = kdl::vector_set<std::string>;

  static const std::string BrushPrimitiveId;
  static const std::string PatchId;

  QuakeMapTokenizer m_tokenizer;

protected:
  Model::MapFormat m_sourceMapFormat;
  Model::MapFormat m_targetMapFormat;

public:
  /**
   * Creates a new parser where the given string is expected to be formatted in the given source map
   * format, and the created objects are converted to the given target format.
   *
   * @param str the string to parse
   * @param sourceMapFormat the expected format of the given string
   * @param targetMapFormat the format to convert the created objects to
   */
  StandardMapParser(
    std::string_view str, Model::MapFormat sourceMapFormat, Model::MapFormat targetMapFormat);

  ~StandardMapParser() override;

protected:
  void parseEntities(ParserStatus& status);
  void parseBrushesOrPatches(ParserStatus& status);
  void parseBrushFaces(ParserStatus& status);

  void reset();

private:
  void parseEntity(ParserStatus& status);
  void parseEntityProperty(
    std::vector<Model::EntityProperty>& properties, EntityPropertyKeys& keys, ParserStatus& status);

  void parseBrushOrBrushPrimitiveOrPatch(ParserStatus& status);
  void parseBrushPrimitive(ParserStatus& status, size_t startLine);
  void parseBrush(ParserStatus& status, size_t startLine, bool primitive);

  void parseFace(ParserStatus& status, bool primitive);
  void parseQuakeFace(ParserStatus& status);
  void parseQuake2Face(ParserStatus& status);
  void parseQuake2ValveFace(ParserStatus& status);
  void parseHexen2Face(ParserStatus& status);
  void parseDaikatanaFace(ParserStatus& status);
  void parseValveFace(ParserStatus& status);
  void parsePrimitiveFace(ParserStatus& status);

  void parsePatch(ParserStatus& status, size_t startLine);

  std::tuple<vm::vec3, vm::vec3, vm::vec3> parseFacePoints(ParserStatus& status);
  std::string parseTextureName(ParserStatus& status);
  std::tuple<vm::vec3, float, vm::vec3, float> parseValveTextureAxes(ParserStatus& status);
  std::tuple<vm::vec3, vm::vec3> parsePrimitiveTextureAxes(ParserStatus& status);

  template <size_t S = 3, typename T = FloatType>
  vm::vec<T, S> parseFloatVector(const QuakeMapToken::Type o, const QuakeMapToken::Type c) {
    expect(o, m_tokenizer.nextToken());
    vm::vec<T, S> vec;
    for (size_t i = 0; i < S; i++) {
      vec[i] = expect(QuakeMapToken::Number, m_tokenizer.nextToken()).toFloat<T>();
    }
    expect(c, m_tokenizer.nextToken());
    return vec;
  }

  float parseFloat();
  int parseInteger();

private: // implement Parser interface
  TokenNameMap tokenNames() const override;
};
} // namespace IO
} // namespace TrenchBroom
