#ifndef CASEINSENSITIVECOMPARER
#define CASEINSENSITIVECOMPARER

#include "./global.h"

#include <string>

#include <iostream>

namespace Media {

/*!
 * \brief The CaseInsensitiveCharComparer struct defines a method for case-insensivive character comparsion (less).
 */
struct TAG_PARSER_EXPORT CaseInsensitiveCharComparer
{
    static constexpr unsigned char toLower(const unsigned char c)
    {
        return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
    }

    bool operator()(const unsigned char lhs, const unsigned char rhs) const
    {
        return toLower(lhs) < toLower(rhs);
    }
};

/*!
 * \brief The CaseInsensitiveStringComparer struct defines a method for case-insensivive string comparsion (less).
 */
struct TAG_PARSER_EXPORT CaseInsensitiveStringComparer
{
    bool operator()(const std::string &lhs, const std::string &rhs) const
    {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), CaseInsensitiveCharComparer());
    }
};

}

#endif // CASEINSENSITIVECOMPARER

