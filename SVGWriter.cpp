//
//
// TODOs
// Convert into a class
// Fix the MANY magic constants that AI generated
// Add options for colors, line widths, etc.
// Correct Major Square grid lines to match Maidenhead spec
// Add Major square labels (optional)
//

//  SVGWriter.cpp
//  VectorGraphics

#include "SVGWriter.h"

#include <cmath>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "Utils.h"
#include "MapTools.h"

#define NEW_MAP_UTILS

#if defined(NEW_MAP_UTILS)
using namespace MapUtils;
#endif

using std::ifstream;
using std::ios;
using std::ofstream;
using std::string;

using std::cerr;
using std::cout;

// TODO: Make into a class with member variables for image dimensions, background, etc., and member functions for writing header, grid lines, squares, footer, etc.

// Convert Maidenhead locator to lat/lon bounding box

void SVGWriter::writeHeader()
{
    svg.start();

    svg.drawBackground();
}

void SVGWriter::writeGridLines()
{
    // draw lat/lon grid lines every 20/10 degrees - should be Major Square (Field) lines every 20deg lon x 10deg lat (? Done)
    // Define min/max lat/lon for mercator projection
    {
        auto stroke = svg.stroke("#888", "0.5", "none");
        const ImageSize imgSize{imageWidth, imageHeight};

        for (int lon = -180; lon <= 180; lon += 20) // TODO: CONSTs
        {
            svg.drawLine(lonlat_to_pix({-85.0, (double)lon}, imgSize),
                         lonlat_to_pix({85.0, (double)lon}, imgSize));
        }

        for (int lat = -80; lat <= 80; lat += 10) // TODO: CONSTs
        {
            svg.drawLine(lonlat_to_pix({(double)lat, -180.0}, imgSize),
                         lonlat_to_pix({(double)lat, 180.0}, imgSize));
        }
    }
}

void SVGWriter::writeSquares(const std::unordered_set<string> &squares4,
                             std::unordered_map<string, std::pair<double, double>> locator_map)
{
    using MapUtils::LatLong;

    // draw 4-char squares (2deg lon x 1deg lat) as rectangles
    {
        auto stroke = svg.stroke("#f88", "0.8", "#f00");

        for (const auto &sq : squares4)
        {
            LatLong min;
            LatLong max;

            if (!maidenhead_bbox(sq, min, max))
                continue;

            const MapUtils::ImageSize imgsize{imageWidth, imageHeight};
            auto topLeft = lonlat_to_pix(min, imgsize);
            auto bottomRight = lonlat_to_pix(max, imgsize);

            auto boxWidth = bottomRight - topLeft;
            if (boxWidth.x < 0)
            {
                topLeft.x = bottomRight.x;
                boxWidth.x = -boxWidth.x;
            }
            if (boxWidth.y < 0)
            {
                topLeft.y = bottomRight.y;
                boxWidth.y = -boxWidth.y;
            }
            svg.drawRect(topLeft, boxWidth);
        }
    }

    if (drawCentres)
    {
        // draw centers for unique locators
        auto stroke = svg.stroke("#000", "0.6", "#000");
        for (auto &kv : locator_map)
        {
            svg.drawCircle(
                lonlat_to_pix(LatLong{kv.second.first, kv.second.second},
                              ImageSize{imageWidth, imageHeight}));
        }
    }
}

void SVGWriter::writeFooter()
{
    ofs << "</svg>\n";
}
