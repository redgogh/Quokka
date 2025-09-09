#include "camera.h"

Camera::Camera(float x, float y, float z)
{
    this->position = { x, y, z };
}

Camera::Camera(float x, float y, float z, float zNear, float zFar)
{
    this->position = { x, y, z };
    this->zNear = zNear;
    this->zFar = zFar;
}

Camera::Camera(float x, float y, float z, float fov, float zNear, float zFar)
{
    this->position = { x, y, z };
    this->fov = fov;
    this->zNear = zNear;
    this->zFar = zFar;
}

void Camera::Update()
{
    viewMatrix = glm::lookAt(position, position + forward, up);
    projectionMatrix = glm::perspectiveRH_ZO(glm::radians(fov), aspect, zNear, zFar);
}

void Camera::Move(float x, float y, float z)
{
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 worldUp = glm::normalize(glm::cross(right, forward));

    position += forward * x * velocity;
    position += right * y * velocity;
    position += worldUp * z * velocity;
}