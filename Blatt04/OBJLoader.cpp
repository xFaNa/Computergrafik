#include "OBJLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
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
                int nIndex = std::stoi(vnString) - 1;

                vertexIndices.push_back(vIndex);
                normalIndices.push_back(nIndex);
            }

            for (size_t i = 1; i + 1 < vertexIndices.size(); ++i) {
                meshVertices.push_back(positions[vertexIndices[0]]);
                meshVertices.push_back(positions[vertexIndices[i]]);
                meshVertices.push_back(positions[vertexIndices[i + 1]]);

                meshNormals.push_back(normals[normalIndices[0]]);
                meshNormals.push_back(normals[normalIndices[i]]);
                meshNormals.push_back(normals[normalIndices[i + 1]]);
            }

            faceCount++;
        }
    }

    std::cout << "OBJ geladen:" << std::endl;
    std::cout << "Vertices: " << vertexCount << std::endl;
    std::cout << "Normalen: " << normalCount << std::endl;
    std::cout << "Faces: " << faceCount << std::endl;

    std::cout << "Positions gespeichert: " << positions.size() << std::endl;
    std::cout << "Normalen gespeichert: " << normals.size() << std::endl;

    std::cout << "Dreiecksvertices: " << meshVertices.size() << std::endl;
    std::cout << "Dreiecksnormalen: " << meshNormals.size() << std::endl;
    

    return true;
}