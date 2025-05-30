# Declare 2 convenience libraries with common code, but do not link directly
# with it, otherwise meson would propagate the sqlite dependency when linking
# with it, while medialibrary already linked with sqlite, causing multiple
# definition of sqlite symbols

common_test_code = static_library('common_test_code',
  [
    'MediaLibraryTester.cpp',
    'mocks/FileSystem.cpp',
    'mocks/filesystem/MockDevice.cpp',
    'mocks/filesystem/MockDirectory.cpp',
    'mocks/filesystem/MockFile.cpp',
  ],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
)

common_test_objs = common_test_code.extract_all_objects(recursive: false)

album_test_exe = executable('album_tests', 'AlbumTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

album_tests = [
  'Create',
  'Fetch',
  'AddTrack',
  'RemoveTrack',
  'NbTrack',
  'TracksByGenre',
  'SetReleaseDate',
  'SetShortSummary',
  'GetThumbnail',
  'FetchAlbumFromTrack',
  'Artists',
  'AlbumArtist',
  'SortAlbumThenArtist',
  'SearchByTitle',
  'SearchByArtist',
  'SearchNoDuplicate',
  'SearchNoUnknownAlbum',
  'SearchAfterDeletion',
  'SearchAfterArtistUpdate',
  'AutoDelete',
  'SortTracks',
  'Sort',
  'SortByPlayCount',
  'SortByArtist',
  'SortByNonSensical',
  'Duration',
  'SearchAndSort',
  'SearchTracks',
  'NbDiscs',
  'CheckDbModel',
  'SortByDuration',
  'SortByInsertionDate',
  'ConvertToExternal',
  'Public',
  'Favorite',
]
foreach t : album_tests
  test('Album.' + t, album_test_exe, args: t, suite: ['unittest', 'Album'])
endforeach

device_fs_test_exe = executable('device_fs_tests', 'DeviceFsTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: false,
)

device_fs_tests = [
  'RemoveDisk',
  'UnmountDisk',
  'ReplugDisk',
  'ReplugDiskWithExtraFiles',
  'RemoveAlbumAndArtist',
  'RemoveArtist',
  'PartialAlbumRemoval',
  'ChangeDevice',
  'UnknownMountpoint',
  'OutdatedDevices',
  'RemovableMountPointName',
  'RemoveShowEpisodes',
  'PartialRemoveShowEpisodes',
  'MediaGroupPresence',
  'GenrePresence',
  'PlaylistPresence',
  'FolderPresence',
  'CompareMountpoints',
  'RelativeMrl',
  'RemoveMissingAlbumTrack',
]

foreach t : device_fs_tests
  test('DeviceFs.' + t, device_fs_test_exe, args: t, suite: ['unittest', 'DeviceFs'])
endforeach

device_test_exe = executable('device_tests', 'DeviceTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

device_tests = [
  'Create',
  'SetPresent',
  'CheckDbModel',
  'MultipleScheme',
  'IsKnown',
  'DeleteRemovable',
  'Mountpoints',
  'FetchCachedMountpoint',
  'UpdateLastSeen',
]

foreach t : device_tests
  test('Device.' + t, device_test_exe, args: t, suite: ['unittest', 'Device'])
endforeach

album_track_tests = [
  'Create',
  'GetAlbum',
]

album_track_test_exe = executable('album_track_tests', 'AlbumTrackTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

foreach t : album_track_tests
  test('AlbumTrack.' + t, album_track_test_exe, args: t, suite: ['unittest', 'AlbumTrack'])
endforeach

misc_test_exe = executable('misc_tests', 'MiscTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  cpp_args: '-DSRC_DIR="' + meson.project_source_root() + '"',
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

misc_tests = [
  'MediaExtensions',
  'PlaylistExtensions',
  'SubtitleExtensions',
  'TrimString',
  'SanitizePattern',
  'Utf8NbChars',
  'Utf8NbBytes',
  'XmlEncode',
  'Defer',
  'XxHashBuffer',
  'XxHashFile',
  'CheckTaskDbModel',
  'ClearDatabaseKeepPlaylist',
  'ClearDatabase',
  'DateFromStr',
  'FilenameCollate',
]

foreach t : misc_tests
  test('Misc.' + t, misc_test_exe, args: t, suite: ['unittest', 'Misc'])
endforeach

db_model_test_exe = executable('db_model_tests', 'DbModelTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  cpp_args: '-DSRC_DIR="' + meson.project_source_root() + '"',
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

db_model_tests = [
  'NbTriggers',
  'Upgrade3to5',
  'Upgrade15to16',
  'Upgrade16to17',
  'Upgrade17to18',
  'Upgrade18to19Broken',
  'Upgrade18to19Noop',
  'Upgrade19to20',
  'Upgrade20to21',
  'Upgrade21to22',
  'Upgrade22to23',
  'Upgrade23to24',
  'Upgrade24to25',
  'Upgrade25to26',
  'Upgrade26to27',
  'Upgrade27to28',
  'Upgrade29to30',
  'Upgrade30to31',
  'Upgrade31to32',
  'Upgrade32to33',
  'Upgrade33to34',
  'Upgrade34to35',
  'Upgrade35to36',
  'Upgrade36to37',
  'Upgrade37to38',
  'Upgrade38to39',
]

foreach t : db_model_tests
  test('DbModel.' + t, db_model_test_exe, args: t, suite: ['unittest', 'DbModel'])
endforeach

artist_test_exe = executable('artist_tests', 'ArtistTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

artist_tests = [
  'Create',
  'ShortBio',
  'ArtworkMrl',
  'GetThumbnail',
  'Albums',
  'NbAlbums',
  'AllSongs',
  'GetAll',
  'GetAllNoAlbum',
  'UnknownAlbum',
  'MusicBrainzId',
  'Search',
  'SearchAfterDelete',
  'SortMedia',
  'SortMediaByAlbum',
  'SortAlbum',
  'Sort',
  'DeleteWhenNoAlbum',
  'UpdateNbTracks',
  'SortTracksMultiDisc',
  'CheckQuery',
  'SearchAlbums',
  'SearchTracks',
  'SearchAll',
  'CheckDbModel',
  'SortByNbAlbums',
  'SortByNbTracks',
  'SortByLastPlayedDate',
  'Public',
  'Favorite',
]

foreach t : artist_tests
  test('Artist.' + t, artist_test_exe, args: t, suite: ['unittest', 'Artist'])
endforeach

audio_track_test_exe = executable('audio_track_tests', 'AudioTrackTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

audio_track_tests = [
  'AddTrack',
  'GetSetProperties',
  'FetchTracks',
  'RemoveTracks',
  'CheckDbModel',
  'UnlinkExternalTrack',
]

foreach t : audio_track_tests
  test('AudioTrack.' + t, audio_track_test_exe, args: t, suite: ['unittest', 'AudioTrack'])
endforeach

bookmark_test_exe = executable('bookmark_tests', 'BookmarkTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

bookmark_tests = [
  'Create',
  'SetName',
  'SetDescription',
  'SetNameAndDesc',
  'List',
  'SortByName',
  'Delete',
  'UniqueTime',
  'Move',
  'DeleteAll',
  'CheckDbModel',
  'OrderByCreationDate',
  'Fetch',
  'FetchByTime',
  'DeleteMedia',
]

foreach t : bookmark_tests
  test('Bookmark.' + t, bookmark_test_exe, args: t, suite: ['unittest', 'Bookmark'])
endforeach

chapter_test_exe = executable('chapter_tests', 'ChapterTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

chapter_tests = [
  'Create',
  'Fetch',
  'CheckDbModel',
]

foreach t : chapter_tests
  test('Chapter.' + t, chapter_test_exe, args: t, suite: ['unittest', 'Chapter'])
endforeach

discoverer_test_exe = executable('discoverer_tests', 'DiscovererTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

discoverer_tests = [
  'SimpleEnqueue',
  'FilterDoubleEnqueue',
  'DontFilterUnrelatedDoubleEnqueue',
  'ReduceDiscoverRemove',
  'ReduceBanUnban',
  'InterruptDiscover',
  'InterruptReload',
]

foreach t : discoverer_tests
  test('Discoverer.' + t, discoverer_test_exe, args: t, suite: ['unittest', 'Discoverer'])
endforeach

file_test_exe = executable('file_tests', 'FileTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

file_tests = [
  'Create',
  'Remove',
  'GetMedia',
  'SetMrl',
  'UpdateFsInfo',
  'Exists',
  'CheckDbModel',
  'SetMediaId',
  'ByMrlNetwork',
]

foreach t : file_tests
  test('File.' + t, file_test_exe, args: t, suite: ['unittest', 'File'])
endforeach

folder_test_exe = executable('folder_tests', 'FolderTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: false,
)

folder_tests = [
  'Add',
  'Load',
  'InvalidPath',
  'List',
  'ListFolders',
  'NewFolderWithFile',
  'NewFileInSubFolder',
  'RemoveFileFromDirectory',
  'RemoveDirectory',
  'UpdateFile',
  'Ban',
  'DiscoverBanned',
  'BanAfterDiscovery',
  'RemoveFromBannedList',
  'BanTwice',
  'BanNonExistant',
  'UnbanNonExistant',
  'NoMediaBeforeDiscovery',
  'InsertNoMedia',
  'InsertNoMediaInRoot',
  'ReloadSubDir',
  'FetchRoots',
  'RemoveRootRoot',
  'RemoveRoot',
  'RemoveNonExistantRoot',
  'RemoveRootFolder',
  'NbMedia',
  'NbMediaDeletionTrigger',
  'IsIndexedDiscovered',
  'IsIndexedNonDiscovered',
  'IsIndexedMultipleMountpoint',
  'IsBannedFolderIndexed',
  'ListWithMedia',
  'SearchMedia',
  'ListSubFolders',
  'SearchFolders',
  'Name',
  'IsBanned',
  'BannedRoots',
  'CheckDbModel',
  'NbMediaAfterExternalInternalConversion',
  'Duration',
  'SetPublic',
  'Favorite',
]

foreach t : folder_tests
  test('Folder.' + t, folder_test_exe, args: t, suite: ['unittest', 'Folder'])
endforeach

fs_utils_test_exe = executable('fs_utils_tests', 'FsUtilsTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

fs_utils_tests = [
  'extension',
  'directory',
  'directoryName',
  'fileName',
  'firstFolder',
  'removePath',
  'parentFolder',
  'toMrl',
  'splitPath',
  'stripExtension',
  'ToFolderPath',
]

foreach t : fs_utils_tests
  test('FsUtils.' + t, fs_utils_test_exe, args: t, suite: ['unittest', 'FsUtils'])
endforeach

genre_test_exe = executable('genre_tests', 'GenreTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

genre_tests = [
  'Create',
  'List',
  'ListAlbumTracks',
  'ListArtists',
  'ListAlbums',
  'Search',
  'SearchAfterDelete',
  'SortTracks',
  'Sort',
  'NbTracks',
  'CaseInsensitive',
  'SearchArtists',
  'SearchTracks',
  'SearchAlbums',
  'WithThumbnail',
  'CheckDbModel',
  'GetThumbnails',
  'ConvertToExternal',
  'Public',
  'Favorite',
]

foreach t : genre_tests
  test('Genre.' + t, genre_test_exe, args: t, suite: ['unittest', 'Genre'])
endforeach

label_test_exe = executable('label_tests', 'LabelTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

label_tests = [
  'Add',
  'Remove',
  'Files',
  'Delete',
  'CheckDbModel',
]

foreach t : label_tests
  test('Label.' + t, label_test_exe, args: t, suite: ['unittest', 'Label'])
endforeach

media_group_test_exe = executable('media_groups_tests', 'MediaGroupTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

media_group_tests = [
  'Create',
  'ListAll',
  'FetchOne',
  'Search',
  'FetchMedia',
  'SearchMedia',
  'UpdateNbMediaTypeChange',
  'UpdateNbMediaNoDelete',
  'SortByNbMedia',
  'FetchFromMedia',
  'Rename',
  'CheckDbModel',
  'Delete',
  'DeleteMedia',
  'DeleteEmpty',
  'CommonPattern',
  'AssignToGroups',
  'CreateFromMedia',
  'CreateWithUnknownMedia',
  'CheckFromMediaName',
  'RemoveMedia',
  'RegroupLocked',
  'ForcedSingletonRestrictions',
  'AddToForcedSingleton',
  'RenameForcedSingleton',
  'UpdateDuration',
  'OrderByDuration',
  'CreationDate',
  'OrderByCreationDate',
  'LastModificationDate',
  'OrderByLastModificationDate',
  'Destroy',
  'RegroupAll',
  'MergeAutoCreated',
  'KoreanTitles',
  'DontReturnExternalMedia',
  'InsertRemoveExternalMedia',
  'ConvertMediaToExternal',
  'ConvertExternalMediaType',
  'ChangeExternalMediaGroup',
  'NbSeen',
  'Favorite',
]

foreach t : media_group_tests
  test('MediaGroup.' + t, media_group_test_exe, args: t, suite: ['unittest', 'MediaGroup'])
endforeach

fetch_media_test_exe = executable('fetch_media_tests', 'FetchMediaTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: false,
)

fetch_media_tests = [
  'FetchNonRemovable',
  'FetchRemovable',
  'FetchRemovableUnplugged',
]

foreach t : fetch_media_tests
  test('FetchMedia.' + t, fetch_media_test_exe, args: t, suite: ['unittest', 'FetchMedia'])
endforeach

media_test_exe = executable('media_tests', 'MediaTests.cpp',
  link_with: [medialib],
  objects: common_test_objs,
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: false,
)

media_tests = [
  'Create',
  'Fetch',
  'Duration',
  'GetThumbnail',
  'SetProgress',
  'SetLastPositionNoDuration',
  'Rating',
  'Search',
  'SearchAndSort',
  'SearchAfterEdit',
  'SearchAfterDelete',
  'SearchByLabel',
  'SearchTracks',
  'SortByLastPlayedDate',
  'SearchWeirdPatterns',
  'ListByType',
  'Favorite',
  'History',
  'StreamHistory',
  'HistoryByType',
  'ClearHistory',
  'RemoveFromHistory',
  'HistorySearch',
  'HistorySearchByType',
  'SetReleaseDate',
  'SortByAlpha',
  'SortByReleaseDate',
  'SortByLastModifDate',
  'SortByFileSize',
  'SortByFilename',
  'SetType',
  'GetMetadata',
  'MetadataOverride',
  'MetadataUnset',
  'MetadataGetBatch',
  'SetBatch',
  'MetadataCheckDbModel',
  'ExternalMrl',
  'AddStream',
  'DuplicatedExternalMrl',
  'SetTitle',
  'Pagination',
  'SortFilename',
  'CreateStream',
  'SearchExternal',
  'VacuumOldExternal',
  'VacuumNeverPlayedMedia',
  'RemoveExternal',
  'NbPlaylists',
  'SortByAlbum',
  'SetFilename',
  'SetPlayCount',
  'CheckDbModel',
  'SortByTrackId',
  'ForceTitle',
  'FetchInProgress',
  'FetchFromSubscriptions',
  'ConvertToExternal',
  'FlushUserProvidedThumbnails',
  'Lyrics',
  'ExternalMrlDifferentPorts',
  'MarkAsPlayed',
  'NbSubscriptions',
  'Description',
  'AddCachedMrl',
  'FilenameNumericalOrder',
]

foreach t : media_tests
  test('Media.' + t, media_test_exe, args: t, suite: ['unittest', 'Media'])
endforeach

movie_test_exe = executable('movie_tests', 'MovieTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

movie_tests = [
  'Create',
  'Fetch',
  'FetchAll',
  'SortByAlpha',
  'Search',
  'SetShortSummary',
  'SetImdbId',
  'AssignToFile',
  'CheckDbModel',
  'DeleteByMediaId',
]

foreach t : movie_tests
  test('Movie.' + t, movie_test_exe, args: t, suite: ['unittest', 'Movie'])
endforeach

playlist_test_exe = executable('playlist_tests', 'PlaylistTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

playlist_tests = [
  'Create',
  'CreateDuplicate',
  'Fetch',
  'DeletePlaylist',
  'SetName',
  'FetchAll',
  'Add',
  'Append',
  'AppendMany',
  'Insert',
  'InsertMany',
  'Move',
  'MoveMany',
  'Remove',
  'RemoveMany',
  'DeleteFile',
  'Search',
  'SearchAndSort',
  'SearchAfterDelete',
  'SearchAfterUpdate',
  'Sort',
  'AddDuplicate',
  'SearchMedia',
  'RemoveMedia',
  'ClearContent',
  'RemoveReAddMedia',
  'InsertRemoveDuplicateMedia',
  'AutoRemoveTask',
  'CheckDbModel',
  'IsReadOnly',
  'SortByCreationDate',
  'NbMedia',
  'Duration',
  'UpdateNbMediaOnInsertAndDelete',
  'UpdateNbMediaOnMediaRemoval',
  'UpdateNbMediaOnMediaTypeChange',
  'FilterByMediaType',
  'Public',
  'SortMediaRequest',
  'Favorite',
]

foreach t : playlist_tests
  test('Playlist.' + t, playlist_test_exe, args: t, suite: ['unittest', 'Playlist'])
endforeach

removal_notifier_test_exe = executable('removal_notifier_tests', 'RemovalNotifierTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: false,
)

removal_notifier_tests = [
  'DeleteOne',
  'DeleteBatch',
  'Flush',
  'ModifyPlaylists',
]

foreach t : removal_notifier_tests
  test('RemovalNotifier.' + t, removal_notifier_test_exe, args: t, suite: ['unittest', 'RemovalNotifier'])
endforeach

show_test_exe = executable('show_tests', 'ShowTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

show_tests = [
  'Create',
  'Fetch',
  'SetReleaseDate',
  'SetShortSummary',
  'SetArtworkMrl',
  'SetTvdbId',
  'AddEpisode',
  'FetchShowFromEpisode',
  'SetEpisodeSummary',
  'SetEpisodeTvdbId',
  'ListAll',
  'ListEpisodes',
  'Search',
  'RemoveFromFts',
  'FileSetShowEpisode',
  'SearchEpisodes',
  'CheckDbModel',
  'CheckShowEpisodeDbModel',
  'NbEpisodes',
  'DeleteEpisodeByMediaId',
]

foreach t : show_tests
  test('Show.' + t, show_test_exe, args: t, suite: ['unittest', 'Show'])
endforeach

subtitle_track_test_exe = executable('subtitle_track_tests', 'SubtitleTrackTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

subtitle_track_tests = [
  'AddTrack',
  'FetchTracks',
  'RemoveTrack',
  'CheckDbModel',
  'UnlinkExternalTrack',
]

foreach t : subtitle_track_tests
  test('SubtitleTrack.' + t, subtitle_track_test_exe, args: t, suite: ['unittest', 'SubtitleTrack'])
endforeach

thumbnail_test_exe = executable('thumbnail_tests', 'ThumbnailTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

thumbnail_tests = [
  'Create',
  'MediaSetThumbnail',
  'Update',
  'MarkFailure',
  'UnshareMedia',
  'UnshareArtist',
  'UpdateIsOwned',
  'CheckMultipleSizes',
  'AutoDelete',
  'AutoDeleteAfterEntityRemoved',
  'ShareThumbnail',
  'AutoDeleteAfterUnlink',
  'CheckDbModel',
  'NbAttempts',
  'OverridePersistentFailure',
  'UpdateAfterSuccessAndFailure',
  'ReplaceFailedThumbnail',
  'GenerateThumbnailTwice',
  'Cleanup',
  'RemoveCleanupRequests',
]

foreach t : thumbnail_tests
  test('Thumbnail.' + t, thumbnail_test_exe, args: t, suite: ['unittest', 'Thumbnail'])
endforeach

title_analyzer_test_exe = executable('title_analyzer_tests', 'TitleAnalyzerTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

title_analyzer_tests = [
  'SimpleTests',
  'RemovePatterns',
  'EpisodeNumber',
  'FullExtraction',
]

foreach t : title_analyzer_tests
  test('TitleAnalyzer.' + t, title_analyzer_test_exe, args: t, suite: ['unittest', 'TitleAnalyzer'])
endforeach

url_test_exe = executable('url_tests', 'UrlTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

url_tests = [
  'encode',
  'stripScheme',
  'scheme',
  'schemeIs',
  'Split',
  'toLocalPath',
  'Path',
  'Decode',
]

foreach t : url_tests
  test('Url.' + t, url_test_exe, args: t, suite: ['unittest', 'Url'])
endforeach

video_track_test_exe = executable('video_track_tests', 'VideoTrackTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

video_track_tests = [
  'AddTrack',
  'FetchTracks',
  'RemoveTrack',
  'CheckDbModel',
]

foreach t : video_track_tests
  test('VideoTrack.' + t, video_track_test_exe, args: t, suite: ['unittest', 'VideoTrack'])
endforeach

subscription_test_exe = executable('subscription_tests', 'SubscriptionTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

subscription_tests = [
  'Create',
  'ListFromService',
  'ListFromMedia',
  'ChildSubscriptions',
  'ListMedia',
  'SearchMedia',
  'CheckDbModel',
  'CachedSize',
  'FetchUncached',
  'MaxCachedMedia',
  'MaxCachedSize',
  'NewMediaNotify',
  'NbMedia',
  'SearchAllMedia',
]

foreach t : subscription_tests
  test('Subscription.' + t, subscription_test_exe, args: t, suite: ['unittest', 'Subscription'])
endforeach

service_test_exe = executable('service_tests', 'ServiceTests.cpp',
  objects: common_test_objs,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep],
  build_by_default: false,
)

service_tests = [
  'FetchCreate',
  'CheckDbModel',
  'AutoDownload',
  'NewMediaNotification',
  'MaxCachedSize',
  'NbSubscriptions',
  'NbUnplayedMedia',
  'ListMedia',
  'Search',
  'SearchMedia',
]

foreach t : service_tests
  test('Service.' + t, service_test_exe, args: t, suite: ['unittest', 'Service'])
endforeach
