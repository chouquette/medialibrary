#include "MediaLibrary.h"

IMediaLibrary* MediaLibraryFactory::create()
{
    return new MediaLibrary();
}
