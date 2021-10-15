#!/bin/sh

NB_ARTISTS=100
NB_ALBUMS_PER_ARTIST=10
NB_TRACKS_PER_ALBUM=10

SCRIPT_DIRECTORY=$(dirname "$0")
CORPUS_DIRECTORY=

if [ $# -gt 2 ]; then
    echo "usage: $0 [dest folder]"
    exit 1
fi

if [ $# -eq 2 ]; then
    CORPUS_DIRECTORY=$1
else
    CORPUS_DIRECTORY=/tmp/medialib_samples/
fi

echo "Generating samples to $CORPUS_DIRECTORY"

for i_art in `seq 1 $NB_ARTISTS`; do
    ARTIST_NAME="artist_$i_art"
    ARTIST_FOLDER=$CORPUS_DIRECTORY/$ARTIST_NAME
    for i_alb in `seq 1 $NB_ALBUMS_PER_ARTIST`; do
        ALBUM_NAME="album_$i_alb"
        ALBUM_FOLDER=$ARTIST_FOLDER/$ALBUM_NAME
        mkdir -p $ALBUM_FOLDER
        for i_track in `seq 1 $NB_TRACKS_PER_ALBUM`; do
            TRACK_NAME="track_$i_track"
            TRACK_PATH=$ALBUM_FOLDER/$TRACK_NAME.mp3
            cp $SCRIPT_DIRECTORY/dummy.mp3 $TRACK_PATH
            id3tag --artist=$ARTIST_NAME --album=$ALBUM_NAME \
                --song=$TRACK_NAME \
                --track=$i_track \
                --total=$NB_TRACKS_PER_ALBUM \
                --year=$((2000 + $i_alb)) \
                --genre=$(shuf -i 1-80 -n 1) \
                $TRACK_PATH
        done
    done
done
