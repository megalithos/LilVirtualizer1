#pragma once
#include <vector>
#include <string>
#include <fstream>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
std::vector<unsigned char> load_file(const std::string& filename);
bool compareStrings(const char* string1, size_t string1Length, const char* string2, size_t string2Length);
void ClearConsole();
void SetCursorPosition(int x, int y);
int ScrollByRelativeCoord(int iRows);
int ScrollByAbsoluteCoord(int iRows);
std::vector<std::string> StringSplit(const std::string& str, char delimiter);
void printHexAsciiDumpLine(std::stringstream&, uint8_t* bytes, int size);