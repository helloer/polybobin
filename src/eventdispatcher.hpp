#pragma once
#include <vector>
#include <memory>
#include "tools/tool.hpp"

using ToolSet = std::vector<std::unique_ptr<Tool>>;

class EventDispatcher
{
public:
    EventDispatcher(ToolSet tools);
    void Select(int toolId);
    void Unselect();
    void OnCanvasLeftMouseButtonClick(const wxMouseEvent &event);
    void OnCanvasMouseMotion(const wxMouseEvent &event);
    void OnCanvasRightMouseButtonRelease(const wxMouseEvent &event);
    ~EventDispatcher();
    
private:
    ToolSet tools;
    int selectedToolId;
};