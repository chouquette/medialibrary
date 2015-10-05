#include "FsDiscoverer.h"

#include <algorithm>
#include "factory/FileSystem.h"
#include "File.h"
#include "Folder.h"
#include <queue>
#include <iostream>

FsDiscoverer::FsDiscoverer( std::shared_ptr<factory::IFileSystem> fsFactory, IMediaLibrary* ml, DBConnection dbConn )
    : m_ml( ml )
    , m_dbConn( dbConn )
{
    if ( fsFactory != nullptr )
        m_fsFactory = fsFactory;
    else
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );
}

bool FsDiscoverer::discover( const std::string &entryPoint )
{
    // Assume :// denotes a scheme that isn't a file path, and refuse to discover it.
    if ( entryPoint.find( "://" ) != std::string::npos )
        return false;

    {
        auto f = Folder::fetch( m_dbConn, entryPoint );
        // If the folder exists, we assume it is up to date
        if ( f != nullptr )
            return true;
    }
    // Otherwise, create a directory, and check it for modifications
    std::unique_ptr<fs::IDirectory> fsDir;
    try
    {
        fsDir = m_fsFactory->createDirectory( entryPoint );
    }
    catch (std::exception& ex)
    {
        LOG_ERROR("Failed to create an IDirectory for ", entryPoint, ": ", ex.what());
        return false;
    }
    auto f = Folder::create( m_dbConn, fsDir.get(), 0 );
    if ( f == nullptr )
        return false;
    checkSubfolders( fsDir.get(), f );
    return true;
}

void FsDiscoverer::reload()
{
    //FIXME: This should probably be in a sql transaction
    //FIXME: This shouldn't be done for "removable"/network files
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent IS NULL";
    auto rootFolders = sqlite::Tools::fetchAll<Folder, IFolder>( m_dbConn, req );
    for ( const auto f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder->lastModificationDate() == f->lastModificationDate() )
            continue;
        checkSubfolders( folder.get(), f );
        f->setLastModificationDate( folder->lastModificationDate() );
    }
}

bool FsDiscoverer::checkSubfolders( fs::IDirectory* folder, FolderPtr parentFolder )
{
    // From here we can have:
    // - New subfolder(s)
    // - Deleted subfolder(s)
    // - New file(s)
    // - Deleted file(s)
    // - Changed file(s)
    // ... in this folder, or in all the sub folders.

    // Load the folders we already know of:
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent = ?";
    auto subFoldersInDB = sqlite::Tools::fetchAll<Folder, IFolder>( m_dbConn, req, parentFolder->id() );
    for ( const auto& subFolderPath : folder->dirs() )
    {
        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [subFolderPath](const std::shared_ptr<IFolder>& f) {
            return f->path() == subFolderPath;
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            //FIXME: In order to add the new folder, we need to use the same discoverer.
            // This probably means we need to store which discoverer was used to add which file
            // and store discoverers as a map instead of a vector
            continue;
        }
        auto subFolder = m_fsFactory->createDirectory( subFolderPath );
        if ( subFolder->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Remove all folders that still exist in FS. That way, the list of folders that
            // will still be in subFoldersInDB when we're done is the list of folders that have
            // been deleted from the FS
            subFoldersInDB.erase( it );
            continue;
        }
        // This folder was modified, let's recurse
        checkSubfolders( subFolder.get(), *it );
        checkFiles( subFolder.get(), *it );
        (*it)->setLastModificationDate( subFolder->lastModificationDate() );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        LOG_INFO( "Folder ", f->path(), " not found in FS, deleting it" );
        m_ml->deleteFolder( f );
    }
    return true;
}

void FsDiscoverer::checkFiles( fs::IDirectory* folder, FolderPtr parentFolder )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE folder_id = ?";
    auto files = sqlite::Tools::fetchAll<File, IFile>( m_dbConn, req, parentFolder->id() );
    for ( const auto& filePath : folder->files() )
    {        
        auto it = std::find_if( begin( files ), end( files ), [filePath](const std::shared_ptr<IFile>& f) {
            return f->mrl() == filePath;
        });
        if ( it == end( files ) )
        {
            m_ml->addFile( filePath, parentFolder );
            continue;
        }
        auto file = m_fsFactory->createFile( filePath );
        if ( file->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        m_ml->deleteFile( filePath );
        m_ml->addFile( filePath, parentFolder );
        files.erase( it );
    }
    for ( auto file : files )
    {
        m_ml->deleteFile( file );
    }
}
