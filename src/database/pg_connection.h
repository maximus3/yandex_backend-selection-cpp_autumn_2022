#ifndef DISK_REST_API_PG_CONNECTION_H
#define DISK_REST_API_PG_CONNECTION_H

#include <memory>
#include <mutex>
#include <libpq-fe.h>

class PGConnection
{
public:
    PGConnection();
    std::shared_ptr<PGconn> GetConnection() const;
    bool ExecuteSQL(const std::string& sql);

private:
    void establish_connection();
    void LoadEnvVariables();

    std::string m_dbhost;
    int         m_dbport;
    std::string m_dbname;
    std::string m_dbuser;
    std::string m_dbpass;

    std::shared_ptr<PGconn>  m_connection;
};

#endif //DISK_REST_API_PG_CONNECTION_H