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

#include "RenderBatch.h"

#include "Ensure.h"
#include "Renderer/Renderable.h"
#include "Renderer/RenderContext.h"

#include <kdl/vector_utils.h>

namespace TrenchBroom {
    namespace Renderer {
        class RenderBatch::IndexedRenderableWrapper : public IndexedRenderable {
        private:
            IndexedRenderable* m_wrappee;
        public:
            IndexedRenderableWrapper(RenderContext& /* renderContext */, IndexedRenderable* wrappee) :
            m_wrappee(wrappee) {
                ensure(m_wrappee != nullptr, "wrappee is null");
            }
        private:
            void prepareVerticesAndIndices(RenderContext& renderContext) override {
                m_wrappee->prepareVerticesAndIndices(renderContext);
            }

            void doRender(RenderState& renderState) override {
                m_wrappee->render(renderState);
            }
        };

        RenderBatch::RenderBatch(RenderContext& renderContext) :
        m_renderContext(renderContext) {}

        RenderBatch::~RenderBatch() {
            kdl::vec_clear_and_delete(m_oneshots);
            kdl::vec_clear_and_delete(m_indexedRenderables);
        }

        void RenderBatch::add(Renderable* renderable) {
            doAdd(renderable);
        }

        void RenderBatch::add(DirectRenderable* renderable) {
            doAdd(renderable);
            m_directRenderables.push_back(renderable);
        }

        void RenderBatch::add(IndexedRenderable* renderable) {
            IndexedRenderableWrapper* wrapper = new IndexedRenderableWrapper(m_renderContext, renderable);
            doAdd(wrapper);
            m_indexedRenderables.push_back(wrapper);
        }

        void RenderBatch::addOneShot(Renderable* renderable) {
            doAdd(renderable);
            m_oneshots.push_back(renderable);
        }

        void RenderBatch::addOneShot(DirectRenderable* renderable) {
            doAdd(renderable);
            m_directRenderables.push_back(renderable);
            m_oneshots.push_back(renderable);
        }

        void RenderBatch::addOneShot(IndexedRenderable* renderable) {
            IndexedRenderableWrapper* wrapper = new IndexedRenderableWrapper(m_renderContext, renderable);

            doAdd(wrapper);
            m_indexedRenderables.push_back(wrapper);
            m_oneshots.push_back(renderable);
        }

        void RenderBatch::render(RenderState& renderState) {
            prepareRenderables();
            renderRenderables(renderState);
        }

        void RenderBatch::doAdd(Renderable* renderable) {
            ensure(renderable != nullptr, "renderable is null");
            m_batch.push_back(renderable);
        }

        void RenderBatch::prepareRenderables() {
            for (DirectRenderable* renderable : m_directRenderables) {
                renderable->prepareVertices(m_renderContext);
            }
            for (IndexedRenderable* renderable : m_indexedRenderables) {
                renderable->prepareVerticesAndIndices(m_renderContext);
            }
        }

        void RenderBatch::renderRenderables(RenderState& renderState) {
            for (Renderable* renderable : m_batch)
                renderable->render(renderState);
        }
    }
}
