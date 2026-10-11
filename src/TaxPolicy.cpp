// Q7: TaxPolicy implementation
#include "bookmgmt/TaxPolicy.h"

namespace bookmgmt {

void TaxPolicy::setRate(ResourceCategory c, TaxRate rate) {
    rates_[c] = rate;
}

TaxRate TaxPolicy::rateFor(ResourceCategory c) const {
    auto it = rates_.find(c);
    return it == rates_.end() ? TaxRate{0} : it->second;
}

Money TaxPolicy::taxOn(ResourceCategory c, Money preTax) const {
    TaxRate r = rateFor(c);
    // tax = preTax * basisPoints / 10000
    return Money::fromMinor(preTax.minorUnits() * r.basisPoints / 10000);
}

Money TaxPolicy::totalWithTax(ResourceCategory c, Money preTax) const {
    return preTax + taxOn(c, preTax);
}

}  // namespace bookmgmt
