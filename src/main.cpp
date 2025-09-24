#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

// Window
#define WIDTH 800
#define HEIGHT 600
GLFWwindow* window;

// Camera position
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float cameraSpeed = 0.8f;

float deltaTime = 0.0f;	// 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间

// OBJ data
std::vector<float> vertices;
std::vector<unsigned int> indices;
float angle_x = 0.0f, angle_y = 0.0f;
bool is_holding_mouse = false;
float scale = 1.0f;  // 全局縮放因子

// Mouse
double lastX=0,lastY=0;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    if(is_holding_mouse){
        angle_y += float(xpos - lastX);
        angle_x += float(ypos - lastY);
    }
    lastX = xpos;
    lastY = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if(button==GLFW_MOUSE_BUTTON_LEFT){
        if(action==GLFW_PRESS) is_holding_mouse=true;
        else if(action==GLFW_RELEASE) is_holding_mouse=false;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // yoffset > 0: 滾上，縮放放大
    // yoffset < 0: 滾下，縮放縮小
    scale *= (1.0f + 0.1f * (float)yoffset);

    // 限制縮放範圍
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 10.0f) scale = 10.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void processInput(GLFWwindow* window) {
    float currentCameraSpeed = cameraSpeed * deltaTime; // 基于时间的移动速度
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += currentCameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= currentCameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * currentCameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * currentCameraSpeed;
}

std::vector<std::string> split(std::string line, std::string delimiter) {

    std::vector<std::string> splitLine;

    size_t pos = 0;
    std::string token;
    while((pos = line.find(delimiter)) != std::string::npos) {
        token = line.substr(0,pos);
        splitLine.push_back(token);
        line.erase(0, pos + delimiter.length());
    }
    splitLine.push_back(line);

    return splitLine;
}

void normalizeVertices(std::vector<glm::vec3>& vertices) {
    if (vertices.empty()) return;

    // Find min and max values for each axis
    float minX = vertices[0].x, maxX = vertices[0].x;
    float minY = vertices[0].y, maxY = vertices[0].y;
    float minZ = vertices[0].z, maxZ = vertices[0].z;

    for (const auto& vertex : vertices) {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
        minZ = std::min(minZ, vertex.z);
        maxZ = std::max(maxZ, vertex.z);
    }

    // Calculate ranges
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;

    // Find the maximum range to maintain aspect ratio
    float maxRange = std::max({rangeX, rangeY, rangeZ});

    // Calculate center point
    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    // Normalize vertices to [-1, 1] range while maintaining aspect ratio
    float scale = 2.0f / maxRange;
    for (auto& vertex : vertices) {
        vertex.x = (vertex.x - centerX) * scale;
        vertex.y = (vertex.y - centerY) * scale;
        vertex.z = (vertex.z - centerZ) * scale;
    }
}

glm::vec3 read_vec3(std::vector<std::string> words, glm::mat4 preTransform, float w) {
    return glm::vec3(preTransform *
                     glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), w));
}

glm::vec2 read_vec2(std::vector<std::string> words) {
    return glm::vec2(std::stof(words[1]), std::stof(words[2]));
}

void read_face_fixed(std::vector<std::string> words, std::vector<glm::vec3>& v,
                    std::vector<glm::vec2>& vt, std::vector<glm::vec3>& vn,
                    std::vector<float>& vertices) {
    
    size_t triangleCount = words.size() - 3;  // words[0] is "f"

    for (size_t i = 0; i < triangleCount; ++i) {
        // Triangle fan: first vertex, current vertex, next vertex
        size_t idx1 = std::stoi(words[1]) - 1;      // First vertex (anchor)
        size_t idx2 = std::stoi(words[2 + i]) - 1;  // Current vertex
        size_t idx3 = std::stoi(words[3 + i]) - 1;  // Next vertex

        // 获取顶点位置
        glm::vec3 v1 = v[idx1];
        glm::vec3 v2 = v[idx2];
        glm::vec3 v3 = v[idx3];

        // 获取法线，如果没有则计算面法线
        glm::vec3 n1, n2, n3;
        if (!vn.empty() && idx1 < vn.size() && idx2 < vn.size() && idx3 < vn.size()) {
            n1 = vn[idx1];
            n2 = vn[idx2];
            n3 = vn[idx3];
        } else {
            // 计算面法线
            glm::vec3 edge1 = v2 - v1;
            glm::vec3 edge2 = v3 - v1;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
            n1 = n2 = n3 = faceNormal;
        }

        // 默认纹理坐标
        glm::vec2 defaultTexCoord(0.0f, 0.0f);

        // 按照 position(3) + texCoord(2) + normal(3) 的格式存储
        vertices.insert(vertices.end(), {
            v1.x, v1.y, v1.z, defaultTexCoord.x, defaultTexCoord.y, n1.x, n1.y, n1.z,
            v2.x, v2.y, v2.z, defaultTexCoord.x, defaultTexCoord.y, n2.x, n2.y, n2.z,
            v3.x, v3.y, v3.z, defaultTexCoord.x, defaultTexCoord.y, n3.x, n3.y, n3.z
        });
    }
}

// claude
bool loadOBJ(const char* filepath, glm::mat4 preTransform) {
    std::vector<glm::vec3> v;
    std::vector<glm::vec2> vt;
    std::vector<glm::vec3> vn;

    size_t vertexCount = 0;
    size_t texcoordCount = 0;
    size_t normalCount = 0;
    size_t triangleCount = 0;

    std::string line;
    std::vector<std::string> words;

    std::ifstream file;

    // 第一次读取：统计数量
    file.open(filepath);
    while (std::getline(file, line)) {
        words = split(line, " ");

        if (!words[0].compare("v")) {
            ++vertexCount;
        }
        else if (!words[0].compare("vt")) {
            ++texcoordCount;
        }
        else if (!words[0].compare("vn")) {
            ++normalCount;
        }
        else if (!words[0].compare("f")) {
            triangleCount += words.size() - 3;
        }
    }
    file.close();

    v.reserve(vertexCount);
    vt.reserve(texcoordCount);
    vn.reserve(normalCount);
    vertices.reserve(triangleCount * 3 * 8);

    // 第二次读取：加载顶点、纹理坐标、法线
    file.open(filepath);
    while (std::getline(file, line)) {
        words = split(line, " ");

        if (!words[0].compare("v")) {
            v.push_back(read_vec3(words, preTransform, 1.0f));
        }
        else if (!words[0].compare("vt")) {
            vt.push_back(read_vec2(words));
        }
        else if (!words[0].compare("vn")) {
            vn.push_back(read_vec3(words, preTransform, 0.0f));
        }
    }
    file.close();

    // 先归一化顶点
    normalizeVertices(v);

    // 第三次读取：处理面并构建最终顶点数据
    file.open(filepath);
    while (std::getline(file, line)) {
        words = split(line, " ");
        if (!words[0].compare("f")) {
            read_face_fixed(words, v, vt, vn, vertices);
        }
    }
    file.close();
    return true;
}

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main(){
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos,1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main(){
    vec3 ambient = vec3(0.2);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.8);
    vec3 result = ambient + diffuse;
    FragColor = vec4(result,1.0);
}
)";

int main(){
    if(!glfwInit()){ std::cerr<<"GLFW init fail\n"; return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Dino", NULL,NULL);
    if(!window){ std::cerr<<"Window fail\n"; glfwTerminate(); return -1;}
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window,cursor_position_callback);
    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr<<"GLAD init fail\n"; return -1;
    }

    // Load OBJ
    glm::mat4 identity = glm::mat4(1.0f);
    loadOBJ("../models/dino.obj", identity);

    // Shader compile
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertexShaderSource,NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragmentShaderSource,NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // VAO VBO
    unsigned int VBO, VAO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(1);


    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/HEIGHT,0.1f,100.0f);

    while(!glfwWindowShouldClose(window)){
        // 计算时间差
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 处理输入
        processInput(window);

        glClearColor(0.4f,0.4f,0.4f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(scale)); // 加入縮放

        // 預設在一個我喜歡的角度
        model = glm::rotate(model, glm::radians(300.0f), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(315.0f), glm::vec3(0, 0, 1));

        // 滑鼠控制物件旋轉
        model = glm::rotate(model, glm::radians(angle_x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(angle_y), glm::vec3(0, 1, 0));

        // glm::mat4 view = glm::translate(glm::mat4(1.0f),glm::vec3(0, -1.5f, -10));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        unsigned int modelLoc = glGetUniformLocation(shaderProgram,"model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram,"view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram,"projection");
        unsigned int lightLoc = glGetUniformLocation(shaderProgram,"lightPos");
        unsigned int viewPosLoc = glGetUniformLocation(shaderProgram,"viewPos");

        glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));
        glUniform3f(lightLoc,-10.0f,10.0f,100.0f);
        glUniform3f(viewPosLoc,0.0f,0.0f,10.0f);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size()/8);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
