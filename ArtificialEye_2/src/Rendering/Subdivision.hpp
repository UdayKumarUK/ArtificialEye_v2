#pragma once

#include "Modeling/Mesh.hpp"

namespace ee
{
    void catmullClarkSubdiv(const VertBuffer& oVertices, const MeshFaceBuffer& oFaces, VertBuffer& nVertices, MeshFaceBuffer& nFaces);
}