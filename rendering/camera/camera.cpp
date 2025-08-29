#include "camera.h"

Camera::Camera(float x, float y, float z, float vAspectRatio) : position(x, y, z)
{
    SetAspectRatio(vAspectRatio);
    MarkViewDirty();
    MarkProjectionDirty();
    Update();
}

Camera::~Camera()
{
    /* do nothing... */
}

void Camera::Update()
{
    if (viewDirty) {
        direction = glm::normalize(direction);
        view = glm::lookAt(position, position + direction, up);
        UnmarkViewDirty();
    }

    if (projectionDirty) {
        projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);
        projection[1][1] *= -1;
        UnmarkProjectionDirty();
    }
}

void Camera::Move(float x, float y, float z)
{
    SetPosition(position.x + x, position.y + y, position.z + z);
}

void Camera::SetPosition(float x, float y, float z)
{
    this->position = glm::vec3(x, y, z);
    MarkViewDirty();
}

void Camera::SetFov(float vFov)
{
    this->fov = vFov;
    MarkProjectionDirty();
}

void Camera::SetAspectRatio(float vAspectRatio)
{
    this->aspectRatio = vAspectRatio;
    MarkProjectionDirty();
}

void Camera::SetFar(float vFar)
{
    this->far = vFar;
    MarkProjectionDirty();
}

void Camera::SetNear(float vNear)
{
    this->near = vNear;
    MarkProjectionDirty();
}

const glm::mat4& Camera::GetProjectionMatrix() const
{
    return projection;
}

const glm::mat4& Camera::GetViewMatrix() const
{
    return view;
}

