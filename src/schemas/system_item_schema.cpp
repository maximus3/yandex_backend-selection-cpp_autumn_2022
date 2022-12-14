#include <iostream>
#include "system_item_schema.h"

namespace schemas {

    SystemItemSchema SystemItemSchema::from_json(const json& j) {
        const std::vector<std::string>& available_fields = {
                "id",
                "url",
                "date",
                "parentId",
                "type",
                "size",
                "children"
        };
        validate_fields(j, available_fields);

        auto _id = j.at("id").get<std::string>();

        std::optional<std::string> _url;
        if (j.contains("url") && !j.at("url").is_null()) {
            _url = j.at("url").get<std::string>();
        } else {
            _url = std::nullopt;
        }

        auto _date = j.at("date").get<std::string>();

        std::optional<std::string> _parentId;
        if (j.contains("parentId") && !j.at("parentId").is_null()) {
            _parentId = j.at("parentId").get<std::string>();
        } else {
            _parentId = std::nullopt;
        }

        auto _type = to_system_item(j.at("type").get<std::string>());

        std::optional<int64_t> _size;
        if (j.contains("size") && !j.at("size").is_null()) {
            //std::cerr << __FILE__ << __LINE__ << ": size: " << j.at("size") << std::endl;
            _size = j.at("size").get<int64_t>();
            //std::cerr << __FILE__ << __LINE__ << ": size: " << _size.value() << std::endl;
        } else {
            _size = std::nullopt;
        }

        std::optional<std::vector<SystemItemSchema>> _children;
        if (j.contains("children") && !j.at("children").is_null()) {
            _children = std::vector<SystemItemSchema>();
            for (auto& child : j.at("children")) {
                _children->push_back(SystemItemSchema::from_json(child));
            }
        } else {
            _children = std::nullopt;
        }

        // Change children FILE to None, Folder to []
        if (_type == SystemItemType::FILE) {
            _children = std::nullopt;
        } else if (_type == SystemItemType::FOLDER && !_children.has_value()) {
            _children.emplace(std::vector<SystemItemSchema>());
        }

        return {
                std::move(_id),
                std::move(_url),
                std::move(_date),
                std::move(_parentId),
                _type,
                _size,
                std::move(_children)
        };
    }

    database::Status SystemItemSchema::database_save(const std::shared_ptr<PGConnection>& a_PGConnection) {
        return database::Status::DATABASE_ERROR;
    }

    database::Status SystemItemSchema::database_delete(const std::shared_ptr<PGConnection>& a_PGConnection, std::stringstream& a_StatusStream) {
        const char command[] = "DELETE FROM system_item WHERE id = $1";
        int nParams = 1;
        const char* paramValues[] = {
            id.c_str()
        };
        const int paramLengths[] = {
            sizeof(paramValues[0])
        };
        const int paramFormats[] = {0};
        int resultFormat = 0;
        PGresult *res = PQexecParams(
            a_PGConnection->GetConnection().get(),
            command,
            nParams,
            nullptr,
            paramValues,
            paramLengths,
            paramFormats,
            resultFormat);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            a_StatusStream << "DELETE failed: " << PQresultErrorMessage(res)
                           << std::endl;
            return database::Status::DATABASE_ERROR;
        }
        return database::Status::OK;
    }

    database::Status SystemItemSchema::database_get(const std::shared_ptr<PGConnection>& a_PGConnection, std::stringstream& a_StatusStream, std::vector<SystemItemSchema>& a_ReturnVector, const std::string& a_Field, const std::string& a_Value, bool need_children) {
//        const char command[] = "SELECT id, url, date, parentId, type, size FROM system_item WHERE $1 = $2;";
//        int nParams = 2;
//        const char* paramValues[] = {
//            a_Field.c_str(),
//            a_Value.c_str()
//        };
//        const int paramLengths[] = {
//            sizeof(paramValues[0]),
//            sizeof(paramValues[1])
//        };
//        const int paramFormats[] = {0, 0};
//        int resultFormat = 0;
//        PGresult *res = PQexecParams(
//            a_PGConnection->GetConnection().get(),
//            sql.str().c_str(),
//            nParams,
//            nullptr,
//            paramValues,
//            paramLengths,
//            paramFormats,
//            resultFormat);
        std::stringstream sql;
        sql << "SELECT id, url, date, parentId, type, size FROM system_item WHERE " << a_Field << " = '" << a_Value << "';";
        PGresult *res = PQexec(a_PGConnection->GetConnection().get(), sql.str().c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            a_StatusStream << "SELECT failed: " << PQresultErrorMessage(res);
            return database::Status::DATABASE_ERROR;
        } else {
            // std::cerr << "Get " << PQntuples(res) << " tuples, each tuple has "
            //           << PQnfields(res) << " fields" << std::endl;
            for (int i = 0; i < PQntuples(res); i++) {
                // std::cerr << "Tuple " << i << std::endl;
                json js;
                for (int j = 0; j < PQnfields(res); j++) {
                    if (PQgetisnull(res, i, j)) {
                        continue;
                    }
                    auto column_name = strcmp(PQfname(res, j), "parentid") ? PQfname(res, j): "parentId";
                    auto column_value = PQgetvalue(res, i, j);
                    js[column_name] = column_value;
                    if (strcmp(column_name, "size") == 0) {
                        std::cerr << __FILE__ << " " << __LINE__ << ": size: " << column_value << std::endl;
                        js[column_name] = std::stoll(column_value);
                    }
                }
                //std::cerr << "JSON: " << js.dump() << std::endl;
                js["children"] = nullptr;
                if (need_children) {
                    // std::cerr << "Need children" << std::endl;
                    js["children"] = json::array();
                    std::vector<SystemItemSchema> children;
                    database::Status status = database_get(a_PGConnection, a_StatusStream, children, "parentId",
                                                           js.at("id"), need_children = true);
                    if (status != database::Status::OK) {
                        return status;
                    }
                    for (auto &child: children) {
                        js["children"].push_back(child.to_json());
                    }
                }
                // std::cerr << "Children done" << std::endl;
                a_ReturnVector.push_back(SystemItemSchema::from_json(js));
                // std::cerr << "Tuple " << i << " done" << std::endl;
            }
            // std::cerr << "Size of a_ReturnVector: " << a_ReturnVector.size() << std::endl;
        }
        return database::Status::OK;
    }
    database::Status SystemItemSchema::database_get(const std::shared_ptr<PGConnection>& a_PGConnection, std::stringstream& a_StatusStream, std::optional<SystemItemSchema>& a_ReturnValue, const std::string& a_Field, const std::string& a_Value, bool need_children) {
        std::vector<SystemItemSchema> returnVector;
        a_ReturnValue = std::nullopt;
        database::Status status = database_get(a_PGConnection, a_StatusStream, returnVector, a_Field, a_Value, need_children);
        if (status != database::Status::OK) {
            return status;
        }
        if (returnVector.empty()) {
            return database::Status::OK;
        } else if (returnVector.size() == 1) {
            a_ReturnValue.emplace(returnVector[0]);
            return database::Status::OK;
        } else {
            a_StatusStream << "DB returned more than one item with " << a_Field << " = " << a_Value;
            return database::Status::DATABASE_ERROR;
        }
    }
}
