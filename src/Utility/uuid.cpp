#include "uuid.hpp"
#include <array>
#include <random>
#include <sstream>
#include <iomanip>

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

string UUID::toString() {
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