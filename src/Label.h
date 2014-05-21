#ifndef LABEL_H
#define LABEL_H

#include <sqlite3.h>
#include <string>

class File;
class Label;

#include "ILabel.h"
#include "Cache.h"

namespace policy
{
struct LabelTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Label::*const PrimaryKey;
};

struct LabelCachePolicy
{
    typedef std::string KeyType;
    static const std::string& key(const std::shared_ptr<ILabel> self );
    static std::string key( sqlite3_stmt* stmt );
};

}

class Label : public ILabel, public Cache<Label, ILabel, policy::LabelTable, policy::LabelCachePolicy>
{
    private:
        typedef Cache<Label, ILabel, policy::LabelTable, policy::LabelCachePolicy> _Cache;
    public:
        Label(sqlite3* dbConnection, sqlite3_stmt* stmt);
        Label( const std::string& name );

    public:
        virtual unsigned int id() const;
        virtual const std::string& name();
        virtual std::vector<FilePtr>& files();

        static LabelPtr create(sqlite3* dbConnection, const std::string& name );
        static bool createTable( sqlite3* dbConnection );
    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        std::vector<std::shared_ptr<IFile>>* m_files;

        friend class Cache<Label, ILabel, policy::LabelTable>;
        friend class policy::LabelTable;
};

#endif // LABEL_H
