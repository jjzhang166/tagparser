#!/bin/sh

testfilespath="$1"

# determine sequences for formatted output
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
blue=$(tput setaf 4)
bold=$(tput bold)
normal=$(tput sgr0)

inform() {
    echo "${bold}==> ${blue}INFO:${normal} ${bold}${1}${normal}"
}

success() {
    echo "${bold}==> ${green}SUCCESS:${normal} ${bold}${1}${normal}"
}

fail() {
    echo "${bold}==> ${red}FAILURE:${normal} ${bold}${1}${normal}"
}

skipping() {
    echo 'archive/files already exist -> skipping download'
}

download() {
    title="$1" url="$2" filename="$3" destdir="$4"

    inform "Downloading \"$title\" ..."
    if [[ ! -d $destdir ]]; then
        # actual download
        pushd '/tmp'
        if [[ ! -f $filename ]]; then
            wget --output-document="$filename" "$url"
            if [[ $? != 0 ]]; then
                fail "unable to download: \"$url\""
                return
            fi
        else
            skipping
        fi
        popd

        # extraction
        mkdir "$destdir"
        pushd "$destdir"
        case "$filename" in
            *.zip)    unzip "/tmp/$filename";;
            *)      fail "unable to extract archive: format \"${filename#*.}\" not supported"
                    return;;
        esac
        if [[ $? != 0 ]]; then
            fail "unable to extract \"/tmp/$filename\""
            return
        fi
        popd
    else
        skipping
    fi
}

download_custom() {
    title="$1" cmd="$2"

    inform "Downloading \"$title\" ..."
    if [[ ! -d $destdir ]]; then
        # actual download
        $cmd
        if [[ $? != 0 ]]; then
            fail "unable to download \"$title\" with rsync"
            return
        fi
    else
        skipping
    fi
}

# enter testfiles directory
if [[ -d $testfilespath ]]; then
    cd "$testfilespath"
else
    fail "specified testfiles directory does not exist"
    exit -1
fi

download \
    'Matroska Test Suite - Wave 1' \
    'http://downloads.sourceforge.net/project/matroska/test_files/matroska_test_w1_1.zip?r=&ts=1454982254&use_mirror=netix' \
    'matroska_test_w1_1.zip' \
    'matroska_wave1'

destdir='mtx-test-data'
download_custom \
    'MTX Test Data' \
    'rsync -rv --links --delete belgarath.bunkus.org::mtx-test-data mtx-test-data'

destdir='samples.mplayerhq.hu'
download_custom \
    'MPlayer samples' \
    'wget -r -np -R index.html* http://samples.mplayerhq.hu/A-codecs/lossless'

# convert FLAC files for FLAC tests with ffmpeg
mkdir -p flac
[[ ! -f flac/test.flac ]] && ffmpeg -i mtx-test-data/alac/othertest-itunes.m4a -c:a flac flac/test.flac # raw FLAC stream
[[ ! -f flac/test.ogg ]] && ffmpeg -i flac/test.flac -c:a copy flac/test.ogg # FLAC in Ogg
