#include "camera.h"

Camera::Camera(float x, float y, float z, float vAspectRatio) : position(x, y, z)
{
    SetAspectRatio(vAspectRatio);
}

Camera::~Camera()
{
    /* do nothing... */
}

void Camera::Update()
{
    direction = glm::normalize(direction);
    view = glm::lookAt(position, position + direction, up);
}

void Camera::SetPosition(float x, float y, float z)
{
    this->position = glm::vec3(x, y, z);
}

void Camera::SetFov(float vFov)
{
    this->fov = vFov;
    UpdateProjection();
}

void Camera::SetAspectRatio(float vAspectRatio)
{
    this->aspectRatio = vAspectRatio;
    UpdateProjection();
}

void Camera::SetFar(float vFar)
{
    this->far = vFar;
    UpdateProjection();
}

void Camera::SetNear(float vNear)
{
    this->near = vNear;
    UpdateProjection();
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return projection;
}

glm::mat4 Camera::GetViewMatrix() const
{
    return view;
}

void Camera::UpdateProjection()
{
    projection = glm::perspective(fov, aspectRatio, near, far);
    projection[1][1] *= -1;
}

