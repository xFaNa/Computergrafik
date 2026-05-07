#include <iostream>
#include <vector>

#include <cmath>
#include <algorithm>

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

// Globale RGB Farbwerte fuer das Viereck
float r = 1.0f;
float g = 0.0f;
float b = 0.0f;

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

Object triangle;
Object quad;

void renderTriangle()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * triangle.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 1 triangle.
  glBindVertexArray(triangle.vao);
  glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void renderQuad()
{
  // Create mvp.
  glm::mat4x4 mvp = projection * view * quad.model;
  
  // Bind the shader program and set uniform(s).
  program.use();
  program.setUniform("mvp", mvp);
  
  // Bind vertex array object so we can render the 2 triangles.
  glBindVertexArray(quad.vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}

void initTriangle()
{
  // Construct triangle. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };
  const std::vector<glm::vec3> colors   = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
  const std::vector<GLushort>  indices  = { 0, 1, 2 };

  GLuint programId = program.getHandle();
  GLuint pos;

  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &triangle.vao);
  glBindVertexArray(triangle.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &triangle.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &triangle.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &triangle.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  triangle.model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.25f, 0.0f, 0.0f));
}

void initQuad()
{
  // Construct Quadrat. These vectors can go out of scope after we have send all data to the graphics card.
  const std::vector<glm::vec3> vertices = { { -1.0f, 1.0f, 0.0f }, { -1.0, -1.0, 0.0 }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } };
  const std::vector<glm::vec3> colors   = { { r, g, b }, { r, g, b }, { r, g, b }, { r, g, b } }; // Eingabewerte fuer das Quadrat auf Globale variablen geaendert
  const std::vector<GLushort>  indices  = { 0, 1, 2, 0, 2, 3 };

  GLuint programId = program.getHandle();
  GLuint pos;
  
  // Step 0: Create vertex array object.
  glGenVertexArrays(1, &quad.vao);
  glBindVertexArray(quad.vao);
  
  // Step 1: Create vertex buffer object for position attribute and bind it to the associated "shader attribute".
  glGenBuffers(1, &quad.positionBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.positionBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
  
  // Bind it to position.
  pos = glGetAttribLocation(programId, "position");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 2: Create vertex buffer object for color attribute and bind it to...
  glGenBuffers(1, &quad.colorBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad.colorBuffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
  
  // Bind it to color.
  pos = glGetAttribLocation(programId, "color");
  glEnableVertexAttribArray(pos);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
  // Step 3: Create vertex buffer object for indices. No binding needed here.
  glGenBuffers(1, &quad.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data(), GL_STATIC_DRAW);
  
  // Unbind vertex array object (back to default).
  glBindVertexArray(0);
  
  // Modify model matrix.
  quad.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.25f, 0.0f, 0.0f));
}

/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  
  // Construct view matrix.
  glm::vec3 eye(0.0f, 0.0f, 4.0f);
  glm::vec3 center(0.0f, 0.0f, 0.0f);
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  
  view = glm::lookAt(eye, center, up);
  
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
  initTriangle();
  initQuad();
  
  return true;
}

/*
 Rendering.
 */
void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderTriangle();
	renderQuad();
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
    // do something
    break;
  case '-':
    // do something
    break;
  case 'x':
    // do something
    break;
  case 'y':
    // do something
    break;
  case 'z':
    // do something
    break;
  }
  glutPostRedisplay();
}

// Umrechnung von RGB zu CMY
void rgbToCmy(float r, float g, float b, float& c, float& m, float& y)
{
    c = 1.0f - r;
    m = 1.0f - g;
    y = 1.0f - b;

}
// Umrehcnung von CMY zu RGB
void cmyToRgb(float c, float m, float y, float& r, float& g, float& b)
{
    r = 1.0f - c;
    g = 1.0f - m;
    b = 1.0f - y;
}
// Umrechnung von HSV nach RGB
void hsvToRgb(float h, float s, float v, float& r, float& g, float& b)
{
    float c = v * s;
    float x = c * (1.0f - std::abs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float match = v - c;

    float rTemp = 0.0f;
    float gTemp = 0.0f;
    float bTemp = 0.0f;

    if (h >= 0.0f && h < 60.0f) {
        rTemp = c;
        gTemp = x;
        bTemp = 0.0f;
    }
    else if (h >= 60.0f && h < 120.0f) {
        rTemp = x;
        gTemp = c;
        bTemp = 0.0f;
    }
    else if (h >= 120.0f && h < 180.0f) {
        rTemp = 0.0f;
        gTemp = c;
        bTemp = x;
    }
    else if (h >= 180.0f && h < 240.0f) {
        rTemp = 0.0f;
        gTemp = x;
        bTemp = c;
    }
    else if (h >= 240.0f && h < 300.0f) {
        rTemp = x;
        gTemp = 0.0f;
        bTemp = c;
    }
    else {
        rTemp = c;
        gTemp = 0.0f;
        bTemp = x;
    }

    r = rTemp + match;
    g = gTemp + match;
    b = bTemp + match;
}
// Umrechnung von RGB nach HSV
void rgbToHsv(float r, float g, float b, float& h, float& s, float& v)
{
    float max = std::max(r, std::max(g, b));
    float min = std::min(r, std::min(g, b));

    v = max;

    float delta = max - min;

    if (max == 0.0f) {
        s = 0.0f;
    }
    else {
        s = delta / max;
    }

    if (delta == 0.0f) {
        h = 0.0f;
    }
    else if (max == r) {
        h = 60.0f * fmod(((g - b) / delta), 6.0f);
    }
    else if (max == g) {
        h = 60.0f * (((b - r) / delta) + 2.0f);
    }
    else {
        h = 60.0f * (((r - g) / delta) + 4.0f);
    }

    if (h < 0.0f) {
        h += 360.0f;
    }
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
  
  glutCreateWindow("Aufgabenblatt 01");
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

 // Auswahlmodus fuer Farbmodell 
  int mode;

  std::cout << "Farbmodell waehlen: " << std::endl;
  std::cout << "1: RGB" << std::endl;
  std::cout << "2: CMY" << std::endl;
  std::cout << "3: HSV" << std::endl;
  std::cout << "Auswahl: ";
  std::cin >> mode;

  // Eingabe der RGB Werte
  if (mode == 1) {
      std::cout << "RGB-Werte einegeben (0.0 bis 1.0" << std::endl;

      std::cout << "r: ";
      std::cin >> r;

      std::cout << "g: ";
      std::cin >> g;

      std::cout << "b: ";
      std::cin >> b;
  }
  // Eingabe der CMY Werte
  else if (mode == 2) {

      float c, m, y;

      std::cout << "CMY-Werte eingeben (0.0 bis 1.0)" << std::endl;

      std::cout << "c: ";
      std::cin >> c;

      std::cout << "m: ";
      std::cin >> m;

      std::cout << "y: ";
      std::cin >> y;

      r = 1.0f - c;
      g = 1.0f - m;
      b = 1.0f - y;

      float h, s, v;
      // Umrechnung von RGB zu HSV
      rgbToHsv(r, g, b, h, s, v);

      std::cout << "\nRGB-Werte:" << std::endl;
      std::cout << "r = " << r << std::endl;
      std::cout << "g = " << g << std::endl;
      std::cout << "b = " << b << std::endl;

      std::cout << "\nHSV-Werte:" << std::endl;
      std::cout << "h = " << h << std::endl;
      std::cout << "s = " << s << std::endl;
      std::cout << "v = " << v << std::endl;
  }
  // Eingabe der HSV Werte
  else if (mode == 3) {

      float h, s, v;

      std::cout << "HSV-Werte eingeben" << std::endl;
      std::cout << "h (0 bis kleiner 360): ";
      std::cin >> h;

      std::cout << "s (0.0 bis 1.0): ";
      std::cin >> s;

      std::cout << "v (0.0 bis 1.0): ";
      std::cin >> v;

      hsvToRgb(h, s, v, r, g, b);

      float c, m, y;

      // Umrechnung von RGB zu CMY
      rgbToCmy(r, g, b, c, m, y);

      std::cout << "\nRGB-Werte:" << std::endl;
      std::cout << "r = " << r << std::endl;
      std::cout << "g = " << g << std::endl;
      std::cout << "b = " << b << std::endl;

      std::cout << "\nCMY-Werte:" << std::endl;
      std::cout << "c = " << c << std::endl;
      std::cout << "m = " << m << std::endl;
      std::cout << "y = " << y << std::endl;
  }


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
