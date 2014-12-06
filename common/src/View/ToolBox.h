/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#ifndef __TrenchBroom__ToolBox__
#define __TrenchBroom__ToolBox__

#include "TrenchBroom.h"
#include "VecMath.h"
#include "StringUtils.h"
#include "Hit.h"
#include "Notifier.h"

#include <wx/wx.h>

#include <map>
#include <vector>

namespace TrenchBroom {
    namespace Renderer {
        class RenderBatch;
        class RenderContext;
    }
    
    namespace View {
        class InputState;
        class Tool;
        class ToolChain;
        
        class ToolBox {
        private:
            Tool* m_dragReceiver;
            Tool* m_modalReceiver;
            Tool* m_dropReceiver;
            Tool* m_savedDropReceiver;
            
            typedef std::vector<Tool*> ToolList;
            typedef std::map<Tool*, ToolList> ToolMap;
            ToolMap m_deactivateWhen;
            
            bool m_clickToActivate;
            bool m_ignoreNextClick;
            wxDateTime m_lastActivation;
            
            bool m_enabled;
        public:
            Notifier1<Tool*> toolActivatedNotifier;
            Notifier1<Tool*> toolDeactivatedNotifier;
        public:
            ToolBox();
        public: // picking
            void pick(ToolChain* chain, const InputState& inputState, Hits& hits);
        public: // event handling
            bool clickToActivate() const;
            void setClickToActivate(bool clickToActivate);
            void updateLastActivation();
            
            bool ignoreNextClick() const;
            void setIgnoreNextClick();
            void clearIgnoreNextClick();
            void clearIgnoreNextClickWithinActivationTime();
            
            bool dragEnter(ToolChain* chain, const InputState& inputState, const String& text);
            bool dragMove(ToolChain* chain, const InputState& inputState, const String& text);
            void dragLeave(ToolChain* chain, const InputState& inputState);
            bool dragDrop(ToolChain* chain, const InputState& inputState, const String& text);

            void modifierKeyChange(ToolChain* chain, const InputState& inputState);
            void mouseDown(ToolChain* chain, const InputState& inputState);
            bool mouseUp(ToolChain* chain, const InputState& inputState);
            void mouseDoubleClick(ToolChain* chain, const InputState& inputState);
            void mouseMove(ToolChain* chain, const InputState& inputState);
            
            bool dragging() const;
            bool startMouseDrag(ToolChain* chain, const InputState& inputState);
            bool mouseDrag(const InputState& inputState);
            void endMouseDrag(const InputState& inputState);
            void cancelDrag();
            
            void mouseWheel(ToolChain* chain, const InputState& inputState);

            bool cancel(ToolChain* chain);
        public: // tool management
            void deactivateWhen(Tool* master, Tool* slave);
            
            bool anyToolActive() const;
            bool toolActive(const Tool* tool) const;
            void toggleTool(Tool* tool);
            void deactivateAllTools();

            bool enabled() const;
            void enable();
            void disable();
        public: // rendering
            void setRenderOptions(ToolChain* chain, const InputState& inputState, Renderer::RenderContext& renderContext);
            void renderTools(ToolChain* chain, const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch);
        private:
            bool activateTool(Tool* tool);
            void deactivateTool(Tool* tool);
        };
    }
}

#endif /* defined(__TrenchBroom__ToolBox__) */
