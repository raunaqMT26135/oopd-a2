// MT26135 - Raunaq Soneja
#pragma once
// Q4: Magazine — derived from Journal.
// Cost also includes postage for each issue over the subscription period.
//
// costFor(copies) = Journal::costFor(copies)
//                 + postagePerIssue × issuesPerYear × subscriptionYears × copies
//
// i.e. each copy incurs postage for every issue it receives.

#include "bookmgmt/Journal.h"

namespace bookmgmt {

class Magazine : public Journal {
public:
    Magazine(std::string id, std::string title, std::string issn,
             std::string publisher, int year, Money unitPrice,
             int issuesPerYear, Money postagePerIssue,
             int subscriptionYears = 1);

    Money postagePerIssue() const { return postagePerIssue_; }

    ResourceCategory category() const override { return ResourceCategory::Magazine; }

    // Adds postage to the base Journal cost.
    Money costFor(int copies) const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    Money postagePerIssue_;
};

}  // namespace bookmgmt
