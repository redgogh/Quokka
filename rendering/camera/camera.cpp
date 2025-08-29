#include "camera.h"

Camera::Camera(const glm::vec3& position, float aspectRatio)
{
    SetPosition(position);
    SetAspectRatio(aspectRatio);
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
        const glm::vec3 dir = glm::normalize(direction);
        const glm::vec3 target = position + dir;
        view = glm::lookAt(position, target, up);
        UnmarkViewDirty();
    }

    if (projectionDirty) {
        projection = glm::perspective(glm::radians(fov), aspectRatio, near, far);
        projection[1][1] *= -1;
        UnmarkProjectionDirty();
    }
}

void Camera::SetPosition(const glm::vec3 &pos)
{
    this->position = pos;
    MarkViewDirty();
}

void Camera::SetDirection(const glm::vec3 &dir)
{
    this->direction = dir;
    MarkViewDirty();
}

void Camera::SetFov(float fov)
{
    this->fov = fov;
    MarkProjectionDirty();
}

void Camera::SetAspectRatio(float aspectRatio)
{
    this->aspectRatio = aspectRatio;
    MarkProjectionDirty();
}

void Camera::SetFar(float far)
{
    this->far = far;
    MarkProjectionDirty();
}

void Camera::SetNear(float near)
{
    this->near = near;
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

