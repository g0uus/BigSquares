#pragma once
//
//
//

#include <algorithm>
#include <cmath>
#include <tuple>
#include <cstdint>

namespace MapUtils
{
    // using Point = std::tuple<double, double>;
    // using LatLong = std::tuple<double, double>;

    struct ImageSize
    {
        uint32_t width{};
        uint32_t height{};
    };

    struct Point
    {
        double x{};
        double y{};

        Point() = default;
        Point(const Point &) = default;
        Point(double xx, double yy) : x(xx), y(yy) {}
        Point(uint32_t xx, uint32_t yy) : x(xx), y(yy) {}
        Point(const ImageSize &sz) : x(sz.width), y(sz.height) {}

        auto operator==(const Point &other) const
        {
            return x == other.x && y == other.y;
        }

        auto operator<=>(const Point &other [[maybe_unused]]) const
        {
            return std::partial_ordering::unordered;
        }

        auto operator+=(const Point &other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        auto operator+(const Point &other) const
        {
            Point value{*this};
            return value += other;
        }

        auto operator-=(const Point &other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        auto operator-(const Point &other) const
        {
            Point value{*this};
            return value -= other;
        }
    };

    struct LatLong
    {
        double lattitude{};
        double longitude{};

        auto &operator+=(const LatLong &other)
        {
            lattitude += other.lattitude;
            longitude += other.longitude;
            return *this;
        }
        auto operator+(const LatLong &&other) const
        {
            LatLong value{*this};
            return value += other;
        }

        auto operator+=(const Point &other)
        {
            lattitude += other.y;
            longitude += other.x;
            return +this;
        }

        auto operator+(const Point &&other) const
        {
            LatLong value{*this};
            return value += other;
        }
    };

    /**
     * @brief defined by EPSG:900913 / EPSG:3785 / OSGEO:41001 (apparently)
     */
    static constexpr auto Mercator_Max_Lattitude{85.05112878};

    /**
     * @brief limit a lattitude value to as required for the mercator projection
     *
     * @param[in]   lat the raw lattitude value
     *
     * @return      the lat value in the target range of [-85.05112878,85.05112878]
     *
     * @todo define a named constant for 85.05112878
     */
    inline auto constexpr clamp_lat_for_mercator(double lat)
    {
        return std::clamp(lat, -Mercator_Max_Lattitude, Mercator_Max_Lattitude);
    }

    /**
     * @brief transforms a WGS84(?) lattitude to a mercator lattitude
     *
     * @param[in]   lat the originam WGS84(?) lattitude
     *
     * @return the mercator equivalent coordinate
     *
     * @see clamp_lat_for_mercator
     */
    inline auto constexpr mercator_y(double lat)
    {
        // lat in degrees
        const auto phi = clamp_lat_for_mercator(lat) * M_PI / 180.0; // TODO Magic const
        return std::log(std::tan(M_PI / 4.0 + phi / 2.0));
    }

    // Map lon/lat to image pixel coordinates (mercator)
    /**
     * @brief calculates the pixel position corresponding to a specified Lattitude and Longitude
     *
     * @param[in]   latLong the lattitude and longitude to be converted
     * @param[in]   imgSize the bounds of the target image
     *
     * @return      the Point value
     *
     * @todo Define named constants for the magic values 180.0, 360.0 and 85.05112878
     * @todo clamp longitude to [-180,180] BUT hanldle as wrap-arounds
     */
    inline auto lonlat_to_pix(const LatLong &latLong, const ImageSize &imgSize)
    {
        // x linear from -180..180
        const auto x = (latLong.longitude + 180.0) / 360.0 * imgSize.width; // TODO: Magic consts
        // y via mercator
        static const auto y_max = mercator_y(Mercator_Max_Lattitude);
        static const auto y_min = mercator_y(-Mercator_Max_Lattitude);
        const auto my = mercator_y(latLong.lattitude);
        // normalized from 0 (top) to 1 (bottom)
        const auto ny = (y_max - my) / (y_max - y_min);
        const auto y = ny * imgSize.height;
        return Point(x, y);
    }
}