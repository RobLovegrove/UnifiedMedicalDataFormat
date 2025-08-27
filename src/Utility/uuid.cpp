#include "uuid.hpp"
#include <array>
#include <random>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

array<uint8_t, 16> UUID::generateUUID() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint8_t> dist(0, 255);

    array<uint8_t, 16> uuid;
    for (auto& byte : uuid) {
        byte = dist(gen);
    }

    // Set UUID version to 4 (random)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    return uuid;
}

string UUID::toString() const {
    ostringstream oss;
    oss << hex << setfill('0');

    for (size_t i = 0; i < uuid.size(); ++i) {
        oss << setw(2) << static_cast<int>(uuid[i]);

        // Add dashes at UUID positions: 8-4-4-4-12
        if (i == 3 || i == 5 || i == 7 || i == 9)
            oss << "-";
    }

    return oss.str();
}

UUID UUID::fromString(const string& str) {
    // Expected format: 36 chars (32 hex + 4 dashes)
    if (str.size() != 36) {
        throw std::runtime_error("Invalid UUID string length");
    }

    std::array<uint8_t, 16> bytes{};
    std::string hexStr;
    hexStr.reserve(32);

    // Remove dashes
    for (char c : str) {
        if (c != '-') {
            hexStr.push_back(c);
        }
    }

    if (hexStr.size() != 32) {
        throw std::runtime_error("Invalid UUID string format");
    }

    // Parse hex pairs into bytes
    for (size_t i = 0; i < 16; ++i) {
        std::string byteStr = hexStr.substr(i * 2, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
        bytes[i] = byte;
    }

    UUID uuid;
    uuid.setData(bytes);
    return uuid;
}




const array<uint8_t, 16>& UUID::data() const {
    return uuid;
}

void UUID::setData(const std::array<uint8_t, 16>& newData) {
    uuid = newData;
}