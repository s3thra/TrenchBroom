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

#include "NodeCollection.h"

#include "Ensure.h"
#include "Model/BrushNode.h"
#include "Model/EntityNode.h"
#include "Model/GroupNode.h"
#include "Model/LayerNode.h"
#include "Model/Node.h"
#include "Model/PatchNode.h"
#include "Model/WorldNode.h"

#include <kdl/overload.h>

#include <algorithm>
#include <vector>

namespace TrenchBroom {
namespace Model {
bool NodeCollection::empty() const {
  return m_nodes.empty();
}

size_t NodeCollection::nodeCount() const {
  return m_nodes.size();
}

size_t NodeCollection::layerCount() const {
  return m_layers.size();
}

size_t NodeCollection::groupCount() const {
  return m_groups.size();
}

size_t NodeCollection::entityCount() const {
  return m_entities.size();
}

size_t NodeCollection::brushCount() const {
  return m_brushes.size();
}

size_t NodeCollection::patchCount() const {
  return m_patches.size();
}

bool NodeCollection::hasLayers() const {
  return !m_layers.empty();
}

bool NodeCollection::hasOnlyLayers() const {
  return !empty() && nodeCount() == layerCount();
}

bool NodeCollection::hasGroups() const {
  return !m_groups.empty();
}

bool NodeCollection::hasOnlyGroups() const {
  return !empty() && nodeCount() == groupCount();
}

bool NodeCollection::hasEntities() const {
  return !m_entities.empty();
}

bool NodeCollection::hasOnlyEntities() const {
  return !empty() && nodeCount() == entityCount();
}

bool NodeCollection::hasBrushes() const {
  return !m_brushes.empty();
}

bool NodeCollection::hasOnlyBrushes() const {
  return !empty() && nodeCount() == brushCount();
}

bool NodeCollection::hasPatches() const {
  return !m_patches.empty();
}

bool NodeCollection::hasOnlyPatches() const {
  return !empty() && nodeCount() == patchCount();
}

std::vector<Node*>::iterator NodeCollection::begin() {
  return std::begin(m_nodes);
}

std::vector<Node*>::iterator NodeCollection::end() {
  return std::end(m_nodes);
}

std::vector<Node*>::const_iterator NodeCollection::begin() const {
  return std::begin(m_nodes);
}

std::vector<Node*>::const_iterator NodeCollection::end() const {
  return std::end(m_nodes);
}

const std::vector<Node*>& NodeCollection::nodes() const {
  return m_nodes;
}

const std::vector<LayerNode*>& NodeCollection::layers() const {
  return m_layers;
}

const std::vector<Model::GroupNode*>& NodeCollection::groups() const {
  return m_groups;
}

const std::vector<EntityNode*>& NodeCollection::entities() const {
  return m_entities;
}

const std::vector<BrushNode*>& NodeCollection::brushes() const {
  return m_brushes;
}

const std::vector<PatchNode*>& NodeCollection::patches() const {
  return m_patches;
}

void NodeCollection::addNodes(const std::vector<Node*>& nodes) {
  for (auto* node : nodes) {
    addNode(node);
  }
}

void NodeCollection::addNode(Node* node) {
  ensure(node != nullptr, "node is null");
  node->accept(kdl::overload(
    [](WorldNode*) {},
    [&](LayerNode* layer) {
      m_nodes.push_back(layer);
      m_layers.push_back(layer);
    },
    [&](GroupNode* group) {
      m_nodes.push_back(group);
      m_groups.push_back(group);
    },
    [&](EntityNode* entity) {
      m_nodes.push_back(entity);
      m_entities.push_back(entity);
    },
    [&](BrushNode* brush) {
      m_nodes.push_back(brush);
      m_brushes.push_back(brush);
    },
    [&](PatchNode* patch) {
      m_nodes.push_back(patch);
      m_patches.push_back(patch);
    }));
}

static const auto doRemoveNodes = [](
                                    auto& nodes, auto& layers, auto& groups, auto& entities,
                                    auto& brushes, auto& patches, auto cur, auto end) {
  auto nodeEnd = std::end(nodes);
  auto layerEnd = std::end(layers);
  auto groupEnd = std::end(groups);
  auto entityEnd = std::end(entities);
  auto brushEnd = std::end(brushes);
  auto patchEnd = std::end(patches);

  while (cur != end) {
    (*cur)->accept(kdl::overload(
      [](WorldNode*) {},
      [&](LayerNode* layer) {
        nodeEnd = std::remove(std::begin(nodes), nodeEnd, layer);
        layerEnd = std::remove(std::begin(layers), layerEnd, layer);
      },
      [&](GroupNode* group) {
        nodeEnd = std::remove(std::begin(nodes), nodeEnd, group);
        groupEnd = std::remove(std::begin(groups), groupEnd, group);
      },
      [&](EntityNode* entity) {
        nodeEnd = std::remove(std::begin(nodes), nodeEnd, entity);
        entityEnd = std::remove(std::begin(entities), entityEnd, entity);
      },
      [&](BrushNode* brush) {
        nodeEnd = std::remove(std::begin(nodes), nodeEnd, brush);
        brushEnd = std::remove(std::begin(brushes), brushEnd, brush);
      },
      [&](PatchNode* patch) {
        nodeEnd = std::remove(std::begin(nodes), nodeEnd, patch);
        patchEnd = std::remove(std::begin(patches), patchEnd, patch);
      }));
    ++cur;
  }

  nodes.erase(nodeEnd, std::end(nodes));
  layers.erase(layerEnd, std::end(layers));
  groups.erase(groupEnd, std::end(groups));
  entities.erase(entityEnd, std::end(entities));
  brushes.erase(brushEnd, std::end(brushes));
  patches.erase(patchEnd, std::end(patches));
};

void NodeCollection::removeNodes(const std::vector<Node*>& nodes) {
  doRemoveNodes(
    m_nodes, m_layers, m_groups, m_entities, m_brushes, m_patches, std::begin(nodes),
    std::end(nodes));
}

void NodeCollection::removeNode(Node* node) {
  ensure(node != nullptr, "node is null");
  doRemoveNodes(
    m_nodes, m_layers, m_groups, m_entities, m_brushes, m_patches, &node, std::next(&node));
}

void NodeCollection::clear() {
  m_nodes.clear();
  m_layers.clear();
  m_groups.clear();
  m_entities.clear();
  m_brushes.clear();
  m_patches.clear();
}
} // namespace Model
} // namespace TrenchBroom
