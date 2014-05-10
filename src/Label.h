#ifndef LABEL_H
#define LABEL_H

#include <sqlite3.h>
#include <string>

#include "ILabel.h"

class Label : public ILabel
{
    public:
        Label(sqlite3* dbConnection, sqlite3_stmt* stmt);

    public:
        virtual const std::string& name();
        virtual std::vector<IFile*> files();

        static bool createTable( sqlite3* dbConnection );

    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        std::vector<IFile*>* m_files;
};

#endif // LABEL_H
