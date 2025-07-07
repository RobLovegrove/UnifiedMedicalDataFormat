#ifndef UUID_HPP
#define UUID_HPP

#include <array>

class UUID {
private:
    std::array<uint8_t, 16> uuid;
    std::array<uint8_t, 16> generateUUID();

public:
    UUID() {
        uuid = generateUUID();
    }

    // Overload == operator
    friend bool operator==(const UUID& lhs, const UUID& rhs) {
        return lhs.uuid == rhs.uuid;
    }

    // Overload != operator
    friend bool operator!=(const UUID& lhs, const UUID& rhs) {
        return !(lhs == rhs);
    }

    std::string toString();

};


#endif