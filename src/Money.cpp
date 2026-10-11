#include "bookmgmt/Money.h"

#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

Money Money::of(std::int64_t major, int minor, const std::string& currency) {
    if (minor < 0 || minor > 99) throw std::invalid_argument("minor units must be 0-99");
    if (major < 0) return Money(major * 100 - minor, currency);
    return Money(major * 100 + minor, currency);
}

std::string Money::toString() const {
    std::ostringstream os;
    os << (minor_ < 0 ? "-" : "") << std::abs(minor_) / 100 << "."
       << std::setfill('0') << std::setw(2) << std::abs(minor_) % 100;
    if (!currency_.empty()) {
        os << " " << currency_;
    }
    return os.str();
}

void Money::checkCurrency(const Money& a, const Money& b) {
    if (a.minor_ != 0 && b.minor_ != 0 && a.currency_ != b.currency_) {
        throw std::invalid_argument("currency mismatch: " + a.currency_ + " vs " + b.currency_);
    }
}

Money& Money::operator+=(const Money& o) {
    checkCurrency(*this, o);
    if (minor_ == 0 && o.minor_ != 0) currency_ = o.currency_;
    minor_ += o.minor_;
    return *this;
}

Money& Money::operator-=(const Money& o) {
    checkCurrency(*this, o);
    if (minor_ == 0 && o.minor_ != 0) currency_ = o.currency_;
    minor_ -= o.minor_;
    return *this;
}

bool operator==(const Money& a, const Money& b) {
    if (a.minor_ == 0 && b.minor_ == 0) return true;
    return a.minor_ == b.minor_ && a.currency_ == b.currency_;
}

bool operator<(const Money& a, const Money& b) {
    Money::checkCurrency(a, b);
    return a.minor_ < b.minor_;
}

std::ostream& operator<<(std::ostream& os, const Money& m) {
    return os << m.toString();
}

}  // namespace bookmgmt
