#include "Camera.h"

#include <cmath>

Camera::Camera(glm::vec3 startPosition)
	: position(startPosition),
	front(glm::vec3(0.0f, 0.0f, -1.0f)),
	up(glm::vec3(0.0f, 1.0f, 0.0f)),
	right(glm::vec3(1.0f, 0.0f, 0.0f)),
	worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	yaw(-90.0f), // Blick in -Z-Richtung
	pitch(0.0f),
    movementSpeed(1.2f),
    mouseSensitivity(0.04f),
	lastX(0.0f),
	lastY(0.0f),
	firstMouse(true)
{
	updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
	// The camera looks from its position into the direction of front;
	return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(unsigned char key, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;

    if (key == 'w')
        position += front * velocity;
    else if (key == 's')
        position -= front * velocity;
    else if (key == 'a')
        position -= right * velocity;
    else if (key == 'd')
        position += right * velocity;
    else if (key == ' ')
        position += worldUp * velocity;
    else if (key == 'c')
        position -= worldUp * velocity;
}

void Camera::processMouseMovement(int xpos, int ypos)
{
    // The mouse is always warped back to the center of the window.
    // Therefore, the offset is measured relative to the window center.
    const float centerX = 2560.0f / 2.0f;
    const float centerY = 1440.0f / 2.0f;

    float xoffset = static_cast<float>(xpos) - centerX;
    float yoffset = centerY - static_cast<float>(ypos);

    // Apply sensitivity so the camera does not rotate too fast.
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    // Update camera angles.
    yaw += xoffset;
    pitch += yoffset;

    // Limit vertical rotation to avoid flipping the camera.
    if (pitch > 89.0f)
    {
        pitch = 89.0f;
    }

    if (pitch < -89.0f)
    {
        pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
	// Calculate the new front vector
	glm::vec3 newFront;
	newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	newFront.y = sin(glm::radians(pitch));
	newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(newFront);
	// Recalculate right and up vectors
	right = glm::normalize(glm::cross(front, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	up = glm::normalize(glm::cross(right, front));
}