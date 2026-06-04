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

#include "Sphere.h"
#include "Camera.h"

// Standard window width
const int WINDOW_WIDTH  = 2560;
// Standard window height
const int WINDOW_HEIGHT = 1440;
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

Camera camera(glm::vec3(0.0f, 0.0f, 8.0f));

// Unsere globalen Variablen
float rotationStep = 5.0f;          // 5 Grad Rotation pro Tastendruck
float sphereRadius = 1.0f;           // Radius der Kugel

float earthOrbit = 0.0f;
float saturnOrbit = 180.0f;

float sunRotation = 0.0f;
float earthRotation = 0.0f;
float saturnRotation = 0.0f;
float orbitAngle = 0.0f;             // Aktueller Winkel der Umlaufbahn des Planeten

float moon1Orbit = 0.0f;
float moon2Orbit = 0.0f;

float speedFactor = 1.0f;
bool animationRunning = true;
bool showAxes = true;
bool keys[256] = { false };

int lastTime = 0;
float deltaTime = 0.0f;

bool ignoreMouseWarp = false;


int subdivisionDepth = 4;           // Detailgrad der Kugel (Anzahl Unterteilungen der Dreiecke 0-4)

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
Object stars;
Object saturnRing;

GLsizei sphereIndexCount = 0;
GLsizei saturnRingVertexCount = 0;


// Rendert die Kugel mithilfe der MVP-Matrix, des VAOs und des Indexbuffers
void renderSphere(glm::mat4 model)
{
    glm::mat4x4 mvp = projection * view * model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(sphere.vao);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

// Rendert das lokale Koordinatensystem der Kugel mithilfe von Linien
void renderAxes(glm::mat4 model)
{
    glm::mat4x4 mvp = projection * view * model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(axes.vao);
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

void renderStars()
{
    glm::mat4 mvp =
        projection *
        view *
        glm::mat4(1.0f);

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(stars.vao);

    glPointSize(2.0f);

    glDrawArrays(
        GL_POINTS,
        0,
        1000
    );

    glBindVertexArray(0);
}

void renderSaturnRing(glm::mat4 model)
{
    glm::mat4x4 mvp = projection * view * model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(saturnRing.vao);

    glDrawArrays(
        GL_LINES,
        0,
        saturnRingVertexCount
    );

    glBindVertexArray(0);
}

// Erzeugt die Kugelgeometrie aus einem unterteilten Oktaeder
// und initialisiert die benötigten OpenGL-Buffer
void initSphere()
{
    // Speichert alle Vertexpositionen der Kugel
    std::vector<glm::vec3> vertices;

    // Speichert die Reihenfolge der Vertices,
    // aus denen später die Dreiecke erzeugt werden
    std::vector<GLushort> indices;

    // Eckpunkte des ursprünglichen Oktaeders
    // Diese Punkte bilden die Basis der Kugel
    glm::vec3 top(0.0f, 1.0f, 0.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 front(0.0f, 0.0f, 1.0f);
    glm::vec3 left(-1.0f, 0.0f, 0.0f);
    glm::vec3 back(0.0f, 0.0f, -1.0f);
    glm::vec3 bottom(0.0f, -1.0f, 0.0f);

    // Obere Hälfte des Oktaeders.
    // Jede Fläche wird in kleine Dreiecke unterteilt
    // und anschließend auf die Kugel projiziert.
    generateTriangleGrid(vertices, indices, top, front, right, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, left, front, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, back, left, subdivisionDepth);
    generateTriangleGrid(vertices, indices, top, right, back, subdivisionDepth);

    // Untere Hälfte des Oktaeders
    generateTriangleGrid(vertices, indices, bottom, right, front, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, front, left, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, left, back, subdivisionDepth);
    generateTriangleGrid(vertices, indices, bottom, back, right, subdivisionDepth);

    // Speichert die Gesamtanzahl der Indices.
    // Diese Anzahl wird später beim Rendering benötigt.
    sphereIndexCount = static_cast<GLsizei>(indices.size());

    // Debug-Ausgabe der erzeugten Geometrie
    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Indices: " << indices.size() << std::endl;

    // Erzeugt für jeden Vertex dieselbe Farbe.
    // Die Kugel wird dadurch komplett gelb dargestellt.
    const std::vector<glm::vec3> colors(
        vertices.size(),
        glm::vec3(1.0f, 1.0f, 0.0f)
    );

    // OpenGL-ID des aktuellen Shaderprogramms
    GLuint programId = program.getHandle();

    // Variable zum Speichern der Attributpositionen
    GLuint pos;

    // Erzeugt ein Vertex Array Object (VAO).
    // Das VAO speichert die Zuordnung aller Buffer und Attribute.
    glGenVertexArrays(1, &sphere.vao);

    // Aktiviert das VAO der Kugel
    glBindVertexArray(sphere.vao);

    // -----------------------------
    // Positionsbuffer erzeugen
    // -----------------------------

    // Erzeugt einen Vertexbuffer für die Positionen
    glGenBuffers(1, &sphere.positionBuffer);

    // Aktiviert den Buffer als aktuellen Array-Buffer
    glBindBuffer(GL_ARRAY_BUFFER, sphere.positionBuffer);

    // Kopiert alle Vertexpositionen in den GPU-Speicher
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // Fragt die Position des "position"-Attributes im Shader ab
    pos = glGetAttribLocation(programId, "position");

    // Aktiviert das Vertexattribut
    glEnableVertexAttribArray(pos);

    // Beschreibt den Aufbau der Positionsdaten:
    // 3 Floats pro Vertex (x,y,z)
    glVertexAttribPointer(
        pos,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    // -----------------------------
    // Farbbuffer erzeugen
    // -----------------------------

    // Erzeugt einen Buffer für die Farben
    glGenBuffers(1, &sphere.colorBuffer);

    // Aktiviert den Farbbuffer
    glBindBuffer(GL_ARRAY_BUFFER, sphere.colorBuffer);

    // Kopiert die Farbdaten in den GPU-Speicher
    glBufferData(
        GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW
    );

    // Fragt die Position des "color"-Attributes im Shader ab
    pos = glGetAttribLocation(programId, "color");

    // Aktiviert das Farb-Attribut
    glEnableVertexAttribArray(pos);

    // Beschreibt den Aufbau der Farbdaten:
    // 3 Floats pro Farbe (r,g,b)
    glVertexAttribPointer(
        pos,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    // -----------------------------
    // Indexbuffer erzeugen
    // -----------------------------

    // Erzeugt einen Elementbuffer für die Indices
    glGenBuffers(1, &sphere.indexBuffer);

    // Aktiviert den Indexbuffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.indexBuffer);

    // Kopiert die Indexdaten in den GPU-Speicher
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(GLushort),
        indices.data(),
        GL_STATIC_DRAW
    );

    // Deaktiviert das aktuelle VAO
    glBindVertexArray(0);

    // Setzt die Model-Matrix auf die Einheitsmatrix.
    // Die Kugel startet dadurch ohne Rotation oder Verschiebung.
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

void initStars()
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int i = 0; i < 1000; i++)
    {
		float x = ((rand() % 2000) / 10.0f) - 100.0f; // Zufällige x-Position zwischen -100 und 100
		float y = ((rand() % 2000) / 10.0f) - 100.0f; // Zufällige y-Position zwischen -100 und 100
		float z = ((rand() % 2000) / 10.0f) - 100.0f; // Zufällige z-Position zwischen -100 und 100

        vertices.push_back(glm::vec3(x, y, z));

        colors.push_back(
            glm::vec3(1.0f, 1.0f, 1.0f) // Weiße Farbe für die Sterne)      
        );
    }

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &stars.vao);
	glBindVertexArray(stars.vao);

	glGenBuffers(1, &stars.positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, stars.positionBuffer);

	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(glm::vec3),
		vertices.data(),
		GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "position");

    glEnableVertexAttribArray(pos);

    glVertexAttribPointer(
        pos,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    glGenBuffers(1, &stars.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, stars.colorBuffer);

    glBufferData(
        GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "color");

    glEnableVertexAttribArray(pos);

    glVertexAttribPointer(
        pos,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        0
    );

    glBindVertexArray(0);
}

void initSaturnRing()
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;

    const int segments = 200;
    const float innerRadius = 1.0f;
    const float outerRadius = 1.6f;

    for (int i = 0; i < segments; i++)
    {
        float angle1 = 2.0f * 3.1415926f * i / segments;
        float angle2 = 2.0f * 3.1415926f * (i + 1) / segments;

        glm::vec3 inner1(cos(angle1) * innerRadius, 0.0f, sin(angle1) * innerRadius);
        glm::vec3 outer1(cos(angle1) * outerRadius, 0.0f, sin(angle1) * outerRadius);

        glm::vec3 inner2(cos(angle2) * innerRadius, 0.0f, sin(angle2) * innerRadius);
        glm::vec3 outer2(cos(angle2) * outerRadius, 0.0f, sin(angle2) * outerRadius);

        // Outer ring line
        vertices.push_back(outer1);
        vertices.push_back(outer2);

        // Inner ring line
        vertices.push_back(inner1);
        vertices.push_back(inner2);

        // Small radial line
        vertices.push_back(inner1);
        vertices.push_back(outer1);

        for (int j = 0; j < 6; j++)
        {
            colors.push_back(glm::vec3(0.9f, 0.8f, 0.5f));
        }
    }

    saturnRingVertexCount = static_cast<GLsizei>(vertices.size());

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &saturnRing.vao);
    glBindVertexArray(saturnRing.vao);

    glGenBuffers(1, &saturnRing.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, saturnRing.positionBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3 , GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &saturnRing.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, saturnRing.colorBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);
}

// Erstellt die Kamerasicht mithilfe von Position, Blickrichtung und Up-Vektor
void updateView()
{
    view = camera.getViewMatrix();

}


/*
 Initialization. Should return true if everything is ok and false if something went wrong.
 */
bool init()
{
  // OpenGL: Set "background" color and enable depth testing.
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  

  // Switch back to wireframe for the other objects.
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
  initStars();
  initSaturnRing();
  
  return true;
}

/*
 Rendering.
 */
void render()
{
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (keys['w']) camera.processKeyboard('w', deltaTime);
    if (keys['s']) camera.processKeyboard('s', deltaTime);
    if (keys['a']) camera.processKeyboard('a', deltaTime);
    if (keys['d']) camera.processKeyboard('d', deltaTime);
    if (keys[' ']) camera.processKeyboard(' ', deltaTime);
    if (keys['c']) camera.processKeyboard('c', deltaTime);

    updateView();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderStars();


    /*
        Sonne rendern erstellen
    */
    glm::mat4 sunModel = glm::mat4(1.0f);
    sunModel = glm::rotate(
        sunModel,
        glm::radians(sunRotation),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    float sunPulse = 0.7f + 0.05f * sin(glm::radians(sunRotation * 3.0f));

    sunModel == glm::scale(
        sunModel,
        glm::vec3(sunPulse)
    );

    // Render sun filled
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    renderSphere(sunModel);

    // Render a larger wireframe glow around the sun
    glm::mat4 sunGlowModel = glm::mat4(1.0f);

    sunGlowModel = glm::scale(
        sunGlowModel,
        glm::vec3(0.95f)
    );

    // Switch back to wireframe for planets
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    renderSphere(sunGlowModel);


    /* Erde
    * 
    * 
    */
    glm::mat4 earth = glm::mat4(1.0f);

    // Umlaufbahn um die Sonne
    earth = glm::rotate(
        earth,
        glm::radians(orbitAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Abstand zur Sonne
    earth = glm::translate(
        earth,
        glm::vec3(5.0f, 0.0f, 0.0f)
    );

    // Achsneigung um 45 Grad
    earth = glm::rotate(
        earth,
		glm::radians(45.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Eigenrotation des Planeten
    earth = glm::rotate(
        earth,
        glm::radians(earthRotation),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

	// Größe des Planeten
    earth = glm::scale(
        earth,
        glm::vec3(0.3f)
    );

    renderSphere(earth);

    /* Mond 1
    * 
    * 
    */

    glm::mat4 moon1Model = earth;

    moon1Model = glm::rotate(
        moon1Model,
        glm::radians(moon1Orbit),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    moon1Model = glm::translate(
        moon1Model,
        glm::vec3(2.0f, 0.0f, 0.0f)
    );

    moon1Model = glm::scale(
        moon1Model,
        glm::vec3(0.4f)
    );

    renderSphere(moon1Model);

    /* Saturn
     *
     */
    float saturnAngle = glm::radians(saturnOrbit);

    float saturnX = cos(saturnAngle) * 50.0f;
    float saturnZ = sin(saturnAngle) * 30.0f;

    // Base model contains Saturn's position and rotation,
    // but not the planet size.
    glm::mat4 saturnBaseModel = glm::mat4(1.0f);

    saturnBaseModel = glm::translate(
        saturnBaseModel,
        glm::vec3(saturnX, 0.0f, saturnZ)
    );

    saturnBaseModel = glm::rotate(
        saturnBaseModel,
        glm::radians(saturnRotation),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Saturn sphere
    glm::mat4 saturnModel = saturnBaseModel;

    saturnModel = glm::scale(
        saturnModel,
        glm::vec3(0.45f)
    );

    renderSphere(saturnModel);

    // Saturn ring
    glm::mat4 saturnRingModel = saturnBaseModel;

    saturnRingModel = glm::rotate(
        saturnRingModel,
        glm::radians(20.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    saturnRingModel = glm::scale(
        saturnRingModel,
        glm::vec3(0.7f)
    );

    renderSaturnRing(saturnRingModel);

    /* Mond 2
    *
    */

    glm::mat4 moon2Model = saturnBaseModel;

    // Mond umläuft Planet 2
    moon2Model = glm::rotate(
        moon2Model,
        glm::radians(moon2Orbit),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Abstand zum Planeten
    moon2Model = glm::translate(
        moon2Model,
        glm::vec3(3.0f, 0.0f, 0.0f)
    );

	// Größe des Mondes
    moon2Model = glm::scale(
        moon2Model,
        glm::vec3(0.4f)
    );

    renderSphere(moon2Model);

    
    if (showAxes)
    {
        glDisable(GL_DEPTH_TEST);

        renderAxes(sunModel);
        renderAxes(earth);
        renderAxes(moon1Model);
        renderAxes(saturnBaseModel);
        renderAxes(moon2Model);

        glEnable(GL_DEPTH_TEST);
    }
    

    if (animationRunning)
    {
        orbitAngle += 0.2f * speedFactor;

        sunRotation += 0.1f * speedFactor;
        earthRotation += 1.0f * speedFactor;
        saturnRotation += 0.8f * speedFactor;

        saturnOrbit += 0.04f * speedFactor;
        moon1Orbit += 2.0f * speedFactor;
        moon2Orbit += 1.5f * speedFactor;
    }

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



void glutMouseMove(int x, int y)
{
    if (ignoreMouseWarp)
    {
        ignoreMouseWarp = false;
        return;
    }

    camera.processMouseMovement(x, y);
    updateView();

    ignoreMouseWarp = true;
    glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    glutPostRedisplay();
}

void glutKeyboardDown(unsigned char keycode, int x, int y)
{
    if (!keys[keycode])
    {
        if (keycode == 'k')
        {
            showAxes = !showAxes;
            std::cout << "showAxes = " << showAxes << std::endl;
        }
    }

    keys[keycode] = true;

    if (keycode == 27)
        glutDestroyWindow(glutID);
}

void glutKeyboardUp(unsigned char keycode, int x, int y)
{
    keys[keycode] = false;
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
  
  glutCreateWindow("Space");
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
  glutIdleFunc(glutDisplay);

  glutKeyboardFunc(glutKeyboardDown);
  glutKeyboardUpFunc(glutKeyboardUp);

  glutPassiveMotionFunc(glutMouseMove);

  glutSetCursor(GLUT_CURSOR_NONE);
  glutWarpPointer(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  lastTime = glutGet(GLUT_ELAPSED_TIME);
 

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
