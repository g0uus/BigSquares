#pragma once
//
//
//
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

class SVGWriter
{
    const std::string filename;
    uint32_t imageWidth{0};
    uint32_t imageHeight{0};
    std::string bgPath; // not currently used
    std::ofstream ofs;
    bool drawCentres{false};

public:
    SVGWriter(const std::string &filename, int imgw, int imgh, const std::string &bgpath)
        : filename(filename),
          imageWidth(imgw),
          imageHeight(imgh),
          bgPath(bgpath)
    {
        ofs.open(filename);
        if (!ofs)
        {
            throw std::runtime_error("Failed to open SVG file for writing");
        }
        writeHeader();
    }

    void writeHeader();

    void writeGridLines();

    void writeSquares(const std::unordered_set<std::string> &squares,
                      std::unordered_map<std::string, std::pair<double, double>> locator_map);

    void writeFooter();

    ~SVGWriter()
    {
        writeFooter();
        ofs.close();
        std::cout << "Wrote SVG: " << filename << " (canvas " << imageWidth << "x" << imageHeight << ")\n";
        //cout << "Wrote SVG: " << filename << " (canvas " << imageWidth << u8"\u00d7" << imageHeight << ")\n";
    }

    auto have_bg() const
    {
        return !bgPath.empty();
    }
};