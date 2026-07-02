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

#include "OBJLoader.h"

// Standard window width
const int WINDOW_WIDTH  = 1280;
// Standard window height
const int WINDOW_HEIGHT = 720;
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
float cameraDistance = 4.0f;        // Kamera distanz
float sphereRadius = 1.0f;           // Radius der Kugel

float planet1Rotation = 0.0f;       // Aktueller Winkel der Eigenrotation
float planet2Rotation = 0.0f;
float orbitAngle = 0.0f;             // Aktueller Winkel der Umlaufbahn des Planeten

float moon1Orbit = 0.0f;            // Umlaufwinkel von Mond1 zu Planet 1
float moon2Orbit = 0.0f;            

float speedFactor = 0.5f;           // Globaler Geschwindigkeitsfaktor für alle Animationen
bool animationRunning = false;
bool showAxes = false;               // komplettes Koordinatensystem
bool showAxisLines = true;          // einzelne Rotationsachsen


int subdivisionDepth = 4;           // Detailgrad der Kugel (Anzahl Unterteilungen der Dreiecke 0-4)
GLsizei sphereIndexCount = 0;

GLuint starDestroyerVAO = 0;
GLuint starDestroyerVBO = 0;
int starDestroyerVertexCount = 0;

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
Object axisLine;

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

void renderStarDestroyer(glm::mat4 model)
{
glm::mat4x4 mvp = projection * view * model;

program.use();
program.setUniform("mvp", mvp);

glBindVertexArray(starDestroyerVAO);
glDrawArrays(GL_TRIANGLES, 0, starDestroyerVertexCount);
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

void initAxisLine()
{
    const std::vector<glm::vec3> vertices =
    {
        glm::vec3(0.0f, -1.5f, 0.0f),
        glm::vec3(0.0f,  1.5f, 0.0f)
    };

    const std::vector<glm::vec3> colors =
    {
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    };

    const std::vector<GLushort> indices =
    {
        0, 1
    };

    GLuint programId = program.getHandle();
    GLuint pos;

    glGenVertexArrays(1, &axisLine.vao);
    glBindVertexArray(axisLine.vao);

    glGenBuffers(1, &axisLine.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, axisLine.positionBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(glm::vec3),
        vertices.data(),
        GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "position");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &axisLine.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, axisLine.colorBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        colors.size() * sizeof(glm::vec3),
        colors.data(),
        GL_STATIC_DRAW
    );

    pos = glGetAttribLocation(programId, "color");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &axisLine.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisLine.indexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(GLushort),
        indices.data(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
}

bool initStarDestroyer()
{
    OBJLoader loader;

    if (!loader.load("StarDestroyer.obj")) {
        return false;
    }

    starDestroyerVertexCount = static_cast<int>(loader.meshVertices.size());

    std::vector<float> data;

    for (size_t i = 0; i < loader.meshVertices.size(); ++i) {
        glm::vec3 position = loader.meshVertices[i];
        glm::vec3 normal = loader.meshNormals[i];

		data.push_back(position.x);
        data.push_back(position.y);
        data.push_back(position.z);

        data.push_back(normal.x);
        data.push_back(normal.y);
        data.push_back(normal.z);
    }

    glGenVertexArrays(1, &starDestroyerVAO);
    glBindVertexArray(starDestroyerVAO);

    glGenBuffers(1, &starDestroyerVBO);
    glBindBuffer(GL_ARRAY_BUFFER, starDestroyerVBO);

    glBufferData(GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float),
		reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
		6 * sizeof(float), 
		reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

// Erstellt die Kamerasicht mithilfe von Position, Blickrichtung und Up-Vektor
void updateView()
{
	glm::vec3 eye(0.0f, 0.0f, cameraDistance);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    view = glm::lookAt(eye, center, up);
}

void renderAxisLine(glm::mat4 model)
{
    glm::mat4x4 mvp = projection * view * model;

    program.use();
    program.setUniform("mvp", mvp);

    glBindVertexArray(axisLine.vao);
    glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, 0);
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
  initAxisLine();

  if (!initStarDestroyer()) {
      return false;
  }
  return true;
}

/*
 Rendering.
 */
void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Startet mit einer Einheitsmatrix als Ausgang für alle Transformationen der Sonne.
    glm::mat4 sunModel = glm::mat4(1.0f);
   

    // Skalierung der Sonne
    sunModel = glm::scale(sunModel, glm::vec3(0.7f));

    renderSphere(sunModel);

    // Sternenzerstörer
    glm::mat4 destroyerModel = glm::mat4(1.0f);

    destroyerModel = glm::rotate(
        destroyerModel,
        glm::radians(orbitAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    destroyerModel = glm::translate(
        destroyerModel, 
        glm::vec3(4.0f, 0.0f, 0.0f)
    );

    destroyerModel = glm::scale(
        destroyerModel, 
        glm::vec3(0.2f)
    );

    renderStarDestroyer(destroyerModel);


    /* Planet 1
    * 
    * 
    */

    // Basismodell des Planeten, enthält Position und Rotation, jedoch keine Skalierung 
    glm::mat4 planet1BaseModel = glm::mat4(1.0f);

    // Roatation um die Sonne, dadurch entsteht die Umlaufbewegung des Planeten.
    planet1BaseModel = glm::rotate(
        planet1BaseModel,
        glm::radians(orbitAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Abstand des Planeten zur sonne
    planet1BaseModel = glm::translate(
        planet1BaseModel,
        glm::vec3(2.0f, 0.0f, 0.0f)
    );

    // Achsenneigung des Planeten
    planet1BaseModel = glm::rotate(
        planet1BaseModel,
        glm::radians(45.0f),            // Die Rotattionsachse wird um 45Grad geneigt
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Eigenrotation des Planeten
    planet1BaseModel = glm::rotate(
        planet1BaseModel,
        glm::radians(planet1Rotation),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );


    // Erzeugt das endgültige Modell des Planeten, Basistransformation bleibt erhalten
    glm::mat4 planet1Model = planet1BaseModel;

    // Skaliert die Kugel auf Planetengröße
    planet1Model = glm::scale(
        planet1Model,
        glm::vec3(0.3f)
    );

    // Zeichnet den Planeten
    renderSphere(planet1Model);

    /* Mond 1
    * 
    * 
    */

    // Der Mond startet im Koordinatensystem von Planet1
    glm::mat4 moon1Model = planet1BaseModel;

    // Umlaufbewegung des Mond1 um Planet1, dadurch das wir von planet1BaseModel starten, passiert die Rotation lokal um den Planeten
    moon1Model = glm::rotate(
        moon1Model,
        glm::radians(moon1Orbit),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Abstand des Mondes zu Planet1, der Mond wird aus dem Mittelpunkt des Planeten herausverschoben
    moon1Model = glm::translate(
        moon1Model,
        glm::vec3(0.8f, 0.0f, 0.0f)
    );

    // Größe des Mondes
    moon1Model = glm::scale(
        moon1Model,
        glm::vec3(0.12f)
    );

    // Zeichne den Mond
    renderSphere(moon1Model);

    /* Planet 2
    * 
    * 
    */
    glm::mat4 planet2BaseModel = glm::mat4(1.0f);

    // Opposite orbit position
    planet2BaseModel = glm::rotate(
        planet2BaseModel,
        glm::radians(orbitAngle + 180.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Abstand zur Sonne
    planet2BaseModel = glm::translate(
        planet2BaseModel,
        glm::vec3(2.0f, 0.0f, 0.0f)
    );

    // Rotation
    planet2BaseModel = glm::rotate(
        planet2BaseModel,
        glm::radians(planet2Rotation),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Planeten skalierung
    glm::mat4 planet2Model = planet2BaseModel;
    planet2Model = glm::scale(
        planet2Model,
        glm::vec3(0.3f)
    );

    renderSphere(planet2Model);

    /* Mond 2
    *
    */

    glm::mat4 moon2Model = planet2BaseModel;

    moon2Model = glm::rotate(
        moon2Model,
        glm::radians(moon2Orbit),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    moon2Model = glm::translate(
        moon2Model,
        glm::vec3(0.8f, 0.0f, 0.0f)
    );

    moon2Model = glm::scale(
        moon2Model,
        glm::vec3(0.12f)
    );

    renderSphere(moon2Model);

   

    glDisable(GL_DEPTH_TEST);

    if (showAxes)
    {
        renderAxes(sunModel);

        renderAxes(planet1Model);
        renderAxes(moon1Model);

        renderAxes(planet2Model);
        renderAxes(moon2Model);
    }

    if (showAxisLines)
    {
        renderAxisLine(sunModel);

        renderAxisLine(planet1Model);
        renderAxisLine(moon1Model);

        renderAxisLine(planet2Model);
        renderAxisLine(moon2Model);
    }

    glEnable(GL_DEPTH_TEST);

    if (animationRunning)
    {
        orbitAngle += 0.2f * speedFactor;

        planet1Rotation += 1.0f * speedFactor;
        planet2Rotation += 0.8f * speedFactor;

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

/*
 Callback for char input.
 */
void glutKeyboard(unsigned char keycode, int x, int y)
{
    switch (keycode) {
    case 27: // ESC
        glutDestroyWindow(glutID);
        return;

        // Änderung des Subdivison-Levels
    case '+':

        if (subdivisionDepth < 6)
        {
            subdivisionDepth++;

            std::cout << "Subdivision Depth: "
                << subdivisionDepth << std::endl;

            initSphere();
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
        }
        break;

        // Zurücksetzen der ganzen Szene
    case 'n':
        cameraDistance = 4.0f;

        orbitAngle = 0.0f;

        planet1Rotation = 0.0f;
        planet2Rotation = 0.0f;

        moon1Orbit = 0.0f;
        moon2Orbit = 0.0f;

        speedFactor = 0.5f;
        animationRunning = false;

        updateView();

        std::cout << "Szene zurückgesetzt" << std::endl;
        break;

        // Skalierung der Kugel
    case 'r':
        if (sphereRadius > 0.3f)
        {
            sphereRadius -= 0.1f;
            initSphere();
            initAxes();
        }
        break;

        // Skalierung der Kugel
    case 'R':
        if (sphereRadius < 2.0f)
        {
            sphereRadius += 0.1f;
            initSphere();
            initAxes();
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

    case 'h':
        showAxes = !showAxes;
        std::cout << "Koordinatensysteme: " << showAxes << std::endl;
        break;

    case 'j':
        showAxisLines = !showAxisLines;
        std::cout << "Achsenlinien: " << showAxisLines << std::endl;
        break;

    case 'd':
        speedFactor -= 0.1f;

        if (speedFactor < 0.1f)
        {
            speedFactor = 0.1f;
        }

        std::cout << "Speed: " << speedFactor << std::endl;
        break;

    case 'f':
        speedFactor += 0.1f;

        if (speedFactor > 5.0f)
        {
            speedFactor = 5.0f;
        }

        std::cout << "Speed: " << speedFactor << std::endl;
        break;

    case 'g':
        animationRunning = !animationRunning;
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
  
  glutCreateWindow("Aufgabenblatt 04");
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
  glutIdleFunc(glutDisplay);
  
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
