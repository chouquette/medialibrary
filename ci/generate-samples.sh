#!/bin/sh

NB_ARTISTS=100
NB_ALBUMS_PER_ARTIST=10
NB_TRACKS_PER_ALBUM=10

SCRIPT_DIRECTORY=$(dirname "$0")
CORPUS_DIRECTORY=/tmp/medialib_samples/

usage()
{
    echo "usage: $0 [-o dest_folder] [-n nb_artists]"
    exit 1
}

while getopts ":o:n:" ARG; do
    case "$ARG" in
        o)
            CORPUS_DIRECTORY=$OPTARG
            ;;
        n)
            NB_ARTISTS=$OPTARG
            ;;
        *)
            usage
            ;;
    esac
done

echo "Generating samples to $CORPUS_DIRECTORY"
echo "Generating $NB_ARTISTS artists with 10 albums of 10 tracks each"

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
