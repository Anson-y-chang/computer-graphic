#include "callbacks.h"
#include <iostream>

// glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);     // 相機位置
glm::vec3 cameraPos = glm::vec3(0.14864, -0.021445, -0.02515); // 相機位置
// glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -0.05f);            // 相機朝向
glm::vec3 cameraFront = glm::vec3(-0.00572018, -0.0588436, 0.998251); // 相機朝向
glm::vec3 cameraUp = glm::vec3(0.0f, 0.05f, 0.0f);                    // 上方向
float cameraSpeed = 0.5f;

float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;

bool is_holding_mouse = false;
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float fov = 45.0f;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
    {
      is_holding_mouse = true;
      firstMouse = true; // ✓ 重要:每次按下時重置
    }
    else if (action == GLFW_RELEASE)
    {
      is_holding_mouse = false;
    }
  }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
  if (is_holding_mouse)
  {
    if (firstMouse)
    {
      lastX = xpos;
      lastY = ypos;
      firstMouse = false;
      return; // ✓ 第一幀不計算偏移,直接返回
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // 限制俯仰角,避免翻轉
    if (pitch > 89.0f)
      pitch = 89.0f;
    if (pitch < -89.0f)
      pitch = -89.0f;

    // 更新相機朝向向量
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);

    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }
}

// ========== 滑鼠滾輪回調 (FOV 縮放) ==========
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
  fov -= (float)yoffset;
  if (fov < 1.0f)
    fov = 1.0f;
  if (fov > 45.0f)
    fov = 45.0f;
}

// ========== 鍵盤輸入處理 ==========
void process_input(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  float velocity = cameraSpeed * deltaTime;

  // W - 前進
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    cameraPos += velocity * cameraFront;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }

  // S - 後退
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    cameraPos -= velocity * cameraFront;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }
  // A - 左移
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {

    // cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    cameraPos -= glm::cross(cameraFront, cameraUp) * velocity;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }
  // D - 右移
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    // cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    cameraPos += glm::cross(cameraFront, cameraUp) * velocity;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }
  // Space - 上升
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    cameraPos += velocity * cameraUp;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }
  // Left Shift - 下降
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
  {
    cameraPos -= velocity * cameraUp;
    std::cout << "camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
    std::cout << "camera front: " << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << std::endl;
    std::cout << "camera up: " << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << std::endl;
  }

  // Left Control - 加速
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    cameraSpeed = 1.0f;
  else
    cameraSpeed = 0.5f;
}

// 關閉視窗
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}