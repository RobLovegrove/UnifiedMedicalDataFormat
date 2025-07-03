#include "patient.hpp"
#include "Xref/xref.hpp"
#include "Header/header.hpp"

using namespace std;

    nlohmann::json Patient::to_json() const {
        return {
            {"id", id},
            {"firstName", firstName},
            {"lastName", lastName},
            {"birthDate", birthDate},
            {"gender", gender}
        };
    }


    Patient Patient::from_json(const nlohmann::json& j) {
        Patient p;
        p.id = j.value("id", "");
        p.firstName = j.value("firstName", "");
        p.lastName = j.value("lastName", "");
        p.birthDate = j.value("birthDate", "");
        p.gender = j.value("gender", "");
        return p;
    }

    bool Patient::writeToFile(std::ofstream& outfile, XRefTable& xref) {

        uint64_t offset = 0;
        if (!getCurrentFilePosition(outfile, offset)) { return false; };

        nlohmann::json header;

        header["moduleType"] = "patient";
        header["schema"] = schema;
        header["compression"] = nullptr;  
        header["dataFormat"] = "json";
        header["encoding"] = "utf-8";
        header["moduleID"] = 1;





        return false;
    }