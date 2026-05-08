#include "../src/BackType_TextData.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect_eq(const std::string &actual, const std::string &expected, const char *message) {
    if (actual != expected) {
        std::cerr << "FAIL: " << message << " expected [" << expected << "] got [" << actual << "]\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    BackTypeTextData data{};

    backtype::set_text_data(&data, "");
    expect_eq(backtype::text_from_data(&data), "", "empty text is preserved");

    backtype::set_text_data(&data, "hello   world");
    expect_eq(backtype::text_from_data(&data), "hello   world", "spaces are preserved");

    backtype::set_text_data(&data, "Hello, BackType! 123.");
    expect_eq(backtype::text_from_data(&data), "Hello, BackType! 123.", "basic punctuation is preserved");

    backtype::set_text_data(&data, "typing gypqj");
    expect_eq(backtype::text_from_data(&data), "typing gypqj", "descender-heavy text is preserved");

    std::cout << "BackType_TextDataTests passed\n";
    return 0;
}
