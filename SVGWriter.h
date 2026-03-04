#pragma once
//
//
//
#include <string>
#include <fstream>
#include <unordered_set>

class SVGWriter
{
    std::ofstream ofs;

public:
    SVGWriter(const std::string &filename, int imgw, int imgh, bool have_bg, const std::string &bgpath)
    {
        ofs.open(filename);
        if (!ofs)
        {
            throw std::runtime_error("Failed to open SVG file for writing");
        }
        writeHeader(imgw, imgh, have_bg, bgpath);
    }

    void writeHeader(int imgw, int imgh, bool have_bg, const std::string &bgpath);

    void writeGridLines(int imgw, int imgh);

    void writeSquares(int imgw, int imgh, const std::unordered_set<std::string> &squares);

    void writeFooter();

    ~SVGWriter()
    {
        writeFooter();
        ofs.close();
    }
};