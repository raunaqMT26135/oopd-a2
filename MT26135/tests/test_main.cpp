// MT26135 - Raunaq Soneja
// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

// =========================================================================
// Original tests (unchanged)
// =========================================================================

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05 USD");
    CHECK(Money::of(-3, 50).toString() == "-3.50 USD");
    CHECK(Money::fromMinor(7).toString() == "0.07 USD");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);

    // Q19 test: different currencies
    Money eur = Money::of(10, 0, "EUR");
    Money usd = Money::of(10, 0, "USD");
    CHECK_THROWS(eur + usd, std::invalid_argument);
    CHECK_THROWS(eur < usd, std::invalid_argument);
    CHECK(eur != usd);
    // 0 adopts the other currency
    CHECK((Money::of(0) + eur).currency() == "EUR");
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // budget exceeded
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}

// =========================================================================
// Q1: Journal
// =========================================================================
static void testJournal() {
    Journal j("J1", "Nature", "0028-0836", "Springer", 2024,
               Money::of(500), 52, 2);
    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(j.issn() == "0028-0836");
    CHECK(j.issuesPerYear() == 52);
    CHECK(j.subscriptionYears() == 2);
    // costFor(3) = 500 * 3 * 2 = 3000
    CHECK(j.costFor(3) == Money::of(3000));
    // Q6 bulk: 10 copies => 10% off: 500*10*2=10000, discounted=9000
    CHECK(j.costFor(10) == Money::of(9000));
    CHECK_THROWS(Journal("J2", "T", "i", "P", 2020, Money::of(1), 12, 0),
                 std::invalid_argument);  // subscriptionYears < 1
    CHECK_THROWS(Journal("J3", "T", "i", "P", 2020, Money::of(1), 0, 1),
                 std::invalid_argument);  // issuesPerYear < 1
    CHECK(std::string(categoryName(ResourceCategory::Journal)) == "Journal");

    // printDetails
    std::ostringstream os;
    j.print(os);
    CHECK(os.str().find("issn: 0028-0836") != std::string::npos);
    CHECK(os.str().find("issues per year: 52") != std::string::npos);
}

// =========================================================================
// Q2: EBook
// =========================================================================
static void testEBook() {
    EBook eb("E1", "CLRS",
             {"Cormen", "Leiserson", "Rivest", "Stein"},
             "978-0262046305", "MIT Press", 2022,
             Money::of(20), "https://mitpress.example",
             EBookFormat::PDF, true);
    CHECK(eb.category() == ResourceCategory::EBook);
    CHECK(eb.category() != ResourceCategory::ElectronicResource);
    CHECK(eb.isDigital());
    CHECK(eb.isDrmProtected());
    CHECK(eb.format() == EBookFormat::PDF);
    CHECK(eb.isbn() == "978-0262046305");
    CHECK(joinAuthors(eb.authors()) == "Cormen, Leiserson, Rivest and Stein");
    // platformFee=0, pricePerSeat=20, 1 seat (<=50) => 20
    CHECK(eb.costFor(1) == Money::of(20));

    std::ostringstream os;
    eb.print(os);
    CHECK(os.str().find("PDF") != std::string::npos);
    CHECK(os.str().find("drm protected: yes") != std::string::npos);
    CHECK(os.str().find("Cormen") != std::string::npos);
    // inherited ElectronicResource fields also printed
    CHECK(os.str().find("access url") != std::string::npos);
}

// =========================================================================
// Q3: AudioBook and Thesis
// =========================================================================
static void testAudioBookAndThesis() {
    AudioBook ab("AB1", "The Pragmatic Programmer",
                 {"Thomas", "Hunt"}, "Narrator X", 480,
                 "Addison-Wesley", 2019, Money::of(25));
    CHECK(ab.category() == ResourceCategory::AudioBook);
    CHECK(!ab.isDigital());
    CHECK(ab.narrator() == "Narrator X");
    CHECK(ab.durationMinutes() == 480);
    CHECK(ab.costFor(2) == Money::of(50));
    CHECK_THROWS(AudioBook("AB2", "T", {}, "N", 0, "P", 2020, Money::of(1)),
                 std::invalid_argument);  // durationMinutes < 1

    std::ostringstream os;
    ab.print(os);
    CHECK(os.str().find("narrator: Narrator X") != std::string::npos);

    // Thesis is free by default
    Thesis th("T1", "Deep Learning Methods", "Alice", "Bob",
               "IIT Delhi", "PhD", "IIT Delhi", 2023);
    CHECK(th.category() == ResourceCategory::Thesis);
    CHECK(th.unitPrice().isZero());
    CHECK(th.costFor(1).isZero());
    CHECK(th.supervisor() == "Bob");
    CHECK(th.university() == "IIT Delhi");
    CHECK(th.degree() == "PhD");

    std::ostringstream os2;
    th.print(os2);
    CHECK(os2.str().find("supervisor: Bob") != std::string::npos);
}

// =========================================================================
// Q4: Magazine
// =========================================================================
static void testMagazine() {
    // unitPrice=100/year/copy, 12 issues/year, 1 year, postage=5/issue
    Magazine mag("M1", "Scientific American", "0036-8733",
                 "Springer Nature", 2024,
                 Money::of(100), 12, Money::of(5), 1);
    CHECK(mag.category() == ResourceCategory::Magazine);
    // costFor(2) = Journal::costFor(2) + postage*12*1*2
    //            = (100*2*1) + (5*12*1*2) = 200 + 120 = 320
    CHECK(mag.costFor(2) == Money::of(320));

    std::ostringstream os;
    mag.print(os);
    CHECK(os.str().find("postage per issue") != std::string::npos);

    CHECK_THROWS(Magazine("M2", "T", "i", "P", 2020, Money::of(1), 12,
                          Money::fromMinor(-1), 1),
                 std::invalid_argument);  // negative postage
}

// =========================================================================
// Q5: Hardcover costs 20% more
// =========================================================================
static void testHardcoverPricing() {
    Book pb("BP", "Test", {"A"}, "i", "P", 2020, Money::of(100),
            1, Binding::Paperback);
    Book hb("BH", "Test", {"A"}, "i", "P", 2020, Money::of(100),
            1, Binding::Hardcover);
    // Paperback: 100 * 3 = 300
    CHECK(pb.costFor(3) == Money::of(300));
    // Hardcover: 100 * 3 * 1.20 = 360
    CHECK(hb.costFor(3) == Money::of(360));
}

// =========================================================================
// Q6: Bulk discount
// =========================================================================
static void testBulkDiscount() {
    // Print: 10% off >= 10 copies
    Book b("B6", "T", {"A"}, "i", "P", 2020, Money::of(100));
    CHECK(b.costFor(9)  == Money::of(900));   // no discount
    CHECK(b.costFor(10) == Money::of(900));   // 1000 * 0.9 = 900
    CHECK(b.costFor(20) == Money::of(1800));  // 2000 * 0.9 = 1800

    // Electronic: seats beyond 50th at half price
    ElectronicResource e("E6", "DB", "P", 2026, Money::of(100), "url",
                         LicenseModel::AnnualSubscription, Money::of(0));
    CHECK(e.costFor(50) == Money::of(5000));  // all full price
    // 51 seats: 50*100 + 1*50 = 5050
    CHECK(e.costFor(51) == Money::of(5050));
    // 60 seats: 50*100 + 10*50 = 5500
    CHECK(e.costFor(60) == Money::of(5500));
}

// =========================================================================
// Q7: Tax policy
// =========================================================================
static void testTaxPolicy() {
    TaxPolicy policy;
    policy.setRate(ResourceCategory::Book,              {1800}); // 18%
    policy.setRate(ResourceCategory::ElectronicResource,{ 500}); //  5%

    CHECK(policy.taxOn(ResourceCategory::Book,              Money::of(100)) == Money::of(18));
    CHECK(policy.taxOn(ResourceCategory::ElectronicResource,Money::of(200)) == Money::of(10));
    CHECK(policy.totalWithTax(ResourceCategory::Book,       Money::of(100)) == Money::of(118));
    // No rate set for Journal → zero tax
    CHECK(policy.taxOn(ResourceCategory::Journal, Money::of(100)).isZero());

    Catalog cat;
    cat.emplace<Book>("BT", "Tax Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(100));
    Budget bud(Money::of(10000));
    AcquisitionManager acq(cat, bud, &policy);
    const auto& rec = acq.purchase("BT", 2);
    CHECK(rec.cost      == Money::of(200));   // pre-tax
    CHECK(rec.tax       == Money::of(36));    // 18% of 200
    CHECK(rec.totalCost == Money::of(236));   // cost + tax
}

// =========================================================================
// Q8: Title count quota
// =========================================================================
static void testTitleQuota() {
    Catalog cat;
    cat.emplace<Book>("Q8_1", "Title1", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(50));
    cat.emplace<Book>("Q8_2", "Title2", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(50));
    cat.emplace<Book>("Q8_3", "Title3", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(50));

    Budget bud(Money::of(10000));
    bud.setQuota(ResourceCategory::Book, {100, Money::of(5000), std::optional<int>(2)});

    AcquisitionManager acq(cat, bud);
    acq.purchase("Q8_1", 1);  // title 1 — OK
    acq.purchase("Q8_2", 1);  // title 2 — OK
    // title 3 is a new title → exceeds maxTitles=2
    CHECK_THROWS(acq.purchase("Q8_3", 1), QuotaExceededError);
    // Buying more of title 1 (not a new title) is still OK
    const auto& rec = acq.purchase("Q8_1", 2);
    CHECK(rec.approved);
}

// =========================================================================
// Q9: Cancel order
// =========================================================================
static void testCancelOrder() {
    Catalog cat;
    cat.emplace<Book>("BC1", "CancelBook", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(100));
    Budget bud(Money::of(1000));
    AcquisitionManager acq(cat, bud);

    const auto& rec = acq.purchase("BC1", 3);
    int orderToCancel = rec.orderNo;  // copy because rec might dangle after reallocation
    CHECK(rec.approved && rec.cost == Money::of(300));
    CHECK(cat.holdings("BC1") == 3);
    CHECK(bud.spent() == Money::of(300));

    const auto& cancel = acq.cancelOrder(orderToCancel);
    CHECK(!cancel.approved);
    CHECK(cancel.reason.find("cancellation") != std::string::npos);
    CHECK(cat.holdings("BC1") == 0);
    CHECK(bud.spent().isZero());
    CHECK(acq.history()[0].cancelled);

    CHECK_THROWS(acq.cancelOrder(orderToCancel), std::invalid_argument); // already cancelled
    CHECK_THROWS(acq.cancelOrder(999),         NotFoundError);          // no such order
}

// =========================================================================
// Q11: Year-end rollover
// =========================================================================
static void testYearEndRollover() {
    Budget bud(Money::of(1000));
    bud.commit(ResourceCategory::Book, 2, Money::of(300));
    CHECK(bud.remaining() == Money::of(700));

    // 50% rollover of unspent 700 = 350
    bud.yearEndRollover(50);
    CHECK(bud.total()  == Money::of(350));
    CHECK(bud.spent().isZero());

    // 100% rollover
    Budget bud2(Money::of(500));
    bud2.commit(ResourceCategory::Book, 1, Money::of(100));
    bud2.yearEndRollover(100);
    CHECK(bud2.total() == Money::of(400));

    // 0% rollover
    Budget bud3(Money::of(500));
    bud3.commit(ResourceCategory::Book, 1, Money::of(200));
    bud3.yearEndRollover(0);
    CHECK(bud3.total().isZero());

    CHECK_THROWS(bud3.yearEndRollover(101), std::invalid_argument);
}

// =========================================================================
// Q13: All-or-nothing batch
// =========================================================================
static void testAllOrNothingBatch() {
    Catalog cat;
    cat.emplace<Book>("AON1", "Book1", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(100));
    cat.emplace<Book>("AON2", "Book2", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(100));
    Budget bud(Money::of(250));
    AcquisitionManager acq(cat, bud);

    // Would partially fail → all rejected
    auto res = acq.processBatch({{"AON1", 2}, {"AON2", 2}}, /*allOrNothing=*/true);
    CHECK(res.size() == 2);
    CHECK(!res[0].approved && !res[1].approved);
    CHECK(bud.spent().isZero());

    // Fits completely → all approved
    auto res2 = acq.processBatch({{"AON1", 1}, {"AON2", 1}}, /*allOrNothing=*/true);
    CHECK(res2.size() == 2);
    CHECK(res2[0].approved && res2[1].approved);
}

// =========================================================================
// Q14: Priority ordering
// =========================================================================
static void testPriorityBatch() {
    Catalog cat;
    cat.emplace<Book>("P_LO", "LowPri",  std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(400));
    cat.emplace<Book>("P_HI", "HighPri", std::vector<std::string>{"A"}, "i", "P", 2020,
                      Money::of(100));

    // Tight budget: can afford HighPri or LowPri, but not both.
    Budget bud(Money::of(450));
    AcquisitionManager acq(cat, bud);

    // Without priority: LowPri first (400) → HighPri rejected (50 left < 100)
    auto res1 = acq.processBatch(
        {{"P_LO", 1, 0}, {"P_HI", 1, 10}},
        /*allOrNothing=*/false, /*sortByPriority=*/false);
    CHECK( res1[0].approved);   // LowPri first, approved
    CHECK(!res1[1].approved);   // HighPri rejected

    // With priority sort: results are in processing order (high-pri first).
    Budget bud2(Money::of(450));
    AcquisitionManager acq2(cat, bud2);
    auto res2 = acq2.processBatch(
        {{"P_LO", 1, 0}, {"P_HI", 1, 10}},
        /*allOrNothing=*/false, /*sortByPriority=*/true);
    // Processing order: P_HI(pri=10), P_LO(pri=0).
    // res2[0] = P_HI (approved), res2[1] = P_LO (rejected — 400 > remaining 350).
    CHECK( res2[0].approved && res2[0].resourceId == "P_HI");
    CHECK(!res2[1].approved && res2[1].resourceId == "P_LO");
}

// =========================================================================
// Q16: Extended catalog search (author, ISBN/ISSN, year range)
// =========================================================================
static void testCatalogExtendedSearch() {
    Catalog cat;
    cat.emplace<Book>("BS1", "C++ Primer",
                      std::vector<std::string>{"Lippman", "Lajoie"},
                      "isbn-cpp", "Addison", 2012, Money::of(500));
    cat.emplace<Book>("BS2", "Effective C++",
                      std::vector<std::string>{"Meyers"},
                      "isbn-eff", "Addison", 2005, Money::of(400));
    cat.emplace<Journal>("JS1", "CACM", "issn-cacm", "ACM", 2020, Money::of(200), 12);
    cat.emplace<AudioBook>("AB1", "Clean Code Audio",
                           std::vector<std::string>{"Martin"},
                           "Narrator", 300, "Prentice", 2014, Money::of(30));

    // searchByAuthor — case-insensitive substring
    auto byLippman = cat.searchByAuthor("lippman");
    CHECK(byLippman.size() == 1 && byLippman[0]->id() == "BS1");
    auto byMartin  = cat.searchByAuthor("Martin");
    CHECK(byMartin.size()  == 1 && byMartin[0]->id()  == "AB1");
    CHECK(cat.searchByAuthor("nobody").empty());

    // searchByIsbn — exact match (works for ISBN and ISSN)
    auto byIsbn = cat.searchByIsbn("isbn-cpp");
    CHECK(byIsbn.size() == 1 && byIsbn[0]->id() == "BS1");
    auto byIssn = cat.searchByIsbn("issn-cacm");
    CHECK(byIssn.size() == 1 && byIssn[0]->id() == "JS1");

    // searchByYearRange — inclusive
    auto range = cat.searchByYearRange(2005, 2012);
    CHECK(range.size() == 2);  // BS1(2012), BS2(2005)
    auto after2015 = cat.searchByYearRange(2015, 2030);
    CHECK(after2015.size() == 1 && after2015[0]->id() == "JS1");
    CHECK(cat.searchByYearRange(1900, 1999).empty());
}

// =========================================================================
// Q10: Department budgets
// =========================================================================
static void testDepartmentBudgets() {
    Catalog cat;
    cat.emplace<Book>("B1", "Title", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));

    Budget defaultBud(Money::of(100));
    Budget csBud(Money::of(500));
    Budget mathBud(Money::of(50));
    
    AcquisitionManager acq(cat, defaultBud);
    acq.addDepartment("CS", csBud);
    acq.addDepartment("Math", mathBud);

    // CS has 500, can buy 3 (300)
    auto res1 = acq.purchase("B1", 3, "CS");
    CHECK(res1.approved);
    CHECK(csBud.spent() == Money::of(300));
    CHECK(defaultBud.spent().isZero());

    // Math has 50, cannot buy 1 (100)
    std::string why;
    CHECK(!acq.canPurchase("B1", 1, &why, "Math"));
    
    // Unknown department throws
    CHECK_THROWS(acq.purchase("B1", 1, "Physics"), std::invalid_argument);
}

// =========================================================================
// Q15: Vendors (cheapest vendor)
// =========================================================================
static void testVendors() {
    Book b("V1", "Title", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    CHECK(b.unitPrice() == Money::of(100));
    CHECK(b.cheapestVendor() == "Default");

    b.addVendor("Amazon", Money::of(90));
    b.addVendor("Local", Money::of(110));
    
    // unitPrice should update to the cheapest
    CHECK(b.unitPrice() == Money::of(90));
    CHECK(b.cheapestVendor() == "Amazon");
    
    b.addVendor("Direct", Money::of(80));
    CHECK(b.unitPrice() == Money::of(80));
    CHECK(b.cheapestVendor() == "Direct");
}

// =========================================================================
// Q18: Lending
// =========================================================================
static void testLending() {
    Catalog cat;
    cat.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    
    // Initially 0 holdings
    CHECK(cat.holdings("B1") == 0);
    CHECK(cat.borrowed("B1") == 0);
    
    CHECK_THROWS(cat.borrowResource("B1"), std::invalid_argument); // none available
    
    cat.addHoldings("B1", 2);
    CHECK(cat.holdings("B1") == 2);
    
    cat.borrowResource("B1");
    CHECK(cat.borrowed("B1") == 1);
    
    cat.borrowResource("B1");
    CHECK(cat.borrowed("B1") == 2);
    
    CHECK_THROWS(cat.borrowResource("B1"), std::invalid_argument); // all borrowed
    
    cat.returnResource("B1");
    CHECK(cat.borrowed("B1") == 1);
    
    cat.returnResource("B1");
    CHECK(cat.borrowed("B1") == 0);
    
    CHECK_THROWS(cat.returnResource("B1"), std::invalid_argument); // none borrowed
}

// =========================================================================
int main() {
    // Original tests
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();

    // Q1 – Q16 tests
    testJournal();
    testEBook();
    testAudioBookAndThesis();
    testMagazine();
    testHardcoverPricing();
    testBulkDiscount();
    testTaxPolicy();
    testTitleQuota();
    testCancelOrder();
    testYearEndRollover();
    testAllOrNothingBatch();
    testPriorityBatch();
    testCatalogExtendedSearch();
    testDepartmentBudgets();
    testVendors();
    testLending();

    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
