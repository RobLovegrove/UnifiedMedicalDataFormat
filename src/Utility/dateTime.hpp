#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

class DateTime {
private:
    std::chrono::system_clock::time_point timePoint;

public:
    DateTime();
    DateTime(const std::string& dateTimeString);
    DateTime(int64_t timestamp) : timestamp(std::chrono::system_clock::from_time_t(timestamp));
    
    void writeBinary(std::ostream& out) const;
    static DateTime readBinary(std::istream& in);

    // Helper Methods
    int64_t getTimestamp() const;
    std::string toString() const; // Human readable format
    std:string toISO860String() const;
}