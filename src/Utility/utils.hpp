#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string getCurrentTimestampUTC();

struct Version {
    int major;
    int minor;
    int patch;

    static Version parse(const std::string& version_str);
    std::string toString() const;
    bool is_compatible_with(const Version& reader_version) const;
};

bool getCurrentFilePosition(std::ofstream& outfile, uint64_t& offset);

#endif