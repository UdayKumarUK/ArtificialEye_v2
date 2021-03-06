#include "Initialization.hpp"

#include "Rendering/Renderer.hpp"
#include "Rendering/TexturePacks/UniColorTextPack.hpp"
#include "Rendering/TexturePacks/LightUniColorTextpack.hpp"
#include "Rendering/Modeling/SkyBox.hpp"
#include "Rendering/MeshTypes.hpp"
#include "Rendering/TexturePacks/SkyBoxTextPack.hpp"
#include "Rendering/TexturePacks/RefractTextPack.hpp"
#include "Rendering/TexturePacks/LineUniColorTextPack.hpp"
#include "RayTracing/RayTracer.hpp"
#include "Rendering/Lens.hpp"
#include "SoftBody/Simulation/SBClosedBodySim.hpp"
#include "SoftBody/ForceGens/SBGravity.hpp"
#include "SoftBody/Constraints/SBPointConstraint.hpp"
#include "SoftBody/Integrators/SBVerletIntegrator.hpp"
#include "SoftBody/Objects/SBFixedPoint.hpp"
#include "SoftBody/SBUtilities.hpp"
#include "Rendering/Subdivision.hpp"
#include "Rendering/Modeling/DrawableMeshContainer.hpp"
#include "Rendering/Modeling/LoadableModel.hpp"

#include "Alglib/interpolation.h"

#include <string>
#include <iostream>
#include <vector>

using namespace ee;

std::vector<std::string> g_cubeMapFaces
{
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "back.jpg",
    "front.jpg"
};

const std::string g_textureDir = "Textures/";
const std::string g_cubeMapDir = "SkyBox";

bool g_startSoftBody = false;
bool g_enableWireFram = false;

std::vector<ee::SBPointConstraint*> g_constraints;
const Float g_constraintMoveSpeed = 0.1;
ee::RayTracer* g_tracer;
bool g_defaultP = true;

void addConstraints(const std::size_t thickness, ee::SBSimulation* sim, const ee::Mesh* mesh)
{
    for (std::size_t i = 0, sub = (thickness / 2 + 1); i < thickness; i++, sub--)
    {
        const std::size_t index = (ARTIFICIAL_EYE_PROP.latitude / 2 - sub) * (ARTIFICIAL_EYE_PROP.longitude) + 1;
        const std::size_t end = index + ARTIFICIAL_EYE_PROP.longitude;

        for (std::size_t j = index; j < end; j++)
        {
            auto ptr = sim->addConstraint(&ee::SBPointConstraint(mesh->getVertex(j).m_position, sim->getVertexObject(j)));
            g_constraints.push_back(ptr);
        }
    }
}

void setSpaceCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
        {
            g_startSoftBody = !g_startSoftBody;
        }
    }
    else if (key == GLFW_KEY_N)
    {
        if (action == GLFW_PRESS)
        {
            g_enableWireFram = !g_enableWireFram;
        }
    }
    else if (key == GLFW_KEY_R)
    {
        if (action == GLFW_PRESS)
        {
            g_defaultP = !g_defaultP;
        }
    }

    if (action == GLFW_PRESS && (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN))
    {
        float dir;
        switch (key)
        {
        case GLFW_KEY_UP:
            dir = 1.f;
            break;
        case GLFW_KEY_DOWN:
            dir = -1.f;
            break;
        }

        for (auto& constraint : g_constraints)
        {
            constraint->m_point += g_constraintMoveSpeed * dir * glm::normalize(Vec3(constraint->m_point.x, 0.0, constraint->m_point.z));
        }
    }
}

int main()
{
    if (!ARTIFICIAL_EYE_PROP.success)
    {
        return -1;
    }

    try
    {
        // The default camera parameters:
        CameraParam cameraParams;
        cameraParams.m_position = glm::vec3(10.f, 0.f, 0.f);
        cameraParams.m_up = glm::vec3(0.f, 1.f, 0.f);
        cameraParams.m_yaw = 180.f;
        cameraParams.m_pitch = 0.f;

        // prepare the renderer:
        Renderer::initialize(ARTIFICIAL_EYE_PROP.shader_dir, ARTIFICIAL_EYE_PROP.render_param, cameraParams);
        Renderer::setCustomKeyboardCallback(setSpaceCallback);

        // loaded cubemap twice, not efficient, but not important right now:
        void* res = Renderer::addTexturePack("refractTextPack", RefractTextPack(glm::vec3(), g_textureDir + g_cubeMapDir, g_cubeMapFaces, ARTIFICIAL_EYE_PROP.refractive_index)); assert(res);
        res =       Renderer::addTexturePack("skyBoxTextPack", SkyBoxTextPack(g_textureDir + g_cubeMapDir, g_cubeMapFaces)); assert(res);

        LoadableModel eyeballModel("Models/eyeball/eyebal_fbx.fbx", 1);
        eyeballModel.load();

        Mat4 mat = glm::translate(Mat4(), Vec3(0.f, 0.f, -2.0f));
        mat = glm::scale(mat, Vec3(1.55f, 1.55f, 1.55f));
        eyeballModel.setTransform(mat);

        // generate the UV Sphere lens (not super efficient)
        Mesh uvSphereMesh = loadUVsphere(ARTIFICIAL_EYE_PROP.longitude, ARTIFICIAL_EYE_PROP.latitude);
        Mesh uvSubDivSphereMesh = uvSphereMesh; // loopSubdiv(uvSphereMesh, ARTIFICIAL_EYE_PROP.subdiv_level);
        Lens lensSphere(&uvSubDivSphereMesh, ARTIFICIAL_EYE_PROP.latitude, ARTIFICIAL_EYE_PROP.longitude);
        DrawableMeshContainer lensDrawable(&uvSubDivSphereMesh, "refractTextPack", true);
        Renderer::addDrawable(&lensDrawable);

        SkyBox skyBox("skyBoxTextPack");
        Renderer::addDrawable(&skyBox);

        // update the positons of the lens
        const Mat4 lensModelTrans = glm::scale(glm::rotate(Mat4(), glm::radians(90.0), Vec3(1.0, 0.0, 0.0)), Vec3(1.0, ARTIFICIAL_EYE_PROP.lens_thickness, 1.0));
        uvSphereMesh.setModelTrans(lensModelTrans);
        uvSubDivSphereMesh.setModelTrans(lensModelTrans);

        // prepare the simulation
        SBClosedBodySim lensSim(ARTIFICIAL_EYE_PROP.pressure, &uvSphereMesh, ARTIFICIAL_EYE_PROP.mass, ARTIFICIAL_EYE_PROP.extspring_coeff, ARTIFICIAL_EYE_PROP.extspring_drag);
        lensSim.m_constIterations = ARTIFICIAL_EYE_PROP.iterations;
        addInteriorSpringsUVSphere(&lensSim, ARTIFICIAL_EYE_PROP.latitude, ARTIFICIAL_EYE_PROP.longitude, ARTIFICIAL_EYE_PROP.intspring_coeff, ARTIFICIAL_EYE_PROP.intspring_drag);
        //addConstraints(5, &lensSim, &lensMesh);
        lensSim.addIntegrator(&ee::SBVerletIntegrator(1.0 / 20.0, ARTIFICIAL_EYE_PROP.extspring_drag));

        // test stuff:
        const std::vector<Vec3> pos = {Vec3(0.0, 0.0, -2.0)};
        RayTracerParam param;
        param.m_widthResolution = 5;
        param.m_heightResolution = 5;
        param.m_lensRefractiveIndex = 1.406;
        param.m_enviRefractiveIndex = 1.0;
        param.m_rayColor = Vec3(1.0, 0.0, 0.0);
        g_constraints = lensSphere.addConstraints(5, &lensSim);
        g_tracer = &ee::RayTracer::initialize(pos, lensSphere, param);

        uvSubDivSphereMesh.calcNormals();
        g_tracer->raytrace();
        while (ee::Renderer::isInitialized())
        {
            assert(glGetError() == 0);

            if (g_enableWireFram)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            if (g_defaultP)
            {
                lensSim.setP(ARTIFICIAL_EYE_PROP.pressure);
            }
            else
            {
                lensSim.setP(0.f);
            }

            ee::Renderer::clearBuffers();

            float time = Renderer::timeElapsed();
            if (g_startSoftBody)
            {
                lensSim.update(time);
                Mesh tempMesh = loopSubdiv(uvSphereMesh, ARTIFICIAL_EYE_PROP.subdiv_level_lens);
                uvSubDivSphereMesh.updateVertices(std::move(tempMesh.getVerticesData()));
                uvSubDivSphereMesh.updateMeshFaces(std::move(tempMesh.getMeshFaceData()));
                g_tracer->raytrace();
            }

            Renderer::drawAll();
            Renderer::update(time);
            Renderer::swapBuffers();
            Renderer::pollEvents();
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "[EXCEP THROWN]: " << std::endl;
        std::cout << e.what() << std::endl;
        std::cout << "Press ENTER to exit." << std::endl;
        std::cin.get();
        Renderer::deinitialize();
        return -1;
    }

    Renderer::deinitialize();
    return 0;
}