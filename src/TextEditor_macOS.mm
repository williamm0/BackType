#include "TextEditor.h"

#import <AppKit/AppKit.h>

namespace backtype {

bool edit_text_dialog(std::string *text) {
    if (!text) {
        return false;
    }

    __block bool accepted = false;
    __block std::string edited = *text;

    auto run_dialog = ^{
      @autoreleasepool {
          NSAlert *alert = [[NSAlert alloc] init];
          alert.messageText = @"BackType Text";
          alert.informativeText = @"Type the text BackType should render.";

          NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0.0, 0.0, 420.0, 24.0)];
          field.stringValue = [NSString stringWithUTF8String:text->c_str()] ?: @"";
          alert.accessoryView = field;

          [alert addButtonWithTitle:@"OK"];
          [alert addButtonWithTitle:@"Cancel"];
          [[alert window] setInitialFirstResponder:field];

          const NSModalResponse response = [alert runModal];
          if (response == NSAlertFirstButtonReturn) {
              edited = std::string([[field stringValue] UTF8String] ?: "");
              accepted = true;
          }
      }
    };

    if ([NSThread isMainThread]) {
        run_dialog();
    } else {
        dispatch_sync(dispatch_get_main_queue(), run_dialog);
    }

    if (accepted) {
        *text = edited;
    }
    return accepted;
}

} // namespace backtype
