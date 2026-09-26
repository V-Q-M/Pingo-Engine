#pragma once

// What a click in a menu triggered
enum class MenuAction {
    None,

    // The item itself was clicked
    Activate,

    // The - next to an adjustable value
    Decrease,

    // The + next to an adjustable value
    Increase,

    // An option was picked from a dropdown, see Menu::Event::option
    Choose
};
