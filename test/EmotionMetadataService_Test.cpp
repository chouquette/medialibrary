#include "Tests.h"

#include <condition_variable>
#include <mutex>
#include "IFile.h"
#include "IMediaLibrary.h"

#include "metadata_services/emotion/Emotion.h"

class EmotionMetadataServiceCb : public IMetadataCb
{
public:
    std::condition_variable waitCond;
    std::mutex mutex;

    virtual void onMetadataUpdated( FilePtr )
    {
        std::unique_lock<std::mutex> lock( mutex );
        waitCond.notify_all();
    }
};

class EmotionMetadataService_Tests : public Tests
{
protected:
    static std::unique_ptr<EmotionMetadataServiceCb> cb;

public:
    static void SetUpTestCase()
    {
        cb.reset( new EmotionMetadataServiceCb );
    }

    virtual void SetUp() override
    {
        Tests::SetUp();
        auto emotionService = std::unique_ptr<EmotionMetadataService>( new EmotionMetadataService );
        ml->addMetadataService( std::move( emotionService ) );
    }
};

std::unique_ptr<EmotionMetadataServiceCb> EmotionMetadataService_Tests::cb;


TEST_F( EmotionMetadataService_Tests, ParseAudio )
{
    auto file = ml->addFile( "mr-zebra.mp3" );
    std::unique_lock<std::mutex> lock( cb->mutex );
    ml->parse( file, cb.get() );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return file->audioTracks().size() > 0;
    } );
    ASSERT_TRUE( res );
}
