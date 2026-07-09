#include "OBJLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include <glm/glm.hpp>

bool OBJLoader::load(const std::string& filename)
{
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    std::cout << "Arbeitsverzeichnis: " << buffer << std::endl;
  
    std::ifstream file("StarDestroyer.obj");

    if (!file.is_open()) {
        std::cout << "Konnte OBJ-Datei nicht öffnen: " << filename << std::endl;
        return false;
    }

    int vertexCount = 0;
    int normalCount = 0;
    int faceCount = 0;

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream stream(line);

        std::string prefix;
        stream >> prefix;

        if (prefix == "v") {
            float x, y, z;
            stream >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
            vertexCount++;
        }
        else if (prefix == "vn") {
            float x, y, z;
            stream >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
            normalCount++;
        }
        else if (prefix == "f") {
            std::vector<int> vertexIndices;
            std::vector<int> normalIndices;
            Face face;

            std::string token;

            while (stream >> token) {
                std::stringstream tokenStream(token);

                std::string vString;
                std::string vtString;
                std::string vnString;

                std::getline(tokenStream, vString, '/');
                std::getline(tokenStream, vtString, '/');
                std::getline(tokenStream, vnString, '/');

                int vIndex = std::stoi(vString) - 1;

                int nIndex = -1;
                if (!vnString.empty()) {
                    nIndex = std::stoi(vnString) - 1;
                }

                vertexIndices.push_back(vIndex);
                normalIndices.push_back(nIndex);

                FaceVertex fv;
                fv.positionIndex = vIndex;
                fv.normalIndex = nIndex;
                face.vertices.push_back(fv);
            }

            faces.push_back(face);

            for (size_t i = 1; i + 1 < vertexIndices.size(); ++i) {
                meshVertices.push_back(positions[vertexIndices[0]]);
                meshVertices.push_back(positions[vertexIndices[i]]);
                meshVertices.push_back(positions[vertexIndices[i + 1]]);

                if (normalIndices[0] >= 0 &&
                    normalIndices[i] >= 0 &&
                    normalIndices[i + 1] >= 0) {
                    meshNormals.push_back(normals[normalIndices[0]]);
                    meshNormals.push_back(normals[normalIndices[i]]);
                    meshNormals.push_back(normals[normalIndices[i + 1]]);
                }
            }

            faceCount++;
        }

    if (!positions.empty()) {
       minPosition = positions[0];
       maxPosition = positions[0];

       for (const glm::vec3& p : positions) {
           if (p.x < minPosition.x) minPosition.x = p.x;
           if (p.y < minPosition.y) minPosition.y = p.y;
           if (p.z < minPosition.z) minPosition.z = p.z;

           if (p.x > maxPosition.x) maxPosition.x = p.x;
           if (p.y > maxPosition.y) maxPosition.y = p.y;
           if (p.z > maxPosition.z) maxPosition.z = p.z;
       }

          center = (minPosition + maxPosition) * 0.5f;

          glm::vec3 size = maxPosition - minPosition;
          maxExtent = glm::max(size.x, glm::max(size.y, size.z));
        }
    }

    hasNormals = !normals.empty();

    if (!hasNormals) {
        std::cout << "OBJ enthält keine Normalen." << std::endl;
    }

    std::cout << "OBJ geladen:" << std::endl;
    std::cout << "Vertices: " << vertexCount << std::endl;
    std::cout << "Normalen: " << normalCount << std::endl;
    std::cout << "Faces: " << faceCount << std::endl;

    std::cout << "Positions gespeichert: " << positions.size() << std::endl;
    std::cout << "Normalen gespeichert: " << normals.size() << std::endl;

    std::cout << "Dreiecksvertices: " << meshVertices.size() << std::endl;
    std::cout << "Dreiecksnormalen: " << meshNormals.size() << std::endl;
    
    std::cout << "BBox Min: " << minPosition.x << ", " << minPosition.y << ", " << minPosition.z << std::endl;
    std::cout << "BBox Max: " << maxPosition.x << ", " << maxPosition.y << ", " << maxPosition.z << std::endl;
    std::cout << "Center: " << center.x << ", " << center.y << ", " << center.z << std::endl;
    std::cout << "Max Extent: " << maxExtent << std::endl;

    return true;
}