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
        Label( DBConnection dbConnection, sqlite3_stmt* stmt);
        Label( const std::string& name );

    public:
        virtual unsigned int id() const;
        virtual const std::string& name();
        virtual bool files( std::vector<FilePtr>& files );

        static LabelPtr create( DBConnection dbConnection, const std::string& name );
        static bool createTable( DBConnection dbConnection );
    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_name;

        friend class Cache<Label, ILabel, policy::LabelTable>;
        friend struct policy::LabelTable;
};

#endif // LABEL_H
