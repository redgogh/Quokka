#ifndef CAMERA_H_
#define CAMERA_H_

#include <quokka/qk_math.h>

class Camera {
public:
    Camera(float x, float y, float z);
   ~Camera();

private:
    glm::vec3 position  = { 0.0f, 0.0f,  3.0f };
    glm::vec3 direction = { 0.0f, 0.0f, -1.0f };
    glm::vec3 up        = { 0.0f, 1.0f,  0.0f };

    float fov           = 90.0f;
    float aspectRatio   = 16.0f / 9.0f;
    float near          = 0.01f;
    float far           = 1000.0f;
};

#endif /* CAMERA_H_ */
