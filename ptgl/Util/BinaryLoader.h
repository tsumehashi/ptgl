#ifndef PTGL_UTIL_BINARYLOADER_H_
#define PTGL_UTIL_BINARYLOADER_H_

#include <vector>
#include <fstream>
#include <iostream>

namespace ptgl {

class BinaryLoader {
public:
    BinaryLoader() {}
    virtual ~BinaryLoader() {}

    static std::vector<uint8_t> load(const std::string& filename)
    {
        std::ifstream ifs(filename, std::ios::in | std::ios::binary);
        return load(ifs);
    }

    static std::vector<uint8_t> load(std::istream& is)
    {
        if (!is) {
            return std::vector<uint8_t>();
        }

        is.seekg(0 ,std::ios::end);
        size_t size = is.tellg();
        is.seekg(0, std::ios_base::beg);

        std::vector<uint8_t> rawData;
        rawData.resize(size);
        is.read((char*)rawData.data(), size);

        return rawData;
    }
};

} /* namespace rgl */

#endif /* PTGL_UTIL_BINARYLOADER_H_ */
