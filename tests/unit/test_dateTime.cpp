#include "Utility/dateTime.hpp"

#include <catch2/catch_all.hpp>

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