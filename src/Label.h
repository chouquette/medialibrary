#ifndef LABEL_H
#define LABEL_H

#include <sqlite3.h>
#include <string>

class File;

#include "ILabel.h"
#include "Cache.h"

namespace policy
{
struct LabelTable
{
    static const std::string Name;
    static const std::string CacheColumn;
};
}

class Label : public ILabel, public Cache<Label, ILabel, policy::LabelTable>
{
    public:

        Label(sqlite3* dbConnection, sqlite3_stmt* stmt);
        Label( const std::string& name );

    public:
        virtual unsigned int id() const;
        virtual const std::string& name();
        virtual std::vector<FilePtr>& files();
        bool insert( sqlite3* dbConnection );

        static bool createTable( sqlite3* dbConnection );
    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        std::vector<std::shared_ptr<IFile>>* m_files;

        friend class Cache<Label, ILabel, policy::LabelTable>;
};

#endif // LABEL_H
