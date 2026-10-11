#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents).
// Q19: Add a currency code. Adding amounts in different currencies throws exception.

#include <cstdint>
#include <iosfwd>
#include <string>
#include <stdexcept>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    // Construct from minor units: Money::fromMinor(12550) == 125.50
    static Money fromMinor(std::int64_t minor, const std::string& currency = "USD") {
        return Money(minor, currency);
    }
    // Construct from major + minor parts: Money::of(125, 50) == 125.50
    static Money of(std::int64_t major, int minor = 0, const std::string& currency = "USD");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currency() const { return currency_; }
    
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;  // "1234.50 USD"

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(const Money& o);
    Money& operator-=(const Money& o);
    Money& operator*=(std::int64_t k) { minor_ *= k; return *this; }

    friend Money operator+(Money a, const Money& b) { return a += b; }
    friend Money operator-(Money a, const Money& b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend bool operator==(const Money& a, const Money& b);
    friend bool operator!=(const Money& a, const Money& b) { return !(a == b); }
    friend bool operator<(const Money& a, const Money& b);
    friend bool operator<=(const Money& a, const Money& b) { return !(b < a); }
    friend bool operator>(const Money& a, const Money& b) { return b < a; }
    friend bool operator>=(const Money& a, const Money& b) { return !(a < b); }

private:
    explicit Money(std::int64_t minor, const std::string& currency = "USD")
        : minor_(minor), currency_(currency) {}
    std::int64_t minor_ = 0;
    std::string currency_ = "USD";

    static void checkCurrency(const Money& a, const Money& b);
};

std::ostream& operator<<(std::ostream& os, const Money& m);

}  // namespace bookmgmt
