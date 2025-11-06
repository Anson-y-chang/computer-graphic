#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils/utils.h"
#include "utils/callbacks.h"
#include "utils/camera_path.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
// #include <unistd.h>
// #include <filesystem>

// Window
#define WIDTH 800
#define HEIGHT 600
GLFWwindow *window;
CameraPath mainPath;
bool useManual = true;
bool usePathCamera = !useManual; // 是否使用路徑相機
bool manualControl = useManual;  // 手動控制模式

// load shader
std::string vertexCode = load_shader_source("../src/shaders/vertex.glsl");
std::string fragmentCode = load_shader_source("../src/shaders/fragment.glsl");
const char *vertexShaderSource = vertexCode.c_str();
const char *fragmentShaderSource = fragmentCode.c_str();

void normalize_vertices(std::vector<glm::vec3> &vertices)
{
  if (vertices.empty())
    return;

  // Find min and max values for each axis
  float minX = vertices[0].x, maxX = vertices[0].x;
  float minY = vertices[0].y, maxY = vertices[0].y;
  float minZ = vertices[0].z, maxZ = vertices[0].z;

  for (const auto &vertex : vertices)
  {
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
  for (auto &vertex : vertices)
  {
    vertex.x = (vertex.x - centerX) * scale;
    vertex.y = (vertex.y - centerY) * scale;
    vertex.z = (vertex.z - centerZ) * scale;
  }
}

glm::vec3 read_vec3(std::vector<std::string> words, glm::mat4 preTransform, float w)
{
  return glm::vec3(preTransform *
                   glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), w));
}

// opengl 紋理座標系與圖片不同，y 軸要翻轉
glm::vec2 read_vec2(std::vector<std::string> words)
{
  return glm::vec2(std::stof(words[1]), 1.0f - std::stof(words[2]));
}

// word: f v:vec3/vt:vec2/vn:vec3x3
void read_face(std::vector<std::string> words,
               std::vector<glm::vec3> &v,
               std::vector<glm::vec2> &vt,
               std::vector<glm::vec3> &vn,
               std::vector<float> &vertices)
{
  size_t triangleCount = words.size() - 3;
  for (size_t i = 0; i < triangleCount; ++i)
  {
    int idxSet[3][3];
    // 記錄 face 每個點的 index 在 idSet, 要 -1
    for (int k = 0; k < 3; ++k)
    {
      std::string w = words[k == 0 ? 1 : 2 + i + (k - 1)];
      std::vector<std::string> parts = split(w, "/");

      idxSet[k][0] = std::stoi(parts[0]) - 1;
      idxSet[k][1] = std::stoi(parts[1]) - 1;
      idxSet[k][2] = std::stoi(parts[2]) - 1;
      // idxSet[k][1] = parts.size() > 1 && !parts[1].empty() ? std::stoi(parts[1]) - 1 : -1;
      // idxSet[k][2] = parts.size() > 2 ? std::stoi(parts[2]) - 1 : -1;
    }

    if (vn.size() > 0)
    {
      for (int k = 0; k < 3; ++k)
      {
        glm::vec3 pos = v[idxSet[k][0]];
        glm::vec2 tex = vt[idxSet[k][1]];
        glm::vec3 norm = vn[idxSet[k][2]];
        vertices.insert(vertices.end(), {pos.x, pos.y, pos.z, tex.x, tex.y, norm.x, norm.y, norm.z});
      }
    }
    else
    {
      // obj 沒給的話，要自己計算的 vertice normal
      glm::vec3 pos0 = v[idxSet[0][0]];
      glm::vec3 pos1 = v[idxSet[1][0]];
      glm::vec3 pos2 = v[idxSet[2][0]];

      glm::vec3 edge1 = pos1 - pos0;
      glm::vec3 edge2 = pos2 - pos0;
      glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

      for (int k = 0; k < 3; ++k)
      {
        glm::vec3 pos = v[idxSet[k][0]];
        glm::vec2 tex = vt[idxSet[k][1]];
        // glm::vec2 tex = idxSet[k][1] >= 0 && idxSet[k][1] < vt.size() ? vt[idxSet[k][1]] : glm::vec2(0.0f);
        glm::vec3 norm = faceNormal;

        vertices.insert(vertices.end(), {pos.x, pos.y, pos.z, tex.x, tex.y, norm.x, norm.y, norm.z});
      }
    }
  }
}

struct Material
{
  std::string name;
  glm::vec3 Ka{0.0f};             // ambient
  glm::vec3 Kd{0.0f};             // diffuse
  glm::vec3 Ks{0.0f};             // specular
  glm::vec3 Ke{0.0f};             // emissive
  float Ns = 0.0f;                // shininess
  float Ni = 1.0f;                // optical density (refraction)
  float d = 1.0f;                 // dissolve
  int illum = 0;                  // illumination model
  std::string diffuseTexPath;     //
  std::string normalTexPath;      //
  std::string specularTexPath;    //
  std::string alphaTexPath;       //
  unsigned int diffuseTexID = 0;  //
  unsigned int specularTexID = 0; //
};

struct Mesh
{
  std::vector<float> vertices;
  Material *material;
  unsigned int VAO, VBO;
};

std::vector<Mesh> meshes;
std::map<std::string, Material> g_materials;

void load_mtl(const std::string &mtlPath, std::map<std::string, Material> &materials)
{
  std::filesystem::path mtlFsPath(mtlPath);
  std::filesystem::path baseDir = mtlFsPath.parent_path();

  std::ifstream file;
  // file.open("../models/tiger/tiger.mtl");
  file.open(mtlPath);
  if (!file.is_open())
  {
    std::cerr << "Failed to open MTL: " << mtlPath << std::endl;
    return;
  }

  Material currentMtl;
  std::string line;
  while (std::getline(file, line))
  {
    if (line.empty())
      continue;

    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "newmtl")
    {
      // save previous
      if (!currentMtl.name.empty())
      {
        materials[currentMtl.name] = currentMtl;
      }
      // read new name
      std::string name;
      iss >> name;
      currentMtl = Material(); // 重置
      currentMtl.name = name;
    }
    else if (token == "Ka")
    {
      iss >> currentMtl.Ka.r >> currentMtl.Ka.g >> currentMtl.Ka.b;
    }
    else if (token == "Kd")
    {
      iss >> currentMtl.Kd.r >> currentMtl.Kd.g >> currentMtl.Kd.b;
    }
    else if (token == "Ks")
    {
      iss >> currentMtl.Ks.r >> currentMtl.Ks.g >> currentMtl.Ks.b;
    }
    else if (token == "Ke")
    {
      iss >> currentMtl.Ke.r >> currentMtl.Ke.g >> currentMtl.Ke.b;
    }
    else if (token == "Ns")
    {
      iss >> currentMtl.Ns;
    }
    else if (token == "Ni")
    {
      iss >> currentMtl.Ni;
    }
    else if (token == "d")
    {
      iss >> currentMtl.d;
    }
    else if (token == "illum")
    {
      iss >> currentMtl.illum;
    }
    else if (token == "map_Kd")
    {
      // map_Kd 可能有 options, 把第一個不是 - 開頭的 token 當成檔名
      std::string t, filename;
      while (iss >> t)
      {
        if (!t.empty() && t[0] == '-')
          continue;   // skip options like -bm
        filename = t; // first non-option token
      }
      if (!filename.empty())
      {
        std::filesystem::path tex = filename;
        if (tex.is_relative())
          tex = baseDir / tex;
        currentMtl.diffuseTexPath = tex.string();
      }
    }
    else if (token == "map_Bump")
    {
      std::string t, filename;
      while (iss >> t)
      {
        if (!t.empty() && t[0] == '-')
          continue;
        filename = t;
      }
      if (!filename.empty())
      {
        std::filesystem::path tex = filename;
        if (tex.is_relative())
          tex = baseDir / tex;
        currentMtl.normalTexPath = tex.string();
      }
    }
    else if (token == "map_Ks")
    {
      std::string t, filename;
      while (iss >> t)
      {
        if (!t.empty() && t[0] == '-')
          continue;
        filename = t;
      }
      if (!filename.empty())
      {
        std::filesystem::path tex = filename;
        if (tex.is_relative())
          tex = baseDir / tex;
        currentMtl.specularTexPath = tex.string();
      }
    }
    else if (token == "map_d")
    { // 透明度貼圖
      std::string t, filename;
      while (iss >> t)
      {
        if (!t.empty() && t[0] == '-')
          continue;
        filename = t;
      }
      if (!filename.empty())
      {
        std::filesystem::path tex = filename;
        if (tex.is_relative())
          tex = baseDir / tex;
        currentMtl.alphaTexPath = tex.string();
      }
    }
  }

  if (!currentMtl.name.empty())
  {
    materials[currentMtl.name] = currentMtl;
  }
}

// image to openGL texture
unsigned int load_texture(const std::string &path)
{
  int width, height, nrChannels;
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
  if (!data)
  {
    std::cerr << "Failed to load texture: " << path << std::endl;
    std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
    return 0;
  }

  GLenum format, internalFormat;
  if (nrChannels == 1)
  {
    format = GL_RED;
    internalFormat = GL_R8;
  }
  else if (nrChannels == 3)
  {
    format = GL_RGB;
    internalFormat = GL_RGB8;
  }
  else if (nrChannels == 4)
  {
    format = GL_RGBA;
    internalFormat = GL_RGBA8;
  }

  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);

  return textureID;
}

// 作業流程
// 讀取 mtl, 更新 g_materials
// 讀取 texture
bool load_obj(const std::string &objPath, glm::mat4 preTransform)
{
  std::vector<glm::vec3> v;
  std::vector<glm::vec2> vt;
  std::vector<glm::vec3> vn;

  std::ifstream file;
  std::string line;
  std::vector<std::string> words;

  Mesh currentMesh;
  std::string currentMatName;
  std::string dir = objPath.substr(0, objPath.find_last_of('/') + 1);

  file.open(objPath);
  if (!file.is_open())
  {
    std::cerr << "Failed to open OBJ: " << objPath << std::endl;
    return false;
  }

  // 一讀 mlt texture v vt vn
  std::cout << "mlt texture v vt vn loading..." << std::endl;
  while (std::getline(file, line))
  {
    words = split(line, " ");
    if (!words[0].compare("mtllib"))
    {
      std::string mtlFile = line.substr(7);
      load_mtl(dir + mtlFile, g_materials);

      // 載入貼圖到 GPU
      for (auto &[name, mat] : g_materials)
      {
        if (!mat.diffuseTexPath.empty())
          mat.diffuseTexID = load_texture(mat.diffuseTexPath);
        if (!mat.specularTexPath.empty())
          mat.specularTexID = load_texture(mat.specularTexPath);
      }
    }
    else if (!words[0].compare("v"))
    {
      v.push_back(read_vec3(words, preTransform, 1.0f));
    }
    else if (!words[0].compare("vt"))
    {
      vt.push_back(read_vec2(words));
    }
    else if (!words[0].compare("vn"))
    {
      vn.push_back(read_vec3(words, preTransform, 0.0f));
    }
  }
  file.close();

  normalize_vertices(v);

  file.open(objPath);
  // 二讀 usemtl, f 建立 meshes
  std::cout << "usemtl loading..." << std::endl;
  while (std::getline(file, line))
  {
    words = split(line, " ");
    if (!words[0].compare("usemtl"))
    {
      // 推舊的
      if (!currentMesh.vertices.empty())
      {
        if (g_materials.find(currentMatName) == g_materials.end())
        {
          std::cerr << "WARNING: Material '" << currentMatName << "' not found!" << std::endl;
        }
        currentMesh.material = &g_materials[currentMatName];
        meshes.push_back(currentMesh);
        currentMesh.vertices.clear();
      }
      // 換新 currentMatName
      std::istringstream iss(line);
      std::string token;
      iss >> token >> currentMatName;
    }
    else if (!words[0].compare("f"))
    {
      read_face(words, v, vt, vn, currentMesh.vertices);
    }
  }
  if (!currentMesh.vertices.empty())
  {
    currentMesh.material = &g_materials[currentMatName];
    meshes.push_back(currentMesh);
    currentMesh.vertices.clear();
  }
  file.close();

  return true;
}

int main()
{
  // char cwd[1024];
  // getcwd(cwd, sizeof(cwd));
  // std::cout << "Current working directory: " << cwd << std::endl;

  // load glfw
  if (!glfwInit())
  {
    std::cerr << "GLFW init fail\n";
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // create window
  window = glfwCreateWindow(WIDTH, HEIGHT, "Scene Animation", NULL, NULL);
  if (!window)
  {
    std::cerr << "Window fail\n";
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // callback
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);

  // load glad
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "GLAD init fail\n";
    return -1;
  }

  // Load model (obj, mtl, png...)
  glm::mat4 identity = glm::mat4(1.0f);

  // std::string obj_name = "tiger";
  std::string obj_name = "SchoolSceneDay";
  // std::string obj_name = "SchoolSceneNight";
  // std::string obj_name = "SchoolSceneAbandoned";
  std::string obj_path = "../models/" + obj_name + "/" + obj_name + ".obj";
  load_obj(obj_path, identity);

  // vertex shader
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // fragment shader
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // link shader to program
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // VBO (Vertex Buffer Object)：存「頂點資料」的緩衝區。
  // VAO (Vertex Array Object)：存「如何讀取這些頂點資料」的設定。
  for (auto &mesh : meshes)
  {
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
  }

  glEnable(GL_DEPTH_TEST);

  // white texture
  GLuint whiteTexture = 0;
  glGenTextures(1, &whiteTexture);
  glBindTexture(GL_TEXTURE_2D, whiteTexture);

  unsigned char whitePixel[3] = {255, 255, 255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 方法 1: 使用預設路徑
  // mainPath = ScenePaths::createCircularPath(glm::vec3(0, 0.5f, 0), 2.0f, 0.5f, 20);
  // mainPath.loop = true;

  // 方法 2: 自訂路徑 (註解掉上面,使用這個)
  mainPath = ScenePaths::createSchoolTour();
  mainPath.addKeyframe(glm::vec3(-0.558581, 0.927852, -1.46058), glm::vec3(0.312254, -0.513225, 0.799436), 2.0f);       // start points from above
  mainPath.addKeyframe(glm::vec3(-0.558581, 0.927852, -1.46058), glm::vec3(0.312254, -0.513225, 0.799436), 1.0f);       // stay at start points for a sec
  mainPath.addKeyframe(glm::vec3(0.146976, 0.00174262, -0.475354), glm::vec3(0.312254, -0.513225, 0.799436), 2.0f);     // steer to school front gate
  mainPath.addKeyframe(glm::vec3(0.146976, 0.00174262, -0.475354), glm::vec3(-0.00572018, -0.0588436, 0.998251), 1.0f); // face school front gate
  mainPath.addKeyframe(glm::vec3(0.14864, -0.021445, -0.02515), glm::vec3(-0.00572018, -0.0588436, 0.998251), 2.0f);    // steer to school hall entrance
  mainPath.addKeyframe(glm::vec3(0.14864, -0.021445, -0.02515), glm::vec3(-0.00572018, -0.0588436, 0.998251), 1.0f);    // face school hall gate
  mainPath.addKeyframe(glm::vec3(0.145497, -0.021445, 0.144062), glm::vec3(-0.00572018, -0.0588436, 0.998251), 2.0f);   // steer tp school hall entrance
  mainPath.addKeyframe(glm::vec3(-0.558581, 0.927852, -1.46058), glm::vec3(0.312254, -0.513225, 0.799436), 2.0f);       // back to start points
  mainPath.loop = true;
  mainPath.play();

  // std::cout << "\n=== Camera Controls ===" << std::endl;
  // std::cout << "P: Play/Pause camera path" << std::endl;
  // std::cout << "R: Reset path" << std::endl;
  // std::cout << "M: Toggle manual control" << std::endl;
  // std::cout << "L: Toggle loop" << std::endl;
  // std::cout << "1: School tour path" << std::endl;
  // std::cout << "2: Circular path" << std::endl;
  // std::cout << "3: Spiral path" << std::endl;
  // std::cout << "WASD: Move (manual mode)" << std::endl;
  // std::cout << "Mouse: Look around (manual mode)" << std::endl;
  // std::cout << "\nPress P to start the camera path!" << std::endl;

  std::cout << "start rendering" << std::endl;

  // 控制處理循環
  while (!glfwWindowShouldClose(window))
  {
    // count deltaTime for tranformation
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // process input
    if (manualControl)
    {
      process_input(window);
    }

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glm::vec3 finalCameraPos;
    glm::vec3 finalLookAt;
    glm::mat4 view;

    if (usePathCamera && mainPath.isPlaying)
    {
      mainPath.update(deltaTime, finalCameraPos, finalLookAt, false);
      view = glm::lookAt(finalCameraPos, finalLookAt, cameraUp);

      // 顯示進度
      static float lastPrintTime = 0.0f;
      if (currentFrame - lastPrintTime > 0.5f)
      {
        int totalKeyframes = mainPath.keyframes.size();
        if (totalKeyframes > 1)
        {
          float progress = (float)(mainPath.currentKeyframe + 1) / (totalKeyframes - 1) * 100.0f;
          std::cout << "Path progress: " << (int)progress << "% "
                    << "(Keyframe " << mainPath.currentKeyframe + 1 << "/"
                    << totalKeyframes - 1 << ")\r" << std::flush;
        }
        lastPrintTime = currentFrame;
      }

      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(finalCameraPos));
    }
    else
    {
      // 使用手動相機
      view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
    }

    // glm 縮放與角度
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.001f, 10.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // light
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 10.0f, 10.0f, 10.0f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);

    // Texture Sampler Units
    glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "specularMap"), 1);

    for (auto &mesh : meshes)
    {
      if (!mesh.material)
        continue;

      Material *mat = mesh.material;

      // 傳遞材質屬性
      glUniform3fv(glGetUniformLocation(shaderProgram, "material_Ka"), 1, glm::value_ptr(mat->Ka));
      glUniform3fv(glGetUniformLocation(shaderProgram, "material_Kd"), 1, glm::value_ptr(mat->Kd));
      glUniform3fv(glGetUniformLocation(shaderProgram, "material_Ks"), 1, glm::value_ptr(mat->Ks));
      glUniform1f(glGetUniformLocation(shaderProgram, "material_Ns"), mat->Ns);
      glUniform1f(glGetUniformLocation(shaderProgram, "material_d"), mat->d);

      // Diffuse 紋理 (Texture Unit 0)
      if (mat->diffuseTexID != 0)
      {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mat->diffuseTexID);
        glUniform1i(glGetUniformLocation(shaderProgram, "hasDiffuseMap"), 1);
      }
      else
      {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "hasDiffuseMap"), 0);
      }

      // Specular 紋理 (Texture Unit 1)
      if (mat->specularTexID != 0)
      {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mat->specularTexID);
        glUniform1i(glGetUniformLocation(shaderProgram, "hasSpecularMap"), 1);
      }
      else
      {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "hasSpecularMap"), 0);
      }

      glBindVertexArray(mesh.VAO);
      glDrawArrays(GL_TRIANGLES, 0, mesh.vertices.size() / 8);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  for (auto &mesh : meshes)
  {
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
  }
  glDeleteProgram(shaderProgram);
  glfwTerminate();
  return 0;
}
