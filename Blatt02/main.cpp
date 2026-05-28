#include <iostream>
#include <vector>

#include <GL/glew.h>
//#include <GL/gl.h> // OpenGL header not necessary, included by GLEW
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "GLSLProgram.h"
#include "GLTools.h"

// Standard window width
const int WINDOW_WIDTH  = 640;
// Standard window height
const int WINDOW_HEIGHT = 480;
// GLUT window id/handle
int glutID = 0;

// Shaderprogramm Vertex- und Fragmentshader
cg::GLSLProgram program;

// View and projection matrix
glm::mat4x4 view;
glm::mat4x4 projection;

// Camera parameters
float zNear = 0.1f;
float zFar  = 100.0f;

// Unsere globalen Variablen
float rotationStep = 5.0f;          // 5 Grad Rotation pro Tastendruck
float sphereRadius = 1.0f;          // Radius der Kugel
float cameraDistance = 4.0f;        // Kamera distanz

int subdivisionDepth = 4;           // Detailgrad der Kugel (Anzahl Unterteilungen der Dreiecke 0-4)
GLsizei sphereIndexCount = 0;

/*
Struct to hold data for object rendering.
*/
class Object
{
public:
  inline Object ()
    : vao(0),
      positionBuffer(0),
      colorBuffer(0),
      indexBuffer(0)
  {}

  inline ~Object () { // GL context must exist on destruction
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &colorBuffer);
    glDeleteBuffers(1, &positionBuffer);
  }

  GLuint vao;        // vertex-array-object ID
  
  GLuint positionBuffer; // ID of vertex-buffer: position
  GLuint colorBuffer;    // ID of vertex-buffer: color
  
  GLuint indexBuffer;    // ID of index-buffer
  
  glm::mat4x4 model; // model matrix
};

// Render Objekte
Object sphere;
Object axes;
Object normals;

// Normalen Visualiesierungseinstellungen
bool showNormals = false;
float normalLength = 0.2f;
GLsizei normalIndexCount = 0;

// Rendert die Kugel mithilfe der MVP-Matrix, des VAOs und des Indexbuffers
void renderSphere()
{
    glm::mat4x4 mvp = projection * view * sphere.model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(sphere.vao);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

// Rendert das lokale Koordinatensystem der Kugel mithilfe von Linien
void renderAxes()
{
    glm::mat4x4 mvp = projection * view * sphere.model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(axes.vao);
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

// Rendert die Normalen der Kugel als Linien im lokalen Koordinatensystem
void renderNormals()
{
    glm::mat4x4 mvp = projection * view * sphere.model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(normals.vao);
    glDrawElements(GL_LINES, normalIndexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

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
// und projiziert alle erzeugten Punkte auf die Kugeloberfläche
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

            glm::vec3 p0 = glm::normalize(alpha0 * a + beta0 * b + (1.0f - alpha0 - beta0) * c) * sphereRadius;
            glm::vec3 p1 = glm::normalize(alpha1 * a + beta1 * b + (1.0f - alpha1 - beta1) * c) * sphereRadius;
            glm::vec3 p2 = glm::normalize(alpha2 * a + beta2 * b + (1.0f - alpha2 - beta2) * c) * sphereRadius;

            addTriangle(vertices, indices, p0, p1, p2);

            if (j < steps - i - 1)
            {
                float alpha3 = static_cast<float>(i + 1) / steps;
                float beta3 = static_cast<float>(j + 1) / steps;

                glm::vec3 p3 = glm::normalize(alpha3 * a + beta3 * b + (1.0f - alpha3 - beta3) * c) * sphereRadius;

                addTriangle(vertices, indices, p1, p3, p2);
            }
        }
    }
}

// Erzeugt die Kugelgeometrie aus einem unterteilten Oktaeder
// und initialisiert die benötigten OpenGL-Buffer
void initSphere()
{
    std::vector<glm::vec3> vertices;
    std::vector<GLushort> indices;

    glm::vec3 top (0.0f, 1.0f, 0.0f);
    glm::vec3 right (1.0f, 0.0f, 0.0f);
    glm::vec3 front (0.0f, 0.0f, 1.0f);
    glm::vec3 left(-1.0f, 0.0f, 0.0f);
    glm::vec3 back(0.0f, 0.0f, -1.0f);
    glm::vec3 bottom(0.0f, -1.0f, 0.0f);

    // obere Hälfte
    generateTriangleGrid(vertices, indices, top, front, right, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, left, front, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, back, left, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, right, back, subdivisionDepth);

    // untere Hälfte
    generateTriangleGrid(vertices, indices, bottom, right, front, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, front, left, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, left, back, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, back, right, subdivisionDepth);

    sphereIndexCount = static_cast<GLsizei>(indices.size());

    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Indices: " << indices.size() << std::endl;

    const std::vector<glm::vec3> colors(
        vertices.size(),
        glm::vec3(1.0f, 1.0f, 0.0f)
    );

    GLuint programId = program.getHandle();
    GLuint pos;


    glGenVertexArrays(1, &sphere.vao);
    glBindVertexArray(sphere.vao);

    glGenBuffers(1, &sphere.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &sphere.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &sphere.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    sphere.model = glm::mat4(1.0f);
    
}

// Initialisiert das lokale Koordinatensystem der Kugel
// und erstellt die benötigten OpenGL-Buffer
void initAxes()
{
    const std::vector<glm::vec3> vertices =
    {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(sphereRadius * 1.5f, 0.0f, 0.0f),

        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f,sphereRadius * 1.5f, 0.0f),

        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f,sphereRadius * 1.5f)
    };

    const std::vector<glm::vec3> colors =
    {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),

        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),

        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };

    const std::vector<GLushort> indices =
    {
        0,1,
        2,3,
        4,5
    };

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &axes.vao);
    glBindVertexArray(axes.vao);

    glGenBuffers(1, &axes.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, axes.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &axes.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, axes.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &axes.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axes.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(GLushort),
        indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

// Erstellt die Kamerasicht mithilfe von Position, Blickrichtung und Up-Vektor
void updateView()
{
	glm::vec3 eye(0.0f, 0.0f, cameraDistance);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    view = glm::lookAt(eye, center, up);
}

// Erzeugt Linien zur Visualisierung der Vertex-Normalen
// und initialisiert dafür die benötigten OpenGL-Buffer
void initNormals()
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<GLushort> indices;

    std::vector<glm::vec3> sphereVertices;
    std::vector<GLushort> sphereIndices;

    glm::vec3 top(0.0f, 1.0f, 0.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 front(0.0f, 0.0f, 1.0f);
    glm::vec3 left(-1.0f, 0.0f, 0.0f);
    glm::vec3 back(0.0f, 0.0f, -1.0f);
    glm::vec3 bottom(0.0f, -1.0f, 0.0f);

    generateTriangleGrid(sphereVertices, sphereIndices, top, front, right, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, top, left, front, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, top, back, left, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, top, right, back, subdivisionDepth);

    generateTriangleGrid(sphereVertices, sphereIndices, bottom, right, front, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, bottom, front, left, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, bottom, left, back, subdivisionDepth);
    generateTriangleGrid(sphereVertices, sphereIndices, bottom, back, right, subdivisionDepth);

    for (glm::vec3 p : sphereVertices)
    {
        glm::vec3 normal = glm::normalize(p);

        glm::vec3 start = p;
        glm::vec3 end = p + normal * normalLength;

        GLushort startIndex = static_cast<GLushort>(vertices.size());

        vertices.push_back(start);
        vertices.push_back(end);

        colors.push_back(glm::vec3(1.0f, 0.0f, 1.0f));
        colors.push_back(glm::vec3(1.0f, 0.0f, 1.0f));

        indices.push_back(startIndex);
        indices.push_back(startIndex + 1);
    }

    normalIndexCount = static_cast<GLsizei>(indices.size());

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &normals.vao);
    glBindVertexArray(normals.vao);

    glGenBuffers(1, &normals.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normals.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &normals.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normals.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW);

    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &normals.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, normals.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(GLushort),
        indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  updateView();
  
  // Create a shader program and set light direction.
  if (!program.compileShaderFromFile("shader/simple.vert", cg::GLSLShader::VERTEX)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.compileShaderFromFile("shader/simple.frag", cg::GLSLShader::FRAGMENT)) {
    std::cerr << program.log();
    return false;
  }
  
  if (!program.link()) {
    std::cerr << program.log();
    return false;
  }

  // Create all objects.
  initSphere();
  initAxes();
  initNormals();
  
  return true;
}

/*
 Rendering.
 */
void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSphere();

    glDisable(GL_DEPTH_TEST);
    renderAxes();

    if (showNormals)
    {
        renderNormals();
    }

    glEnable(GL_DEPTH_TEST);
}

void glutDisplay ()
{
   render();
   glutSwapBuffers();
}

/*
 Resize callback.
 */
void glutResize (int width, int height)
{
  // Division by zero is bad...
  height = height < 1 ? 1 : height;
  glViewport(0, 0, width, height);
  
  // Construct projection matrix.
  projection = glm::perspective(45.0f, (float) width / height, zNear, zFar);
}

/*
 Callback for char input.
 */
void glutKeyboard (unsigned char keycode, int x, int y)
{
  switch (keycode) {
  case 27: // ESC
    glutDestroyWindow ( glutID );
    return;
  
    // Änderung des Subdivison-Levels
  case '+':

      if (subdivisionDepth < 4)
      {
          subdivisionDepth++;

          std::cout << "Subdivision Depth: "
              << subdivisionDepth << std::endl;

          initSphere();
          initNormals();
      }
      break;

    // Änderung des Subdivison-Levels
  case '-':

      if (subdivisionDepth > 0)
      {
          subdivisionDepth--;

          std::cout << "Subdivision Depth: "
              << subdivisionDepth << std::endl;

          initSphere();
          initNormals();
      }
    break;

    // Rotation der Kugel um die lokalen Achsen
  case 'x':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.f),
          glm::radians(rotationStep),
          glm::vec3(1.0f, 0.0f, 0.0f)
      );
    break;

    // Rotation der Kugel um die lokalen Achsen
  case 'y':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.0f),
          glm::radians(rotationStep),
          glm::vec3(0.0f, 1.0f, 0.0f)
      );
    break;

    // Rotation der Kugel um die lokalen Achsen
  case 'z':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.0f),
          glm::radians(rotationStep),
          glm::vec3(0.0f, 0.0f, 1.0f)
      );
    break;

	// Zurücksetzen der Kugeltransformation
  case 'n':
      sphere.model = glm::mat4(1.0f);
    break;

    // Skalierung der Kugel
  case 'r':
      if (sphereRadius > 0.3f)
      {
          sphereRadius -= 0.1f;
          initSphere();
          initAxes();
          initNormals();
      }
      break;
      
      // Skalierung der Kugel
  case 'R':
      if (sphereRadius < 2.0f)
      {
          sphereRadius += 0.1f;
          initSphere();
          initAxes();
          initNormals();
      }
      break;

      // Kamera-Zoom durch Veränderung der Kameradistanz
  case 'a':

      if (cameraDistance > 1.0f)
      {
          cameraDistance -= 0.2f;
          updateView();
      }

      break;
      
      // Kamera-Zoom durch Veränderung der Kameradistanz
  case 's':

      if (cameraDistance < 10.0f)
      {
          cameraDistance += 0.2f;
          updateView();
      }

      break;

      // Normalen Anzeigen 
  case 'v':
      showNormals = !showNormals;
      break;
  }

  glutPostRedisplay();
}


int main(int argc, char** argv)
{
  // GLUT: Initialize freeglut library (window toolkit).
  glutInitWindowSize    (WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition(40,40);
  glutInit(&argc, argv);
  
  // GLUT: Create a window and opengl context (version 4.3 core profile).
  glutInitContextVersion(4, 3);
  glutInitContextFlags  (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
  glutInitDisplayMode   (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  
  glutCreateWindow("Aufgabenblatt 02");
  glutID = glutGetWindow();
  
  // GLEW: Load opengl extensions
  //glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    return -1;
  }
#if _DEBUG
  if (glDebugMessageCallback) {
    std::cout << "Register OpenGL debug callback " << std::endl;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(cg::glErrorVerboseCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE,
			  GL_DONT_CARE,
			  GL_DONT_CARE,
			  0,
			  nullptr,
			  true); // get all debug messages
  } else {
    std::cout << "glDebugMessageCallback not available" << std::endl;
  }
#endif

  // GLUT: Set callbacks for events.
  glutReshapeFunc(glutResize);
  glutDisplayFunc(glutDisplay);
  //glutIdleFunc   (glutDisplay); // redisplay when idle
  
  glutKeyboardFunc(glutKeyboard);

 

  // init vertex-array-objects.
  bool result = init();
  if (!result) {
    return -2;
  }

  // GLUT: Loop until the user closes the window
  // rendering & event handling
  glutMainLoop ();
  
  // Cleanup in destructors:
  // Objects will be released in ~Object
  // Shader program will be released in ~GLSLProgram
  
  return 0;
}
