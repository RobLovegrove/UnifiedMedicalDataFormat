#include "dateTime.hpp"

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>



int64_t DateTime::getTimestamp() const {
    return std::chrono::system_clock::to_time_t(timePoint);
}

void DateTime::writeBinary(std::ostream& out) const {
    
}

DateTime DateTime::readBinary(std::istream& in) {

}

// Helper Methods
std::string DateTime::toString() const {

}

std::string DateTime::toISO860String() const {
}