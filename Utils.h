#pragma once
//
//
//
#include <string>

void lonlat_to_pix(double lon, double lat, int imgw, int imgh, double &x, double &y);
bool maidenhead_bbox(const std::string &grid_in, double &minlat, double &minlon, double &maxlat, double &maxlon);
