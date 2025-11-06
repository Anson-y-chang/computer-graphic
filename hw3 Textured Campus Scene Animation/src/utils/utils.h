#ifndef UTILS_H
#define UTILS_H
#include <vector>

std::string load_shader_source(const std::string &filePath);
std::vector<std::string> split(std::string line, std::string delimiter);

#endif