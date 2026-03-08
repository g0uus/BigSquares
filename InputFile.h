#pragma once

#include <string>
#include <expected>
#include <fstream>
#include <memory>

namespace grh::file
{

    struct FileError
    {
        uint32_t errCode{0};
        std::string errDescription;
    };

    using FileResult = std::expected<std::string, FileError>;

    class InputFile
    {
        uint32_t maxLineSize; // sensible default for the buffer??
        uint32_t minLineSize{0};
        std::ifstream istr;
        uint32_t linecount{0};
        uint32_t bufferSize{0};
        std::shared_ptr<char[]> buffer;

    public:
        // std::expected<std::string, FileError> readLine();
        FileResult readLine();

        auto isOpen() const
        {
            return istr.is_open();
        }
        auto lineCount() const
        {
            return linecount;
        }
    };
}