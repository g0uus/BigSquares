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

    ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    ofs << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"" << imageWidth << "\" height=\"" << imageHeight << "\" viewBox=\"0 0 " << imageWidth << " " << imageHeight << "\">\n";

    if (have_bg())
    {
        // place background image stretched to canvas (assumed mercator-projected world)
        ofs << "  <image href=\"" << bgPath << "\" x=\"0\" y=\"0\" width=\"" << imageWidth << "\" height=\"" << imageHeight << "\" preserveAspectRatio=\"xMidYMid slice\"/>\n";
    }
    else
    {
        // blank white background
        ofs << "  <rect x=\"0\" y=\"0\" width=\"" << imageWidth << "\" height=\"" << imageHeight << "\" fill=\"#ffffff\"/>\n";
    }
}

void SVGWriter::writeGridLines()
{
    // draw lat/lon grid lines every 10 degrees - should be Major Square (Field) lines every 20deg lon x 10deg lat (? Done)
    // Define min/max lat/lon for mercator projection
    ofs << "  <g stroke=\"#888\" stroke-width=\"0.5\" fill=\"none\">\n";
    for (int lon = -180; lon <= 180; lon += 20) // CONSTs
    {
        double x1, y1, x2, y2;
        lonlat_to_pix((double)lon, -85.0, imageWidth, imageHeight, x1, y1); // CONSTs
        lonlat_to_pix((double)lon, 85.0, imageWidth, imageHeight, x2, y2);  // CONSTs
        ofs << "    <line x1=\"" << std::fixed << std::setprecision(2) << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\"/>\n";
    }
    for (int lat = -80; lat <= 80; lat += 10) // CONSTs
    {
        double x1, y1, x2, y2;
        lonlat_to_pix(-180.0, (double)lat, imageWidth, imageHeight, x1, y1);
        lonlat_to_pix(180.0, (double)lat, imageWidth, imageHeight, x2, y2);
        ofs << "    <line x1=\"" << std::fixed << std::setprecision(2) << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\"/>\n";
    }
    ofs << "  </g>\n";
}

void SVGWriter::writeSquares(const std::unordered_set<string> &squares4,
                             std::unordered_map<string, std::pair<double, double>> locator_map)
{
    using MapUtils::LatLong;

    // draw 4-char squares (2deg lon x 1deg lat) as rectangles
    ofs << "  <g stroke=\"#f88\" stroke-width=\"0.8\" fill=\"#f00\">\n";
    for (const auto &sq : squares4)
    {
        LatLong min;
        LatLong max;
#if 1
        if (!maidenhead_bbox(sq, min, max))
            continue;
#else
        if (!maidenhead_bbox(sq, min.lattitude, min.longitude, max.lattitude, max.longitude))
            continue;
#endif

        const MapUtils::ImageSize imgsize{imageWidth, imageHeight};
        auto topLeft = lonlat_to_pix(min, imgsize);
        auto bottomRight = lonlat_to_pix(max, imgsize);

#if 1
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
        ofs << "    <rect x=\"" << std::fixed << std::setprecision(2) << topLeft.x << "\" y=\"" << topLeft.y 
        << "\" width=\"" << boxWidth.x << "\" height=\"" << boxWidth.y << "\"/>\n";
#else
        double rx = topLeft.x;
        double ry = topLeft.y;
        double rw = bottomRight.x - topLeft.x;
        double rh = bottomRight.y - topLeft.y;

        if (rw < 0)
        {
            rx = bottomRight.x;
            rw = -rw;
        }
        if (rh < 0)
        {
            ry = bottomRight.y;
            rh = -rh;
        }

#endif 
#if 0
        double minlat,
            minlon, maxlat, maxlon;
        if (!maidenhead_bbox(sq, minlat, minlon, maxlat, maxlon))
            continue;
        double x1, y1, x2, y2;
        lonlat_to_pix(minlon, maxlat, imageWidth, imageHeight, x1, y1); // top-left
        lonlat_to_pix(maxlon, minlat, imageWidth, imageHeight, x2, y2); // bottom-right

        double rx = x1;
        double ry = y1;
        double rw = x2 - x1;
        double rh = y2 - y1;
                
        if (rw < 0)
        {
            rx = x2;
            rw = -rw;
        }
        if (rh < 0)
        {
            ry = y2;
            rh = -rh;
        }
        ofs << "    <rect x=\"" << std::fixed << std::setprecision(2) << rx << "\" y=\"" << ry << "\" width=\"" << rw << "\" height=\"" << rh << "\"/>\n";
#endif 

        
    }

    ofs << "  </g>\n";


    if(drawCentres)
    {
        // draw centers for unique locators
        ofs << "  <g stroke=\"#000\" stroke-width=\"0.6\" fill=\"#000\">\n";
        for (auto &kv : locator_map)
        {
            // const string &loc = kv.first;  // not usedcmo
            double lat = kv.second.first;
            double lon = kv.second.second;
            double x, y;
            lonlat_to_pix(lon, lat, imageWidth, imageHeight, x, y);
            ofs << "    <circle cx=\"" << std::fixed << std::setprecision(2) << x << "\" cy=\"" << y << "\" r=\"3\"/>\n";
        }
        ofs << "  </g>\n";
    }

}

void SVGWriter::writeFooter()
{
    ofs << "</svg>\n";
}
