#ifndef UTILS_H
#define UTILS_H
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

std::vector<std::string> split(std::string line, std::string delimiter)
{
  std::vector<std::string> splitLine;

  size_t pos = 0;
  std::string token;
  while ((pos = line.find(delimiter)) != std::string::npos)
  {
    token = line.substr(0, pos);
    splitLine.push_back(token);
    line.erase(0, pos + delimiter.length());
  }
  splitLine.push_back(line);

  return splitLine;
}

std::string load_shader_source(const std::string &filePath)
{
  std::ifstream file(filePath);
  if (!file.is_open())
  {
    std::cerr << "Failed to open shader file: " << filePath << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

#endif