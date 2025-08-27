#include "Utility/dateTime.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("DateTime timestamp constructor", "[datetime]") {
    SECTION("Timestamp constructor works") {
        int64_t knownTimestamp = 1234567890;
        DateTime dt(knownTimestamp);  // Direct constructor call
        REQUIRE(dt.getTimestamp() == knownTimestamp);
    }
}

TEST_CASE("DateTime constructors", "[datetime]") {
    SECTION("Default constructor sets current time") {
        DateTime dt1;
        REQUIRE(dt1.getTimestamp() > 0);
    }
    
    SECTION("Unix timestamp constructor works") {
        int64_t knownTimestamp = 1234567890;
        DateTime dt2(knownTimestamp);
        REQUIRE(dt2.getTimestamp() == knownTimestamp);
    }
}

TEST_CASE("DateTime string formatting", "[datetime]") {
    SECTION("toString produces readable format") {
        DateTime dt(1234567890);  // 2009-02-13 23:31:30 UTC
        std::string str = dt.toString();
        
        // Should contain the date components
        REQUIRE(str.find("2009") != std::string::npos);
        REQUIRE(str.find("02") != std::string::npos);
        REQUIRE(str.find("13") != std::string::npos);
        REQUIRE(str.find("23") != std::string::npos);
        REQUIRE(str.find("31") != std::string::npos);
        REQUIRE(str.find("30") != std::string::npos);
    }
    
    SECTION("toISO860String produces ISO format") {
        DateTime dt(1234567890);  // 2009-02-13 23:31:30 UTC
        std::string iso = dt.toISO860String();
        
        // Should be ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
        REQUIRE(iso.find("2009-02-13T23:31:30Z") != std::string::npos);
    }
}

TEST_CASE("DateTime binary serialization", "[datetime]") {
    SECTION("readBinary reads back correctly") {
        DateTime original(1234567890);
        std::stringstream ss;
        int64_t timestamp = original.getTimestamp();
        ss.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        
        // Reset stream and read back
        ss.seekg(0);
        DateTime restored = DateTime::readBinary(ss);
        
        // Should be equal
        REQUIRE(restored.getTimestamp() == original.getTimestamp());
        REQUIRE(restored.toString() == original.toString());
        REQUIRE(restored.toISO860String() == original.toISO860String());
    }
    
    SECTION("round-trip serialization preserves data") {
        DateTime dt1(1234567890);
        DateTime dt2(9876543210);
        
        std::stringstream ss;
        int64_t timestamp1 = dt1.getTimestamp();
        int64_t timestamp2 = dt2.getTimestamp();
        ss.write(reinterpret_cast<const char*>(&timestamp1), sizeof(timestamp1));
        ss.write(reinterpret_cast<const char*>(&timestamp2), sizeof(timestamp2));
        
        ss.seekg(0);
        DateTime restored1 = DateTime::readBinary(ss);
        DateTime restored2 = DateTime::readBinary(ss);
        
        REQUIRE(restored1.getTimestamp() == dt1.getTimestamp());
        REQUIRE(restored2.getTimestamp() == dt2.getTimestamp());
    }
}

TEST_CASE("DateTime comparison operators", "[datetime]") {
    SECTION("Equality operators work") {
        DateTime dt1(1000);
        DateTime dt2(1000);
        DateTime dt3(2000);
        
        REQUIRE(dt1 == dt2);
        REQUIRE(dt1 != dt3);
    }
    
    SECTION("Ordering operators work") {
        DateTime dt1(1000);   // Earlier
        DateTime dt2(2000);   // Later
        DateTime dt3(1000);   // Same as dt1
        
        REQUIRE(dt1 < dt2);
        REQUIRE(dt2 > dt1);
        REQUIRE(dt1 <= dt3);
        REQUIRE(dt3 >= dt1);
        REQUIRE_FALSE(dt2 < dt1);
        REQUIRE_FALSE(dt1 > dt2);
    }
}

TEST_CASE("DateTime static methods", "[datetime]") {
    SECTION("now() returns current time") {
        DateTime dt1 = DateTime::now();
        DateTime dt2 = DateTime::now();
        
        // Both should be recent
        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        REQUIRE(dt1.getTimestamp() <= now);
        REQUIRE(dt2.getTimestamp() <= now);
        
        // Should be close in time (within 1 second)
        REQUIRE(std::abs(dt1.getTimestamp() - dt2.getTimestamp()) <= 1);
    }
}