#include "./avcinfo.h"

#include "../exceptions.h"

#include <c++utilities/io/binaryreader.h>
#include <c++utilities/io/bitreader.h>
#include <c++utilities/io/catchiofailure.h>

#include <unordered_map>
#include <memory>

using namespace std;
using namespace IoUtilities;

namespace Media {

/*!
 * \struct Media::SpsInfo
 * \brief The SpsInfo struct holds the sequence parameter set.
 */

/*!
 * \brief Parses the SPS info.
 */
void SpsInfo::parse(BinaryReader &reader, uint32 maxSize)
{
    // read (and check) size
    if(maxSize < 2) {
        throw TruncatedDataException();
    }
    maxSize -= 2;
    if((size = reader.readUInt16BE()) > maxSize) {
        throw TruncatedDataException();
    }

    // buffer data for reading with BitReader
    auto buffer = make_unique<char[]>(size);
    reader.read(buffer.get(), size);
    BitReader bitReader(buffer.get(), size);

    try {
        // read general values
        bitReader.skipBits(3);
        if(bitReader.readBits<byte>(5) != 7) {
            throw InvalidDataException();
        }
        profileIndication = bitReader.readBits<byte>(8);
        profileConstraints = bitReader.readBits<byte>(8);
        levelIndication = bitReader.readBits<byte>(8);
        id = bitReader.readUnsignedExpGolombCodedBits<ugolomb>();

        // read chroma profile specific values
        switch(profileIndication) {
        case 44: case 83: case 86: case 100: case 110:
        case 118: case 122: case 128: case 244:
            // high-level profile
            if((chromaFormatIndication = bitReader.readUnsignedExpGolombCodedBits<ugolomb>()) == 3) {
                bitReader.skipBits(1); // separate color plane flag
            }
            bitReader.readUnsignedExpGolombCodedBits<byte>(); // bit depth luma minus8
            bitReader.readUnsignedExpGolombCodedBits<byte>(); // bit depth chroma minus8
            bitReader.skipBits(1); // qpprime y zero transform bypass flag
            if(bitReader.readBit()) { // sequence scaling matrix present flag
                for(byte i = 0; i < 8; ++i) {
                    // TODO: store values
                    if(bitReader.readBit()) { // sequence scaling list present
                        if(i < 6) {
                            bitReader.skipBits(16); // scalingList4x4[i]
                        } else {
                            bitReader.skipBits(64); // scalingList8x8[i - 6]
                        }
                    }
                }
            }
            break;
        default:
            chromaFormatIndication = 1; // assume YUV 4:2:0
        }

        // read misc values
        log2MaxFrameNum = bitReader.readUnsignedExpGolombCodedBits<ugolomb>() + 4;
        switch(pictureOrderCountType = bitReader.readUnsignedExpGolombCodedBits<ugolomb>()) {
        case 0:
            log2MaxPictureOrderCountLsb = bitReader.readUnsignedExpGolombCodedBits<ugolomb>() + 4;
            break;
        case 1:
            deltaPicOrderAlwaysZeroFlag = bitReader.readBit();
            offsetForNonRefPic = bitReader.readSignedExpGolombCodedBits<sgolomb>();
            offsetForTopToBottomField = bitReader.readSignedExpGolombCodedBits<sgolomb>();
            numRefFramesInPicOrderCntCycle = bitReader.readUnsignedExpGolombCodedBits<ugolomb>();
            for(byte i = 0; i < numRefFramesInPicOrderCntCycle; ++i) {
                bitReader.readUnsignedExpGolombCodedBits<ugolomb>(); // offset for ref frames
            }
            break;
        case 2:
            break;
        default:
            throw InvalidDataException();
        }
        bitReader.readUnsignedExpGolombCodedBits<ugolomb>(); // ref frames num
        bitReader.skipBits(1); // gaps in frame num value allowed flag

        // read picture size related values
        Size mbSize;
        mbSize.setWidth(bitReader.readUnsignedExpGolombCodedBits<uint32>() + 1);
        mbSize.setHeight(bitReader.readUnsignedExpGolombCodedBits<uint32>() + 1);
        if(!(frameMbsOnly = bitReader.readBit())) { // frame mbs only flag
            bitReader.readBit(); // mb adaptive frame field flag
        }
        bitReader.skipBits(1); // distinct 8x8 inference flag

        // read cropping values
        if(bitReader.readBit()) { // frame cropping flag
            cropping.setLeft(bitReader.readUnsignedExpGolombCodedBits<uint32>());
            cropping.setRight(bitReader.readUnsignedExpGolombCodedBits<uint32>());
            cropping.setTop(bitReader.readUnsignedExpGolombCodedBits<uint32>());
            cropping.setBottom(bitReader.readUnsignedExpGolombCodedBits<uint32>());
        }

        // calculate actual picture size
        if(!cropping.isNull()) {
            // determine cropping scale
            ugolomb croppingScaleX, croppingScaleY;
            switch(chromaFormatIndication) {
            case 1: // 4:2:0
                croppingScaleX = 2;
                croppingScaleY = frameMbsOnly ? 2 : 4;
                break;
            case 2: // 4:2:2
                croppingScaleX = 2;
                croppingScaleY = 2 - frameMbsOnly;
                break;
            default: // case 0: monochrome, case 3: 4:4:4
                croppingScaleX = 1;
                croppingScaleY = 2 - frameMbsOnly;
                break;
            }
            pictureSize.setWidth(mbSize.width() * 16 - croppingScaleX * (cropping.left() + cropping.right()));
            pictureSize.setHeight((2 - frameMbsOnly) * mbSize.height() * 16 - croppingScaleY * (cropping.top() + cropping.bottom()));
        } else {
            pictureSize.setWidth(mbSize.width() * 16);
            pictureSize.setHeight((2 - frameMbsOnly) * mbSize.height() * 16);
        }

        // read VUI (video usability information)
        if((vuiPresent = bitReader.readBit())) {
            if((bitReader.readBit())) { // PAR present flag
                pixelAspectRatio = AspectRatio(bitReader.readBits<byte>(8));
                if(pixelAspectRatio.isExtended()) {
                    // read extended SAR
                    pixelAspectRatio.numerator = bitReader.readBits<uint16>(16);
                    pixelAspectRatio.denominator = bitReader.readBits<uint16>(16);
                }
            }

            // read/skip misc values
            if(bitReader.readBit()) { // overscan info present
                bitReader.skipBits(1); // overscan appropriate
            }
            if(bitReader.readBit()) { // video signal type present
                bitReader.skipBits(4); // video format and video full range
                if(bitReader.readBit()) { // color description present
                    bitReader.skipBits(24); // color primaries, transfer characteristics, matrix coefficients
                }
            }
            if(bitReader.readBit()) { // chroma loc info present
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // chroma sample loc type top field
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // chroma sample loc type bottom field
            }

            // read timing info
            if((timingInfo.isPresent = bitReader.readBit())) {
                timingInfo.unitsInTick = bitReader.readBits<uint32>(32);
                timingInfo.timeScale = bitReader.readBits<uint32>(32);
                timingInfo.fixedFrameRate = bitReader.readBit();
            }

            // skip hrd parameters
            hrdParametersPresent = 0;
            if(bitReader.readBit()) { // nal hrd parameters present
                nalHrdParameters.parse(bitReader);
                hrdParametersPresent = 1;
            }
            if(bitReader.readBit()) { // vcl hrd parameters present
                vclHrdParameters.parse(bitReader);
                hrdParametersPresent = 1;
            }
            if(hrdParametersPresent) {
                bitReader.skipBits(1); // low delay hrd flag
            }

            pictureStructPresent = bitReader.readBit();

            // TODO: investigate error (truncated data) when parsing mtx-test-data/mkv/attachment-without-fileuid.mkv
            if(bitReader.readBit()) { // bitstream restriction flag
                bitReader.skipBits(1); // motion vectors over pic boundries flag
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // max bytes per pic denom
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // max bytes per mb denom
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // log2 max mv length horizontal
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // log2 max mv length vertical
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // reorder frames num
                bitReader.readUnsignedExpGolombCodedBits<byte>(); // max decoder frame buffering
            }
        }

    } catch(...) {
        catchIoFailure();
        throw TruncatedDataException();
    }
}

/*!
 * \struct Media::PpsInfo
 * \brief The PpsInfo struct holds the picture parameter set.
 */

/*!
 * \brief Parses the PPS info.
 */
void PpsInfo::parse(BinaryReader &reader, uint32 maxSize)
{
    // read (and check) size
    if(maxSize < 2) {
        throw TruncatedDataException();
    }
    maxSize -= 2;
    if((size = reader.readUInt16BE()) > maxSize) {
        throw TruncatedDataException();
    }

    // buffer data for reading with BitReader
    auto buffer = make_unique<char[]>(size);
    reader.read(buffer.get(), size);
    BitReader bitReader(buffer.get(), size);

    try {
        // read general values
        bitReader.skipBits(1); // zero bit
        if(bitReader.readBits<byte>(5) != 8) { // nal unit type
            throw InvalidDataException();
        }
        id = bitReader.readUnsignedExpGolombCodedBits<ugolomb>();
        spsId = bitReader.readUnsignedExpGolombCodedBits<ugolomb>();
        bitReader.skipBits(1); // entropy coding mode flag
        picOrderPresent = bitReader.readBit();
    } catch(...) {
        catchIoFailure();
        throw TruncatedDataException();
    }
}

/*!
 * \struct Media::HrdParameters
 * \brief The HrdParameters struct holds "Hypothetical Reference Decoder" parameters.
 * \remarks This is "a model for thinking about the decoding process".
 */

/*!
 * \brief Parses HRD parameters.
 */
void HrdParameters::parse(IoUtilities::BitReader &reader)
{
    cpbCount = reader.readUnsignedExpGolombCodedBits<ugolomb>() + 1;
    bitRateScale = reader.readBits<byte>(4);
    cpbSizeScale = reader.readBits<byte>(4);
    for(ugolomb i = 0; i < cpbCount; ++i) {
        // just skip those values
        reader.readUnsignedExpGolombCodedBits<byte>(); // bit rate value minus 1
        reader.readUnsignedExpGolombCodedBits<byte>(); // cpb size value minus 1
        reader.skipBits(1); // cbr flag
    }
    initialCpbRemovalDelayLength = reader.readBits<byte>(5) + 1;
    cpbRemovalDelayLength = reader.readBits<byte>(5) + 1;
    cpbOutputDelayLength = reader.readBits<byte>(5) + 1;
    timeOffsetLength = reader.readBits<byte>(5);
}

/*!
 * \struct Media::TimingInfo
 * \brief The TimingInfo struct holds timing information (part of SPS info).
 */

/*!
 * \struct Media::SliceInfo
 * \brief The SliceInfo struct holds the slice information of an AVC frame.
 * \remarks currently not useful, might be removed
 */

/*!
 * \struct Media::AvcFrame
 * \brief The AvcFrame struct holds an AVC frame.
 * \remarks currently not useful, might be removed
 */

}
