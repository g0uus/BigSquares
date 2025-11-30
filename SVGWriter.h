#pragma once
//
//
//
#include <string>
#include <fstream>
#include <unordered_set>

void writeSVGHeader(std::ofstream &ofs, int imgw, int imgh, bool have_bg, const std::string &bgpath);
void writeGridLines(std::ofstream &ofs, int imgw, int imgh);
void writeSVGSquares(std::ofstream &ofs, int imgw, int imgh, const std::unordered_set<std::string> &squares);
void writeSVGFooter(std::ofstream &ofs);
