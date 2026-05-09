#pragma once

namespace backtype {

enum class AnchorMode {
    FirstCharacterLocked = 1,
    NewestCharacterLocked = 2,
    CenterLocked = 3,
    LastCharacterLocked = 4
};

enum class Direction {
    MoveLeft = 1,
    MoveRight = 2,
    MoveUp = 3,
    MoveDown = 4
};

enum class RevealMode {
    Character = 1,
    Word = 2
};

enum class CursorStyle {
    Line = 1,
    Block = 2,
    Underscore = 3
};

} // namespace backtype
