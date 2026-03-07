#pragma once
//
//
//
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <unordered_set>
#include <unordered_map>

#include "MapTools.h"

class SVGWriter
{
    const std::string filename;
    uint32_t imageWidth{0};
    uint32_t imageHeight{0};
    std::string bgPath; // not currently used
    std::ofstream ofs;
    bool drawCentres{false};

    friend class SVGObj;

public:
    SVGWriter(const std::string &filename, int imgw, int imgh, const std::string &bgpath)
        : filename(filename),
          imageWidth(imgw),
          imageHeight(imgh),
          bgPath(bgpath),
          svg(*this)
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
    }

    auto have_bg() const
    {
        return !bgPath.empty();
    }

    class SVGObj
    {
        friend class SVGWriter;
        auto &ofs() { return parent.ofs; }

        SVGWriter &parent;
        SVGObj(SVGWriter &parent)
            : parent(parent)
        {
        }

        void start()
        {
            ofs() << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                  << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
                  << "width=\"" << parent.imageWidth
                  << "\" height=\"" << parent.imageHeight
                  << "\" viewBox=\"0 0 " << parent.imageWidth << " " << parent.imageHeight
                  << "\">\n";
        }

        void drawBackground()
        {
            if (parent.have_bg())
            {
                // place background image stretched to canvas (assumed mercator-projected world)
                ofs() << "  <image href=\"" << parent.bgPath << "\" x=\"0\" y=\"0\""
                      << " width=\"" << parent.imageWidth
                      << "\" height=\"" << parent.imageHeight
                      << "\" preserveAspectRatio=\"xMidYMid slice\"/>\n";
            }
            else
            {
                // blank white background
                drawRect(MapUtils::Point{},
                         MapUtils::ImageSize{parent.imageWidth, parent.imageHeight},
                         "#ffffff");
            }
        }

        void drawLine(const MapUtils::Point &p1, const MapUtils::Point &p2)
        {
            ofs() << "    <line x1=\"" << std::fixed << std::setprecision(2)
                  << p1.x << "\" y1=\"" << p1.y
                  << "\" x2=\"" << p2.x << "\" y2=\"" << p2.y << "\"/>\n";
        }

        void drawRect(const MapUtils::Point &topLeft, const MapUtils::Point &boxWidth, const std::string &fill = std::string())
        {
            ofs() << "  <rect x=\"" << std::fixed << std::setprecision(2) << topLeft.x
                  << "\" y=\"" << topLeft.y
                  << "\" width=\"" << boxWidth.x << "\" height=\"" << boxWidth.y;
            if (!fill.empty())
            {
                ofs() << "\" fill=\"" << fill;
            }
            ofs() << "\"/>\n";
        }

        void drawCircle(MapUtils::Point pt)
        {
            ofs()
                << "    <circle cx=\"" << std::fixed << std::setprecision(2) << pt.x
                << "\" cy=\"" << pt.y << "\" r=\"3\"/>\n";
        }

        auto stroke(const std::string &colour, const std::string &width, const std::string &fill)
        {
            return SVGStroke(ofs(), colour, width, fill);
        }

    private:
        class SVGStroke
        {
            friend class SVGObj;
            std::ostream &ofs;

        private:
            SVGStroke(std::ostream &ofs, const std::string &colour, const std::string &width, const std::string &fill)
                : ofs(ofs)
            {
                ofs << "  <g stroke=\"" << colour << "\" stroke-width=\"" << width << "\" fill=\"" << fill << "\">\n";
            }

        public:
            ~SVGStroke()
            {
                ofs << "  </g>\n";
            }
        };
    };

    SVGObj svg;
};