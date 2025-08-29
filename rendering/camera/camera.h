#ifndef CAMERA_H_
#define CAMERA_H_

#include <quokka/qk_math.h>

class Camera {
public:
    Camera(const glm::vec3& position, float aspectRatio);
   ~Camera();

    void Update();

    void SetPosition(const glm::vec3& pos);
    void SetDirection(const glm::vec3& dir);

    void SetFov(float fov);
    void SetAspectRatio(float aspectRatio);
    void SetNear(float near);
    void SetFar(float far);

    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;

private:
    void MarkViewDirty() { viewDirty = true; }
    void MarkProjectionDirty() { projectionDirty = true; }

    void UnmarkViewDirty() { viewDirty = false; }
    void UnmarkProjectionDirty() { projectionDirty = false; }

    bool viewDirty = false;
    bool projectionDirty = false;

    /* 相机核心参数 */
    glm::vec3 position        = { 0.0f, 0.0f,  3.0f };
    glm::vec3 direction       = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up              = { 0.0f, 1.0f,  0.0f };

    float fov                 = 45.0f;
    float aspectRatio         = 16.0f / 9.0f;
    float near                = 0.01f;
    float far                 = 1000.0f;

    glm::mat4 projection      = glm::mat4(1.0f);
    glm::mat4 view            = glm::mat4(1.0f);

};

#endif /* CAMERA_H_ */
