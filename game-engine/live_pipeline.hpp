#pragma once

#include <string>
#include <vector>

namespace live {
    class LivePipeline {
        public:
            LivePipeline(const std::string& vertexFilePath, const std::string& fragmentFilePath);
        private:
            static std::vector<char> readFile(const std::string& filePath);
            
            void createGraphicsPipeline(const std::string& vertexFilePath, const std::string& fragmentFilePath);
    };
}
