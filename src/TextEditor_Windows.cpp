#include "TextEditor.h"

#ifdef WIN_ENV
#include <Windows.h>
#endif

namespace backtype {

bool edit_text_dialog(std::string *text) {
    (void)text;
#ifdef WIN_ENV
    MessageBoxA(nullptr,
                "BackType text editing needs a small Windows dialog implementation. "
                "This prerelease keeps the Windows build compiling but does not edit text on Windows yet.",
                "BackType",
                MB_OK | MB_ICONINFORMATION);
#endif
    return false;
}

} // namespace backtype
