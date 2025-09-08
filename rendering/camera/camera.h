#ifndef CAMERA_H_
#define CAMERA_H_

#include <quokka/qk_math.h>

class Camera {
public:
    Camera(float x, float y, float z);
    Camera(float x, float y, float z, float zNear, float zFar);
    Camera(float x, float y, float z, float fov, float zNear, float zFar);
   ~Camera() = default;

    void Update();

    void Move(float x, float y, float z);

    void SetFov(float v_fov) { fov = v_fov; }
    void SetAspectRatio(float v_aspect) { aspect = v_aspect; }
    void SetZNear(float v_zNear) { zNear = v_zNear; }
    void SetZFar(float v_zFar) { zFar = v_zFar; }

   glm::vec3& GetPosition() { return position; }
   float GetFov() const { return fov; }
   float GetAspectRatio() const { return aspect; }
   float GetZNear() const { return zNear; }
   float GetZFar() const { return zFar; }

   const glm::mat4& GetViewMatrix() const { return viewMatrix; }
   const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

private:
    /* parameters */
    glm::vec3 position = { 0.0f, 0.0f, -3.0f };
    glm::vec3 forward = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up = { 0.0f, 1.0f, 0.0f };

    float fov = 60.0f;
    float aspect = 1.6f;
    float zNear = 0.1f;
    float zFar = 1000.0f;

    /* matrix */
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);

    float velocity = 0.05f;
};

#endif /* CAMERA_H_ */
