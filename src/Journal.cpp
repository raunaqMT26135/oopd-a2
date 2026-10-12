// MT26135 - Raunaq Soneja
// Q1: Journal implementation
#include "bookmgmt/Journal.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string issn,
                 std::string publisher, int year, Money unitPrice,
                 int issuesPerYear, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (issuesPerYear_ < 1)
        throw std::invalid_argument("issuesPerYear must be >= 1");
    if (subscriptionYears_ < 1)
        throw std::invalid_argument("subscriptionYears must be >= 1");
}

Money Journal::costFor(int copies) const {
    requirePositive(copies);
    // Annual subscription price per copy × number of copies × subscription length in years.
    Money base = unitPrice() * copies * subscriptionYears_;
    // Q6: 10% bulk discount for print items buying >= 10 copies.
    return applyPrintBulkDiscount(base, copies);
}

void Journal::printDetails(std::ostream& os) const {
    os << "  issn: "              << issn_             << "\n"
       << "  issues per year: "  << issuesPerYear_    << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt
