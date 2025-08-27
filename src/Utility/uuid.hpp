#ifndef UUID_HPP
#define UUID_HPP

#include <array>

class UUID {
private:
    std::array<uint8_t, 16> uuid;
    std::array<uint8_t, 16> generateUUID();

public:
    UUID() { uuid = generateUUID(); }
    static UUID fromString(const std::string& str);

    // Overload == operator
    friend bool operator==(const UUID& lhs, const UUID& rhs) {
        return lhs.uuid == rhs.uuid;
    }

    // Overload != operator
    friend bool operator!=(const UUID& lhs, const UUID& rhs) {
        return !(lhs == rhs);
    }

    std::string toString() const;

    const std::array<uint8_t, 16>& data() const;
    void setData(const std::array<uint8_t, 16>& newData);
};

// Hash function specialization for UUID
namespace std {
    template<>
    struct hash<UUID> {
        size_t operator()(const UUID& uuid) const {
            const auto& data = uuid.data();
            size_t hash = 0;
            for (uint8_t byte : data) {
                hash = hash * 31 + byte;
            }
            return hash;
        }
    };
}

#endif