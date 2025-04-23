#pragma once

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_Drawer.hxx>
#include "Backend.h"
#include "MFCApp1Doc.h"


class CMFCApp1View : public CView
{
protected:
    CMFCApp1View() noexcept;
    DECLARE_DYNCREATE(CMFCApp1View)

private:
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(AIS_Shape) m_shapeHandle;
    Handle(Prs3d_Drawer) customStyle = new Prs3d_Drawer();
    Backend m_backend;
    CTreeCtrl m_modelTree;
    Handle(AIS_InteractiveObject) m_highlightedShape;
    Quantity_Color m_defaultColor; // Default color for shapes
    Quantity_Color m_highlightColor; // Color for highlighted shape
    std::vector<Handle(AIS_Shape)> m_displayedShapes;
    std::map<Handle(AIS_Shape), Quantity_Color> m_shapeToColorMap;//for colors

    Handle(AIS_InteractiveObject) m_hoverHighlightedShape;//hover Hilight
    Quantity_Color m_hoverHighlightColor;//hover color


public:
    void InitOCCT();
    void Redraw();
    void DisplayAssembly(const Assembly& assembly);

public:
    virtual void OnDraw(CDC* pDC);

protected:
    virtual void OnInitialUpdate();

public:
    virtual ~CMFCApp1View();
    void OnSize(UINT nType, int cx, int cy);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

#ifdef _DEBUG
#endif

protected:
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    void OnLButtonUp(UINT nFlags, CPoint point);    

    void OnMouseMove(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP();

public:
    afx_msg void OnLoadFile();
    void PopulateTreeRecursive(HTREEITEM parentItem, const Assembly& assembly);
    void DebugPrintAssembly(const Assembly& assembly, int indentLevel);
    std::map<HTREEITEM, TopoDS_Shape> m_treeItemToShapeMap;
    void CMFCApp1View::OnTvnSelchangedModelTree(NMHDR* pNMHDR, LRESULT* pResult);
    bool m_isMousePressed = false;
    

     bool m_isRotating;
     CPoint m_rotationStartPoint;

};