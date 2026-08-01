#pragma once

#include <string>
#include <vector>

enum class Language
{
    English,
    Chinese, // Simplified Chinese
    TraditionalChinese
};

namespace Translation
{
void SetLanguage(Language lang);
Language GetLanguage();
const char* GetLanguageName(Language lang);
const std::vector<Language>& GetAllLanguages();
const char* Get(const char* key);
const char* Get(const std::string& key);
void Init();
} // namespace Translation
