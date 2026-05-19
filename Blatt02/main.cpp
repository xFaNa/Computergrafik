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

cg::GLSLProgram program;

glm::mat4x4 view;
glm::mat4x4 projection;

float zNear = 0.1f;
float zFar  = 100.0f;

float rotationStep = 5.0f;
float sphereRadius = 1.0f;
float cameraDistance = 4.0f;

int subdivisionDepth = 4;
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

Object sphere;


void renderSphere()
{
    glm::mat4x4 mvp = projection * view * sphere.model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(sphere.vao);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

glm::vec3 midpoint(glm::vec3 a, glm::vec3 b)
{
    glm::vec3 mid = (a + b) * 0.5f;

    return glm::normalize(mid);
}


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

void subdivideTriangle(
    std::vector<glm::vec3>& vertices,
    std::vector<GLushort>& indices,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c,
    int depth
)
{
    if (depth == 0) 
    {
        addTriangle(vertices, indices, a, b, c);
        return;
    }


    glm::vec3 ab = midpoint(a, b);
    glm::vec3 bc = midpoint(b, c);
    glm::vec3 ca = midpoint(c, a);

    subdivideTriangle(vertices, indices, a, ab, ca, depth - 1);
    subdivideTriangle(vertices, indices, ab, b, bc, depth - 1);
    subdivideTriangle(vertices, indices, ca, bc, c, depth -1);
    subdivideTriangle(vertices, indices, ab, bc, ca, depth -1);
} 

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

void updateView()
{
	glm::vec3 eye(0.0f, 0.0f, cameraDistance);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    view = glm::lookAt(eye, center, up);
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
  
  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSphere();
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
    
  case '+':

      if (subdivisionDepth < 4)
      {
          subdivisionDepth++;

          std::cout << "Subdivision Depth: "
              << subdivisionDepth << std::endl;

          initSphere();
      }

      break;
    break;
  
  case '-':

      if (subdivisionDepth > 0)
      {
          subdivisionDepth--;

          std::cout << "Subdivision Depth: "
              << subdivisionDepth << std::endl;

          initSphere();
      }

      break;
    
    break;
  case 'x':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.f),
          glm::radians(rotationStep),
          glm::vec3(1.0f, 0.0f, 0.0f)
      );
    break;

  case 'y':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.0f),
          glm::radians(rotationStep),
          glm::vec3(0.0f, 1.0f, 0.0f)
      );
    break;

  case 'z':
      sphere.model = sphere.model * glm::rotate(
          glm::mat4(1.0f),
          glm::radians(rotationStep),
          glm::vec3(0.0f, 0.0f, 1.0f)
      );
    break;

  case 'n':
      sphere.model = glm::mat4(1.0f);
    break;

  case 'r':
      if (sphereRadius > 0.3f)
      {
          sphereRadius -= 0.1f;
          initSphere();
      }
      break;

  case 'R':
      if (sphereRadius < 2.0f)
      {
          sphereRadius += 0.1f;
          initSphere();
      }
      break;

  case 'a':

      if (cameraDistance > 1.0f)
      {
          cameraDistance -= 0.2f;
          updateView();
      }

      break;

  case 's':

      if (cameraDistance < 10.0f)
      {
          cameraDistance += 0.2f;
          updateView();
      }

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
