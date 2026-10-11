// Q2: EBook implementation
#include "bookmgmt/EBook.h"

#include <ostream>

#include "bookmgmt/Book.h"  // for joinAuthors

namespace bookmgmt {

const char* ebookFormatName(EBookFormat f) {
    switch (f) {
        case EBookFormat::PDF:  return "PDF";
        case EBookFormat::EPUB: return "EPUB";
        case EBookFormat::HTML: return "HTML";
    }
    return "Unknown";
}

EBook::EBook(std::string id, std::string title,
             std::vector<std::string> authors, std::string isbn,
             std::string publisher, int year,
             Money pricePerSeat, std::string accessUrl,
             EBookFormat format, bool drmProtected,
             LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher),
                         year, pricePerSeat, std::move(accessUrl), license, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

void EBook::printDetails(std::ostream& os) const {
    // First print the ElectronicResource fields (url, license, platform fee).
    ElectronicResource::printDetails(os);
    // Then add EBook-specific fields.
    os << "  authors: "       << joinAuthors(authors_) << "\n"
       << "  isbn: "          << isbn_                 << "\n"
       << "  format: "        << ebookFormatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt
