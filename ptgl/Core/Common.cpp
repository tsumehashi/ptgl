#include "Common.h"

namespace ptgl {

bool keyToAsciiChar(char& result, ptgl::Key key, int mod)
{
    switch (key) {
    case ptgl::Key::Key_Space: result = ' '; return true; break;
    default:
        break;
    }

    if (mod == ptgl::ModifierKey_None) {
        switch (key) {
        case ptgl::Key::Key_0: result = '0'; return true; break;
        case ptgl::Key::Key_1: result = '1'; return true; break;
        case ptgl::Key::Key_2: result = '2'; return true; break;
        case ptgl::Key::Key_3: result = '3'; return true; break;
        case ptgl::Key::Key_4: result = '4'; return true; break;
        case ptgl::Key::Key_5: result = '5'; return true; break;
        case ptgl::Key::Key_6: result = '6'; return true; break;
        case ptgl::Key::Key_7: result = '7'; return true; break;
        case ptgl::Key::Key_8: result = '8'; return true; break;
        case ptgl::Key::Key_9: result = '9'; return true; break;
        case ptgl::Key::Key_Colon:        result = ':'; return true; break;
        case ptgl::Key::Key_Semicolon:    result = ';'; return true; break;
        case ptgl::Key::Key_Comma:        result = ','; return true; break;
        case ptgl::Key::Key_Minus:        result = '-'; return true; break;
        case ptgl::Key::Key_Period:       result = '.'; return true; break;
        case ptgl::Key::Key_Slash:        result = '/'; return true; break;
        case ptgl::Key::Key_LeftBracket:  result = '['; return true; break;
        case ptgl::Key::Key_RightBracket: result = ']'; return true; break;
        case ptgl::Key::Key_At:           result = '@'; return true; break;
        case ptgl::Key::Key_Caret:        result = '^'; return true; break;
        case ptgl::Key::Key_BackSlash_VerticalBar: result = '\\'; return true; break;
        case ptgl::Key::Key_BackSlash:    result = '\\'; return true; break;
        case ptgl::Key::Key_A: result = 'a'; return true; break;
        case ptgl::Key::Key_B: result = 'b'; return true; break;
        case ptgl::Key::Key_C: result = 'c'; return true; break;
        case ptgl::Key::Key_D: result = 'd'; return true; break;
        case ptgl::Key::Key_E: result = 'e'; return true; break;
        case ptgl::Key::Key_F: result = 'f'; return true; break;
        case ptgl::Key::Key_G: result = 'g'; return true; break;
        case ptgl::Key::Key_H: result = 'h'; return true; break;
        case ptgl::Key::Key_I: result = 'i'; return true; break;
        case ptgl::Key::Key_J: result = 'j'; return true; break;
        case ptgl::Key::Key_K: result = 'k'; return true; break;
        case ptgl::Key::Key_L: result = 'l'; return true; break;
        case ptgl::Key::Key_M: result = 'm'; return true; break;
        case ptgl::Key::Key_N: result = 'n'; return true; break;
        case ptgl::Key::Key_O: result = 'o'; return true; break;
        case ptgl::Key::Key_P: result = 'p'; return true; break;
        case ptgl::Key::Key_Q: result = 'q'; return true; break;
        case ptgl::Key::Key_R: result = 'r'; return true; break;
        case ptgl::Key::Key_S: result = 's'; return true; break;
        case ptgl::Key::Key_T: result = 't'; return true; break;
        case ptgl::Key::Key_U: result = 'u'; return true; break;
        case ptgl::Key::Key_V: result = 'v'; return true; break;
        case ptgl::Key::Key_W: result = 'w'; return true; break;
        case ptgl::Key::Key_X: result = 'x'; return true; break;
        case ptgl::Key::Key_Y: result = 'y'; return true; break;
        case ptgl::Key::Key_Z: result = 'z'; return true; break;
        default:
            return false;
            break;
        }

    } else if (mod == ptgl::ModifierKey_Shift) {
        switch (key) {
        case ptgl::Key::Key_0: result = '~'; return true; break;
        case ptgl::Key::Key_1: result = '!'; return true; break;
        case ptgl::Key::Key_2: result = '\"'; return true; break;
        case ptgl::Key::Key_3: result = '#'; return true; break;
        case ptgl::Key::Key_4: result = '$'; return true; break;
        case ptgl::Key::Key_5: result = '%'; return true; break;
        case ptgl::Key::Key_6: result = '&'; return true; break;
        case ptgl::Key::Key_7: result = '\''; return true; break;
        case ptgl::Key::Key_8: result = '('; return true; break;
        case ptgl::Key::Key_9: result = ')'; return true; break;
        case ptgl::Key::Key_Colon:        result = '*'; return true; break;
        case ptgl::Key::Key_Semicolon:    result = '+'; return true; break;
        case ptgl::Key::Key_Comma:        result = '<'; return true; break;
        case ptgl::Key::Key_Minus:        result = '='; return true; break;
        case ptgl::Key::Key_Period:       result = '>'; return true; break;
        case ptgl::Key::Key_Slash:        result = '?'; return true; break;
        case ptgl::Key::Key_LeftBracket:  result = '{'; return true; break;
        case ptgl::Key::Key_RightBracket: result = '}'; return true; break;
        case ptgl::Key::Key_At:           result = '`'; return true; break;
        case ptgl::Key::Key_Caret:        result = '~'; return true; break;
        case ptgl::Key::Key_BackSlash_VerticalBar: result = '|'; return true; break;
        case ptgl::Key::Key_BackSlash:    result = '_'; return true; break;
        case ptgl::Key::Key_A: result = 'A'; return true; break;
        case ptgl::Key::Key_B: result = 'B'; return true; break;
        case ptgl::Key::Key_C: result = 'C'; return true; break;
        case ptgl::Key::Key_D: result = 'D'; return true; break;
        case ptgl::Key::Key_E: result = 'E'; return true; break;
        case ptgl::Key::Key_F: result = 'F'; return true; break;
        case ptgl::Key::Key_G: result = 'G'; return true; break;
        case ptgl::Key::Key_H: result = 'H'; return true; break;
        case ptgl::Key::Key_I: result = 'I'; return true; break;
        case ptgl::Key::Key_J: result = 'J'; return true; break;
        case ptgl::Key::Key_K: result = 'K'; return true; break;
        case ptgl::Key::Key_L: result = 'L'; return true; break;
        case ptgl::Key::Key_M: result = 'M'; return true; break;
        case ptgl::Key::Key_N: result = 'N'; return true; break;
        case ptgl::Key::Key_O: result = 'O'; return true; break;
        case ptgl::Key::Key_P: result = 'P'; return true; break;
        case ptgl::Key::Key_Q: result = 'Q'; return true; break;
        case ptgl::Key::Key_R: result = 'R'; return true; break;
        case ptgl::Key::Key_S: result = 'S'; return true; break;
        case ptgl::Key::Key_T: result = 'T'; return true; break;
        case ptgl::Key::Key_U: result = 'U'; return true; break;
        case ptgl::Key::Key_V: result = 'V'; return true; break;
        case ptgl::Key::Key_W: result = 'W'; return true; break;
        case ptgl::Key::Key_X: result = 'X'; return true; break;
        case ptgl::Key::Key_Y: result = 'Y'; return true; break;
        case ptgl::Key::Key_Z: result = 'Z'; return true; break;
        default:
            return false;
            break;
        }
    }

    return false;
}

}
