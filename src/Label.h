#ifndef LABEL_H
#define LABEL_H

#include <sqlite3.h>
#include <string>

#include "ILabel.h"
class File;

class Label : public ILabel
{
    public:
        Label(sqlite3* dbConnection, sqlite3_stmt* stmt);
        Label( const std::string& name );

    public:
        virtual unsigned int id() const;
        virtual const std::string& name();
        virtual std::vector<IFile*> files();
        bool insert( sqlite3* dbConnection );

        static bool createTable( sqlite3* dbConnection );
        bool link( IFile* file );
        bool unlink( IFile* file ) const;
    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        std::vector<IFile*>* m_files;
};

#endif // LABEL_H
