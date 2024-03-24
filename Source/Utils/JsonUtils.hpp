#include <string>
#include <fstream>

namespace Kaamoo {
    class JsonUtils {
    public:
        static std::string ReadJsonFile(const std::string &path) {
            std::ifstream jsonFile(path);
            if (!jsonFile.is_open()) {
                throw std::runtime_error("Failed to open: " + path);
            }
            jsonFile.seekg(0, std::ios::end);
            std::streampos length = jsonFile.tellg();
            jsonFile.seekg(0, std::ios::beg);

            std::string jsonString;
            jsonString.resize(static_cast<size_t>(length));
            jsonFile.read(&jsonString[0], length);
            jsonFile.close();
            return jsonString;
        }
    };
}