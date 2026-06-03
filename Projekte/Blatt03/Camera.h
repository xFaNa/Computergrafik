#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	// Creates a camera at a given world position
	Camera(glm::vec3 startPosition);

	//Returns the current view matrix for rendering
	glm::mat4 getViewMatrix() const;

	void processKeyboard(unsigned char key, float deltaTime);

	// Rotates the camera based on mouse movement
	void processMouseMovement(int xpos, int ypos);


private:
	//Recalculates the front vector from yaw and pitch
	void updateCameraVectors();


private:
	// Camera position in world space
	glm::vec3 position;

	// Direction the camera is looking at
	glm::vec3 front;

	// Up direction of the camera
	glm::vec3 up;

	//Right direction of the camera
	glm::vec3 right;

	// Fixed world up direction
	glm::vec3 worldUp;

	// Horizontal rotation angle (yaw)
	float yaw;

	// Vertical rotation angle (pitch)
	float pitch;

	// Movement speed per key press
	float movementSpeed;

	// Mouse sensitivity
	float mouseSensitivity;

	// Last mouse position
	float lastX;
	float lastY;

	// Used to avoid a large camera jump on the first mouse event
	bool firstMouse;
};
