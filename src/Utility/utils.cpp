#include "utils.hpp"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

string getCurrentTimestampUTC() {

    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    tm utcTime = *std::gmtime(&time); // Convert to UTC

    ostringstream oss;
    oss << put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
    return oss.str();
}

Version Version::parse(const string& versionStr) {
    Version v{0, 0, 0};
    int matched = sscanf(versionStr.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch);
    if (matched <= 0) {
        throw invalid_argument("Invalid version string: " + versionStr);
    }
    return v;
}

string Version::toString() const {
    return to_string(major) + "." + to_string(minor) + "." + to_string(patch);
}

bool Version::is_compatible_with(const Version& reader_version) const {
    return major == reader_version.major && minor <= reader_version.minor;
}

bool getCurrentFilePosition(ostream& outfile, uint64_t& offset) {
    auto pos = outfile.tellp();
    if (pos == std::ofstream::pos_type(-1)) {
        // Error: tellp() failed
        return false;
    }
    // Convert to integer offset
    offset = static_cast<uint64_t>(pos);
    return true;
}