#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

class OBJLoader
{
public:
    bool load(const std::string& filename);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> meshVertices;
    std::vector<glm::vec3> meshNormals;
};