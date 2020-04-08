#include "archiverutils.h"

namespace utils {

std::string system_call(const std::string& bash_cmd) {
    redi::ipstream is(bash_cmd);
    std::string cmd_res;
    std::getline(is, cmd_res);
    is.close();
    return cmd_res;
}

std::string extractFromJson(const std::string& jsonReply, const std::string& searchVal) {
    int searchValStart = jsonReply.find(searchVal) + searchVal.length() + 3;
    int searchValStop = jsonReply.find("\"", searchValStart);
    std::string jsonEntry;

    jsonEntry = jsonReply.substr(searchValStart, searchValStop - searchValStart);

    return jsonEntry;
}


}
