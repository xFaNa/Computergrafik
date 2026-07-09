#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct FaceVertex {
    int positionIndex;
    int normalIndex;
};

struct Face {
    std::vector<FaceVertex> vertices;
};

class OBJLoader
{
public:
    bool load(const std::string& filename);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> meshVertices;
    std::vector<glm::vec3> meshNormals;

    std::vector<Face> faces;
    bool hasNormals = false;

    glm::vec3 minPosition;
    glm::vec3 maxPosition;
    glm::vec3 center;
    float maxExtent = 1.0f;
};