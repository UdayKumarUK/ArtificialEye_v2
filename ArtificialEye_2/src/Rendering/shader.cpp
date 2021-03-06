#include "shader.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>

using namespace std;

ee::Shader::Shader() :
    m_programID(0)
{
}

void ee::Shader::assignTexture(const std::string name, const int textureUnit)
{
    int unit = textureUnit - GL_TEXTURE0;
    glUniform1i(glGetUniformLocation(m_programID, name.c_str()), unit);
}

void ee::Shader::assignMat4(const std::string name, const Mat4& mat)
{
    glm::mat4 mat4(mat);
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat4));
}

void ee::Shader::assignBool(const std::string name, const bool value)
{
    glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
}

void ee::Shader::assignColor(const std::string name, const Vec3 vec)
{
    glm::vec3 vec3(vec);
    glUniform3f(glGetUniformLocation(m_programID, name.c_str()), vec3.x, vec3.y, vec3.z);
}

void ee::Shader::assignColor(const std::string name, const Vec4 vec)
{
    glm::vec4 vec4(vec);
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniform4f(loc, vec4.x, vec4.y, vec4.z, vec4.w);
}

void ee::Shader::assignfloat(std::string name, Float val)
{
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniform1f(loc, static_cast<float>(val));
}

void ee::Shader::bindTexture(GLenum target, GLuint number, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + number);
    glBindTexture(target, texture);
}

void ee::Shader::unbindTexture(GLenum target)
{
    glBindTexture(target, 0);
}

void ee::Shader::assignVec3(const std::string name, const Vec3 vec)
{
    glm::vec3 vec3(vec);
    GLint loc = glGetUniformLocation(m_programID, name.c_str());
    glUniform3f(loc, vec3.x, vec3.y, vec3.z);
}

bool ee::Shader::initialize(const std::string& vtxPath, const std::string& frgPath, const std::string& geomPath)
{
    bool geomShader = !geomPath.empty();

	std::ifstream vtxStream, frgStream, geomStream;
	std::stringstream vtxCode, frgCode, geomCode;
	vtxStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
	frgStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);

	// open and load code into stream
    try
    {
        vtxStream.open(vtxPath);
        vtxCode << vtxStream.rdbuf();
        vtxStream.close();
    }
    catch (std::ifstream::failure excp)
    {
        std::cout << "Failed to open vertex shader: " << vtxPath << std::endl;
        return false;
    }

	try
	{
		frgStream.open(frgPath);
		frgCode << frgStream.rdbuf();
		frgStream.close();
	}
	catch (std::ifstream::failure excp)
	{
        std::cout << "Failed to open fragment shader: " << frgPath << std::endl;
        return false;
	}

    if (geomShader)
    {
        try
        {
            geomStream.open(geomPath);
            geomCode << geomStream.rdbuf();
            geomStream.close();
        }
        catch (std::ifstream::failure excp)
        {
            std::cout << "Failed to open geometry shader: " << geomPath << std::endl;
            return false;
        }
    }

	GLuint vtxID, frgID, geomID;
	GLint resultChk;
	GLchar resultLog[512];

	vtxID = glCreateShader(GL_VERTEX_SHADER);
	frgID = glCreateShader(GL_FRAGMENT_SHADER);
    if (geomShader)
    {
        geomID = glCreateShader(GL_GEOMETRY_SHADER);
    }
	
	std::string strTmpCode = std::move(vtxCode.str());
	const GLchar *ptrCode = strTmpCode.c_str();
	glShaderSource(vtxID, 1, &ptrCode, nullptr);
	glCompileShader(vtxID);

	glGetShaderiv(vtxID, GL_COMPILE_STATUS, &resultChk);
	if (!resultChk)
	{
		glGetShaderInfoLog(vtxID, 512, nullptr, resultLog);
        std::cout << "Issue when compiling vertex shader: " << vtxPath << endl;
		std::cout << resultLog << endl;
        return false;
	}

	strTmpCode = std::move(frgCode.str());
	ptrCode = strTmpCode.c_str();
	glShaderSource(frgID, 1, &ptrCode, nullptr);
	glCompileShader(frgID);

	glGetShaderiv(frgID, GL_COMPILE_STATUS, &resultChk);
	if (!resultChk)
	{
		glGetShaderInfoLog(frgID, 512, nullptr, resultLog);
        std::cout << "Issue when compiling fragment shader: " << frgPath << endl;
		std::cout << resultLog << endl;
        return false;
	}

    if (geomShader)
    {
        strTmpCode = std::move(geomCode.str());
        ptrCode = strTmpCode.c_str();
        glShaderSource(geomID, 1, &ptrCode, nullptr);
        glCompileShader(geomID);

        glGetShaderiv(geomID, GL_COMPILE_STATUS, &resultChk);
        if (!resultChk)
        {
            glGetShaderInfoLog(geomID, 512, nullptr, resultLog);
            std::cout << "Issue when compiling geometry shader: " << geomPath << endl;
            std::cout << resultLog << endl;
            return false;
        }
    }

	m_programID = glCreateProgram();
	glAttachShader(m_programID, vtxID);
	glAttachShader(m_programID, frgID);
    if (geomShader)
    {
        glAttachShader(m_programID, geomID);
    }

	glLinkProgram(m_programID);
	glGetProgramiv(m_programID, GL_LINK_STATUS, &resultChk);
	if (!resultChk)
	{
		glGetProgramInfoLog(m_programID, 512, nullptr, resultLog);
        std::cout << "Issue when linking shaders: " << "vert: " << vtxPath << ", frag: " << frgPath << ", geom: " << geomPath << endl;
        std::cout << resultLog << endl;
        return false;
	}

	glDeleteShader(vtxID);
	glDeleteShader(frgID);
    if (geomShader)
    {
        glDeleteShader(geomID);
    }

    return true;
}

ee::Shader::~Shader()
{
	glDeleteProgram(m_programID);
}

ee::Shader::Shader(Shader&& shader) :
    m_programID(shader.m_programID)
{
    shader.m_programID = 0;
}

void ee::Shader::use() const
{
	glUseProgram(m_programID);
}