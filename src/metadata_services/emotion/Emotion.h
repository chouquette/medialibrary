#pragma once

#include "IMetadataService.h"

class EmotionMetadataService : public IMetadataService
{
public:
    EmotionMetadataService();
    virtual ~EmotionMetadataService();
    virtual bool initialize(IMetadataServiceCb *callback, IMediaLibrary *ml) override;
    virtual unsigned int priority() const override;
    virtual bool run(FilePtr file, void *data) override;

private:
    //FIXME: This should be a shared_ptr
    IMetadataServiceCb* m_cb;
    IMediaLibrary* m_ml;
    bool m_initialized;
};
