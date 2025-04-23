#include "pch.h"
#include "Backend.h"
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Tool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <TDataStd_Name.hxx>
#include <Quantity_Color.hxx>
#include <TCollection_AsciiString.hxx>
#include <BRep_Tool.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopoDS_Iterator.hxx>
#include <TDF_Label.hxx>
#include <XCAFApp_Application.hxx>
#include <windows.h>

Backend::Backend()
    : m_doc(nullptr), m_shapeTool(nullptr), m_colorTool(nullptr)
{
}

bool Backend::LoadStepFile(const std::string& filePath)
{
    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    app->NewDocument("MDTV-XCAF", m_doc);
    if (m_doc.IsNull())
    {
        OutputDebugString(_T("Failed to create XCAF document.\n"));
        return false;
    }

    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(filePath.c_str());
    if (status != IFSelect_RetDone)
    {
        OutputDebugString(_T("Failed to load STEP file.\n"));
        return false;
    }

    if (!reader.Transfer(m_doc))
    {
        OutputDebugString(_T("Failed to transfer STEP data to XCAF document.\n"));
        return false;
    }

    m_shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_doc->Main());
    m_colorTool = XCAFDoc_DocumentTool::ColorTool(m_doc->Main());
    if (m_shapeTool.IsNull() || m_colorTool.IsNull())
    {
        OutputDebugString(_T("Failed to initialize shape or color tool.\n"));
        return false;
    }

    TDF_LabelSequence topLabels;
    m_shapeTool->GetFreeShapes(topLabels);
    if (!topLabels.IsEmpty())
    {
        m_shape = m_shapeTool->GetShape(topLabels.Value(1));
    }

    OutputDebugString(_T("STEP file loaded successfully.\n"));
    return true;
}

const TopoDS_Shape& Backend::GetLoadedShape() const
{
    return m_shape;
}

Assembly Backend::BuildAssemblyHierarchy()
{
    if (m_doc.IsNull() || m_shapeTool.IsNull())
    {
        OutputDebugString(_T("Document or ShapeTool is null, cannot build assembly hierarchy.\n"));
        return Assembly{};
    }

    TDF_LabelSequence topLabels;
    m_shapeTool->GetFreeShapes(topLabels);
    if (topLabels.IsEmpty())
    {
        OutputDebugString(_T("No top-level shapes found.\n"));
        return Assembly{};
    }

    Assembly rootAssembly;
    rootAssembly.name = "RootAssembly";
    CString logLine;
    logLine.Format(_T("Building hierarchy for RootAssembly, %d top-level labels\n"), topLabels.Length());
    OutputDebugString(logLine);

    for (Standard_Integer i = 1; i <= topLabels.Length(); ++i)
    {
        TDF_Label label = topLabels.Value(i);
        TopoDS_Shape shape = m_shapeTool->GetShape(label);
        TDF_LabelSequence subChildren;
        m_shapeTool->GetComponents(label, subChildren);

        std::string labelName = "Unnamed";
        Handle(TDataStd_Name) nameAttr;
        if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr))
        {
            TCollection_AsciiString asciiName = nameAttr->Get();
            labelName = asciiName.ToCString();
        }

        if (!subChildren.IsEmpty())
        {
            // Assembly with components
            Assembly childAsm;
            childAsm.name = labelName;
            childAsm.shape = shape;
            TraverseLabel(label, childAsm);
            rootAssembly.subAssemblies.push_back(childAsm);
            logLine.Format(_T("Added top-level sub-assembly: %s, %d parts, %d sub-assemblies\n"),
                CString(labelName.c_str()), childAsm.parts.size(), childAsm.subAssemblies.size());
            OutputDebugString(logLine);
        }
        else if (!shape.IsNull())
        {
            // Top-level part
            Part part;
            part.name = labelName;
            part.shape = shape;
            part.color = GetShapeColor(label);
            rootAssembly.parts.push_back(part);
            logLine.Format(_T("Added top-level part: %s, shape hash: %d, color: R=%.2f, G=%.2f, B=%.2f\n"),
                CString(labelName.c_str()), shape.HashCode(INT_MAX),
                part.color.Red(), part.color.Green(), part.color.Blue());
            OutputDebugString(logLine);
        }
        else
        {
            //logLine.Format(_T("Skipped top-level label: %s, no shape or components\n"), CString(labelName.c_str()));
            OutputDebugString(logLine);
        }

        logLine.Format(_T("Processed top-level: %s, isAssembly: %d\n"),
            CString(labelName.c_str()), !subChildren.IsEmpty());
        OutputDebugString(logLine);
    }

    logLine.Format(_T("RootAssembly complete: %d parts, %d sub-assemblies\n"),
        rootAssembly.parts.size(), rootAssembly.subAssemblies.size());
    OutputDebugString(logLine);
    return rootAssembly;
}

void Backend::TraverseLabel(const TDF_Label& label, Assembly& assembly)
{
    CString logLine;
    logLine.Format(_T("Traversing label: %d\n"), label.Tag());
    OutputDebugString(logLine);

    Handle(TDataStd_Name) nameAttr;
    if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr))
    {
        TCollection_AsciiString asciiName = nameAttr->Get();
        assembly.name = asciiName.ToCString();
        //logLine.Format(_T("Label name: %s\n"), CString(assembly.name.c_str()));
        OutputDebugString(logLine);
    }
    else
    {
        assembly.name = "Unnamed";
        OutputDebugString(_T("Label has no name, set to Unnamed\n"));
    }

    TopoDS_Shape shape = m_shapeTool->GetShape(label);
    if (!shape.IsNull())
    {
        assembly.shape = shape;
        //logLine.Format(_T("Assigned shape to: %s, hash: %d\n"), CString(assembly.name.c_str()), shape.HashCode(INT_MAX));
        OutputDebugString(logLine);
    }
    else
    {
        //logLine.Format(_T("Label has no shape for: %s\n"), CString(assembly.name.c_str()));
        OutputDebugString(logLine);
    }

    bool isAssembly = m_shapeTool->IsAssembly(label);
    //logLine.Format(_T("Label %s isAssembly: %s\n"), CString(assembly.name.c_str()), isAssembly ? _T("true") : _T("false"));
    OutputDebugString(logLine);

    TDF_LabelSequence children;
    m_shapeTool->GetComponents(label, children);
    //logLine.Format(_T("Number of children for %s: %d\n"), CString(assembly.name.c_str()), children.Length());
    OutputDebugString(logLine);

    std::vector<Part> tempParts;
    std::map<std::string, std::vector<Part>> partGroups;

    for (Standard_Integer i = 1; i <= children.Length(); ++i)
    {
        TDF_Label child = children.Value(i);
        TopoDS_Shape childShape = m_shapeTool->GetShape(child);

        std::string childName = "Unnamed";
        Handle(TDataStd_Name) childNameAttr;
        if (child.FindAttribute(TDataStd_Name::GetID(), childNameAttr))
        {
            TCollection_AsciiString asciiName(childNameAttr->Get());
            childName = asciiName.ToCString();
        }

        bool childIsAssembly = m_shapeTool->IsAssembly(child);
        bool childIsReference = m_shapeTool->IsReference(child);



        TDF_LabelSequence subChildren;
        m_shapeTool->GetComponents(child, subChildren);
        logLine.Format(_T("Child: %s, isAssembly: %s, isReference: %s, has %d sub-components\n"),
            CString(childName.c_str()), childIsAssembly ? _T("true") : _T("false"),
            childIsReference ? _T("true") : _T("false"), subChildren.Length());
        OutputDebugString(logLine);

        if (childIsAssembly || !subChildren.IsEmpty())
        {
            Assembly subAsm;
            subAsm.name = childName;
            subAsm.shape = childShape;
            TraverseLabel(child, subAsm);
            assembly.subAssemblies.push_back(subAsm);
            logLine.Format(_T("Added sub-assembly: %s to %s, now has %d sub-assemblies\n"),
                CString(childName.c_str()), CString(assembly.name.c_str()), assembly.subAssemblies.size());
            OutputDebugString(logLine);
        }
        else if (childIsReference)
        {
            TDF_Label refLabel = child;
            bool isSubAssembly = false;
            int refDepth = 0;
            const int maxRefDepth = 5;

            while (childIsReference && refDepth < maxRefDepth)
            {
                if (m_shapeTool->GetReferredShape(refLabel, refLabel))
                {
                    bool refIsAssembly = m_shapeTool->IsAssembly(refLabel);
                    TDF_LabelSequence refChildren;
                    m_shapeTool->GetComponents(refLabel, refChildren);
                    logLine.Format(_T("Reference child: %s, depth: %d, referred label isAssembly: %s, has %d sub-components\n"),
                        CString(childName.c_str()), refDepth, refIsAssembly ? _T("true") : _T("false"), refChildren.Length());
                    OutputDebugString(logLine);

                    if (refIsAssembly || !refChildren.IsEmpty())
                    {
                        Assembly subAsm;
                        subAsm.name = childName;
                        subAsm.shape = childShape;
                        logLine.Format(_T("Creating sub-assembly (via reference): %s\n"), CString(childName.c_str()));
                        OutputDebugString(logLine);
                        TraverseLabel(refLabel, subAsm);
                        assembly.subAssemblies.push_back(subAsm);
                        logLine.Format(_T("Added sub-assembly (via reference): %s to %s, now has %d sub-assemblies\n"),
                            CString(childName.c_str()), CString(assembly.name.c_str()), assembly.subAssemblies.size());
                        OutputDebugString(logLine);
                        isSubAssembly = true;
                        break;
                    }
                    childIsReference = m_shapeTool->IsReference(refLabel);
                    refDepth++;
                }
                else
                {
                    logLine.Format(_T("Skipped reference child: %s, no referred label\n"), CString(childName.c_str()));
                    OutputDebugString(logLine);
                    break;
                }
            }

            if (!isSubAssembly && !childShape.IsNull())
            {
                Part part;
                part.name = childName;
                part.shape = childShape;
                part.color = GetShapeColor(child);
                tempParts.push_back(part);
                logLine.Format(_T("Added part (via reference): %s, shape hash: %d, color: R=%.2f, G=%.2f, B=%.2f to tempParts\n"),
                    CString(childName.c_str()), childShape.HashCode(INT_MAX),
                    part.color.Red(), part.color.Green(), part.color.Blue());
                OutputDebugString(logLine);
            }
            else if (!isSubAssembly)
            {
                logLine.Format(_T("Skipped reference child: %s, no shape\n"), CString(childName.c_str()));
                OutputDebugString(logLine);
            }
        }
        else if (!childShape.IsNull())
        {
            Part part;
            part.name = childName;
            part.shape = childShape;
            part.color = GetShapeColor(child);
            tempParts.push_back(part);
            logLine.Format(_T("Added part: %s, shape hash: %d, color: R=%.2f, G=%.2f, B=%.2f to tempParts\n"),
                CString(childName.c_str()), childShape.HashCode(INT_MAX),
                part.color.Red(), part.color.Green(), part.color.Blue());
            OutputDebugString(logLine);
        }
        else
        {
            logLine.Format(_T("Skipped child: %s, no shape or sub-components\n"), CString(childName.c_str()));
            OutputDebugString(logLine);
        }
    }

    // Group parts by base name
    for (const Part& part : tempParts)
    {
        std::string baseName = part.name;
        size_t colonPos = baseName.rfind(':');
        if (colonPos != std::string::npos)
        {
            baseName = baseName.substr(0, colonPos);
        }
        partGroups[baseName].push_back(part);
        logLine.Format(_T("Grouped part: %s under baseName: %s\n"),
            CString(part.name.c_str()), CString(baseName.c_str()));
        OutputDebugString(logLine);
    }

    // Create sub-assemblies for groups with multiple parts
    for (auto& group : partGroups)
    {
        const std::string& baseName = group.first;
        std::vector<Part>& parts = group.second;
        if (parts.size() > 1)
        {
            Assembly subAsm;
            subAsm.name = baseName + "_Assembly";
            subAsm.shape = parts[0].shape; // Placeholder shape
            subAsm.parts = std::move(parts);
            assembly.subAssemblies.push_back(subAsm);
            logLine.Format(_T("Created sub-assembly: %s with %d parts\n"),
                CString(subAsm.name.c_str()), subAsm.parts.size());
            OutputDebugString(logLine);
        }
        else
        {
            assembly.parts.push_back(parts[0]);
            logLine.Format(_T("Added single part: %s to %s\n"),
                CString(parts[0].name.c_str()), CString(assembly.name.c_str()));
            OutputDebugString(logLine);
        }
    }

    logLine.Format(_T("Finished traversing: %s, %d parts, %d sub-assemblies\n"),
        CString(assembly.name.c_str()), assembly.parts.size(), assembly.subAssemblies.size());
    OutputDebugString(logLine);
}
Quantity_Color Backend::GetShapeColor(const TDF_Label& label)
{
    Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(label);
    Quantity_Color color;
    CString logLine;

    // Check direct color on label
    if (colorTool->GetColor(label, XCAFDoc_ColorGen, color) ||
        colorTool->GetColor(label, XCAFDoc_ColorSurf, color) ||
        colorTool->GetColor(label, XCAFDoc_ColorCurv, color))
    {
        logLine.Format(_T("Direct color found: R=%.2f, G=%.2f, B=%.2f\n"),
            color.Red(), color.Green(), color.Blue());
        OutputDebugString(logLine);
        return color;
    }

    // Check colors linked to the shape
    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(label);
    TopoDS_Shape shape = shapeTool->GetShape(label);
    if (!shape.IsNull())
    {
        // Check color directly associated with the shape
        TDF_Label shapeLabel;
        if (shapeTool->FindShape(shape, shapeLabel))
        {
            if (colorTool->GetColor(shapeLabel, XCAFDoc_ColorGen, color) ||
                colorTool->GetColor(shapeLabel, XCAFDoc_ColorSurf, color) ||
                colorTool->GetColor(shapeLabel, XCAFDoc_ColorCurv, color))
            {
                logLine.Format(_T("Shape-linked color found: R=%.2f, G=%.2f, B=%.2f\n"),
                    color.Red(), color.Green(), color.Blue());
                OutputDebugString(logLine);
                return color;
            }
        }

        // Check sub-shapes (e.g., faces, edges)
        TDF_LabelSequence subShapes;
        shapeTool->GetSubShapes(label, subShapes);
        for (Standard_Integer i = 1; i <= subShapes.Length(); ++i)
        {
            const TDF_Label& subLabel = subShapes.Value(i);
            if (colorTool->GetColor(subLabel, XCAFDoc_ColorGen, color) ||
                colorTool->GetColor(subLabel, XCAFDoc_ColorSurf, color) ||
                colorTool->GetColor(subLabel, XCAFDoc_ColorCurv, color))
            {
                logLine.Format(_T("Sub-shape color found: R=%.2f, G=%.2f, B=%.2f\n"),
                    color.Red(), color.Green(), color.Blue());
                OutputDebugString(logLine);
                return color;
            }
        }
    }

    // Check all color labels in the document
    TDF_LabelSequence colorLabels;
    colorTool->GetColors(colorLabels);
    for (Standard_Integer i = 1; i <= colorLabels.Length(); ++i)
    {
        TDF_Label colorLabel = colorLabels.Value(i);
        if (!shape.IsNull() && (colorTool->IsSet(shape, XCAFDoc_ColorGen) ||
            colorTool->IsSet(shape, XCAFDoc_ColorSurf) ||
            colorTool->IsSet(shape, XCAFDoc_ColorCurv)))
        {
            if (colorTool->GetColor(colorLabel, XCAFDoc_ColorGen, color) ||
                colorTool->GetColor(colorLabel, XCAFDoc_ColorSurf, color) ||
                colorTool->GetColor(colorLabel, XCAFDoc_ColorCurv, color))
            {
                logLine.Format(_T("Document color linked to shape: R=%.2f, G=%.2f, B=%.2f\n"),
                    color.Red(), color.Green(), color.Blue());
                OutputDebugString(logLine);
                return color;
            }
        }
    }

    // Default color
    OutputDebugString(_T("No color found for label, using default gray\n"));
    return Quantity_NOC_GRAY38;
}