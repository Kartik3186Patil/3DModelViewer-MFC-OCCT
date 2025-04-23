#pragma once


#include <string>
#include <vector>
#include <TopoDS_Shape.hxx>

struct Part {
    std::string name;
    TopoDS_Shape shape;
    Quantity_Color color;
};

struct Assembly {
    std::string name;
    TopoDS_Shape shape;
    std::vector<Part> parts;
    std::vector<Assembly> subAssemblies;

};
