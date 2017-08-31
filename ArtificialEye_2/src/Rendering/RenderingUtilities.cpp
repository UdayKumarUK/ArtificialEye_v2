#include "RenderingUtilities.hpp"

#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace cube
{
    const ee::VertBuffer VERTICES =
    {
        // front
        ee::Vertex(ee::Vector3(-1.f, -1.f,  1.f)),
        ee::Vertex(ee::Vector3( 1.f, -1.f,  1.f)),
        ee::Vertex(ee::Vector3( 1.0,  1.0,  1.0)),
        ee::Vertex(ee::Vector3(-1.0,  1.0,  1.0)),
        // back
        ee::Vertex(ee::Vector3(-1.0, -1.0, -1.0)),
        ee::Vertex(ee::Vector3( 1.0, -1.0, -1.0)),
        ee::Vertex(ee::Vector3( 1.0,  1.0, -1.0)),
        ee::Vertex(ee::Vector3(-1.0,  1.0, -1.0)),
    };

    const ee::IndexBuffer INDICES =
    {
        // -- winding is counter-clockwise (facing camera)
        {0, 1, 2},                // pos z
        {0, 2, 3},
        {1, 5, 6},                // pos x
        {1, 6, 2},
        {2, 6, 7},                // pos y
        {2, 7, 3},
        // -- winding is clockwise (facing away from camera)
        {3, 4, 0},                // neg x
        {3, 7, 4},
        {4, 5, 1},                // neg y
        {4, 1, 0},
        {5, 7, 6},                // neg z
        {5, 4, 7},
    };
}

// Based on information from: https://schneide.wordpress.com/2016/07/15/generating-an-icosphere-in-c/
namespace icosphere
{
    const float t = (1.f + std::sqrtf(5.f)) / 2.f;

    const ee::VertBuffer VERTICES =
    {
        ee::Vertex(glm::normalize(ee::Vector3(-1.f,    t,  0.f))),
        ee::Vertex(glm::normalize(ee::Vector3( 1.f,    t,  0.f))),
        ee::Vertex(glm::normalize(ee::Vector3(-1.f,   -t,  0.f))),
        ee::Vertex(glm::normalize(ee::Vector3( 1.f,   -t,  0.f))),

        ee::Vertex(glm::normalize(ee::Vector3( 0.f, -1.f,    t))),
        ee::Vertex(glm::normalize(ee::Vector3( 0.f,  1.f,    t))),
        ee::Vertex(glm::normalize(ee::Vector3( 0.f, -1.f,   -t))),
        ee::Vertex(glm::normalize(ee::Vector3( 0.f,  1.f,   -t))),

        ee::Vertex(glm::normalize(ee::Vector3(   t,  0.f, -1.f))),
        ee::Vertex(glm::normalize(ee::Vector3(   t,  0.f,  1.f))),
        ee::Vertex(glm::normalize(ee::Vector3(  -t,  0.f, -1.f))),
        ee::Vertex(glm::normalize(ee::Vector3(  -t,  0.f,  1.f))),
    };

    const std::vector<ee::MeshFace> INDICES =
    {
        { 0, 11,  5},
        { 0,  5,  1},
        { 0,  1,  7},
        { 0,  7, 10},
        { 0, 10, 11},

        { 1,  5,  9},
        { 5, 11,  4},
        {11, 10,  2},
        {10,  7,  6},
        { 7,  1,  8},

        { 3,  9,  4},
        { 3,  4,  2},
        { 3,  2,  6},
        { 3,  6,  8},
        { 3,  8,  9},

        { 4,  9,  5},
        { 2,  4, 11},
        { 6,  2, 10},
        { 8,  6,  7},
        { 9,  8,  1},
    };

    ////////////////////
    /// Helper Function:
    ////////////////////

    std::unordered_map<uint64_t, GLuint> g_cachedMiddlePoints;
    GLuint getMiddlePoint(GLuint i0, GLuint i1, ee::VertBuffer* list)
    {
        uint64_t minInd = std::min(i0, i1);
        uint64_t maxInd = std::max(i0, i1);
        uint64_t key = (minInd << 32) + maxInd;

        auto it = g_cachedMiddlePoints.find(key);
        if (it != g_cachedMiddlePoints.end())
        {
            return it->second;
        }

        ee::Vector3 p0 = (*list)[i0].m_position;
        ee::Vector3 p1 = (*list)[i1].m_position;
        ee::Vector3 m = glm::normalize((p0 + p1) * 0.5f);

        list->push_back(m);
        g_cachedMiddlePoints.insert(std::make_pair(key, list->size() - 1));
        return list->size() - 1;
    }
}

void ee::loadIndexedRectangle(VertBuffer* const vertList, IndexBuffer* const indexList)
{
    vertList->clear();
    indexList->clear();

    vertList->push_back(Vertex(Vector3(0.5f, 0.5f, 0.f)));
    vertList->push_back(Vertex(Vector3(0.5f, -0.5f, 0.f)));
    vertList->push_back(Vertex(Vector3(-0.5f, -0.5f, 0.f)));
    vertList->push_back(Vertex(Vector3(-0.5f, 0.5f, 0.0f)));

    indexList->push_back({0, 1, 3});
    indexList->push_back({1, 2, 3});
}

 void ee::loadIndexedCube(VertBuffer* vertList, IndexBuffer* indexList)
 {
     *vertList = cube::VERTICES;
     *indexList = cube::INDICES;
 }

 void ee::loadIcosphere(unsigned recursionLevel, VertBuffer* vertList, IndexBuffer* indexList)
 {
     icosphere::g_cachedMiddlePoints.clear();
     *vertList = icosphere::VERTICES;

     IndexBuffer tempIndList0 = icosphere::INDICES;

     for (unsigned i = 0; i < recursionLevel; i++)
     {
         IndexBuffer tempIndList1;
         for (const MeshFace& face : tempIndList0)
         {
             const GLuint* indices = face.m_indices;

             GLuint i0 = icosphere::getMiddlePoint(indices[0], indices[1], vertList);
             GLuint i1 = icosphere::getMiddlePoint(indices[1], indices[2], vertList);
             GLuint i2 = icosphere::getMiddlePoint(indices[2], indices[0], vertList);

             tempIndList1.push_back({indices[0], i0, i2});
             tempIndList1.push_back({indices[1], i1, i0});
             tempIndList1.push_back({indices[2], i2, i1});
             tempIndList1.push_back({i0, i1, i2});
         }
         tempIndList0 = tempIndList1;
     }

     *indexList = tempIndList0;
 }