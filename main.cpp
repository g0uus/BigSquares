#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>

using std::cerr;
using std::cout;
using std::ifstream;
using std::ios;
using std::istream;
using std::make_pair;
using std::ofstream;
using std::ostream;
using std::pair;
using std::string;
using std::string_view;
using std::unordered_map;
using std::unordered_set;
using std::vector;

#include "SVGWriter.h"
#include "Utils.h"

//
// TODO
//
// Fix all the magic constants that AI generated
// Add band/mode into raw data
// Add band/mode filtering
//
// Add Batch mode to read data once and produce multiple maps
//

// TODO make this a utility function in Utils.h; also add trim_left and trim_right variants if needed
// Trim both ends
static inline string trim(const string &s)
{
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a]))
    {
        ++a;
    }
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b - 1]))
    {
        --b;
    }
    return s.substr(a, b - a);
}

// Simple CSV line parser supporting quoted fields
vector<string> parse_csv_line(const string &line)
{
    vector<string> out;
    string cur;
    bool inq = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (inq)
        {
            if (c == '"')
            {
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    cur.push_back('"');
                    ++i;
                }
                else
                    inq = false;
            }
            else
                cur.push_back(c);
        }
        else
        {
            if (c == '"')
                inq = true;
            else if (c == ',')
            {
                out.push_back(cur);
                cur.clear();
            }
            else
                cur.push_back(c);
        }
    }
    out.push_back(cur);
    for (auto &s : out)
        s = trim(s);
    return out;
}

// Convert Maidenhead locator to center lat/lon. Returns true on success.
bool maidenhead_to_latlon(const string &grid_in, double &lat, double &lon)
{
    // remove whitespace and ignore case; also check for empty after removing whitespace
    string g;
    for (const auto c : grid_in)
    {
        if (!isspace(static_cast<unsigned char>(c)))
        {
            g.push_back(toupper(static_cast<unsigned char>(c)));
        }
    }
    if (g.size() < 2)
        return false;
    // work with chars; accept mixed case
    // positions: 0 field lon letter A-R, 1 field lat letter A-R
    // 2 square lon digit 0-9, 3 square lat digit
    // 4 subsquare lon letter A-X, 5 subsquare lat letter A-X
    // 6 ext lon digit 0-9, 7 ext lat digit

    auto lon_deg = -180.0;
    auto lat_deg = -90.0;

    const char f_lon = toupper(static_cast<unsigned char>(g[0]));
    const char f_lat = toupper(static_cast<unsigned char>(g[1]));
    if (f_lon < 'A' || f_lon > 'R' || f_lat < 'A' || f_lat > 'R')
        return false;
    const auto f_lon_i = f_lon - 'A';
    const auto f_lat_i = f_lat - 'A';
    lon_deg += f_lon_i * 20.0;
    lat_deg += f_lat_i * 10.0;
    double lon_size = 20.0;
    double lat_size = 10.0;
    if (g.size() >= 4)
    {
        const auto s_lon = g[2];
        const auto s_lat = g[3];
        if (!isdigit((unsigned char)s_lon) || !isdigit((unsigned char)s_lat))
            return false;
        const auto s_lon_i = s_lon - '0';
        const auto s_lat_i = s_lat - '0';
        lon_deg += s_lon_i * 2.0;
        lat_deg += s_lat_i * 1.0;
        lon_size = 2.0;
        lat_size = 1.0;
    }
    if (g.size() >= 6)
    {
        const auto sub_lon = toupper((unsigned char)g[4]);
        const auto sub_lat = toupper((unsigned char)g[5]);
        if (sub_lon < 'A' || sub_lon > 'X' || sub_lat < 'A' || sub_lat > 'X')
            return false;
        int sub_lon_i = sub_lon - 'A';
        int sub_lat_i = sub_lat - 'A';
        lon_deg += sub_lon_i * (lon_size / 24.0);
        lat_deg += sub_lat_i * (lat_size / 24.0);
        lon_size /= 24.0;
        lat_size /= 24.0;
    }
    if (g.size() >= 8)
    {
        const auto ext_lon = g[6];
        const auto ext_lat = g[7];
        if (!isdigit((unsigned char)ext_lon) || !isdigit((unsigned char)ext_lat))
            return false;
        int ex_lon_i = ext_lon - '0';
        int ex_lat_i = ext_lat - '0';
        lon_deg += ex_lon_i * (lon_size / 10.0);
        lat_deg += ex_lat_i * (lat_size / 10.0);
        lon_size /= 10.0;
        lat_size /= 10.0;
    }
    lon = lon_deg + lon_size / 2.0;
    lat = lat_deg + lat_size / 2.0;
    return true;
}

// NOTE:
// the only difference between this and maidenhead_to_latlon is that we don't add half the cell size at the end,
// and instead return the min and max lat/lon of the bounding box for the locator.
// This is used to draw the grid squares.
// SUGGESTION: refactor to have a single function that returns the bounding box,
//  and then have maidenhead_to_latlon call that and compute the center from the bounding box.
//  Also, this function is currently duplicated in SVGWriter.cpp for convenience; ideally it would be in Utils.h and shared.
//  Also , Improve error handling and validation in both functions; currently they just return false on invalid input
//  but don't provide details.
//  Use a std::expected () - Required C++23 - to return either the bounding box or an error message on failure,
//  instead of using output parameters and a bool return value.

// Obtain bounding box for locator (minlat,minlon,maxlat,maxlon)
bool maidenhead_bbox(const string &grid_in, double &minlat, double &minlon, double &maxlat, double &maxlon)
{
    // remove whitespace and ignore case; also check for empty after removing whitespace
    string g;
    for (const auto c : grid_in)
    {
        if (!isspace(static_cast<unsigned char>(c)))
        {
            g.push_back(toupper(static_cast<unsigned char>(c)));
        }
    }
    if (g.size() < 2)
        return false;
    auto lon_deg = -180.0;
    auto lat_deg = -90.0;
    const auto f_lon = toupper((unsigned char)g[0]);
    const auto f_lat = toupper((unsigned char)g[1]);
    if (f_lon < 'A' || f_lon > 'R' || f_lat < 'A' || f_lat > 'R')
        return false;
    const auto f_lon_i = f_lon - 'A';
    const auto f_lat_i = f_lat - 'A';
    lon_deg += f_lon_i * 20.0;
    lat_deg += f_lat_i * 10.0;
    auto lon_size = 20.0;
    auto lat_size = 10.0;
    if (g.size() >= 4)
    {
        const auto s_lon = g[2];
        const auto s_lat = g[3];
        if (!isdigit((unsigned char)s_lon) || !isdigit((unsigned char)s_lat))
            return false;
        auto s_lon_i = s_lon - '0';
        auto s_lat_i = s_lat - '0';
        lon_deg += s_lon_i * 2.0;
        lat_deg += s_lat_i * 1.0;
        lon_size = 2.0;
        lat_size = 1.0;
    }
    if (g.size() >= 6)
    {
        const auto sub_lon = toupper((unsigned char)g[4]);
        const auto sub_lat = toupper((unsigned char)g[5]);
        if (sub_lon < 'A' || sub_lon > 'X' || sub_lat < 'A' || sub_lat > 'X')
            return false;
        const auto sub_lon_i = sub_lon - 'A';
        const auto sub_lat_i = sub_lat - 'A';
        lon_deg += sub_lon_i * (lon_size / 24.0);
        lat_deg += sub_lat_i * (lat_size / 24.0);
        lon_size /= 24.0;
        lat_size /= 24.0;
    }
    if (g.size() >= 8)
    {
        const auto ext_lon = g[6];
        const auto ext_lat = g[7];
        if (!isdigit((unsigned char)ext_lon) || !isdigit((unsigned char)ext_lat))
            return false;
        const auto ex_lon_i = ext_lon - '0';
        const auto ex_lat_i = ext_lat - '0';
        lon_deg += ex_lon_i * (lon_size / 10.0);
        lat_deg += ex_lat_i * (lat_size / 10.0);
        lon_size /= 10.0;
        lat_size /= 10.0;
    }
    minlon = lon_deg;
    minlat = lat_deg;
    maxlon = lon_deg + lon_size;
    maxlat = lat_deg + lat_size;
    return true;
}

// Mercator helpers
static double clamp_lat_for_mercator(double lat)
{
    const double maxlat = 85.05112878;
    if (lat > maxlat)
        return maxlat;
    if (lat < -maxlat)
        return -maxlat;
    return lat;
}

static double mercator_y(double lat)
{
    // lat in degrees
    double phi = clamp_lat_for_mercator(lat) * M_PI / 180.0;
    return std::log(std::tan(M_PI / 4.0 + phi / 2.0));
}

// Read image size for PNG and BMP (basic). Returns true if size read.
static bool read_image_size(const string &path, int &w, int &h)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        return false;
    unsigned char sig[8];
    ifs.read((char *)sig, 8);
    if (!ifs)
        return false;
    const unsigned char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (std::memcmp(sig, png_sig, 8) == 0)
    {
        ifs.seekg(16, std::ios::beg);
        unsigned char buf[8];
        ifs.read((char *)buf, 8);
        if (ifs)
        {
            uint32_t wi = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
            uint32_t hi = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
            w = (int)wi;
            h = (int)hi;
            return true;
        }
        return false;
    }
    if (sig[0] == 'B' && sig[1] == 'M')
    {
        ifs.seekg(18, std::ios::beg);
        unsigned char buf[8];
        ifs.read((char *)buf, 8);
        if (ifs)
        {
            uint32_t wi = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
            uint32_t hi = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) | ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
            w = (int)wi;
            h = (int)hi;
            return true;
        }
        return false;
    }
    return false;
}

// Map lon/lat to image pixel coordinates (mercator)
void lonlat_to_pix(double lon, double lat, int imgw, int imgh, double &x, double &y)
{
    // x linear from -180..180
    x = (lon + 180.0) / 360.0 * imgw;
    // y via mercator
    double maxlat = 85.05112878;
    double y_max = mercator_y(maxlat);
    double y_min = mercator_y(-maxlat);
    double my = mercator_y(lat);
    // normalized from 0 (top) to 1 (bottom)
    double ny = (y_max - my) / (y_max - y_min);
    y = ny * imgh;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " input.csv output.svg [background.png|bg.png]\n";
        return 2;
    }
    const string infile = argv[1];
    const string outsvg = argv[2];
    string bgpath;
    bool have_bg = false;
    if (argc >= 4)
    {
        bgpath = argv[3];
        have_bg = true;
    }

    //
    // Read input CSV and parse locators, keeping track of unique 4-char squares for drawing
    //
    ifstream ifs(infile);
    if (!ifs)
    {
        cerr << "Failed to open input file: " << infile << '\n';
        return 3;
    }

    unordered_map<string, pair<double, double>> locator_map;
    unordered_set<string> squares4;
    string line;
    while (std::getline(ifs, line))
    {
        if (line.empty())
            continue;
        auto fields = parse_csv_line(line);
        if (fields.empty())
            continue;
        const string locator = trim(fields[0]);
        if (locator.empty())
            continue;
        string loc2;
        for (char c : locator)
        {
            if (!isspace((unsigned char)c)) // skip whitespace in locator
            {
                loc2.push_back(c);
            }
        }
        if (loc2.empty())
            continue;
        if (locator_map.find(loc2) != locator_map.end())
            continue;
        double lat, lon;
        if (!maidenhead_to_latlon(loc2, lat, lon))
        {
            string loc_upper;
            for (char c : loc2)
                loc_upper.push_back(toupper((unsigned char)c));
            if (!maidenhead_to_latlon(loc_upper, lat, lon))
            {
                cerr << "Warning: invalid locator skipped: '" << loc2 << "'\n";
                continue;
            }
            loc2 = loc_upper;
        }
        locator_map.emplace(loc2, make_pair(lat, lon));
        // record its 4-char parent if possible
        if (loc2.size() >= 4)
        {
            string s4 = loc2.substr(0, 4);
            for (char &c : s4)
                c = toupper((unsigned char)c);
            squares4.insert(s4);
        }
    }

    // determine image size
    int imgw = 2048, imgh = 1024; // default
    if (have_bg)
    {
        int rw = 0, rh = 0;
        if (read_image_size(bgpath, rw, rh))
        {
            if (rw > 0 && rh > 0)
            {
                imgw = rw;
                imgh = rh;
            }
        }
        else
        {
            // warn but continue using default dimensions and still reference image (SVG will scale it)
            cerr << "Warning: couldn't read background image size; using default canvas size " << imgw << "x" << imgh << "\n";
        }
    }

    {
        SVGWriter svg(outsvg, imgw, imgh, have_bg, bgpath);

        svg.writeGridLines(imgw, imgh);

        svg.writeSquares(imgw, imgh, squares4);

        // file closed and footer written in destructor
    }
    cout << "Wrote SVG: " << outsvg << " (canvas " << imgw << "x" << imgh << ")\n";
    return 0;
}

int main2(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " input.csv [output.csv]\n";
        return 2;
    }
    string infile = argv[1];
    string outfile;
    bool write_out = false;

    if (argc >= 3)
    {
        outfile = argv[2];
        write_out = true;
    }

    ifstream ifs(infile);
    if (!ifs)
    {
        cerr << "Failed to open input file: " << infile << '\n';
        return 3;
    }

    unordered_map<string, pair<double, double>> map;
    string line;
    while (std::getline(ifs, line))
    {
        if (line.empty())
            continue;
        auto fields = parse_csv_line(line);
        if (fields.empty())
            continue;
        string locator = fields[0];
        // remove surrounding quotes and whitespace already trimmed
        locator = trim(locator);
        if (locator.empty())
            continue;
        // canonicalize: remove spaces
        string loc2;
        for (char c : locator)
            if (!isspace((unsigned char)c))
                loc2.push_back(c);
        if (loc2.empty())
            continue;
        // convert only if not already present
        if (map.find(loc2) != map.end())
            continue;
        double lat, lon;
        if (!maidenhead_to_latlon(loc2, lat, lon))
        {
            // try with case-insensitivity and trimming; if still invalid, skip
            string loc_upper;
            for (char c : loc2)
                loc_upper.push_back(toupper((unsigned char)c));
            if (!maidenhead_to_latlon(loc_upper, lat, lon))
            {
                cerr << "Warning: invalid locator skipped: '" << loc2 << "'\n";
                continue;
            }
            loc2 = loc_upper;
        }
        map.emplace(loc2, make_pair(lat, lon));
    }

    // Prepare output
    ostream *outp = &cout;
    ofstream ofs;
    if (write_out)
    {
        ofs.open(outfile);
        if (!ofs)
        {
            cerr << "Failed to open output file: " << outfile << '\n';
            return 4;
        }
        outp = &ofs;
    }
    *outp << "locator,latitude,longitude\n";
    // stable order: sort locators
    vector<string> keys;
    keys.reserve(map.size());
    for (auto &kv : map)
        keys.push_back(kv.first);
    sort(keys.begin(), keys.end());
    outp->setf(ios::fixed);
    outp->precision(6);
    for (auto &k : keys)
    {
        auto p = map[k];
        *outp << k << ',' << p.first << ',' << p.second << '\n';
    }

    return 0;
}
