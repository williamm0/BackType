#pragma once

#include <cstddef>
#include <string>

constexpr std::size_t kBackTypeTextMaxBytes = 1024;

struct BackTypeTextData {
    char text[kBackTypeTextMaxBytes];
};

namespace backtype {

void set_text_data(BackTypeTextData *data, const std::string &text);
std::string text_from_data(const BackTypeTextData *data);
std::string append_utf8_character(const std::string &text, unsigned int codepoint);
std::string erase_last_utf8_character(const std::string &text);

} // namespace backtype
