#pragma once
#include <afxstr.h>
#include <AIS_InteractiveContext.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <vector>
#include <string>
#include <map>
#include <XCAFApp_Application.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include "ModelStructure.h"

class Backend
{
public:
    Backend();
    bool LoadStepFile(const std::string& filePath);
    const TopoDS_Shape& GetLoadedShape() const;
    Assembly BuildAssemblyHierarchy();
    void TraverseLabel(const TDF_Label& label, Assembly& assembly);
    Quantity_Color GetShapeColor(const TDF_Label& label);

private:
    Handle(AIS_InteractiveContext) m_context;
    Handle(TDocStd_Document) m_doc;
    Handle(XCAFDoc_ShapeTool) m_shapeTool;
    Handle(XCAFDoc_ColorTool) m_colorTool;
    TopoDS_Shape m_shape;
    Assembly m_rootAssembly;
};