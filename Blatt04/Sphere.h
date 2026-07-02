#pragma once

#include <vector>

#include <GL/glew.h>

#include <glm/glm.hpp>

// Fügt ein Dreieck zur Vertex- und Indexliste hinzu
void addTriangle(
	std::vector<glm::vec3>& vertices,
	std::vector<GLushort>& indices,
	glm::vec3 a,
	glm::vec3 b,
	glm::vec3 c
);

// Erzeugt ein unterteiltes Dreiecksgitter
void generateTriangleGrid(
	std::vector<glm::vec3>& vertices,
	std::vector<GLushort>& indices,
	glm::vec3 a,
	glm::vec3 b,
	glm::vec3 c,
	int n
);