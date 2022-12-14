#ifndef DISK_REST_API_SCHEMAS_SYSTEM_ITEM_IMPORT_REQUEST_H
#define DISK_REST_API_SCHEMAS_SYSTEM_ITEM_IMPORT_REQUEST_H

#include <string>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

#include "validators.h"
#include "base_schema.h"
#include "system_item_import_schema.h"
#include "pg_connection.h"
#include "status.h"

namespace schemas {

    class SystemItemImportRequest : public BaseSchema {
    public:
        const std::vector<SystemItemImportSchema> items;
        const std::string updateDate;

        SystemItemImportRequest() = default;

        SystemItemImportRequest(
                std::vector<SystemItemImportSchema> items,
                std::string updateDate
        )
                : items(std::move(items))
                , updateDate(std::move(updateDate)) {}

        json to_json() const override {
            json j;
            j["items"] = json::array();
            for (const auto &item : items) {
                j["items"].push_back(item.to_json());
            }
            j["updateDate"] = updateDate;
            return j;
        }

        static SystemItemImportRequest from_json(const json& j);

        database::Status database_save(const std::shared_ptr<PGConnection>& a_PGConnection, std::stringstream& a_StatusStream) const;
    };
}

#endif //DISK_REST_API_SCHEMAS_SYSTEM_ITEM_IMPORT_REQUEST_H
