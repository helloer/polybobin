#include "glcanvas.hpp"
#include "../app.hpp"
#include "../constants.hpp"

GLCanvas::GLCanvas(wxWindow *parent, Settings settings, const wxGLAttributes &glCanvasAttributes, Map *map)
    : wxGLCanvas(parent, glCanvasAttributes, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW | wxEXPAND)
{
    wxGLContextAttrs glContextAttributes;
    glContextAttributes.PlatformDefaults().CoreProfile().EndList();

    m_glManager = new GLManager(settings, map);

    m_map = map;

    m_addedPolygonVerticesCount = 0;

    Bind(wxEVT_MOTION, &GLCanvas::OnMouseMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &GLCanvas::OnMouseWheel, this);
    Bind(wxEVT_PAINT, &GLCanvas::OnPaint, this);
    Bind(wxEVT_SIZE, &GLCanvas::OnResize, this);
}

GLCanvas::~GLCanvas()
{
    delete m_glManager;
}

void GLCanvas::SetDisplaySetting(int setting, bool display)
{
    m_displaySettings.SetDisplaySetting(setting, display);
    Refresh();
}

void GLCanvas::HandleClick(int selectedToolId)
{
    switch (selectedToolId)
    {
        case ID_TOOL_CREATE_POLYGON:
            {
                PMSVertex vertex = CreateVertexOnMouse();
                if (m_addedPolygonVerticesCount == 0)
                {
                    PMSVector perpendicular;
                    perpendicular.x = 0.0f;
                    perpendicular.y = 0.0f;
                    perpendicular.z = 2.0f;

                    PMSPolygon polygon;
                    for (unsigned int i = 0; i < 3; ++i)
                    {
                        polygon.vertices[i] = vertex;
                        polygon.perpendiculars[i] = perpendicular;
                    }
                    polygon.polygonType = m_newPolygonType;

                    m_map->AddPolygon(polygon);
                    m_glManager->AddPolygon(polygon.polygonType, vertex);
                }
                else
                {
                    unsigned int polygonIndex = m_map->GetPolygonsCount() - 1;
                    unsigned int vertexIndex = m_addedPolygonVerticesCount;
                    m_map->EditPolygonVertex(polygonIndex, vertexIndex, vertex);
                    m_glManager->EditPolygonVertex(polygonIndex, m_newPolygonType, vertexIndex, vertex);
                }

                ++m_addedPolygonVerticesCount;
                if (m_addedPolygonVerticesCount == 3)
                {
                    m_addedPolygonVerticesCount = 0;
                }

                Refresh();
            }
            break;

        case ID_TOOL_SELECTION:
            {
                wxVector<PMSPolygon> polygons = m_map->GetPolygons();
                wxVector<PMSScenery> scenery = m_map->GetSceneryInstances();

                bool addSelectionKeyPressed = wxGetKeyState(ADD_SELECTION_KEY),
                     removeSelectionKeyPressed = wxGetKeyState(REMOVE_SELECTION_KEY);
                bool skippedPolygon = false,
                     skippedScenery = false;
                unsigned int skippedPolygonId,
                             skippedSceneryId;
                unsigned int i;

                for (i = 0; i < polygons.size(); ++i)
                {
                    if (polygons.at(i).Contains((float) m_mousePositionOnMap.x,
                                                (float) m_mousePositionOnMap.y))
                    {
                        if (m_selectedPolygons.IsSelected(i))
                        {
                            if (removeSelectionKeyPressed)
                            {
                                m_selectedPolygons.Unselect(i);
                                Refresh();
                                return;
                            }
                            skippedPolygon = true;
                            skippedPolygonId = i;
                        }
                        else
                        {
                            if (!addSelectionKeyPressed && !removeSelectionKeyPressed)
                            {
                                m_selectedPolygons.UnselectAll();
                                m_selectedScenery.UnselectAll();
                            }

                            if (!removeSelectionKeyPressed)
                            {
                                m_selectedPolygons.Select(i);
                            }

                            Refresh();
                            return;
                        }
                    }
                }

                for (i = 0; i < scenery.size(); ++i)
                {
                    if (scenery.at(i).Contains((float) m_mousePositionOnMap.x,
                                               (float) m_mousePositionOnMap.y))
                    {
                        if (m_selectedScenery.IsSelected(i))
                        {
                            if (removeSelectionKeyPressed)
                            {
                                m_selectedScenery.Unselect(i);
                                Refresh();
                                return;
                            }
                            skippedScenery = true;
                            skippedSceneryId = i;
                        }
                        else
                        {
                            if (!addSelectionKeyPressed && !removeSelectionKeyPressed)
                            {
                                m_selectedPolygons.UnselectAll();
                                m_selectedScenery.UnselectAll();
                            }

                            if (!removeSelectionKeyPressed)
                            {
                                m_selectedScenery.Select(i);
                            }

                            Refresh();
                            return;
                        }
                    }
                }

                if (!addSelectionKeyPressed && !removeSelectionKeyPressed)
                {
                    m_selectedPolygons.UnselectAll();
                    m_selectedScenery.UnselectAll();
                }

                if (!skippedPolygon && skippedScenery && !addSelectionKeyPressed)
                {
                    m_selectedScenery.Select(skippedSceneryId);
                }

                if (skippedPolygon && !skippedScenery && !addSelectionKeyPressed)
                {
                    m_selectedPolygons.Select(skippedPolygonId);
                }
                Refresh();
            }
            break;
    }
}

void GLCanvas::HandleRightMouseButtonRelease(int selectedToolId)
{
    switch (selectedToolId)
    {
        case ID_TOOL_CREATE_POLYGON:
            wxMenu *newPolygonTypeSelection = new wxMenu();
            for (unsigned int i = 0; i < POLYGON_TYPES_COUNT; ++i)
            {
                newPolygonTypeSelection->Append(ID_POLYGON_TYPE_NORMAL + i, POLYGON_TYPES_NAMES[i]);
            }
            newPolygonTypeSelection->Bind(wxEVT_MENU, &GLCanvas::OnNewPolygonTypeSelected, this);
            wxWindow::PopupMenu(newPolygonTypeSelection);

            break;
    }
}

void GLCanvas::SelectAll()
{
    m_selectedPolygons.SelectAll(m_map->GetPolygons().size());
    m_selectedScenery.SelectAll(m_map->GetSceneryInstances().size());
    Refresh();
}

PMSVertex GLCanvas::CreateVertexOnMouse()
{
    float textureWidth = (float) m_glManager->GetTextureWidth(),
        textureHeight = (float) m_glManager->GetTextureHeight();

    PMSVertex vertex;
    vertex.x = m_mousePositionOnMap.x;
    vertex.y = m_mousePositionOnMap.y;
    vertex.z = 1.0f;
    vertex.rhw = 1.0f;
    // TODO: color should depend on current choice in color picker.
    vertex.color = PMSColor();
    vertex.textureS = (float) m_mousePositionOnMap.x / textureWidth;
    vertex.textureT = (float) m_mousePositionOnMap.y / textureHeight;

    return vertex;
}

void GLCanvas::OnMouseMotion(wxMouseEvent &event)
{
    wxPoint newMousePositionOnCanvas = event.GetPosition();

    if (event.MiddleIsDown() && event.Dragging())
    {
        wxPoint oldMousePositionOnMap = GetMousePositionOnMap(m_mousePositionOnCanvas);
        wxPoint newMousePositionOnMap = GetMousePositionOnMap(newMousePositionOnCanvas);

        m_camera.ScrollX((float) (oldMousePositionOnMap.x - newMousePositionOnMap.x));
        m_camera.ScrollY((float) (oldMousePositionOnMap.y - newMousePositionOnMap.y));

        Refresh();
    }

    m_mousePositionOnCanvas = newMousePositionOnCanvas;

    m_mousePositionOnMap = GetMousePositionOnMap(m_mousePositionOnCanvas);

    if (AddingPolygon())
    {
        unsigned int polygonIndex = m_map->GetPolygonsCount() - 1;
        PMSVertex vertex = CreateVertexOnMouse();

        // Update the positions of the vertices that haven't been set yet.
        for (unsigned int i = m_addedPolygonVerticesCount; i < 3; ++i)
        {
            m_glManager->EditPolygonVertex(polygonIndex, m_newPolygonType, i, vertex);
        }

        Refresh();
    }
}

void GLCanvas::OnMouseWheel(wxMouseEvent &event)
{
    int wheelRotation = event.GetWheelRotation();

    if (wheelRotation > 0 && m_camera.CanZoomIn())
    {
        m_camera.ZoomIn();
        Refresh();
    }
    else if (wheelRotation < 0 && m_camera.CanZoomOut())
    {
        m_camera.ZoomOut();
        Refresh();
    }
}

void GLCanvas::OnNewPolygonTypeSelected(wxCommandEvent &event)
{
    if (!AddingPolygon())
    {
        int selectedPolygonType = event.GetId() - ID_POLYGON_TYPE_NORMAL;
        m_newPolygonType = (PMSPolygonType) selectedPolygonType;
    }
}

void GLCanvas::OnPaint(wxPaintEvent &event)
{
    wxPaintDC(this);

    if (!m_glManager->IsGLReady())
    {
        InitGL();
    }

    wxGetApp().GetGLContext(this);

    m_glManager->Render(m_camera, this->GetSize(), m_displaySettings,
                        m_selectedPolygons, m_selectedScenery,
                        AddingPolygon());
    SwapBuffers();
}

void GLCanvas::OnResize(wxSizeEvent &event)
{
    event.Skip();

    if (!IsShownOnScreen())
    {
        return;
    }

    if (!m_glManager->IsGLReady())
    {
        InitGL();
    }

    wxSize canvasSize = this->GetSize();
    glViewport(0, 0, canvasSize.GetWidth(), canvasSize.GetHeight());

    Refresh(false);
}


wxPoint GLCanvas::GetMousePositionOnMap(wxPoint mousePositionOnCanvas)
{
    wxPoint result;
    wxSize canvasSize = GetSize();

    /**
     * Proportion from which we take the formula:
     * mousePositionOnCanvas.x : canvasSize.GetWidth() = result.x : m_camera.GetWidth()
     */

    result.x = (int) (m_camera.GetX() +
                     (float) mousePositionOnCanvas.x * m_camera.GetWidth(canvasSize) / (float) canvasSize.GetWidth());

    result.y = (int) (m_camera.GetY() +
                     (float) mousePositionOnCanvas.y * m_camera.GetHeight(canvasSize) / (float)canvasSize.GetHeight());

    return result;
}

void GLCanvas::InitGL()
{
    wxGetApp().GetGLContext(this);

    try
    {
        m_glManager->Init();
        m_glManager->SetupShaders();
        m_glManager->SetupTextures();
    }
    catch (wxString errorMessage)
    {
        // TODO: handle properly.
    }

    m_glManager->SetupVertices();
}
