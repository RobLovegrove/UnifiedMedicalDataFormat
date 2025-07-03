#ifndef PATIENT_HPP
#define PATIENT_HPP

#include <string>
#include <nlohmann/json.hpp> 
#include <Xref/xref.hpp>

class Patient {
private:
    std::string id;
    std::string firstName;
    std::string lastName;
    std::string birthDate;
    std::string gender;

    std::string schema = "http://localhost:8080/schemas/patient/v1.0.json";

    nlohmann::json to_json() const;
    static Patient from_json(const nlohmann::json& j);

public:
    bool writeToFile(std::ofstream& outfile, XRefTable& xref);
    string getSchema() const { return schema; }


};

#endif