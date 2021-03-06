#ifndef MEDIA_VORBISCOMMENT_H
#define MEDIA_VORBISCOMMENT_H

#include "./vorbiscommentfield.h"

#include "../caseinsensitivecomparer.h"
#include "../fieldbasedtag.h"
#include "../mediaformat.h"

namespace Media {

class OggIterator;
class VorbisComment;

class TAG_PARSER_EXPORT VorbisComment : public FieldMapBasedTag<VorbisCommentField, CaseInsensitiveStringComparer>
{
public:
    VorbisComment();

    static constexpr TagType tagType = TagType::VorbisComment;
    TagType type() const;
    const char *typeName() const;
    TagTextEncoding proposedTextEncoding() const;
    bool canEncodingBeUsed(TagTextEncoding encoding) const;

    const TagValue &value(KnownField field) const;
    bool setValue(KnownField field, const TagValue &value);
    std::string fieldId(KnownField field) const;
    KnownField knownField(const std::string &id) const;

    void parse(OggIterator &iterator, VorbisCommentFlags flags = VorbisCommentFlags::None);
    void parse(std::istream &stream, uint64 maxSize, VorbisCommentFlags flags = VorbisCommentFlags::None);
    void make(std::ostream &stream, VorbisCommentFlags flags = VorbisCommentFlags::None);

    const TagValue &vendor() const;
    void setVendor(const TagValue &vendor);

private:
    template<class StreamType>
    void internalParse(StreamType &stream, uint64 maxSize, VorbisCommentFlags flags);

private:
    TagValue m_vendor;
};

/*!
 * \brief Constructs a new Vorbis comment.
 */
inline VorbisComment::VorbisComment()
{}

inline TagType VorbisComment::type() const
{
    return TagType::VorbisComment;
}

inline const char *VorbisComment::typeName() const
{
    return "Vorbis comment";
}

inline TagTextEncoding VorbisComment::proposedTextEncoding() const
{
    return TagTextEncoding::Utf8;
}

inline bool VorbisComment::canEncodingBeUsed(TagTextEncoding encoding) const
{
    return encoding == TagTextEncoding::Utf8;
}

/*!
 * \brief Returns the vendor.
 * \remarks Also accessable via value(KnownField::Vendor).
 */
inline const TagValue &VorbisComment::vendor() const
{
    return m_vendor;
}

/*!
 * \brief Sets the vendor.
 * \remarks Also accessable via setValue(KnownField::Vendor, vendor).
 */
inline void VorbisComment::setVendor(const TagValue &vendor)
{
    m_vendor = vendor;
}

}

#endif // MEDIA_VORBISCOMMENT_H
