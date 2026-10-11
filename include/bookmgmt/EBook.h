#pragma once
// Q2: EBook — derived from ElectronicResource.
//
// Extra fields : authors, ISBN, file format (PDF / EPUB / HTML), DRM flag.
// Pricing      : inherited unchanged from ElectronicResource
//                (platformFee + pricePerSeat × seats).
// Category     : EBook (separate from ElectronicResource so budgets can
//                set an e-book quota independently).
//
// DESIGN NOTE (answer to Q2 comment question):
//   Both Book and EBook store authors_ (vector<string>) and isbn_ (string).
//   The duplication could be avoided by extracting a mixin / base class that
//   carries those two fields, e.g. a non-polymorphic "Authored" struct that
//   both Book and EBook inherit from (or aggregate).  Alternatively, a free
//   helper function joinAuthors() is already shared — only the data members
//   and their accessors would remain duplicated without such a mixin.

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

enum class EBookFormat { PDF, EPUB, HTML };
const char* ebookFormatName(EBookFormat f);

class EBook : public ElectronicResource {
public:
    EBook(std::string id, std::string title,
          std::vector<std::string> authors, std::string isbn,
          std::string publisher, int year,
          Money pricePerSeat, std::string accessUrl,
          EBookFormat format        = EBookFormat::PDF,
          bool        drmProtected  = false,
          LicenseModel license      = LicenseModel::AnnualSubscription,
          Money        platformFee  = Money{});

    const std::vector<std::string>& authors()  const { return authors_; }
    const std::string&              isbn()      const { return isbn_; }
    EBookFormat                     format()    const { return format_; }
    bool                            isDrmProtected() const { return drmProtected_; }

    // EBook has its own category separate from ElectronicResource.
    ResourceCategory category() const override { return ResourceCategory::EBook; }

protected:
    // Calls ElectronicResource::printDetails() first, then adds own fields.
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    EBookFormat format_;
    bool drmProtected_;
};

}  // namespace bookmgmt
