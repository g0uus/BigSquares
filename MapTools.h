#pragma once
//
//
//

#include <cmath>
#include <tuple>
#include <cstdint>

namespace MapUtils
{
    //using Point = std::tuple<double, double>;
    //using LatLong = std::tuple<double, double>;

    struct ImageSize 
    {
        uint32_t width{};
        uint32_t height{};
    };
   
    struct Point
    {
        double x{};
        double y{}; 

        auto operator == ( const Point &other) const
        {
            return x == other.x && y == other.y;
        }

        auto operator <=> (const Point& other[[maybe_unused]]) const
        {
            return std::partial_ordering::unordered;
        }

        auto operator += (const Point& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        auto operator + (const Point &other) const
        {
            Point value{*this};
            return value += other;
        }

        auto operator -= (const Point& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        auto operator - (const Point & other) const
        {
            Point value{*this};
            return value -= other;
        }
    };
   
    struct LatLong
    {
        double lattitude{};
        double longitude{};

        auto &operator+=(const LatLong& other)
        {
            lattitude += other.lattitude;
            longitude += other.longitude;
            return *this;
        }
        auto operator + (const LatLong&& other) const
        {
            LatLong value{*this};
            return value += other;
        }

        auto operator += (const Point& other)
        {
            lattitude += other.y;
            longitude += other.x;
            return +this;
        }

        auto operator + (const Point && other) const
        {
            LatLong value{*this};
            return value += other;
        }
    };

    
    inline auto constexpr clamp_lat_for_mercator(double lat)
    {
        constexpr auto maxlat = 85.05112878; // TODO Magic Const
        if (lat > maxlat)
            return maxlat;
        if (lat < -maxlat)
            return -maxlat;
        return lat;
    }

    inline auto constexpr mercator_y(double lat)
    {
        // lat in degrees
        const auto phi = clamp_lat_for_mercator(lat) * M_PI / 180.0; // TODO Magic const
        return std::log(std::tan(M_PI / 4.0 + phi / 2.0));
    }

    // Map lon/lat to image pixel coordinates (mercator)
  

    inline void lonlat_to_pix(const LatLong &latLong, const ImageSize & imgSize, double &x, double &y)
    {
        // x linear from -180..180
        x = (latLong.longitude + 180.0) / 360.0 * imgSize.width; // TODO: Magic consts
        // y via mercator
        constexpr auto maxlat = 85.05112878; // TODO: Magic const - defined by EPSG:900913 / EPSG:3785 / OSGEO:41001 (apparently)
        static const auto y_max = mercator_y(maxlat);
        static const auto y_min = mercator_y(-maxlat);
        const auto my = mercator_y(latLong.lattitude);
        // normalized from 0 (top) to 1 (bottom)
        const auto ny = (y_max - my) / (y_max - y_min);
        y = ny * imgSize.height;
    }

    inline auto lonlat_to_pix(const LatLong &latLong, const ImageSize & imgSize)
    {
        Point value;
        // x linear from -180..180
        const auto x = (latLong.longitude + 180.0) / 360.0 * imgSize.width; // TODO: Magic consts
        // y via mercator
        constexpr auto maxlat = 85.05112878; // TODO: Magic const - defined by EPSG:900913 / EPSG:3785 / OSGEO:41001 (apparently)
        static const auto y_max = mercator_y(maxlat);
        static const auto y_min = mercator_y(-maxlat);
        const auto my = mercator_y(latLong.lattitude);
        // normalized from 0 (top) to 1 (bottom)
        const auto ny = (y_max - my) / (y_max - y_min);
        const auto y = ny * imgSize.height;
        return Point(x, y);
    }

    inline auto lonlat_to_pix_old(const LatLong &latLong, const ImageSize &imgSize)
    {
        Point value;
        lonlat_to_pix(latLong, imgSize, value.x, value.y);
        return value;
    }

    inline void lonlat_to_pix(double lon, double lat, uint32_t imgw, uint32_t imgh, double &x, double &y)
    {
        const auto val = lonlat_to_pix(LatLong{lat, lon}, ImageSize{imgw, imgh});
        x = val.x;
        y = val.y;
        return;

        // x linear from -180..180
        x = (lon + 180.0) / 360.0 * imgw;
        // y via mercator
        constexpr auto maxlat = 85.05112878; // TOSO Magic const
        static const auto y_max = mercator_y(maxlat);
        static const auto y_min = mercator_y(-maxlat);
        const auto my = mercator_y(lat);
        // normalized from 0 (top) to 1 (bottom)
        const auto ny = (y_max - my) / (y_max - y_min);
        y = ny * imgh;
    }

}