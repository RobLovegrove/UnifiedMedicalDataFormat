#include "dateTime.hpp"

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

int64_t DateTime::getTimestamp() const {
    return std::chrono::system_clock::to_time_t(timeStamp);
}

// Helper Methods
std::string DateTime::toString() const {
    auto time_t = std::chrono::system_clock::to_time_t(timeStamp);
    std::tm* timeinfo = std::gmtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S UTC");
    return oss.str();
}

std::string DateTime::toISO860String() const {
    auto time_t = std::chrono::system_clock::to_time_t(timeStamp);
    std::tm* timeinfo = std::gmtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}