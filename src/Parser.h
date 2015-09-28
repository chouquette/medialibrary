#ifndef PARSER_H
#define PARSER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>

#include "IMetadataService.h"
#include "IMediaLibrary.h"

class Parser : public IMetadataServiceCb
{
    public:

    public:
        Parser();
        ~Parser();
        void addService(std::unique_ptr<IMetadataService> service );
        void parse( FilePtr file, IMetadataCb* cb );

    private:
        virtual void done( FilePtr file, ServiceStatus status, void* data );
        void run();

    private:
        typedef std::unique_ptr<IMetadataService> ServicePtr;
        typedef std::vector<ServicePtr> ServiceList;
        struct Task
        {
            Task(FilePtr file, ServiceList& serviceList , IMetadataCb* metadataCb);
            FilePtr                 file;
            ServiceList::iterator   it;
            ServiceList::iterator   end;
            IMetadataCb*            cb;
        };

    private:
        ServiceList m_services;
        std::vector<Task*> m_tasks;
        std::unique_ptr<std::thread> m_thread;
        std::mutex m_lock;
        std::condition_variable m_cond;
        std::atomic_bool m_stopParser;
};

#endif // PARSER_H
