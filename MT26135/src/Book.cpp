// MT26135 - Raunaq Soneja
#include "bookmgmt/Book.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

std::string joinAuthors(const std::vector<std::string>& authors) {
    std::string out;
    for (std::size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) out += (i + 1 == authors.size()) ? " and " : ", ";
        out += authors[i];
    }
    return out;
}

Book::Book(std::string id, std::string title, std::vector<std::string> authors,
           std::string isbn, std::string publisher, int year, Money unitPrice,
           int edition, Binding binding)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      edition_(edition),
      binding_(binding) {
    if (edition_ < 1) throw std::invalid_argument("edition must be >= 1");
}

// Q5: Hardcover books cost 20% more. Q6: 10% off buying >= 10 copies.
// The hardcover surcharge is applied first, then the bulk discount.
Money Book::costFor(int quantity) const {
    requirePositive(quantity);
    Money base = unitPrice() * quantity;
    if (binding_ == Binding::Hardcover) {
        // Q5: +20% surcharge for hardcover.
        base = Money::fromMinor(base.minorUnits() * 120 / 100);
    }
    // Q6: 10% bulk discount for >= 10 copies.
    return applyPrintBulkDiscount(base, quantity);
}

void Book::printDetails(std::ostream& os) const {
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  edition: " << edition_ << "\n"
       << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
       << "\n";
}

}  // namespace bookmgmt
