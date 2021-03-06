#ifndef MEDIA_MATROSKAEDITIONENTRY_H
#define MEDIA_MATROSKAEDITIONENTRY_H

#include "./matroskachapter.h"

namespace Media {

class EbmlElement;

class TAG_PARSER_EXPORT MatroskaEditionEntry : public StatusProvider
{
public:
    MatroskaEditionEntry(EbmlElement *editionEntryElement);
    ~MatroskaEditionEntry();

    EbmlElement *editionEntryElement() const;
    uint64 id() const;
    bool isHidden() const;
    bool isDefault() const;
    bool isOrdered() const;
    std::string label() const;
    const std::vector<std::unique_ptr<MatroskaChapter> > &chapters() const;

    void parse();
    void parseNested();
    void clear();

private:
    EbmlElement *m_editionEntryElement;
    uint64 m_id;
    bool m_hidden;
    bool m_default;
    bool m_ordered;
    std::vector<std::unique_ptr<MatroskaChapter> > m_chapters;
};

/*!
 * \brief Returns the "EditionEntry"-element specified when constructing the object.
 */
inline EbmlElement *MatroskaEditionEntry::editionEntryElement() const
{
    return m_editionEntryElement;
}

/*!
 * \brief Returns the edition ID.
 */
inline uint64 MatroskaEditionEntry::id() const
{
    return m_id;
}

/*!
 * \brief Returns whether the edition is hidden.
 */
inline bool MatroskaEditionEntry::isHidden() const
{
    return m_hidden;
}

/*!
 * \brief Returns whether the edition is flagged as default edition.
 */
inline bool MatroskaEditionEntry::isDefault() const
{
    return m_default;
}

/*!
 * \brief Returns whether the edition is ordered.
 */
inline bool MatroskaEditionEntry::isOrdered() const
{
    return m_ordered;
}

/*!
 * \brief Returns the chapters the edition contains.
 */
inline const std::vector<std::unique_ptr<MatroskaChapter> > &MatroskaEditionEntry::chapters() const
{
    return m_chapters;
}

} // namespace Media

#endif // MEDIA_MATROSKAEDITIONENTRY_H
