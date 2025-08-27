#include "Utility/dateTime.hpp"

#include <catch2/catch_all.hpp>

// TEST_CASE("DateTime constructors", "[datetime]") {
//     SECTION("Default constructor sets current time") {
//         DateTime dt1;
//         REQUIRE(dt1.getUnixTimestamp() > 0);
//     }
    
//     SECTION("Unix timestamp constructor works") {
//         int64_t knownTimestamp = 1234567890;
//         DateTime dt2(knownTimestamp);
//         REQUIRE(dt2.getUnixTimestamp() == knownTimestamp);
//     }
// }