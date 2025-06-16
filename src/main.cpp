#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio> 
#include <nlohmann/json.hpp> 

using namespace std;
using json = nlohmann::json;

struct ModuleDescriptor {
    string name;
    string type;
    string resourceType;
    uint64_t offset;
    uint64_t length;
};

int main() {
    return 0;
}