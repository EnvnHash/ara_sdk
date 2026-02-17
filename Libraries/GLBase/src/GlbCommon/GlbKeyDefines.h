//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once


#define ARA_KEY_UNKNOWN (-1)

#define ARA_KEY_LEFT 18
#define ARA_KEY_UP 19
#define ARA_KEY_RIGHT 20
#define ARA_KEY_DOWN 21

#define ARA_KEY_SPACE 32
#define ARA_KEY_APOSTROPHE 39 /* ' */
#define ARA_KEY_COMMA 44      /* , */
#define ARA_KEY_MINUS 45      /* - */
#define ARA_KEY_PERIOD 46     /* . */
#define ARA_KEY_SLASH 47      /* / */
#define ARA_KEY_0 48
#define ARA_KEY_1 49
#define ARA_KEY_2 50
#define ARA_KEY_3 51
#define ARA_KEY_4 52
#define ARA_KEY_5 53
#define ARA_KEY_6 54
#define ARA_KEY_7 55
#define ARA_KEY_8 56
#define ARA_KEY_9 57
#define ARA_KEY_SEMICOLON 59 /* ; */
#define ARA_KEY_EQUAL 61     /* = */
#define ARA_KEY_A 65
#define ARA_KEY_B 66
#define ARA_KEY_C 67
#define ARA_KEY_D 68
#define ARA_KEY_E 69
#define ARA_KEY_F 70
#define ARA_KEY_G 71
#define ARA_KEY_H 72
#define ARA_KEY_I 73
#define ARA_KEY_J 74
#define ARA_KEY_K 75
#define ARA_KEY_L 76
#define ARA_KEY_M 77
#define ARA_KEY_N 78
#define ARA_KEY_O 79
#define ARA_KEY_P 80
#define ARA_KEY_Q 81
#define ARA_KEY_R 82
#define ARA_KEY_S 83
#define ARA_KEY_T 84
#define ARA_KEY_U 85
#define ARA_KEY_V 86
#define ARA_KEY_W 87
#define ARA_KEY_X 88
#define ARA_KEY_Y 89
#define ARA_KEY_Z 90
#define ARA_KEY_LEFT_BRACKET 91  /* [ */
#define ARA_KEY_BACKSLASH 92     /* \ */
#define ARA_KEY_RIGHT_BRACKET 93 /* ] */
#define ARA_KEY_GRAVE_ACCENT 96  /* ` */
#define ARA_KEY_WORLD_1 161      /* non-US #1 */
#define ARA_KEY_WORLD_2 162      /* non-US #2 */

/* std::function keys */
#define ARA_KEY_ESCAPE 256
#define ARA_KEY_ENTER 257
#define ARA_KEY_TAB 258
#define ARA_KEY_BACKSPACE 259
#define ARA_KEY_INSERT 260
#define ARA_KEY_DELETE 261
#define ARA_KEY_PAGE_UP 266
#define ARA_KEY_PAGE_DOWN 267
#define ARA_KEY_HOME 268
#define ARA_KEY_END 269
#define ARA_KEY_CAPS_LOCK 280
#define ARA_KEY_SCROLL_LOCK 281
#define ARA_KEY_NUM_LOCK 282
#define ARA_KEY_PRINT_SCREEN 283
#define ARA_KEY_PAUSE 284
#define ARA_KEY_F1 290
#define ARA_KEY_F2 291
#define ARA_KEY_F3 292
#define ARA_KEY_F4 293
#define ARA_KEY_F5 294
#define ARA_KEY_F6 295
#define ARA_KEY_F7 296
#define ARA_KEY_F8 297
#define ARA_KEY_F9 298
#define ARA_KEY_F10 299
#define ARA_KEY_F11 300
#define ARA_KEY_F12 301
#define ARA_KEY_F13 302
#define ARA_KEY_F14 303
#define ARA_KEY_F15 304
#define ARA_KEY_F16 305
#define ARA_KEY_F17 306
#define ARA_KEY_F18 307
#define ARA_KEY_F19 308
#define ARA_KEY_F20 309
#define ARA_KEY_F21 310
#define ARA_KEY_F22 311
#define ARA_KEY_F23 312
#define ARA_KEY_F24 313
#define ARA_KEY_F25 314
#define ARA_KEY_KP_0 320
#define ARA_KEY_KP_1 321
#define ARA_KEY_KP_2 322
#define ARA_KEY_KP_3 323
#define ARA_KEY_KP_4 324
#define ARA_KEY_KP_5 325
#define ARA_KEY_KP_6 326
#define ARA_KEY_KP_7 327
#define ARA_KEY_KP_8 328
#define ARA_KEY_KP_9 329
#define ARA_KEY_KP_DECIMAL 330
#define ARA_KEY_KP_DIVIDE 331
#define ARA_KEY_KP_MULTIPLY 332
#define ARA_KEY_KP_SUBTRACT 333
#define ARA_KEY_KP_ADD 334
#define ARA_KEY_KP_ENTER 335
#define ARA_KEY_KP_EQUAL 336
#define ARA_KEY_LEFT_SHIFT 340
#define ARA_KEY_LEFT_CONTROL 341
#define ARA_KEY_LEFT_ALT 342
#define ARA_KEY_LEFT_SUPER 343
#define ARA_KEY_RIGHT_SHIFT 344
#define ARA_KEY_RIGHT_CONTROL 345
#define ARA_KEY_RIGHT_ALT 346
#define ARA_KEY_RIGHT_SUPER 347
#define ARA_KEY_MENU 348

#define ARA_KEY_LAST ARA_KEY_MENU

/*! @brief If this bit is set one or more Shift keys were held down.
 *
 *  If this bit is set one or more Shift keys were held down.
 */
#define ARA_MOD_SHIFT 0x0001
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
#define ARA_MOD_CONTROL 0x0002
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
#define ARA_MOD_ALT 0x0004
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
#define ARA_MOD_SUPER 0x0008
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define ARA_MOD_CAPS_LOCK 0x0010
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define ARA_MOD_NUM_LOCK 0x0020

/*! @name Key and button actions
 *  @{ */
/*! @brief The key or mouse button was released.
 *
 *  The key or mouse button was released.
 *
 *  @ingroup input
 */
#define ARA_RELEASE 0
/*! @brief The key or mouse button was pressed.
 *
 *  The key or mouse button was pressed.
 *
 *  @ingroup input
 */
#define ARA_PRESS 1
/*! @brief The key was held down until it repeated.
 *
 *  The key was held down until it repeated.
 *
 *  @ingroup input
 */
#define ARA_REPEAT 2
/*! @} */

/*! @defgroup buttons Mouse buttons
 *  @brief Mouse button IDs.
 *
 *  See [mouse button input](@ref input_mouse_button) for how these are used.
 *
 *  @ingroup input
 *  @{ */
#define ARA_MOUSE_BUTTON_1 0
#define ARA_MOUSE_BUTTON_2 1
#define ARA_MOUSE_BUTTON_3 2
#define ARA_MOUSE_BUTTON_4 3
#define ARA_MOUSE_BUTTON_5 4
#define ARA_MOUSE_BUTTON_6 5
#define ARA_MOUSE_BUTTON_7 6
#define ARA_MOUSE_BUTTON_8 7
#define ARA_MOUSE_BUTTON_LAST ARA_MOUSE_BUTTON_8
#define ARA_MOUSE_BUTTON_LEFT ARA_MOUSE_BUTTON_1
#define ARA_MOUSE_BUTTON_RIGHT ARA_MOUSE_BUTTON_2
#define ARA_MOUSE_BUTTON_MIDDLE ARA_MOUSE_BUTTON_3
/*! @} */

#define ARA_CURSOR 0x00033001
#define ARA_STICKY_KEYS 0x00033002
#define ARA_STICKY_MOUSE_BUTTONS 0x00033003
#define ARA_LOCK_KEY_MODS 0x00033004
#define ARA_RAW_MOUSE_MOTION 0x00033005

#define ARA_CURSOR_NORMAL 0x00034001
#define ARA_CURSOR_HIDDEN 0x00034002
#define ARA_CURSOR_DISABLED 0x00034003

// Internal key state used for sticky keys
#define _ARA_STICK 3