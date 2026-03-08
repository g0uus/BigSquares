//
//
//

#include "InputFile.h"

namespace grh::file
{

    FileResult InputFile::readLine()
    {

        if (isOpen())
        {
            return std::unexpected(FileError{1, "File Not Open"});
        }
        istr.getline(buffer.get(), bufferSize);
        //
        // TODO: check for EOF, empty lines etc
        //
        return buffer.get();
    }
}