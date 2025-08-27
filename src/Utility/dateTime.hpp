#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

class DateTime {
private:
    std::chrono::system_clock::time_point timeStamp;

public:
    DateTime() : timeStamp(std::chrono::system_clock::now()) {}
    DateTime(int64_t timestamp) : timeStamp(std::chrono::system_clock::from_time_t(timestamp)) {}

    static DateTime now() { return DateTime(); }

    // Helper Methods
    int64_t getTimestamp() const;
    std::string toString() const; // Human readable format
    std::string toISO860String() const;

    bool operator==(const DateTime& other) const { return timeStamp == other.timeStamp; }
    bool operator!=(const DateTime& other) const { return timeStamp != other.timeStamp; }
    bool operator<(const DateTime& other) const { return timeStamp < other.timeStamp; }
    bool operator<=(const DateTime& other) const { return timeStamp <= other.timeStamp; }
    bool operator>(const DateTime& other) const { return timeStamp > other.timeStamp; }
    bool operator>=(const DateTime& other) const { return timeStamp >= other.timeStamp; }
};
