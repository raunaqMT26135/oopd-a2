#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category. Categories with no quota are limited only by the total budget.
//
// Q8:  Quota optionally includes maxTitles (distinct title limit per category).
// Q11: yearEndRollover() carries a configurable % of unspent budget forward.

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits;              // maximum copies/seats that may be bought
    Money maxSpend;            // maximum money that may be spent
    std::optional<int> maxTitles;  // Q8: max distinct titles (nullopt = unlimited)
};

struct Usage {
    int units = 0;
    Money spent;
    std::set<std::string> titlesOrdered;  // Q8: IDs of titles bought
};

class Budget {
public:
    explicit Budget(Money total);

    Money total()     const { return total_; }
    Money spent()     const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int>   unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;
    std::optional<int>   titlesRemaining(ResourceCategory c) const;  // Q8

    // Returns an empty string if the purchase fits, otherwise the reason it
    // does not. Does not change state.
    // Q8: titleId is the resource id; used to check the title-count quota.
    std::string check(ResourceCategory c, int units, Money cost,
                      const std::string& titleId = "") const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError
    // (and changes nothing) if it would not fit.
    // Q8: titleId tracks distinct titles per category.
    void commit(ResourceCategory c, int units, Money cost,
                const std::string& titleId = "");

    // Q9: Refund a purchase (reverse a commit). units/cost must match what was committed.
    void refund(ResourceCategory c, int units, Money cost,
                const std::string& titleId = "");

    // Q11: Year-end rollover — carry rolloverPercent % of unspent budget forward.
    // Resets spending and usage to zero, adjusts total.
    // rolloverPercent must be in [0, 100].
    void yearEndRollover(int rolloverPercent);

    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost,
                     const std::string& titleId, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota>  quotas_;
    std::map<ResourceCategory, Usage>  usage_;
};

}  // namespace bookmgmt
