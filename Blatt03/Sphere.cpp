#include "Sphere.h"

extern float sphereRadius;

// Fügt ein Dreieck mit drei Vertices zur Vertex- und Indexliste hinzu
void addTriangle(
    std::vector<glm::vec3>& vertices,
    std::vector<GLushort>& indices,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c
)
{
    GLushort startIndex = static_cast<GLushort>(vertices.size());

    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);

    indices.push_back(startIndex);
    indices.push_back(startIndex + 1);
    indices.push_back(startIndex + 2);
}

// Erzeugt ein Dreiecksgitter innerhalb eines Oktaederdreiecks
void generateTriangleGrid(
    std::vector<glm::vec3>& vertices,
    std::vector<GLushort>& indices,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c,
    int n
)
{
    int steps = n + 1;

    for (int i = 0; i < steps; i++)
    {
        for (int j = 0; j < steps - i; j++)
        {
            float alpha0 = static_cast<float>(i) / steps;
            float beta0 = static_cast<float>(j) / steps;

            float alpha1 = static_cast<float>(i + 1) / steps;
            float beta1 = static_cast<float>(j) / steps;

            float alpha2 = static_cast<float>(i) / steps;
            float beta2 = static_cast<float>(j + 1) / steps;

            glm::vec3 p0 =
                glm::normalize(
                    alpha0 * a +
                    beta0 * b +
                    (1.0f - alpha0 - beta0) * c
                ) * sphereRadius;

            glm::vec3 p1 =
                glm::normalize(
                    alpha1 * a +
                    beta1 * b +
                    (1.0f - alpha1 - beta1) * c
                ) * sphereRadius;

            glm::vec3 p2 =
                glm::normalize(
                    alpha2 * a +
                    beta2 * b +
                    (1.0f - alpha2 - beta2) * c
                ) * sphereRadius;

            addTriangle(vertices, indices, p0, p1, p2);

            if (j < steps - i - 1)
            {
                float alpha3 = static_cast<float>(i + 1) / steps;
                float beta3 = static_cast<float>(j + 1) / steps;

                glm::vec3 p3 =
                    glm::normalize(
                        alpha3 * a +
                        beta3 * b +
                        (1.0f - alpha3 - beta3) * c
                    ) * sphereRadius;

                addTriangle(vertices, indices, p1, p3, p2);
            }
        }
    }
}