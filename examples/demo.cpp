// MT26135 - Raunaq Soneja
// Demo: comprehensive showcase of all BookManagement features (Q1-Q19)

#include <iostream>
#include <iomanip>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    std::cout << "========================================\n"
              << "       BOOK MANAGEMENT DEMO\n"
              << "========================================\n\n";

    Catalog catalog;

    // 1. Core items (Book, ElectronicResource) + Q5 (Hardcover surcharge)
    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover); // 20% surcharge
    
    // Q6 bulk discount test for E-Resource
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    
    // Q1: Journal
    catalog.emplace<Journal>("J001", "Communications of the ACM", "0001-0782",
                             "ACM", 2024, Money::of(500), 12, 1);
                             
    // Q2: EBook
    catalog.emplace<EBook>("E001", "Effective Modern C++",
                           std::vector<std::string>{"Scott Meyers"}, "978-1491903995",
                           "O'Reilly", 2014, Money::of(30), "https://oreilly.com/ebook",
                           EBookFormat::EPUB, true);

    // Q3: AudioBook & Thesis
    catalog.emplace<AudioBook>("AB01", "The Pragmatic Programmer",
                               std::vector<std::string>{"David Thomas", "Andrew Hunt"},
                               "Anna Narrator", 500, "Addison-Wesley", 2019, Money::of(25));
    catalog.emplace<Thesis>("T001", "High Performance C++ Containers", "Alice Student",
                            "Bob Professor", "Tech University", "PhD", "Tech University", 2023);

    // Q4: Magazine
    catalog.emplace<Magazine>("M001", "Wired", "1059-1028", "Condé Nast", 2024,
                              Money::of(100), 12, Money::of(5), 1);

    std::cout << "=== Catalog (Various Resource Types) ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";
    
    std::cout << "\n=== Details of specific items ===\n";
    std::cout << catalog.get("J001") << "\n";
    std::cout << catalog.get("E001") << "\n";

    // Q16: Extended Search
    std::cout << "\n=== Extended Search ===\n";
    std::cout << "Search author 'Martin':\n";
    for (auto* r : catalog.searchByAuthor("Martin")) std::cout << "  - " << r->title() << "\n";
    std::cout << "Search Year Range 2020-2025:\n";
    for (auto* r : catalog.searchByYearRange(2020, 2025)) std::cout << "  - " << r->title() << "\n";

    // Q7: Tax Policy Setup
    TaxPolicy tax;
    tax.setRate(ResourceCategory::Book, {500}); // 5% tax on books
    tax.setRate(ResourceCategory::ElectronicResource, {1800}); // 18% tax on generic E-resources
    tax.setRate(ResourceCategory::EBook, {1200}); // 12% tax on EBooks

    // Budget Setup with Title Quotas (Q8)
    Budget budget(Money::of(30000));
    budget.setQuota(ResourceCategory::Book, {20, Money::of(10000), std::optional<int>(2)});
    budget.setQuota(ResourceCategory::ElectronicResource, {60, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {10, Money::of(5000)});
    budget.setQuota(ResourceCategory::EBook, {5, Money::of(1000)});

    AcquisitionManager acq(catalog, budget, &tax);

    // Purchase Requests using batch mode
    std::cout << "\n=== Q14: Processing Batch (Priority Order) ===\n";
    // Using Q14 priority: Higher priority processed first
    auto results = acq.processBatch({
        {"B001", 10, 0},   // priority 0 (lowest)
        {"B002", 5, 10},   // priority 10 (highest)
        {"J001", 3, 5},    // priority 5
        {"E001", 5, 8},    // priority 8
        {"B001", 5, 2},    // another order for B001
        {"T001", 1, 0}     // Thesis is free
    }, false, true);       // allOrNothing=false, sortByPriority=true

    // Q12 Quota Warnings might have been printed to stderr

    std::cout << "\n=== Acquisition Report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget after purchases ===\n";
    budget.print(std::cout);

    // Q9: Cancel an order
    std::cout << "\n=== Q9: Cancelling an order ===\n";
    int orderToCancel = results[0].orderNo; // the B002 order (processed first)
    acq.cancelOrder(orderToCancel);
    std::cout << "Order #" << orderToCancel << " cancelled. Budget refunded.\n";
    budget.print(std::cout);

    // Q11: Year-End Rollover
    std::cout << "\n=== Q11: Year-End Rollover ===\n";
    std::cout << "Rolling over 50% of remaining funds...\n";
    budget.yearEndRollover(50);
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all()) {
        int holds = catalog.holdings(r->id());
        if (holds > 0)
            std::cout << "  " << std::setw(6) << r->id() << ": " << holds
                      << (r->isDigital() ? " seats" : " copies") << "\n";
    }

    return 0;
}
