#include "pch.h"
#include "MFCApp1.h"
#include "MFCApp1View.h"
#include "MFCApp1Doc.h"
#include <afxwin.h>
#include <afxdlgs.h>
#include <atlconv.h>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include "Backend.h"
#include <TopExp_Explorer.hxx>

IMPLEMENT_DYNCREATE(CMFCApp1View, CView)

BEGIN_MESSAGE_MAP(CMFCApp1View, CView)
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMFCApp1View::OnFilePrintPreview)
    ON_WM_CONTEXTMENU()
    ON_WM_RBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SIZE()
    ON_WM_CREATE()
    ON_NOTIFY(TVN_SELCHANGED, ID_LOAD_STEP, &CMFCApp1View::OnTvnSelchangedModelTree)
    ON_COMMAND(ID_LOAD_STEP, &CMFCApp1View::OnLoadFile)
END_MESSAGE_MAP()

CMFCApp1View::CMFCApp1View() noexcept
    : m_defaultColor(Quantity_NOC_BLACK), m_highlightColor(Quantity_NOC_RED), m_hoverHighlightColor(Quantity_NOC_YELLOW)
{
}

CMFCApp1View::~CMFCApp1View() {}

void CMFCApp1View::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    int treeWidth = cx / 4;
    if (::IsWindow(m_modelTree.GetSafeHwnd()))
    {
        m_modelTree.MoveWindow(0, 0, treeWidth, cy);
        m_modelTree.BringWindowToTop();
    }

    if (!m_view.IsNull())
    {
        m_view->MustBeResized();
    }
}

void CMFCApp1View::OnInitialUpdate()
{
 CView::OnInitialUpdate();
    InitOCCT();
}
void CMFCApp1View::InitOCCT()
{
    Handle(Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection();
    Handle(Graphic3d_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
    HWND hwnd = this->GetSafeHwnd();
    Handle(WNT_Window) aWindow = new WNT_Window(hwnd);
    Handle(V3d_Viewer) aViewer = new V3d_Viewer(aGraphicDriver);
    aViewer->SetDefaultLights();
    m_view = aViewer->CreateView();
    m_view->SetWindow(aWindow);
    m_view->SetBackgroundColor(Quantity_NOC_GRAY);
    m_view->MustBeResized();
    m_view->SetLightOn();
    m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.09, V3d_ZBUFFER);

    m_context = new AIS_InteractiveContext(aViewer);
    Redraw();
}
void CMFCApp1View::Redraw()
{
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}
void CMFCApp1View::OnDraw(CDC* pDC)
{
    CView::OnDraw(pDC);
    Redraw();
}
void CMFCApp1View::OnRButtonUp(UINT, CPoint point)
{
    ClientToScreen(&point);
    OnContextMenu(this, point);
}
void CMFCApp1View::OnContextMenu(CWnd*, CPoint point)
{
    theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}

int CMFCApp1View::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_modelTree.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
        CRect(0, 0, 250, 500), this, ID_LOAD_STEP))
    {
        AfxMessageBox(_T("Failed to create the model tree control"));
        return -1;
    }

    m_modelTree.BringWindowToTop();
    return 0;
}

void CMFCApp1View::OnLoadFile()
{
    CFileDialog dlg(TRUE, _T("step"), nullptr, OFN_FILEMUSTEXIST, _T("STEP Files (*.step;*.stp)|*.step;*.stp||"));

    if (dlg.DoModal() != IDOK)
        return;


    CString path = dlg.GetPathName();
    CT2A asciiPath(path);
    std::string filePath = asciiPath;

    if (!m_backend.LoadStepFile(filePath))
    {
        AfxMessageBox(_T("Failed to load STEP file."));
        return;
    }

    // Clear previous state
    m_modelTree.DeleteAllItems();
    m_treeItemToShapeMap.clear();
    m_shapeToColorMap.clear();
    m_context->RemoveAll(true);
    m_displayedShapes.clear();
    if (!m_highlightedShape.IsNull())
    {
        m_highlightedShape.Nullify();
    }
    if (!m_hoverHighlightedShape.IsNull())
    {
        m_hoverHighlightedShape.Nullify();
    }

    // Load and display
    Assembly rootAssembly = m_backend.BuildAssemblyHierarchy();
    DebugPrintAssembly(rootAssembly, 0);

    HTREEITEM hRoot = m_modelTree.InsertItem(CString(rootAssembly.name.c_str()));
    m_treeItemToShapeMap[hRoot] = rootAssembly.shape;

    PopulateTreeRecursive(hRoot, rootAssembly);
    m_modelTree.Expand(hRoot, TVE_EXPAND);

    DisplayAssembly(rootAssembly);
    m_view->FitAll();

    Redraw();
}
void CMFCApp1View::DisplayAssembly(const Assembly& assembly)
{
    CString logLine;
    logLine.Format(_T("Displaying Assembly: %s, %d sub-assemblies, %d parts\n"),
        CString(assembly.name.c_str()), assembly.subAssemblies.size(), assembly.parts.size());
    OutputDebugString(logLine);

    // Skip rendering assembly shape (placeholder for grouped assemblies)
    if (!assembly.shape.IsNull() && assembly.parts.empty() && !assembly.subAssemblies.empty())
    {
        logLine.Format(_T("Skipping assembly shape for: %s (has sub-assemblies)\n"),
            CString(assembly.name.c_str()));
        OutputDebugString(logLine);
    }

    for (const Assembly& subAsm : assembly.subAssemblies)
    {
        DisplayAssembly(subAsm);
    }

    for (const Part& part : assembly.parts)
    {
        if (!part.shape.IsNull())
        {
            Handle(AIS_Shape) aisShape = new AIS_Shape(part.shape);
            m_context->SetColor(aisShape, part.color, Standard_False);
            m_context->SetMaterial(aisShape, Graphic3d_NOM_DEFAULT, Standard_False);
            m_context->Display(aisShape, AIS_Shaded, 0, Standard_True);
            m_shapeToColorMap[aisShape] = part.color;
            logLine.Format(_T("Displayed part: %s, color: R=%.2f, G=%.2f, B=%.2f, shape hash: %d\n"),
                CString(part.name.c_str()), part.color.Red(), part.color.Green(), part.color.Blue(),
                part.shape.HashCode(INT_MAX));
            OutputDebugString(logLine);
        }
        else
        {
            logLine.Format(_T("Skipped part: %s, null shape\n"), CString(part.name.c_str()));
            OutputDebugString(logLine);
        }
    }
}
void CMFCApp1View::PopulateTreeRecursive(HTREEITEM parentItem, const Assembly& assembly)
{
    CString logLine;
    logLine.Format(_T("Populating Assembly: %s (Parent: 0x%p) with %d sub-assemblies, %d parts\n"),
        CString(assembly.name.c_str()), parentItem, assembly.subAssemblies.size(), assembly.parts.size());
    OutputDebugString(logLine);

    for (const Assembly& subAsm : assembly.subAssemblies)
    {
        HTREEITEM hAsmItem = m_modelTree.InsertItem(CString(subAsm.name.c_str()), parentItem);
        m_treeItemToShapeMap[hAsmItem] = subAsm.shape;

        logLine.Format(_T("Inserted Assembly: %s (Item: 0x%p, Parent: 0x%p)\n"),
            CString(subAsm.name.c_str()), hAsmItem, parentItem);
        OutputDebugString(logLine);

        PopulateTreeRecursive(hAsmItem, subAsm);
    }

    for (const Part& part : assembly.parts)
    {
        HTREEITEM hPartItem = m_modelTree.InsertItem(CString(part.name.c_str()), parentItem);
        m_treeItemToShapeMap[hPartItem] = part.shape;

        logLine.Format(_T("Inserted Part: %s (Item: 0x%p, Parent: 0x%p)\n"),
            CString(part.name.c_str()), hPartItem, parentItem);
        OutputDebugString(logLine);
    }
}
void CMFCApp1View::DebugPrintAssembly(const Assembly& assembly, int indentLevel)
{
    CString indent(_T(' '), indentLevel * 2);
    CString asmLine;
    asmLine.Format(_T("%sAssembly: %s (%d sub-assemblies, %d parts)\n"),
        indent, CString(assembly.name.c_str()),
        assembly.subAssemblies.size(), assembly.parts.size());
    OutputDebugString(asmLine);

    for (const Part& part : assembly.parts)
    {
        CString partLine;
        partLine.Format(_T("%s  Part: %s\n"), indent, CString(part.name.c_str()));
        OutputDebugString(partLine);
    }

    for (const Assembly& subAsm : assembly.subAssemblies)
    {
        DebugPrintAssembly(subAsm, indentLevel + 1);
    }
}
void CMFCApp1View::OnLButtonDown(UINT nFlags, CPoint point)
{
    CView::OnLButtonDown(nFlags, point);

    CString debugMsg;

    // Clear any previous selection
    m_context->ClearSelected(false);
    m_context->MoveTo(point.x, point.y, m_view, true);
    m_context->SelectDetected(AIS_SelectionScheme_Replace);

    debugMsg.Format(_T("NbSelected: %d\n"), m_context->NbSelected());
    OutputDebugString(debugMsg);

    // Restore the original color of the previously highlighted shape
    if (!m_highlightedShape.IsNull())
    {
        auto it = m_shapeToColorMap.find(Handle(AIS_Shape)::DownCast(m_highlightedShape));
        if (it != m_shapeToColorMap.end())
        {
            m_context->SetColor(m_highlightedShape, it->second, false); // Restore original color
        }
        else
        {
            m_context->SetColor(m_highlightedShape, m_defaultColor, false); // Fallback to gray
        }
        m_context->Redisplay(m_highlightedShape, true);
        m_highlightedShape.Nullify();
    }

    Handle(AIS_InteractiveObject) selectedShape;
    m_context->InitSelected();

    if (m_context->HasSelectedShape())
    {
        selectedShape = m_context->SelectedInteractive();
        OutputDebugString(_T("Shape selected\n"));

        // Highlight the newly selected shape
        m_highlightedShape = selectedShape;
        m_context->SetColor(m_highlightedShape, m_highlightColor, false);
        m_context->Redisplay(m_highlightedShape, true);
        OutputDebugString(_T("Highlighted shape\n"));

        // Match with TopoDS_Shape and update model tree
        TopoDS_Shape selectedTopoShape;
        if (m_highlightedShape->IsKind(STANDARD_TYPE(AIS_Shape)))
        {
            selectedTopoShape = Handle(AIS_Shape)::DownCast(m_highlightedShape)->Shape();
            OutputDebugString(_T("Selected AIS_Shape\n"));
            debugMsg.Format(_T("Selected shape hash: %d\n"), selectedTopoShape.HashCode(INT_MAX));
            OutputDebugString(debugMsg);

            for (const auto& pair : m_treeItemToShapeMap)
            {
                // Reset all tree items to normal state
                m_modelTree.SetItemState(pair.first, 0, TVIS_BOLD);

                if (!selectedTopoShape.IsNull() && pair.second.IsEqual(selectedTopoShape))
                {
                    m_modelTree.SelectItem(pair.first);
                    m_modelTree.SetItemState(pair.first, TVIS_BOLD, TVIS_BOLD);
                    OutputDebugString(_T("Tree node selected\n"));
                }
            }
        }
    }
    else
    {
        OutputDebugString(_T("No shape selected\n"));
    }
   

    m_isRotating = true;
    m_rotationStartPoint = point;

    Redraw();
}
void CMFCApp1View::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_isRotating = false;
    OutputDebugString(_T("Rotation stopped\n"));
    CView::OnLButtonUp(nFlags, point);
}
void CMFCApp1View::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_isRotating && !m_view.IsNull())
    {
        // Handle rotation (unchanged)
        Standard_Real dx = point.x - m_rotationStartPoint.x;
        Standard_Real dy = point.y - m_rotationStartPoint.y;
        Standard_Real angleScale = 0.005;
        Standard_Real rotX = dy * angleScale;
        Standard_Real rotY = dx * angleScale;
        m_view->Rotate(rotY, rotX, 0.0);
        m_rotationStartPoint = point;
        Redraw();

        CString debugMsg;
        debugMsg.Format(_T("Rotating: dx=%.2f, dy=%.2f, rotX=%.4f, rotY=%.4f\n"),
            dx, dy, rotX, rotY);
        OutputDebugString(debugMsg);
    }
    
    Redraw();
    CView::OnMouseMove(nFlags, point);
}
void CMFCApp1View::OnTvnSelchangedModelTree(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    HTREEITEM selectedItem = m_modelTree.GetSelectedItem();
    *pResult = 0;

    if (!selectedItem)
        return;

    auto it = m_treeItemToShapeMap.find(selectedItem);
    if (it == m_treeItemToShapeMap.end())
        return;

    const TopoDS_Shape& selectedShape = it->second;

    // Restore original color of previous highlight
    if (!m_highlightedShape.IsNull())
    {
        auto colorIt = m_shapeToColorMap.find(Handle(AIS_Shape)::DownCast(m_highlightedShape));
        if (colorIt != m_shapeToColorMap.end())
        {
            m_context->SetColor(m_highlightedShape, colorIt->second, false); // Restore original color
        }
        else
        {
            m_context->SetColor(m_highlightedShape, m_defaultColor, false); // Fallback to gray
        }
        m_context->Redisplay(m_highlightedShape, true);
        m_highlightedShape.Nullify();
    }

    // Get displayed objects
    AIS_ListOfInteractive displayed;
    m_context->DisplayedObjects(displayed);

    for (AIS_ListOfInteractive::Iterator itObj(displayed); itObj.More(); itObj.Next())
    {
        Handle(AIS_InteractiveObject) aisObj = itObj.Value();
        if (!aisObj.IsNull() && aisObj->IsKind(STANDARD_TYPE(AIS_Shape)))
        {
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObj);
            if (!aisShape.IsNull() && aisShape->Shape().IsEqual(selectedShape))
            {
                m_highlightedShape = aisShape;
                m_context->ClearSelected(false);
                m_context->AddOrRemoveSelected(aisShape, false);
                m_context->SetColor(aisShape, m_highlightColor, false);
                m_context->Redisplay(aisShape, true);
                break;
            }
        }
    }

    Redraw();
}
