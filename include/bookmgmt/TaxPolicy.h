#pragma once
// Q7: Tax policy — print and electronic items taxed at different rates.
// Rates are stored as integer basis points (1/100 of a percent).
// e.g. 18% GST = 1800 basis points.
//
// TaxPolicy is optional: if not attached to AcquisitionManager the behaviour
// is unchanged (zero tax).

#include <map>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct TaxRate {
    int basisPoints = 0;  // e.g. 1800 = 18 %
};

class TaxPolicy {
public:
    // Set the tax rate for a category. Default rate for any unset category = 0.
    void setRate(ResourceCategory c, TaxRate rate);
    TaxRate rateFor(ResourceCategory c) const;

    // Compute tax amount for a given pre-tax cost.
    Money taxOn(ResourceCategory c, Money preTax) const;
    // Compute post-tax total.
    Money totalWithTax(ResourceCategory c, Money preTax) const;

private:
    std::map<ResourceCategory, TaxRate> rates_;
};

}  // namespace bookmgmt
