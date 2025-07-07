#include "patient.hpp"
#include "Xref/xref.hpp"
#include "Header/header.hpp"
#include "Utility/uuid.hpp"

using namespace std;

    nlohmann::json Patient::to_json() const {
        return {
            {"patient_id", id},
            {"name", {
                {"family", familyName},
                {"given", givenNames}
            }},
            {"birth_date", birthDate},
            {"gender", gender},
            {"birth_sex", birthSex}
        };
    }

    Patient Patient::from_json(const nlohmann::json& j) {
        Patient p;
        p.id = j.value("patient_id", "");
        p.familyName = j.at("name").at("family").get<std::string>();
        p.givenNames = j.at("name").at("given").get<std::vector<std::string>>();
        p.birthDate = j.value("birth_date", "");
        p.gender = j.value("gender", "");
        p.birthSex = j.value("birth_sex", "");
        return p;
    }

    bool Patient::writeToFile(std::ofstream& outfile, XRefTable& xref) const {

        UUID moduleID;

        uint64_t offset = 0;
        if (!getCurrentFilePosition(outfile, offset)) { return false; };

        nlohmann::json header;

        header["moduleType"] = "patient";
        header["schema"] = schema;
        header["compression"] = nullptr;  
        header["dataFormat"] = "json";
        header["encoding"] = "utf-8";
        header["moduleID"] = moduleID.toString();

        string headerStr = header.dump(4);
        uint32_t headerSize = static_cast<uint32_t>(headerStr.size());

        outfile.write(reinterpret_cast<const char*>(&headerSize), sizeof(headerSize));
        outfile.write(headerStr.c_str(), headerSize);

        nlohmann::json patientJSON = this->to_json();
        string patientStr = patientJSON.dump(4); 
        uint32_t dataSize = static_cast<uint32_t>(patientStr.size());

        outfile.write(patientStr.c_str(), dataSize);

        xref.addEntry(ModuleType::Patient, moduleID, offset, headerSize + dataSize);

        return outfile.good();
    }