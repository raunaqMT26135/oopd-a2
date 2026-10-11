// Q4: Magazine implementation
#include "bookmgmt/Magazine.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

Magazine::Magazine(std::string id, std::string title, std::string issn,
                   std::string publisher, int year, Money unitPrice,
                   int issuesPerYear, Money postagePerIssue,
                   int subscriptionYears)
    : Journal(std::move(id), std::move(title), std::move(issn),
              std::move(publisher), year, unitPrice, issuesPerYear, subscriptionYears),
      postagePerIssue_(postagePerIssue) {
    if (postagePerIssue_.isNegative())
        throw std::invalid_argument("postagePerIssue must not be negative");
}

Money Magazine::costFor(int copies) const {
    // Base cost (unitPrice × copies × subscriptionYears) from Journal,
    // plus postage for every issue delivered to each copy.
    Money baseCost = Journal::costFor(copies);
    Money totalPostage = postagePerIssue_ * issuesPerYear() * subscriptionYears() * copies;
    return baseCost + totalPostage;
}

void Magazine::printDetails(std::ostream& os) const {
    // Journal fields first (ISSN, issues/year, subscription years).
    Journal::printDetails(os);
    os << "  postage per issue: " << postagePerIssue_ << "\n";
}

}  // namespace bookmgmt
