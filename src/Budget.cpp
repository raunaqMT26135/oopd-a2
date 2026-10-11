// Budget implementation — Q8, Q9, Q11 extensions included.
#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
// Q1–Q4: new categories added.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::ElectronicResource,
    ResourceCategory::Journal,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis,
    ResourceCategory::Magazine,
};
}  // namespace

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");
    if (q.maxTitles && *q.maxTitles < 0)  // Q8
        throw std::invalid_argument("maxTitles must not be negative");
    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

// Q8: titles remaining in category quota.
std::optional<int> Budget::titlesRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q || !q->maxTitles) return std::nullopt;
    int used = static_cast<int>(usageFor(c).titlesOrdered.size());
    return *q->maxTitles - used;
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 const std::string& titleId, std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
              std::to_string(units) + ", " + std::to_string(*left) + " remaining";
        return Failure::Quota;
    }
    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
              cost.toString() + ", " + left->toString() + " remaining";
        return Failure::Quota;
    }
    // Q8: check title count quota only if the title is new for this category.
    if (!titleId.empty()) {
        const Usage& u = usageFor(c);
        bool isNewTitle = (u.titlesOrdered.count(titleId) == 0);
        if (isNewTitle) {
            if (auto left = titlesRemaining(c); left && *left <= 0) {
                why = std::string(categoryName(c)) + " title quota exceeded";
                return Failure::Quota;
            }
        }
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c, int units, Money cost,
                           const std::string& titleId) const {
    std::string why;
    evaluate(c, units, cost, titleId, why);
    return why;
}

void Budget::commit(ResourceCategory c, int units, Money cost,
                    const std::string& titleId) {
    std::string why;
    switch (evaluate(c, units, cost, titleId, why)) {
        case Failure::None:     break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota:    throw QuotaExceededError(why);
        case Failure::Overall:  throw BudgetExceededError(why);
    }
    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    if (!titleId.empty()) u.titlesOrdered.insert(titleId);  // Q8
    spent_ += cost;
}

// Q9: Refund a committed purchase.
void Budget::refund(ResourceCategory c, int units, Money cost,
                    const std::string& titleId) {
    if (units <= 0) throw std::invalid_argument("refund units must be positive");
    Usage& u = usage_[c];
    u.units -= units;
    u.spent -= cost;
    // Note: we do NOT remove the title from titlesOrdered because the library
    // still has a record that the title was ordered at some point.
    (void)titleId;
    spent_ -= cost;
}

// Q11: Year-end rollover.
void Budget::yearEndRollover(int rolloverPercent) {
    if (rolloverPercent < 0 || rolloverPercent > 100)
        throw std::invalid_argument("rolloverPercent must be in [0, 100]");
    Money unspent = remaining();
    Money carryover = Money::fromMinor(unspent.minorUnits() * rolloverPercent / 100);
    // Reset: new total = carryover only (next year's base budget not added here).
    // Caller should add next year's fresh budget on top.
    total_  = carryover;
    spent_  = Money{};
    usage_.clear();
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << std::setw(18) << "Spend used/max" << "Titles used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto  q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        // Q8: titles column
        const std::string titles = [&]() -> std::string {
            int used = static_cast<int>(u.titlesOrdered.size());
            if (!q || !q->maxTitles) return std::to_string(used) + "/-";
            return std::to_string(used) + "/" + std::to_string(*q->maxTitles);
        }();
        os << "  " << std::setw(20) << categoryName(c)
           << std::setw(18) << units
           << std::setw(18) << spend
           << titles << "\n";
    }
}

}  // namespace bookmgmt
