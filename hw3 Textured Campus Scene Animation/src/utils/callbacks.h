#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// 這些 extern 代表 main.cpp 定義的全域變數
extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;
extern float cameraSpeed;

extern float yaw;         // 偏航角 (左右旋轉)
extern float pitch;       // 俯仰角 (上下旋轉)
extern float sensitivity; // 滑鼠靈敏度

extern bool is_holding_mouse;
extern bool firstMouse;
extern float lastX, lastY;

extern float deltaTime;
extern float lastFrame;

extern float fov;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void process_input(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

#endif
