#include "BackType_TextData.h"

#include <algorithm>
#include <cstring>

namespace backtype {

namespace {

bool is_utf8_continuation(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

void append_codepoint(std::string &text, unsigned int cp) {
    if (cp <= 0x7FU) {
        text.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FFU) {
        text.push_back(static_cast<char>(0xC0U | ((cp >> 6U) & 0x1FU)));
        text.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    } else if (cp <= 0xFFFFU) {
        text.push_back(static_cast<char>(0xE0U | ((cp >> 12U) & 0x0FU)));
        text.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    } else if (cp <= 0x10FFFFU) {
        text.push_back(static_cast<char>(0xF0U | ((cp >> 18U) & 0x07U)));
        text.push_back(static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    }
}

} // namespace

void set_text_data(BackTypeTextData *data, const std::string &text) {
    if (!data) {
        return;
    }

    std::memset(data->text, 0, sizeof(data->text));
    auto copy_size = std::min(text.size(), sizeof(data->text) - 1);
    std::memcpy(data->text, text.data(), copy_size);

    while (copy_size > 0 && is_utf8_continuation(static_cast<unsigned char>(data->text[copy_size - 1]))) {
        data->text[copy_size - 1] = '\0';
        --copy_size;
    }
}

std::string text_from_data(const BackTypeTextData *data) {
    if (!data) {
        return {};
    }

    std::size_t length = 0;
    while (length < sizeof(data->text) && data->text[length] != '\0') {
        ++length;
    }
    return std::string(data->text, length);
}

std::string append_utf8_character(const std::string &text, unsigned int codepoint) {
    std::string result = text;
    append_codepoint(result, codepoint);
    if (result.size() >= kBackTypeTextMaxBytes) {
        result.resize(kBackTypeTextMaxBytes - 1);
        while (!result.empty() && is_utf8_continuation(static_cast<unsigned char>(result.back()))) {
            result.pop_back();
        }
    }
    return result;
}

std::string erase_last_utf8_character(const std::string &text) {
    if (text.empty()) {
        return {};
    }

    std::size_t index = text.size() - 1;
    while (index > 0 && is_utf8_continuation(static_cast<unsigned char>(text[index]))) {
        --index;
    }
    return text.substr(0, index);
}

} // namespace backtype
