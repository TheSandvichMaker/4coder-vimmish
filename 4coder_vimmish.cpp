#ifndef FCODER_VIMMISH_CPP
#define FCODER_VIMMISH_CPP

// TODO: Make the list of user exposed defines in the usage comment up to date

/* USAGE:
    #include "4coder_default_include.cpp"

    You could override some defines here:
    // #define VIM_MOUSE_SELECT_MODE             [Visual / VisualLine / VisualBlock]        [default: Visual]
    // #define VIM_ESCAPE_SEQUENCE               [two character string]                     [default: "jk"]
    // #define VIM_AUTO_LINE_COMMENTS            [0 / 1]                                    [default: 1]
    // #define VIM_CASE_SENSITIVE_CHARACTER_SEEK [0 / 1]                                    [default: 1]
    // #define VIM_AUTO_INDENT_ON_PASTE          [0 / 1]                                    [default: 1]
    // #define VIM_DEFAULT_REGISTER              [unnamed_register / clipboard_register]    [default: unnamed_register]
    // #define VIM_JUMP_HISTORY_SIZE             [integer]                                  [default: 100]
    // #define VIM_MAXIMUM_OP_COUNT              [integer]                                  [default: 256]
    // #define VIM_USE_CUSTOM_COLORS             [0 / 1]                                    [default: 1]
    // #define VIM_USE_CHARACTER_SEEK_HIGHLIGHTS [0 / 1]                                    [default: 1]
    // #define VIM_FILE_BAR_ON_BOTTOM            [0 / 1]                                    [default: 0]
    // #define VIM_DRAW_PANEL_MARGINS            [0 / 1 / 2]                                [default: 2] (0: don't draw, 1: minimal vertical margins, 2: full margins)
    // #define VIM_PANEL_MARGIN_THICKNESS        [float]                                    [default: 3.0f]
    // #define VIM_USE_ECHO_BAR                  [0 / 1]                                    [default: 0]
    // #define VIM_CURSOR_ROUNDNESS              [float]                                    [default: 0.9f]

    #include "4coder_vimmish.cpp"

    // You could add your own operators / motions.

    #include "generated/managed_id_metadata.cpp"

    void custom_layer_init(Application_Links *app) {
        Thread_Context *tctx = get_thread_context(app);

        default_framework_init(app);
        set_all_default_hooks(app);
        mapping_init(tctx, &framework_mapping);

        vim_init(app);

        // You can - much like the base 4coder design - copy these default hook and mapping setup functions and make your own changes.
        // You will find them near the bottom of this file.

        vim_set_default_hooks(app);
        Vim_Key vim_leader_key = vim_key(KeyCode_Space); // Or whatever you prefer
        vim_setup_default_mapping(app, &framework_mapping, vim_leader_key);
    }

    // If you enable VIM_USE_CUSTOM_COLORS, you will be able to use the following colors in your theme file:
    // defcolor_vim_bar_normal -- filebar color in normal mode
    // defcolor_vim_bar_insert -- filebar color in insert mode
    // defcolor_vim_bar_visual -- filebar color in visual mode
    // defcolor_vim_bar_recording_macro -- filebar color when recording a macro
    // defcolor_vim_cursor_normal -- cursor color in normal mode
    // defcolor_vim_cursor_insert -- cursor color in insert mode
    // defcolor_vim_cursor_visual -- cursor color in visual mode
    // defcolor_vim_character_highlight -- color of character highlights from character seek
*/

// @TODO: Make window movements not come from an include
#include "clearfeld_windmove.cpp"

//
// User-Configuable Defines
//

#ifndef VIM_MOUSE_SELECT_MODE
#define VIM_MOUSE_SELECT_MODE Visual
#endif

#ifndef VIM_ESCAPE_SEQUENCE
#define VIM_ESCAPE_SEQUENCE "jk"
#endif

#ifndef VIM_AUTO_LINE_COMMENTS
#define VIM_AUTO_LINE_COMMENTS 1
#endif

#ifndef VIM_CASE_SENSITIVE_CHARACTER_SEEK
#define VIM_CASE_SENSITIVE_CHARACTER_SEEK 1
#endif

#ifndef VIM_AUTO_INDENT_ON_PASTE
#define VIM_AUTO_INDENT_ON_PASTE 1            // @TODO: Maybe just make this a separate command
#endif

#ifndef VIM_DEFAULT_REGISTER
#define VIM_DEFAULT_REGISTER unnamed_register
#endif

#ifndef VIM_JUMP_HISTORY_SIZE
#define VIM_JUMP_HISTORY_SIZE 100
#endif

#ifndef VIM_MAXIMUM_OP_COUNT
#define VIM_MAXIMUM_OP_COUNT 256
#endif

#ifndef VIM_USE_CUSTOM_COLORS
#define VIM_USE_CUSTOM_COLORS 0
#endif

#ifndef VIM_USE_CHARACTER_SEEK_HIGHLIGHTS
#define VIM_USE_CHARACTER_SEEK_HIGHLIGHTS 1
#endif

#ifndef VIM_FILE_BAR_ON_BOTTOM
#define VIM_FILE_BAR_ON_BOTTOM 0
#endif

#ifndef VIM_DRAW_PANEL_MARGINS
#define VIM_DRAW_PANEL_MARGINS 2 // 0: don't draw, 1: minimal vertical margins, 2: full margins
#endif

#ifndef VIM_PANEL_MARGIN_THICKNESS
#define VIM_PANEL_MARGIN_THICKNESS 3.0f
#endif

#ifndef VIM_USE_ECHO_BAR
#define VIM_USE_ECHO_BAR 0
#endif

#ifndef VIM_CURSOR_ROUNDNESS
#define VIM_CURSOR_ROUNDNESS 0.9f
#endif

//
//
//

// Important major changes:
// - Make command repetition work by storing the command and the required motion / selection information to execute it rather than using macros

// Things to do:
// - Option to disable mouse during insert mode (to avoid stupid palming of my touchpad)
// - Explore how to enable virtual whitespace with these vimmish things
// - Create a non-stopgap solution to case sensitive / whole word searches (tab during search query would be better suited to auto complete)
// - Do stuff with 0-9 registers (or yeet them because... does anybody use these?)
// - vim_align_range breaks when aligning characters at the end of lines, it seems.
// - Check for finnicky auto indent things (indenting one line more than expected, not indenting preprocessor defines properly)
// - Make the miblo number things handle negative numbers and booleans
// - Support marks as motions
// - Apparently, vim has some weird timing thing regarding conflicting binds. I've never noticed it, and it seems awful. But if you're weird, you could implement it.
// - Check out what's going on with text objects with a count > 1
// - Why does the word motion no longer stop on empty lines properly? Is it because of crlf line endings?
// - Review what should go into the view attachment versus the buffer attachment versus the global state.
// - Investigate: Next search gets caught on push_render_element in handmade_render_group.cpp
// - Persistent echo
// - Make ^N ^P style autocomplete with dropdown
// - Search under cursor / search selection is bork sometimes. Why?
// - Make my mind up about how to special case change ops that open a new line (cc, ci{, etc)
// - Dead keys (needs support from Allen)
// - Figure out how to change the size of the "global" region properly, so people can handle the echo bar.
// - Edit autoindent proper instead of using my hacky function
// - Figure out the right way to handle the view regions to make the echo bar and chin filebar not mess stuff up.

//
// Internal Defines
//

#define VIM_DYNAMIC_STRINGS_ARE_TOO_CLEVER_FOR_THEIR_OWN_GOOD 0 // NOTE: I don't really think this is good for my use, but it's here if you like things that complicate your life
#define VIM_PRINT_COMMANDS 1

#define VIM_DEFAULT_BINDING_MAP_SLOT_COUNT 32
#define VIM_DEFAULT_NESTED_BINDING_MAP_SLOT_COUNT 8

//
// Managed IDs
//

CUSTOM_ID(command_map, vim_mapid_normal);
CUSTOM_ID(command_map, vim_mapid_visual);
CUSTOM_ID(attachment, vim_view_attachment);
CUSTOM_ID(attachment, vim_buffer_insert_map_id);

//
// Custom colors
//

CUSTOM_ID(colors, defcolor_vim_bar_normal);
CUSTOM_ID(colors, defcolor_vim_bar_insert);
CUSTOM_ID(colors, defcolor_vim_bar_visual);
CUSTOM_ID(colors, defcolor_vim_bar_recording_macro);
CUSTOM_ID(colors, defcolor_vim_cursor_normal);
CUSTOM_ID(colors, defcolor_vim_cursor_insert);
CUSTOM_ID(colors, defcolor_vim_cursor_visual);
CUSTOM_ID(colors, defcolor_vim_character_highlight);

//
// Maps
//

typedef Table_u64_u64 Vim_Binding_Map; 
global Vim_Binding_Map vim_map_normal;
global Vim_Binding_Map vim_map_visual;
global Vim_Binding_Map vim_map_text_objects;
global Vim_Binding_Map vim_map_operator_pending;

//
//
//

#define cast(type) (type)

internal b32 vim_character_is_newline(char c) {
    return c == '\r' || c == '\n';
}

internal void printf_message(Application_Links* app, char* fmt, ...) {
    Scratch_Block scratch(app);
    va_list args;
    va_start(args, fmt);
    print_message(app, push_stringfv(scratch, fmt, args));
    va_end(args);
}

internal void vim_string_copy_dynamic(Base_Allocator* alloc, String_u8* dest, String_Const_u8 source) {
    u64 new_cap = 0;

#if VIM_DYNAMIC_STRINGS_ARE_TOO_CLEVER_FOR_THEIR_OWN_GOOD
    if (dest->cap < source.size || dest->cap > 4*source.size) {
        new_cap = 2*source.size;
    }
#else
    if (dest->cap < source.size) {
        new_cap = source.size;
    }
#endif

    if (new_cap) {
        new_cap = Max(256, new_cap);
        if (new_cap != dest->cap) {
            if (dest->str) {
                base_free(alloc, dest->str);
            }

            Data memory = base_allocate(alloc, new_cap);
            dest->str = cast(u8*) memory.data;
            dest->cap = memory.size;
        }
    }

    Assert(dest->cap >= source.size);

    dest->size = source.size;
    block_copy(dest->str, source.str, source.size);
}

internal void vim_string_append_dynamic(Base_Allocator* alloc, String_u8* dest, String_Const_u8 source) {
    u64 result_size = dest->size + source.size;

    u64 new_cap = 0;

#if VIM_DYNAMIC_STRINGS_ARE_TOO_CLEVER_FOR_THEIR_OWN_GOOD
    if (dest->cap < result_size || dest->cap > 4*result_size) {
        new_cap = 2*result_size;
    }
#else
    if (dest->cap < result_size) {
        new_cap = result_size;
    }
#endif

    if (new_cap) {
        new_cap = Max(256, new_cap);
        
        if (new_cap != dest->cap) {
            Data memory = base_allocate(alloc, new_cap);

            if (dest->str) {
                block_copy(memory.data, dest->str, dest->size);
                base_free(alloc, dest->str);
            }

            dest->str = cast(u8*) memory.data;
            dest->cap = memory.size;
        }
    }

    Assert(dest->cap >= result_size);

    u8* append_dest = dest->str + dest->size;

    dest->size = result_size;
    block_copy(append_dest, source.str, source.size);
}

internal void vim_string_free(Base_Allocator* alloc, String_u8* string) {
    if (string->str) {
        base_free(alloc, string->str);
    }
    block_zero_struct(string);
}

enum Vim_Mode {
    VimMode_Normal,
    VimMode_Insert,
    VimMode_VisualInsert,
    VimMode_Visual,
    VimMode_VisualLine,
    VimMode_VisualBlock,
};

internal b32 is_vim_insert_mode(Vim_Mode mode) {
    b32 result = (mode == VimMode_Insert) ||
                 (mode == VimMode_VisualInsert);
    return result;
}

internal b32 is_vim_visual_mode(Vim_Mode mode) {
    b32 result = ((mode == VimMode_Visual)     ||
                  (mode == VimMode_VisualLine) ||
                  (mode == VimMode_VisualBlock));
    return result;
}

enum Vim_Selection_Kind {
    VimSelectionKind_None = 0,

    VimSelectionKind_Range,
    VimSelectionKind_Line,
    VimSelectionKind_Block,
};

struct Vim_Visual_Selection {
    Vim_Selection_Kind kind;
    Range_Cursor range;
};

enum VimRegisterFlag {
    VimRegisterFlag_FromBlockCopy    = 0x1,
    VimRegisterFlag_Append           = 0x2,
    VimRegisterFlag_ReadOnly         = 0x4,
};

struct Vim_Register {
    u32 flags;
    String_u8 string;
};

enum Vim_Search_Flag {
    VimSearchFlag_CaseSensitive = 0x1,
    VimSearchFlag_WholeWord     = 0x2,
};

global u32 vim_search_mode_cycle[] = {
    0,
    VimSearchFlag_CaseSensitive,
    VimSearchFlag_WholeWord,
    VimSearchFlag_CaseSensitive|VimSearchFlag_WholeWord,
};

global String_Const_u8 vim_search_mode_prompt_forward[] = {
    string_u8_litexpr("I-Search: "),
    string_u8_litexpr("I-Search (Case Sensitive): "),
    string_u8_litexpr("I-Search (Whole Word): "),
    string_u8_litexpr("I-Search (Case Sensitive, Whole Word): "),
};

global String_Const_u8 vim_search_mode_prompt_reverse[] = {
    string_u8_litexpr("Reverse-I-Search: "),
    string_u8_litexpr("Reverse-I-Search (Case Sensitive): "),
    string_u8_litexpr("Reverse-I-Search (Whole Word): "),
    string_u8_litexpr("Reverse-I-Search (Case Sensitive, Whole Word): "),
};

struct Vim_Insert_Node {
    Vim_Insert_Node* next;
    i64 rel_line;
    i64 rel_col;
    String_Const_u8 text_forward;
    String_Const_u8 text_backward;
};

struct Vim_Abbreviation {
    Vim_Abbreviation* next;
    String_Const_u8 match;
    String_Const_u8 replacement;
};

enum Vim_Range_Style {
    VimRangeStyle_Exclusive,
    VimRangeStyle_Inclusive,
    VimRangeStyle_Linewise,
};

enum Vim_Motion_Flag {
    VimMotionFlag_SetPreferredX             = 0x2,
    VimMotionFlag_IsJump                    = 0x4,
    VimMotionFlag_VisualBlockForceToLineEnd = 0x8,
    VimMotionFlag_AlwaysSeek                = 0x10,
    VimMotionFlag_LogJumpPostSeek           = 0x20,
    VimMotionFlag_IgnoreMotionCount         = 0x40,
    VimMotionFlag_InvalidMotion             = 0x80,
};

struct Vim_Motion_Result {
    Range_i64 range_; // @TODO: Can we remove this?
    i64 seek_pos;
    u32 flags;
    Vim_Range_Style style;
};

internal Vim_Motion_Result vim_motion(i64 pos) {
    Vim_Motion_Result result = {};
    result.seek_pos = pos;
    result.style = VimRangeStyle_Exclusive;
    return result;
}

internal Vim_Motion_Result vim_motion_inclusive(i64 pos) {
    Vim_Motion_Result result = {};
    result.seek_pos = pos;
    result.style = VimRangeStyle_Inclusive;
    return result;
}

internal Vim_Motion_Result vim_motion_linewise(i64 pos) {
    Vim_Motion_Result result = {};
    result.seek_pos = pos;
    result.style = VimRangeStyle_Linewise;
    return result;
}

enum Vim_Text_Object_Flag {
    VimTextObjectFlag_IgnoreMotionCount = 0x40,
};

struct Vim_Text_Object_Result {
    Range_i64 range;
    Vim_Range_Style style;
    u32 flags;
};

internal Vim_Text_Object_Result vim_text_object(i64 pos) {
    Vim_Text_Object_Result result = {};
    result.range = Ii64(pos, pos);
    result.style = VimRangeStyle_Exclusive;
    return result;
}

internal Vim_Text_Object_Result vim_text_object_inclusive(i64 pos) {
    Vim_Text_Object_Result result = {};
    result.range = Ii64(pos, pos);
    result.style = VimRangeStyle_Inclusive;
    return result;
}

internal Vim_Text_Object_Result vim_text_object_linewise(i64 pos) {
    Vim_Text_Object_Result result = {};
    result.range = Ii64(pos, pos);
    result.style = VimRangeStyle_Linewise;
    return result;
}

#define VIM_MOTION(name) Vim_Motion_Result name(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, i32 motion_count, b32 motion_count_was_set)
typedef VIM_MOTION(Vim_Motion);

#define VIM_TEXT_OBJECT(name) Vim_Text_Object_Result name(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, i32 motion_count, b32 motion_count_was_set)
typedef VIM_TEXT_OBJECT(Vim_Text_Object);

#define VIM_OPERATOR(name) void name(Application_Links* app, struct Vim_Operator_State* state, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, b32 count_was_set)
typedef VIM_OPERATOR(Vim_Operator);

enum Vim_Operator_Flag {
    VimOpFlag_QueryMotion     = 0x1,
    VimOpFlag_ChangeBehaviour = 0x2,
};

struct Vim_Operator_State {
    Application_Links* app;
    
    View_ID view;
    Buffer_ID buffer;
    
    i32 op_count;
    Vim_Operator* op;
    
    i32 motion_count;
    b32 motion_count_was_set;
    Vim_Motion* motion;
    Vim_Text_Object* text_object;
    
    Vim_Visual_Selection selection;
    Range_i64 total_range;
};

enum Vim_Command_Rep_Kind {
    VimCommandRep_None,
    VimCommandRep_Operator,
    VimCommandRep_4CoderCommand,
};

struct Vim_Command_Rep {
    Vim_Command_Rep_Kind kind;
    i32 count;
    b32 count_was_set;
    union {
        struct {
            i32 motion_count;
            b32 motion_count_was_set;
            Vim_Motion* motion;
            Vim_Text_Object* text_object;
            Vim_Visual_Selection selection;
            Vim_Register* reg;
            Vim_Operator* op;
        };
        Custom_Command_Function* fcoder_command;
    };
};

enum Vim_Modifier {
    VimModifier_Control = 0x1,
    VimModifier_Alt     = 0x2,
    VimModifier_Shift   = 0x4,
    VimModifier_Command = 0x8,
};

struct Vim_Key {
    u16 kc;
    u16 mods;
    u32 codepoint;
};

internal u64 vim_key_code_hash(Vim_Key key) {
    u64 result = 0;
    if (key.kc) {
        result = (cast(u64) key.kc) | ((cast(u64) key.mods) << 16);
    }
    return result;
}

internal u64 vim_codepoint_hash(Vim_Key key) {
    u64 result = 0;
    if (key.codepoint) {
        key.mods &= ~VimModifier_Shift; // NOTE: Because the shift modifier is already "baked" into the text
        result = ((cast(u64) key.codepoint) << 32) | ((cast(u64) key.mods) << 16);
    }
    return result;
}

struct Vim_Key_Sequence {
    i32 count;
    Vim_Key keys[8];
};

internal u16 key_code_to_vim_modifier(Key_Code mod) {
    switch (mod) {
        case KeyCode_Control: return VimModifier_Control;
        case KeyCode_Alt:     return VimModifier_Alt;
        case KeyCode_Shift:   return VimModifier_Shift;
        case KeyCode_Command: return VimModifier_Command;
    }
    return 0;
}

internal Key_Code vim_modifier_to_key_code(u16 mod) {
    switch (mod) {
        case VimModifier_Control: return KeyCode_Control;
        case VimModifier_Alt:     return KeyCode_Alt;
        case VimModifier_Shift:   return KeyCode_Shift;
        case VimModifier_Command: return KeyCode_Command;
    }
    return 0;
}

internal Input_Modifier_Set_Fixed vim_modifiers_to_input_modifier_set_fixed(u16 mods) {
    Input_Modifier_Set_Fixed result = {};
    i32 shift_count = 0;
    while (mods) {
        Key_Code kc = vim_modifier_to_key_code((mods & 1) << shift_count);
        if (kc) {
            result.mods[result.count++] = kc;
        }
        mods = mods >> 1;
        shift_count++;
    }
    return result;
}

internal u16 input_modifier_set_to_vim_modifiers_internal(i32 count, Key_Code* mods) {
    u16 result = 0;
    for (i32 mod_index = 0; mod_index < count; mod_index++) {
        result |= key_code_to_vim_modifier(mods[mod_index]);
    }
    return result;
}

internal u16 input_modifier_set_to_vim_modifiers(Input_Modifier_Set mods) {
    u16 result = input_modifier_set_to_vim_modifiers_internal(mods.count, mods.mods);
    return result;
}

internal u16 input_modifier_set_fixed_to_vim_modifiers(Input_Modifier_Set_Fixed mods) {
    u16 result = input_modifier_set_to_vim_modifiers_internal(mods.count, mods.mods);
    return result;
}

internal Vim_Key vim_key(
    Key_Code kc,
    Key_Code mod1 = 0,
    Key_Code mod2 = 0,
    Key_Code mod3 = 0,
    Key_Code mod4 = 0,
    Key_Code mod5 = 0,
    Key_Code mod6 = 0,
    Key_Code mod7 = 0,
    Key_Code mod8 = 0
) {
    Input_Modifier_Set_Fixed mods = {};
    if (mod1) mods.mods[mods.count++] = mod1;
    if (mod2) mods.mods[mods.count++] = mod2;
    if (mod3) mods.mods[mods.count++] = mod3;
    if (mod4) mods.mods[mods.count++] = mod4;
    if (mod5) mods.mods[mods.count++] = mod5;
    if (mod6) mods.mods[mods.count++] = mod6;
    if (mod7) mods.mods[mods.count++] = mod7;
    if (mod8) mods.mods[mods.count++] = mod8;

    Vim_Key result = {};
    result.kc = cast(u16) kc;
    result.mods = input_modifier_set_fixed_to_vim_modifiers(mods);
    
    return result;
}

internal Vim_Key vim_char(
    u32 codepoint,
    Key_Code mod1 = 0,
    Key_Code mod2 = 0,
    Key_Code mod3 = 0,
    Key_Code mod4 = 0,
    Key_Code mod5 = 0,
    Key_Code mod6 = 0,
    Key_Code mod7 = 0,
    Key_Code mod8 = 0
) {
    Input_Modifier_Set_Fixed mods = {};
    if (mod1) { Assert(mod1 != KeyCode_Shift); mods.mods[mods.count++] = mod1; }
    if (mod2) { Assert(mod2 != KeyCode_Shift); mods.mods[mods.count++] = mod2; }
    if (mod3) { Assert(mod3 != KeyCode_Shift); mods.mods[mods.count++] = mod3; }
    if (mod4) { Assert(mod4 != KeyCode_Shift); mods.mods[mods.count++] = mod4; }
    if (mod5) { Assert(mod5 != KeyCode_Shift); mods.mods[mods.count++] = mod5; }
    if (mod6) { Assert(mod6 != KeyCode_Shift); mods.mods[mods.count++] = mod6; }
    if (mod7) { Assert(mod7 != KeyCode_Shift); mods.mods[mods.count++] = mod7; }
    if (mod8) { Assert(mod8 != KeyCode_Shift); mods.mods[mods.count++] = mod8; }

    Vim_Key result = {};
    result.codepoint = codepoint;
    result.mods = input_modifier_set_fixed_to_vim_modifiers(mods);
    return result;
}

internal Vim_Key_Sequence vim_key_sequence(
    Vim_Key key1,
    Vim_Key key2 = {},
    Vim_Key key3 = {},
    Vim_Key key4 = {},
    Vim_Key key5 = {},
    Vim_Key key6 = {},
    Vim_Key key7 = {},
    Vim_Key key8 = {}
) {
    Vim_Key_Sequence result = {};
    result.keys[result.count++] = key1;
    if (key2.kc || key2.codepoint) result.keys[result.count++] = key2;
    if (key3.kc || key3.codepoint) result.keys[result.count++] = key3;
    if (key4.kc || key4.codepoint) result.keys[result.count++] = key4;
    if (key5.kc || key5.codepoint) result.keys[result.count++] = key5;
    if (key6.kc || key6.codepoint) result.keys[result.count++] = key6;
    if (key7.kc || key7.codepoint) result.keys[result.count++] = key7;
    if (key8.kc || key8.codepoint) result.keys[result.count++] = key8;
    return result;
}

inline b32 vim_append_key_to_sequence(Vim_Key_Sequence* seq, Vim_Key key) {
    b32 result = false;
    if (seq->count < ArrayCount(seq->keys)) {
        result = true;
        seq->keys[seq->count++] = key;
    }
    return result;
}

struct Vim_Global_State {
    Arena                arena;
    Heap                 heap;
    Base_Allocator       alloc;

    Vim_Mode             mode;

    Buffer_Cursor        insert_cursor;
    i32                  insert_history_index;
    b32                  insert_sequence;
    i64                  insert_sequence_pos;
    i64                  insert_sequence_start_pos;
    u8                   prev_insert_char;

    b32                  visual_block_force_to_line_end;
    b32                  right_justify_visual_insert;
    Range_i64            visual_insert_line_range;

    Vim_Insert_Node*     first_insert_node;
    Vim_Insert_Node*     last_insert_node;
    Vim_Insert_Node*     first_free_insert_node;

    Vim_Abbreviation*    first_abbreviation;

    b32                  current_key_is_retired;
    Vim_Key              current_key;
    Vim_Key_Sequence     current_key_sequence;

    union {
        Vim_Register alphanumeric_registers[26 + 10];
        struct {
            Vim_Register alpha_registers[26];   // "a-"z
            Vim_Register numeric_registers[10]; // "0-"9
        };
    };

    Vim_Register         unnamed_register;      // ""
    Vim_Register         clipboard_register;    // "+ (and "* for now)
    Vim_Register         last_search_register;  // "/
    Vim_Register*        active_register;

    union {
        Tiny_Jump all_marks[26 + 26];        
        struct {
            Tiny_Jump lower_marks[26]; // 'a-'z
            Tiny_Jump upper_marks[26]; // 'A-'Z
        };
    };

    u8                   most_recent_macro_register;
    b32                  recording_macro;
    b32                  played_macro;
    i64                  current_macro_start_pos;
    i64                  command_start_pos;
    History_Group        macro_history;
    
    b32                  executing_queried_motion;
    
    Vim_Command_Rep      next_command_rep;
    Vim_Command_Rep      command_rep;
    b32                  command_in_progress;
    b32                  playing_back_command;
    History_Group        command_history;

    b32                  search_show_highlight;
    u32                  search_flags;
    Scan_Direction       search_direction;
    u32                  search_mode_index;

    b32                  character_seek_show_highlight;
    Scan_Direction       character_seek_highlight_dir;
    u8                   most_recent_character_seek_storage[8];
    String_u8            most_recent_character_seek;
    Scan_Direction       most_recent_character_seek_dir;
    b32                  most_recent_character_seek_till;
    b32                  most_recent_character_seek_inclusive;

    b32                  capture_queries_into_chord_bar;
    u8                   chord_bar_storage[64];
    String_u8            chord_bar;

    i32                  definition_stack_count;
    i32                  definition_stack_cursor;
    Tiny_Jump            definition_stack[16];

    u8                   echo_string_storage[256];
    String_u8            echo_string;
    FColor               echo_color;
    f32                  persistent_echo_string_timeout;
    u8                   persistent_echo_string_storage[256];
    String_u8            persistent_echo_string;
    ARGB_Color           persistent_echo_color;
};

global Vim_Global_State vim_state;

internal Vim_Register* vim_get_register(u8 register_char) {
    Vim_Register* result = 0;

    b32 append = false;
    if (character_is_lower(register_char)) {
        result = vim_state.alpha_registers + (register_char - 'a');
    } else if (character_is_upper(register_char)) {
        result = vim_state.alpha_registers + (register_char - 'A');
        append = true;
    } else if (register_char >= '0' && register_char <= '9') {
        result = vim_state.numeric_registers + (register_char - '0');
    } else if (register_char == '*' || register_char == '+') {
        // @TODO: I don't know if on Linux it's worth it to make a distinction between "* and "+
        result = &vim_state.clipboard_register;
    } else if (register_char == '/') {
        result = &vim_state.last_search_register;
    } else if (register_char == '"') {
        result = &vim_state.unnamed_register;
    }

    if (result) {
        if (append) result->flags |=  VimRegisterFlag_Append;
        else        result->flags &= ~VimRegisterFlag_Append;
    }

    return result;
}

internal b32 vim_write_register(Application_Links* app, Vim_Register* reg, String_Const_u8 string, b32 from_block_copy = false) {
    b32 result = true;

    if (reg) {
        if (HasFlag(reg->flags, VimRegisterFlag_ReadOnly)) {
            result = false;
        } else {
            if (from_block_copy) {
                reg->flags |= VimRegisterFlag_FromBlockCopy;
            } else {
                reg->flags &= ~VimRegisterFlag_FromBlockCopy;
            }

            if (HasFlag(reg->flags, VimRegisterFlag_Append)) {
                vim_string_append_dynamic(&vim_state.alloc, &reg->string, string);
            } else {
                vim_string_copy_dynamic(&vim_state.alloc, &reg->string, string);
            }
            if (reg == &vim_state.clipboard_register) {
                reg->flags &= ~VimRegisterFlag_FromBlockCopy;
                clipboard_post(0, reg->string.string);
            }
            if (reg == &vim_state.last_search_register) {
                vim_state.search_show_highlight = true;
            }
        }
    } else {
        result = false;
    }

    return result;
}

internal String_Const_u8 vim_read_register(Application_Links* app, Vim_Register* reg) {
    String_Const_u8 result = SCu8();

    if (reg) {
        if (reg == &vim_state.clipboard_register) {
            Scratch_Block scratch(app);
            clipboard_update_history_from_system(app, 0);
            String_Const_u8 clipboard = push_clipboard_index(scratch, 0, 0);
            vim_string_copy_dynamic(&vim_state.alloc, &reg->string, clipboard);
        }

        result = reg->string.string;
    }

    return result;
}

internal void vim_echo_varargs(Application_Links* app, char* fmt, va_list args) {
    Scratch_Block scratch(app);

    String_Const_u8 formatted = push_u8_stringfv(scratch, fmt, args);

    vim_state.echo_string.size = 0;
    string_append(&vim_state.echo_string, formatted);
}

internal void vim_echo(Application_Links* app, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vim_state.echo_color = fcolor_id(defcolor_text_default);
    vim_echo_varargs(app, fmt, args);
    va_end(args);
}

internal void vim_echo_alert(Application_Links* app, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vim_state.echo_color = fcolor_id(defcolor_pop1);
    vim_echo_varargs(app, fmt, args);
    va_end(args);
}

internal void vim_echo_error(Application_Links* app, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vim_state.echo_color = fcolor_id(defcolor_pop2);
    vim_echo_varargs(app, fmt, args);
    va_end(args);
}

struct Vim_View_Attachment {
    Buffer_ID most_recent_known_buffer;
    i64       most_recent_known_pos;

    Buffer_ID previous_buffer;
    i64       pos_in_previous_buffer;

    b32       dont_log_this_buffer_jump;

    i32       jump_history_first;
    i32       jump_history_one_past_last;
    i32       jump_history_cursor;
    Tiny_Jump jump_history[VIM_JUMP_HISTORY_SIZE];
};

internal void vim_delete_jump_history_at_index(Application_Links* app, Vim_View_Attachment* vim_view, i32 jump_index) {
    if (jump_index >= vim_view->jump_history_first && jump_index < vim_view->jump_history_one_past_last) {
        i32 jumps_left = vim_view->jump_history_one_past_last - jump_index;

        vim_view->jump_history_one_past_last--;
        vim_view->jump_history_cursor = Min(vim_view->jump_history_cursor, vim_view->jump_history_one_past_last);

        if (jumps_left > 1) {
            // @Speed
            for (i32 shift = 0; shift < jumps_left - 1; shift++) {
                i32 shift_index = jump_index + shift;
                ArraySafe(vim_view->jump_history, shift_index) = ArraySafe(vim_view->jump_history, shift_index + 1);
            }
        }
    }
}

internal void vim_delete_jump_history_at_index(Application_Links* app, i32 jump_index) {
    View_ID view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);
    vim_delete_jump_history_at_index(app, vim_view, jump_index);
}

internal void vim_log_jump_history_internal(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_View_Attachment* vim_view, i64 pos) {
    ProfileScope(app, "[vim] log jump history");
    i64 line = get_line_number_from_pos(app, buffer, pos);

    Scratch_Block scratch(app);
    for (i32 jump_index = vim_view->jump_history_first; jump_index < vim_view->jump_history_one_past_last; jump_index++) {
        Tiny_Jump test_jump = ArraySafe(vim_view->jump_history, jump_index);
        if (test_jump.buffer == buffer) {
            i64 test_line = get_line_number_from_pos(app, buffer, test_jump.pos); // @Speed
            if (test_line == line) {
                vim_delete_jump_history_at_index(app, vim_view, jump_index);
                break;
            }
        }
    }

    Tiny_Jump jump;
    jump.buffer = buffer;
    jump.pos = pos;

    ArraySafe(vim_view->jump_history, vim_view->jump_history_cursor++) = jump;

    vim_view->jump_history_one_past_last = vim_view->jump_history_cursor;
    i32 unsafe_index = vim_view->jump_history_one_past_last - ArrayCount(vim_view->jump_history);
    vim_view->jump_history_first = Max(0, Max(vim_view->jump_history_first, unsafe_index));
}

internal void vim_log_jump_history(Application_Links* app) {
    View_ID view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);

    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 pos = view_get_cursor_pos(app, view);

    vim_log_jump_history_internal(app, view, buffer, vim_view, pos);
}

CUSTOM_COMMAND_SIG(vim_step_back_jump_history) {
    View_ID view = get_active_view(app, Access_Always);
    // Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);

    if (vim_view->jump_history_cursor > vim_view->jump_history_first) {
        if (vim_view->jump_history_cursor == vim_view->jump_history_one_past_last) {
            i32 pre_jump_cursor = vim_view->jump_history_cursor;
            vim_log_jump_history(app);
            vim_view->jump_history_cursor = pre_jump_cursor;
        }

        Tiny_Jump jump = ArraySafe(vim_view->jump_history, --vim_view->jump_history_cursor);
        jump_to_location(app, view, jump.buffer, jump.pos);

        vim_view->dont_log_this_buffer_jump = true;
    }
}

CUSTOM_COMMAND_SIG(vim_step_forward_jump_history) {
    View_ID view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);

    if (vim_view->jump_history_cursor + 1 < vim_view->jump_history_one_past_last) {
        Tiny_Jump jump = ArraySafe(vim_view->jump_history, ++vim_view->jump_history_cursor);
        jump_to_location(app, view, jump.buffer, jump.pos);

        vim_view->dont_log_this_buffer_jump = true;
    }
}

internal Vim_Key vim_get_key_from_input(User_Input in) {
    Vim_Key key = {};
    if (in.event.kind == InputEventKind_KeyStroke) {
        key.kc = cast(u16) in.event.key.code;
        key.mods = input_modifier_set_to_vim_modifiers(in.event.key.modifiers);
    } else if (in.event.kind == InputEventKind_TextInsert) {
        String_Const_u8 writable = to_writable(&in);
        if (writable.size) {
            key.codepoint = utf8_consume(writable.str, writable.size).codepoint;
        }
    }
    return key;
}

enum Vim_Binding_Kind {
    VimBindingKind_None,

    VimBindingKind_Map,
    VimBindingKind_Motion,
    VimBindingKind_TextObject,
    VimBindingKind_Operator,
    VimBindingKind_4CoderCommand,
};

enum Vim_Binding_Flag {
    VimBindingFlag_IsRepeatable = 0x1,
    VimBindingFlag_WriteOnly    = 0x2,
    VimBindingFlag_TextCommand  = VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly,
};

struct Vim_Key_Binding {
    Vim_Binding_Kind kind;
    String_Const_u8  description;
    u32              flags;
    union {
        void*                    generic;
        Vim_Binding_Map*         map;
        Vim_Motion*              motion;
        Vim_Text_Object*         text_object;
        Vim_Operator*            op;
        Custom_Command_Function* fcoder_command;
    };
};

internal Vim_Binding_Map vim_make_binding_map(u32 slot_count) {
    Vim_Binding_Map result = make_table_u64_u64(&vim_state.alloc, slot_count);
    return result;
}

internal Vim_Binding_Map* vim_new_binding_map(u32 slot_count) {
    Vim_Binding_Map* result = push_array(&vim_state.arena, Vim_Binding_Map, 1);
    *result = vim_make_binding_map(slot_count);
    return result;
}

internal void vim_add_parent_binding_map(Vim_Binding_Map* dest, Vim_Binding_Map* source) {
    for (u32 slot_index = 0; slot_index < source->slot_count; slot_index++) {
        u64 key   = source->keys[slot_index];
        u64 value = source->vals[slot_index];
        if (value) {
            Vim_Key_Binding* source_bind = cast(Vim_Key_Binding*) IntAsPtr(value);
            Vim_Key_Binding* dest_bind = 0;
            if (!table_read(dest, key, cast(u64*) &dest_bind)) {
                dest_bind = push_array_zero(&vim_state.arena, Vim_Key_Binding, 1);
            }

            if (source_bind->kind == VimBindingKind_Map) {
                if (dest_bind->kind != VimBindingKind_Map || !dest_bind->map) {
                    dest_bind->map = vim_new_binding_map(VIM_DEFAULT_NESTED_BINDING_MAP_SLOT_COUNT);
                }
                vim_add_parent_binding_map(dest_bind->map, source_bind->map);
            } else {
                dest_bind->generic = source_bind->generic;
            }

            dest_bind->kind        = source_bind->kind;
            dest_bind->description = source_bind->description;
            dest_bind->flags       = source_bind->flags;

            table_insert(dest, key, PtrAsInt(dest_bind));
        }
    }
}

internal Vim_Key_Binding* vim_make_or_retrieve_binding_internal(Vim_Binding_Map* map, Vim_Key key, b32 make_if_doesnt_exist) {
    Assert(map);

    Vim_Key_Binding* result = 0;

    u64 hash_hi = vim_codepoint_hash(key);
    u64 hash_lo = vim_key_code_hash(key);

    {
        u64 location = 0;
        if (table_read(map, hash_hi, &location)) {
            result = cast(Vim_Key_Binding*) IntAsPtr(location);
        }
    }

    if (!result) {
        u64 location = 0;
        if (table_read(map, hash_lo, &location)) {
            result = cast(Vim_Key_Binding*) IntAsPtr(location);
        }
    }

    if (!result && make_if_doesnt_exist) {
        if (hash_hi) {
            result = push_array_zero(&vim_state.arena, Vim_Key_Binding, 1);
            table_insert(map, hash_hi, PtrAsInt(result));
        }
        if (hash_lo) {
            result = push_array_zero(&vim_state.arena, Vim_Key_Binding, 1);
            table_insert(map, hash_lo, PtrAsInt(result));
        }
    }

    return result;
}

internal Vim_Key_Binding* vim_retrieve_binding(Vim_Binding_Map* map, Vim_Key key) {
    return vim_make_or_retrieve_binding_internal(map, key, false);
}

internal Vim_Key_Binding* vim_make_or_retrieve_binding(Vim_Binding_Map* map, Vim_Key key) {
    return vim_make_or_retrieve_binding_internal(map, key, true);
}

internal void vim_append_chord_bar(String_Const_u8 string, u32 vim_mods) {
    String_Const_u8 append_string = string;
    if (string_match(string, string_u8_litexpr(" "))) {
        append_string = string_u8_litexpr("<Space>");
    }
    if (HasFlag(vim_mods, VimModifier_Alt)) {
        string_append(&vim_state.chord_bar, string_u8_litexpr("M-"));
    }
    if (HasFlag(vim_mods, VimModifier_Control)) {
        string_append_character(&vim_state.chord_bar, '^');
    }
    string_append(&vim_state.chord_bar, append_string);
}

internal User_Input vim_get_next_keystroke(Application_Links* app) {
    User_Input in = {};
    do {
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
        if (in.abort) break;
    } while (key_code_to_vim_modifier(in.event.key.code));
    return in;
}

internal String_Const_u8 vim_get_next_writable(Application_Links* app) {
    User_Input in = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
    String_Const_u8 result = to_writable(&in);
    if (vim_state.capture_queries_into_chord_bar) {
        vim_append_chord_bar(result, 0);
    }
    return result;
}

enum Vim_Query_Mode {
    VimQuery_CurrentInput,
    VimQuery_NextInput,
};

internal Vim_Key vim_get_key_from_input_query(Application_Links* app, Vim_Query_Mode mode) {
    Vim_Key result = {};

    User_Input in = {};
    if (mode == VimQuery_CurrentInput) {
        in = get_current_input(app);
    } else if (mode == VimQuery_NextInput) {
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
    } else {
        InvalidPath;
    }

    if (in.abort) {
        return result;
    }

    if (in.event.kind == InputEventKind_KeyStroke) {
        while (key_code_to_vim_modifier(in.event.key.code)) {
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
            if (in.abort) {
                return result;
            }
        }

        result.kc = cast(u16) in.event.key.code;
        result.mods = input_modifier_set_to_vim_modifiers(in.event.key.modifiers);

        if (in.event.key.first_dependent_text) {
            leave_current_input_unhandled(app);
            in = get_next_input(app, EventProperty_TextInsert, EventProperty_ViewActivation|EventProperty_Exit);
        }
    }

    if (!in.abort && in.event.kind == InputEventKind_TextInsert) {
        String_Const_u8 writable = to_writable(&in);

        if (vim_state.capture_queries_into_chord_bar) {
            vim_append_chord_bar(writable, result.mods);
        }

        u32 codepoint = 0;
        if (writable.size) {
            codepoint = utf8_consume(writable.str, writable.size).codepoint;
        }

        result.codepoint = codepoint;
    }

    return result;
}

internal Vim_Key vim_get_key_from_current_input(Application_Links* app) {
    return vim_get_key_from_input_query(app, VimQuery_CurrentInput);
}

inline Vim_Key vim_get_current_key(Application_Links* app) {
    if (vim_state.current_key_is_retired) {
        vim_state.current_key_is_retired = false;
        vim_state.current_key = vim_get_key_from_input_query(app, VimQuery_NextInput);
    }
    return vim_state.current_key;
}

inline void vim_retire_key() {
    if (!vim_state.current_key_is_retired) {
        vim_state.current_key_is_retired = true;
        vim_append_key_to_sequence(&vim_state.current_key_sequence, vim_state.current_key);
    } else {
        InvalidPath;
    }
}

inline Vim_Key vim_get_next_key(Application_Links* app) {
    vim_retire_key();
    return vim_get_current_key(app);
}

inline void vim_begin_query(Application_Links* app, Vim_Query_Mode mode) {
    vim_state.capture_queries_into_chord_bar = true;
    vim_state.current_key_sequence.count = 0;
    vim_state.current_key_is_retired = false;
    vim_state.current_key = vim_get_key_from_input_query(app, mode);
}

inline void vim_end_query() {
    vim_state.capture_queries_into_chord_bar = false;
    vim_state.chord_bar.size = 0;
}

internal void vim_print_bind(Application_Links* app, Vim_Key_Binding* bind) {
    if (bind && bind->description.size) {
        if (get_current_input_is_virtual(app)) {
            print_message(app, string_u8_litexpr("    "));
        }
        print_message(app, bind->description);
        if (bind->kind == VimBindingKind_Map) {
            print_message(app, string_u8_litexpr(" -> "));
        } else {
            print_message(app, string_u8_litexpr("\n"));
        }
    }
}

//
// NOTE: vim_query functions all look at the _current_ input via vim_get_current_key(), and they always consume keys they parse successfully.
//

internal i64 vim_query_number(Application_Links* app, b32 handle_sign = false) {
    i64 result = 0;
    Vim_Key key = vim_get_current_key(app);

    if (!key.codepoint) {
        return result;
    }

    i64 sign = 1;
    if (handle_sign && key.codepoint == '-') {
        sign = -1;
        key = vim_get_next_key(app);
    }

    if (!key.codepoint) {
        return result;
    }

    if (key.codepoint != '0') {
        for (;;) {
            u32 digit = key.codepoint - '0';
            if (digit >= 0 && digit <= 9) {
                // @TODO: Check if these cases are actually correct
                if (result > (max_i64 / 10 - digit)) {
                    result = max_i64;
                } else if (result < (min_i64 / 10 + digit)) {
                    result = min_i64;
                } else {
                    result = 10*result + sign*cast(i32) digit;
                }
                key = vim_get_next_key(app); // @ConsumeKeyOnSuccess
            } else {
                break;
            }
        }
    }

    return result;
}

inline b32 vim_keys_are_equal(Vim_Key a, Vim_Key b) {
    b32 result = (a.kc == b.kc && a.mods == b.mods && a.codepoint == b.codepoint);
    return result;
}

// @Cleanup: Rearrage to make the forward declare unnecessary. Maybe.
internal VIM_TEXT_OBJECT(vim_text_object_line);

// @TODO: Is this how we want to do this?
Vim_Key_Binding vim_text_object_line_bind = {
    VimBindingKind_TextObject,
    string_u8_litexpr("vim_text_object_line (from key repeat)"),
    0,
    vim_text_object_line
};

internal Vim_Key_Binding* vim_query_binding(Application_Links* app, Vim_Binding_Map* map, b32 line_object_on_repeat_input) {
    Vim_Key_Sequence prev_seq = vim_state.current_key_sequence; // @Speed: Copies a 68 byte struct. If that matters, at all.
    Vim_Key key = vim_get_current_key(app);
    Vim_Key previous_final_key = prev_seq.keys[prev_seq.count - 1];
    if (line_object_on_repeat_input && (prev_seq.count > 0) && vim_keys_are_equal(key, previous_final_key)) {
        vim_retire_key(); // @ConsumeKeyOnSuccess
#if VIM_PRINT_COMMANDS
        vim_print_bind(app, &vim_text_object_line_bind);
#endif
        return &vim_text_object_line_bind;
    }

    Vim_Key_Binding* bind = vim_retrieve_binding(map, key);

#if VIM_PRINT_COMMANDS
    vim_print_bind(app, bind);
#endif

    i32 prev_seq_index = 0;
    b32 matches_previous_sequence = (line_object_on_repeat_input && (prev_seq.count > 0) && vim_keys_are_equal(key, prev_seq.keys[prev_seq_index++]));
    while (bind && bind->kind == VimBindingKind_Map) {
        key = vim_get_next_key(app);

        if (matches_previous_sequence) {
            matches_previous_sequence = vim_keys_are_equal(key, prev_seq.keys[prev_seq_index++]);
            if (matches_previous_sequence && (prev_seq_index == prev_seq.count)) {
#if VIM_PRINT_COMMANDS
                vim_print_bind(app, &vim_text_object_line_bind);
#endif
                return &vim_text_object_line_bind;
            }
        }

        bind = vim_retrieve_binding(bind->map, key);

        if (!bind) {
            break;
        }

#if VIM_PRINT_COMMANDS
        vim_print_bind(app, bind);
#endif
    }

    if (bind) {
        vim_retire_key(); // @ConsumeKeyOnSuccess
    }

    return bind;
}

CUSTOM_COMMAND_SIG(vim_register) {
    // NOTE: Stub... Because picking a register for a command is a special kind
    // of operation, it is implemented outside the regular binding system. You
    // can still pick which key is used to denote a register selection by
    // binding this command, but it must be a single key binding.
}

internal Vim_Register* vim_query_and_set_register(Application_Links* app, Vim_Binding_Map* map) {
    Vim_Register* result = 0;
    Vim_Key key = vim_get_current_key(app);
    Vim_Key_Binding* bind = vim_retrieve_binding(map, key);
    if (bind && bind->fcoder_command == vim_register) {
        key = vim_get_next_key(app);
        result = vim_get_register(cast(u8) key.codepoint);
        if (result) {
            vim_state.active_register = result;
            vim_retire_key(); // @ConsumeKeyOnSuccess
#if VIM_PRINT_COMMANDS
            Scratch_Block scratch(app);
            print_message(app, push_u8_stringf(scratch, "Active Register: \"%c\n", cast(u8) key.codepoint));
#endif
        }
    }
    return result;
}

internal void vim_begin_macro(Application_Links* app, u8 reg_char) {
    if (vim_state.recording_macro) {
        return;
    }

    Vim_Register* reg = vim_get_register(reg_char);
    b32 reg_is_alphanumeric = (reg >= vim_state.alphanumeric_registers) &&
                              (reg < vim_state.alphanumeric_registers + ArrayCount(vim_state.alphanumeric_registers));
    if (reg && reg_is_alphanumeric) {
        vim_state.recording_macro = true;
        vim_state.most_recent_macro_register = reg_char;

        Buffer_ID buffer = get_keyboard_log_buffer(app);
        Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));
        vim_state.current_macro_start_pos = cursor.pos;
    }
}

internal void vim_end_macro(Application_Links* app, u8 reg_char) {
    if (!vim_state.recording_macro) {
        return;
    }

    Vim_Register* reg = vim_get_register(reg_char);
    b32 reg_is_alphanumeric = (reg >= vim_state.alphanumeric_registers) &&
                              (reg < vim_state.alphanumeric_registers + ArrayCount(vim_state.alphanumeric_registers));
    if (reg && reg_is_alphanumeric) {
        vim_state.recording_macro = false;
        vim_state.most_recent_macro_register = reg_char;

        Buffer_ID buffer = get_keyboard_log_buffer(app);

        i64 end_macro_pos = vim_state.command_start_pos;
        Range_i64 range = Ii64(vim_state.current_macro_start_pos, end_macro_pos);

        Scratch_Block scratch(app);
        String_Const_u8 macro_string = push_buffer_range(app, scratch, buffer, range);

        vim_write_register(app, reg, macro_string);
    }
}

CUSTOM_COMMAND_SIG(vim_record_macro) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) return;

    if (vim_state.recording_macro && vim_state.most_recent_macro_register) {
        vim_end_macro(app, vim_state.most_recent_macro_register);
    } else {
        String_Const_u8 reg_str = vim_get_next_writable(app);
        if (reg_str.size) {
            char reg_char = reg_str.str[0];
            // @TODO: The key release associated with the writable gets caught in the macro. That's not causing problems, but it is a little sloppy.
            vim_begin_macro(app, reg_char);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_replay_macro) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    String_Const_u8 reg_str = vim_get_next_writable(app);
    if (reg_str.size) {
        char reg_char = reg_str.str[0];
        if (reg_char == '@') {
            reg_char = vim_state.most_recent_macro_register;
        }

        Vim_Register* reg = vim_get_register(reg_char);
        if (reg) {
            String_Const_u8 macro_string = vim_read_register(app, reg);
            keyboard_macro_play(app, macro_string);

            vim_state.most_recent_macro_register = reg_char;
            vim_state.played_macro = true;
            vim_state.macro_history = history_group_begin(app, buffer);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_view_move_line_to_top)
CUSTOM_DOC("[vim] move current line to the top of the view") {
    View_ID view = get_active_view(app, Access_ReadVisible);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.line_number = cursor.line;
    scroll.target.pixel_shift.y = 0.f;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

CUSTOM_COMMAND_SIG(vim_view_move_line_to_bottom)
CUSTOM_DOC("[vim] move current line to the bottom of the view") {
    View_ID view = get_active_view(app, Access_ReadVisible);

    Rect_f32 region = view_get_buffer_region(app, view);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.line_number = cursor.line - 1;
    scroll.target.pixel_shift.y = -region.p1.y;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

CUSTOM_COMMAND_SIG(vim_set_mark) {
    // @TODO: From :help marks
    // "Lowercase marks are restored when using undo and redo."
    // What exactly does this mean, and what do I need to do to support it?
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) return;

    String_Const_u8 str = vim_get_next_writable(app);
    if (str.size) {
        u8 mark_char = str.str[0];
        Tiny_Jump* mark = 0;
        if (character_is_lower(mark_char)) {
            mark = vim_state.lower_marks + (mark_char - 'a');
        } else if (character_is_upper(mark_char)) {
            mark = vim_state.upper_marks + (mark_char - 'A');
        }
        if (mark) {
            mark->buffer = buffer;
            mark->pos = view_get_cursor_pos(app, view);
        } else {
            vim_echo_error(app, "Unknown mark '%c'", mark_char);
        }
    }
}

internal void vim_go_to_mark_internal(Application_Links* app, b32 reduced_jump_history) {
    // @TODO: Support this being a motion
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) return;

    String_Const_u8 str = vim_get_next_writable(app);
    if (str.size) {
        u8 mark_char = str.str[0];
        Tiny_Jump* mark = 0;
        b32 upper_mark = false;
        if (character_is_lower(mark_char)) {
            mark = vim_state.lower_marks + (mark_char - 'a');
        } else if (character_is_upper(mark_char)) {
            upper_mark = true;
            mark = vim_state.upper_marks + (mark_char - 'A');
        }
        if (mark) {
            if (upper_mark || mark->buffer == buffer) {
                if (!reduced_jump_history || mark->buffer == buffer) {
                    vim_log_jump_history(app);
                }
                jump_to_location(app, view, mark->buffer, mark->pos);
            }
        } else {
            vim_echo_error(app, "Unknown mark '%c'", mark_char);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_go_to_mark) {
    vim_go_to_mark_internal(app, false);
}

CUSTOM_COMMAND_SIG(vim_go_to_mark_less_history) {
    vim_go_to_mark_internal(app, true);
}

internal Vim_Visual_Selection vim_get_selection(Application_Links* app, View_ID view, Buffer_ID buffer) {
    Vim_Visual_Selection selection = {};
    selection.range.first         = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
    selection.range.one_past_last = buffer_compute_cursor(app, buffer, seek_pos(view_get_mark_pos(app, view)));
    if (selection.range.first.pos > selection.range.one_past_last.pos) {
        Swap(Buffer_Cursor, selection.range.first, selection.range.one_past_last);
    }

    switch (vim_state.mode) {
        case VimMode_Visual: {
            selection.kind = VimSelectionKind_Range;
        } break;

        case VimMode_VisualLine: {
            selection.kind = VimSelectionKind_Line;
            selection.range.first.col = 0;
            selection.range.one_past_last.col = -1;
        } break;

        case VimMode_VisualBlock: {
            selection.kind = VimSelectionKind_Block;
            if (selection.range.first.line > selection.range.one_past_last.line) {
                Swap(i64, selection.range.first.line, selection.range.one_past_last.line);
            }
            if (selection.range.first.col > selection.range.one_past_last.col) {
                Swap(i64, selection.range.first.col, selection.range.one_past_last.col);
            }
            selection.range.first = buffer_compute_cursor(app, buffer, seek_line_col(selection.range.first.line, selection.range.first.col));
            selection.range.one_past_last = buffer_compute_cursor(app, buffer, seek_line_col(selection.range.one_past_last.line, selection.range.one_past_last.col));
            if (vim_state.visual_block_force_to_line_end) {
                selection.range.one_past_last.col = -1;
            }
        } break;
    }

    selection.range.one_past_last.line++;
    selection.range.one_past_last.col++;
    selection.range.one_past_last.pos++;

    return selection;
}

internal b32 vim_selection_consume_line(Application_Links* app, Buffer_ID buffer, Vim_Visual_Selection* selection, Range_i64* out_range, b32 inclusive) {
    b32 result = false;

    if (selection->kind == VimSelectionKind_Block) {
        if (selection->range.first.line < selection->range.one_past_last.line) {
            *out_range = Ii64(buffer_compute_cursor(app, buffer, seek_line_col(selection->range.first.line, selection->range.first.col)).pos,
                              buffer_compute_cursor(app, buffer, seek_line_col(selection->range.first.line, selection->range.one_past_last.col + (inclusive ? 0 : -1))).pos);
            selection->range.first.line++;
            result = true;
        }
    } else if (selection->kind == VimSelectionKind_Line && inclusive) {
        if (!(selection->range.first.line == selection->range.one_past_last.line && selection->range.first.col == selection->range.one_past_last.col)) {
            Buffer_Cursor min_cursor = buffer_compute_cursor(app, buffer, seek_line_col(selection->range.first.line, selection->range.first.col));
            Buffer_Cursor max_cursor = buffer_compute_cursor(app, buffer, seek_line_col(selection->range.one_past_last.line, 1));
            *out_range = Ii64(min_cursor.pos, max_cursor.pos);
            selection->range.first = selection->range.one_past_last;
            result = true;
        }
    } else {
        if (!(selection->range.first.line == selection->range.one_past_last.line && selection->range.first.col == selection->range.one_past_last.col)) {
            Buffer_Cursor min_cursor = buffer_compute_cursor(app, buffer, seek_line_col(selection->range.first.line, selection->range.first.col));
            Buffer_Cursor max_cursor = buffer_compute_cursor(app, buffer, seek_line_col(selection->range.one_past_last.line - 1, selection->range.one_past_last.col + (inclusive ? 0 : -1)));
            *out_range = Ii64(min_cursor.pos, max_cursor.pos);
            selection->range.first = selection->range.one_past_last;
            result = true;
        }
    }

    return result;
}

internal void vim_rel_move(Application_Links* app, Buffer_Seek seek) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    if (seek.type == buffer_seek_pos) {
        pos = view_compute_cursor(app, view, seek_pos(cursor.pos + seek.pos)).pos;
    } else {
        Assert(seek.type == buffer_seek_line_col);
        pos = view_compute_cursor(app, view, seek_line_col(cursor.line + seek.line, cursor.col + seek.col)).pos;
    }
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

// @TODO: Replace this random global buffer?
global u32 vim_insert_node_buffer_used;
global u8 vim_insert_node_buffer[KB(2)];

internal Vim_Insert_Node* vim_add_insert_node_from_record(Application_Links* app, Buffer_ID buffer, Record_Info record, Buffer_Cursor insert_cursor) {
    if (vim_insert_node_buffer_used == sizeof(vim_insert_node_buffer)) {
        return 0;
    }

    if (!vim_state.first_free_insert_node) {
        vim_state.first_free_insert_node = push_array(&vim_state.arena, Vim_Insert_Node, 1);
        vim_state.first_free_insert_node->next = 0;
    }
    Vim_Insert_Node* node = vim_state.first_free_insert_node;
    vim_state.first_free_insert_node = vim_state.first_free_insert_node->next;

    sll_queue_push(vim_state.first_insert_node, vim_state.last_insert_node, node);

    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(record.single_first));

    node->rel_line = cursor.line - insert_cursor.line;
    node->rel_col  = cursor.col  - insert_cursor.col;

    String_Const_u8 fwd = record.single_string_forward;
    String_Const_u8 bck = record.single_string_backward;

    Assert((vim_insert_node_buffer_used + fwd.size + bck.size) < sizeof(vim_insert_node_buffer));

    {
        u8* buffer_pos = vim_insert_node_buffer + vim_insert_node_buffer_used;
        u32 buffer_left = sizeof(vim_insert_node_buffer) - vim_insert_node_buffer_used;
        u32 size = Min(cast(u32) fwd.size, buffer_left);
        block_copy(buffer_pos, fwd.str, size);
        vim_insert_node_buffer_used += cast(u32) size;
        node->text_forward = SCu8(buffer_pos, size);
    }

    {
        u8* buffer_pos = vim_insert_node_buffer + vim_insert_node_buffer_used;
        u32 buffer_left = sizeof(vim_insert_node_buffer) - vim_insert_node_buffer_used;
        u32 size = Min(cast(u32) bck.size, buffer_left);
        block_copy(buffer_pos, bck.str, size);
        vim_insert_node_buffer_used += cast(u32) size;
        node->text_backward = SCu8(buffer_pos, size);
    }

    return node;
}

#define vim_add_abbreviation(match, replacement) vim_add_abbreviation_(string_u8_litexpr(match), string_u8_litexpr(replacement))
internal void vim_add_abbreviation_(String_Const_u8 match, String_Const_u8 replacement) {
    Vim_Abbreviation* abbreviation = push_array(&vim_state.arena, Vim_Abbreviation, 1);
    sll_stack_push(vim_state.first_abbreviation, abbreviation);
    abbreviation->match = match;
    abbreviation->replacement = replacement;
}

internal void vim_apply_abbreviations_for_range(Application_Links* app, View_ID view, Range_i64 word_range) {
    ProfileScope(app, "[vim] apply abbreviations for range");
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (buffer_exists(app, buffer)) {
        Scratch_Block scratch(app);
        String_Const_u8 word = push_buffer_range(app, scratch, buffer, word_range);

        // @TODO: Maybe consider creating some cool acceleration structure for this. Or not. Should be profiled, I guess.
        for (Vim_Abbreviation* abbreviation = vim_state.first_abbreviation; abbreviation; abbreviation = abbreviation->next) {
            if (string_match(word, abbreviation->match)) {
                buffer_replace_range(app, buffer, word_range, abbreviation->replacement);
                view_set_cursor_and_preferred_x(app, view, seek_pos(word_range.min + abbreviation->replacement.size));
            }
        }
    }
}

internal void vim_begin_command(Application_Links* app, Buffer_ID buffer, Vim_Key_Binding* bind, i32 count, i32 count_was_set, Vim_Visual_Selection selection) {
    if (!vim_state.command_in_progress) {
        // @TODO: Are there commands that are not repeatable that want grouped history?
        if (HasFlag(bind->flags, VimBindingFlag_IsRepeatable)) {
            vim_state.command_history = history_group_begin(app, buffer);
            vim_state.command_in_progress = true;

            Vim_Command_Rep* rep = &vim_state.next_command_rep;
            block_zero_struct(rep);
            rep->count = count;
            rep->count_was_set = count_was_set;
            switch (bind->kind) {
                case VimBindingKind_Operator: {
                    Vim_Visual_Selection local_selection = selection;
                    local_selection.range.one_past_last.line -= local_selection.range.first.line;
                    local_selection.range.one_past_last.col  -= local_selection.range.first.col;
                    local_selection.range.first.line = 0;
                    local_selection.range.first.col  = 0;
                    rep->kind = VimCommandRep_Operator;
                    rep->selection = local_selection;
                    rep->reg = vim_state.active_register;
                    rep->op = bind->op;
                } break;
                case VimBindingKind_4CoderCommand: {
                    rep->kind = VimCommandRep_4CoderCommand;
                    rep->fcoder_command = bind->fcoder_command;
                } break;
            }
        }
    }
}

internal void vim_end_command(Application_Links* app, b32 command_was_complete) {
    if (vim_state.command_in_progress) {
        history_group_end(vim_state.command_history);
        vim_state.command_in_progress = false;
        if (command_was_complete) {
            vim_state.command_rep = vim_state.next_command_rep;
        }
    }
}

internal void vim_select_mapid_for_mode(Application_Links* app, Buffer_ID buffer, Vim_Mode mode) {
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
    Command_Map_ID* map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
    Command_Map_ID* insert_map_id_ptr = scope_attachment(app, buffer_scope, vim_buffer_insert_map_id, Command_Map_ID);

    switch (mode) {
        case VimMode_Normal: {
            *map_id_ptr = vim_mapid_normal;
        } break;
        case VimMode_Insert:
        case VimMode_VisualInsert: {
            u32 access_flags = buffer_get_access_flags(app, buffer);
            if (HasFlag(access_flags, Access_Write)) {
                Assert(*insert_map_id_ptr);
                *map_id_ptr = *insert_map_id_ptr;
            } else {
                *map_id_ptr = mapid_file;
            }
        } break;
        case VimMode_Visual:
        case VimMode_VisualLine:
        case VimMode_VisualBlock: {
            *map_id_ptr = vim_mapid_visual;
        } break;
    }
}

internal void vim_enter_mode(Application_Links* app, Vim_Mode mode, b32 append = false) {
    u32 access_flags = Access_ReadVisible;
    if (is_vim_insert_mode(mode)) {
        access_flags |= Access_Write;
    }

    View_ID view = get_active_view(app, access_flags);
    Buffer_ID buffer = view_get_buffer(app, view, access_flags);
    if (!buffer_exists(app, buffer)) {
        return;
    }

#if VIM_PRINT_COMMANDS
    print_message(app, string_u8_litexpr("Switching mode: "));
    switch (mode) {
#define PRINT_MODE(MODE) case VimMode_##MODE: { print_message(app, string_u8_litexpr("VimMode_" #MODE "\n")); } break;
        PRINT_MODE(Normal);
        PRINT_MODE(Insert);
        PRINT_MODE(VisualInsert);
        PRINT_MODE(Visual);
        PRINT_MODE(VisualLine);
        PRINT_MODE(VisualBlock);
#undef PRINT_MODE
    }
#endif

    switch (mode) {
        case VimMode_Normal: {
            if (is_vim_insert_mode(vim_state.mode)) {
                if (vim_state.insert_sequence) {
                    vim_apply_abbreviations_for_range(app, view, Ii64(vim_state.insert_sequence_start_pos, vim_state.insert_sequence_pos));
                    vim_state.insert_sequence = false;
                }

                vim_rel_move(app, seek_line_col(0, -1));

                History_Record_Index insert_history_index = vim_state.insert_history_index;
                History_Record_Index current_history_index = buffer_history_get_current_state_index(app, buffer);

                if (!vim_state.playing_back_command && insert_history_index <= current_history_index) {
                    vim_state.first_free_insert_node = vim_state.first_insert_node;
                    vim_state.first_insert_node = vim_state.last_insert_node = 0;
                    vim_insert_node_buffer_used = 0;

                    // NOTE: Merge before processing so that a later merge won't shift around the memory of the strings
                    buffer_history_merge_record_range(app, buffer, insert_history_index, current_history_index, RecordMergeFlag_StateInRange_MoveStateForward);
                    current_history_index = buffer_history_get_current_state_index(app, buffer);

                    for (i32 history_index = insert_history_index; history_index <= current_history_index; history_index++) {
                        Record_Info record = buffer_history_get_record_info(app, buffer, history_index);
                        if (record.kind == RecordKind_Group) {
                            for (i32 sub_index = 0; sub_index < record.group_count; sub_index++) {
                                Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, history_index, sub_index);
                                vim_add_insert_node_from_record(app, buffer, sub_record, vim_state.insert_cursor);
                            }
                        } else {
                            vim_add_insert_node_from_record(app, buffer, record, vim_state.insert_cursor);
                        }
                    }
                }
            }

            if (vim_state.mode == VimMode_VisualInsert) {
                b32 nodes_occupy_first_line = true;
                for (Vim_Insert_Node* node = vim_state.first_insert_node; node; node = node->next) {
                    if (node->rel_line != 0) {
                        nodes_occupy_first_line = false;
                        break;
                    }
                }

                if (nodes_occupy_first_line) {
                    Range_i64 line_range = vim_state.visual_insert_line_range;
                    for (i64 line = line_range.first + 1; line < line_range.one_past_last; line++) {
                        Buffer_Cursor line_end_cursor = buffer_compute_cursor(app, buffer, seek_line_col(line, max_i64));

                        i64 col_adjust = 0;
                        if (vim_state.right_justify_visual_insert) {
                            col_adjust = line_end_cursor.col - vim_state.insert_cursor.col;
                        }

                        for (Vim_Insert_Node* node = vim_state.first_insert_node; node; node = node->next) {
                            i64 col = vim_state.insert_cursor.col + node->rel_col + col_adjust;
                            Buffer_Cursor abs_cursor = buffer_compute_cursor(app, buffer, seek_line_col(line, col));

                            if (vim_state.right_justify_visual_insert || (line_end_cursor.col >= abs_cursor.col)) {
                                Range_i64 replace_range = Ii64_size(abs_cursor.pos, node->text_backward.size);
                                buffer_replace_range(app, buffer, replace_range, node->text_forward);
                            }
                        }
                    }
                }
            }

            vim_state.most_recent_character_seek.size = 0;
            vim_state.command_rep = vim_state.next_command_rep;
        } break;

        case VimMode_VisualInsert: {
            Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);
            vim_state.visual_insert_line_range = Ii64(selection.range.first.line, selection.range.one_past_last.line);

            if (append) {
                if (selection.kind == VimSelectionKind_Block) {
                    vim_state.right_justify_visual_insert = vim_state.visual_block_force_to_line_end;
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.range.first.line, selection.range.one_past_last.col));
                } else {
                    vim_state.right_justify_visual_insert = true;
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.range.first.line, max_i64));
                }
            } else {
                vim_state.right_justify_visual_insert = false;
                view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.range.first.line, selection.range.first.col));
            }
        } case VimMode_Insert: {
            vim_state.insert_history_index = buffer_history_get_current_state_index(app, buffer) + 1;
            vim_state.insert_cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
            vim_state.character_seek_show_highlight = false;
        } break;

        case VimMode_Visual:
        case VimMode_VisualLine:
        case VimMode_VisualBlock: {
            // ...
        } break;
    }

    vim_select_mapid_for_mode(app, buffer, mode);
    vim_state.mode = mode;
}

CUSTOM_COMMAND_SIG(vim_enter_normal_mode_escape) {
    b32 end_command = false;
    if (is_vim_insert_mode(vim_state.mode)) {
        end_command = true;
    }
    vim_enter_mode(app, VimMode_Normal);
    if (end_command) {
        vim_end_command(app, true);
    }
}

CUSTOM_COMMAND_SIG(vim_enter_normal_mode) {
    vim_enter_mode(app, VimMode_Normal);
}

CUSTOM_COMMAND_SIG(vim_enter_insert_mode) {
    vim_enter_mode(app, VimMode_Insert);
}

internal void vim_enter_visual_insert_mode_internal(Application_Links* app, b32 append) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);
    if (selection.kind == VimSelectionKind_None || selection.kind == VimSelectionKind_Range) {
        vim_enter_mode(app, VimMode_Insert);
    } else {
        vim_enter_mode(app, VimMode_VisualInsert, append);
    }
}

CUSTOM_COMMAND_SIG(vim_enter_visual_insert_mode) {
    vim_enter_visual_insert_mode_internal(app, false);
}

CUSTOM_COMMAND_SIG(vim_enter_visual_append_mode) {
    vim_enter_visual_insert_mode_internal(app, true);
}

CUSTOM_COMMAND_SIG(vim_enter_insert_sol_mode) {
    vim_enter_mode(app, VimMode_Insert);
    seek_beginning_of_line(app);
}

CUSTOM_COMMAND_SIG(vim_enter_append_mode) {
    vim_enter_mode(app, VimMode_Insert);
    vim_rel_move(app, seek_line_col(0, 1));
}

CUSTOM_COMMAND_SIG(vim_enter_append_eol_mode) {
    vim_enter_mode(app, VimMode_Insert);
    seek_end_of_line(app);
}

CUSTOM_COMMAND_SIG(vim_toggle_visual_mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    if (!is_vim_visual_mode(vim_state.mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }

    if (vim_state.mode == VimMode_Visual) {
        vim_enter_mode(app, VimMode_Normal, true);
    } else {
        vim_enter_mode(app, VimMode_Visual);
    }
}

CUSTOM_COMMAND_SIG(vim_toggle_visual_line_mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    if (!is_vim_visual_mode(vim_state.mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }

    if (vim_state.mode == VimMode_VisualLine) {
        vim_enter_mode(app, VimMode_Normal, true);
    } else {
        vim_enter_mode(app, VimMode_VisualLine);
    }
}

CUSTOM_COMMAND_SIG(vim_toggle_visual_block_mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    if (!is_vim_visual_mode(vim_state.mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
        vim_state.visual_block_force_to_line_end = false;
    }

    if (vim_state.mode == VimMode_VisualBlock) {
        vim_enter_mode(app, VimMode_Normal, true);
    } else {
        vim_enter_mode(app, VimMode_VisualBlock);
    }
}

internal void vim_set_selection(Application_Links* app, View_ID view, Vim_Visual_Selection selection) {
    if (selection.kind) {
        Buffer_Cursor first = selection.range.first;
        Buffer_Cursor one_past_last = selection.range.one_past_last;
        if (!is_vim_visual_mode(vim_state.mode)) {
            switch (selection.kind) {
                case VimSelectionKind_Range: {
                    vim_toggle_visual_mode(app);
                } break;
                case VimSelectionKind_Line: {
                    vim_toggle_visual_line_mode(app);
                } break;
                case VimSelectionKind_Block: {
                    vim_toggle_visual_block_mode(app);
                } break;
            }
        }
        view_set_mark(app, view, seek_line_col(first.line, first.col));
        view_set_cursor_and_preferred_x(app, view, seek_line_col(one_past_last.line - 1, one_past_last.col - 1));
    }
}

internal Range_i64 vim_apply_range_style(Application_Links* app, View_ID view, Buffer_ID buffer, Range_i64 range, Vim_Range_Style style) {
    Range_i64 result = range;
    /* @TODO: Implement this:
           Which motions are linewise, inclusive or exclusive is mentioned with the
           command.  There are however, two general exceptions:
           1. If the motion is exclusive and the end of the motion is in column 1, the
              end of the motion is moved to the end of the previous line and the motion
              becomes inclusive.  Example: "}" moves to the first line after a paragraph,
              but "d}" will not include that line.
           2. If the motion is exclusive, the end of the motion is in column 1 and the
              start of the motion was at or before the first non-blank in the line, the
              motion becomes linewise.  Example: If a paragraph begins with some blanks
              and you do "d}" while standing on the first non-blank, all the lines of
              the paragraph are deleted, including the blanks.  If you do a put now, the
              deleted lines will be inserted below the cursor position.
    */
    switch (style) {
        case VimRangeStyle_Inclusive: {
            result.max = view_set_pos_by_character_delta(app, view, result.max, 1);
        } break;

        case VimRangeStyle_Linewise: {
            // @TODO: Should the resulting range be inclusive of the ending newline? I think so...
            Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
            result = Ii64(get_line_start_pos(app, buffer, line_range.min), get_line_end_pos(app, buffer, line_range.max));
            if (buffer_get_char(app, buffer, result.max) == '\r') result.max++;
            if (buffer_get_char(app, buffer, result.max) == '\n') result.max++;
        } break;
    }
    return result;
}

internal Vim_Motion_Result vim_execute_motion(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Motion* motion, i64 start_pos, i32 count, b32 count_was_set) {
    if (count < 0) count = 1;
    Vim_Motion_Result result = vim_motion(start_pos);
    for (i32 i = 0; i < count; ++i) {
        result = motion(app, view, buffer, result.seek_pos, count, count_was_set);
        if (HasFlag(result.flags, VimMotionFlag_IgnoreMotionCount)) {
            break;
        }
    }
    result.range_ = vim_apply_range_style(app, view, buffer, Ii64(start_pos, result.seek_pos), result.style);
    return result;
}

internal Vim_Text_Object_Result vim_execute_text_object(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Text_Object* text_object, i64 start_pos, i32 count, b32 count_was_set) {
    if (count < 0) count = 1;
    Vim_Text_Object_Result result = vim_text_object(start_pos);
    i64 pos = start_pos;
    result.range = Ii64_neg_inf;
    for (i32 i = 0; i < count; ++i) {
        // @TODO: How do text objects with a count greater than 1 work??
        Vim_Text_Object_Result inner = text_object(app, view, buffer, pos, count, count_was_set);
        Range_i64 new_range = range_union(inner.range, result.range);
        result = inner;
        result.range = new_range;
        pos = result.range.max;
        if (HasFlag(result.flags, VimTextObjectFlag_IgnoreMotionCount)) {
            break;
        }
    }
    // @TODO: When should the range style be applied, after or inside the loop?
    result.range = vim_apply_range_style(app, view, buffer, result.range, result.style);
    return result;
}

internal Range_i64 vim_seek_motion_result(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Motion_Result mo_res) {
    i64 old_pos = view_get_cursor_pos(app, view);
    i64 new_pos = mo_res.seek_pos;

    Range_i64 result = mo_res.range_; // vim_apply_range_style(app, buffer, Ii64(old_pos, new_pos), mo_res.style);

    i64 old_line = get_line_number_from_pos(app, buffer, old_pos);
    i64 new_line = get_line_number_from_pos(app, buffer, new_pos);

    if (HasFlag(mo_res.flags, VimMotionFlag_IsJump) && !HasFlag(mo_res.flags, VimMotionFlag_LogJumpPostSeek)) {
        if (old_line != new_line) {
            vim_log_jump_history(app);
        }
    }

    if (HasFlag(mo_res.flags, VimMotionFlag_SetPreferredX)) {
        view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
    } else {
        view_set_cursor(app, view, seek_pos(new_pos));
    }

    if (HasFlag(mo_res.flags, VimMotionFlag_IsJump) && HasFlag(mo_res.flags, VimMotionFlag_LogJumpPostSeek)) {
        vim_log_jump_history(app);
    }

    return result;
}

internal Vim_Binding_Map* vim_get_map_for_mode(Vim_Mode mode) {
    Vim_Binding_Map* map = 0;
    if (is_vim_visual_mode(mode)) {
        map = &vim_map_visual;
    } else if (mode == VimMode_Normal) {
        map = &vim_map_normal;
    }
    return map;
}

internal void vim_select_range(Application_Links* app, View_ID view, Range_i64 range) {
    view_set_mark(app, view, seek_pos(range.min));
    view_set_cursor(app, view, seek_pos(range.max - 1)); // @Unicode
}

internal VIM_MOTION(vim_motion_left) {
    Vim_Motion_Result result = vim_motion(start_pos);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
    result.seek_pos = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, Max(1, cursor.col - 1))).pos;
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_right) {
    Vim_Motion_Result result = vim_motion(start_pos);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
    Buffer_Cursor eol_cursor = buffer_compute_cursor(app, buffer, seek_pos(get_line_end_pos_from_pos(app, buffer, start_pos)));
    result.seek_pos = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, Min(cursor.col + 1, eol_cursor.col - 1))).pos;
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_down) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);
    Buffer_Cursor start_cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
    i64 target_line = start_cursor.line + 1;
    if (!is_valid_line(app, buffer, target_line)) {
        target_line = start_cursor.line;
    }
    f32 preferred_x = view_get_preferred_x(app, view);
    i64 end_pos = view_pos_at_relative_xy(app, view, target_line, V2f32(preferred_x, 0.0f));
    result.seek_pos = end_pos;
    return result;
}

internal VIM_MOTION(vim_motion_up) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);
    Buffer_Cursor start_cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
    i64 target_line = start_cursor.line - 1;
    if (!is_valid_line(app, buffer, target_line)) {
        target_line = start_cursor.line;
    }
    f32 preferred_x = view_get_preferred_x(app, view);
    i64 end_pos = view_pos_at_relative_xy(app, view, target_line, V2f32(preferred_x, 0.0f));
    result.seek_pos = end_pos;
    return result;

}

// @TODO: My word motions seem kind of stupidly complicated, but I keep running into little limitations of the built in helper boundary functions and such,
//        so at some point I should put some effort into going a bit lower level and simplifying these. But until then, they work.

internal i64 vim_boundary_word(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    i64 result = 0;
    if (direction == Scan_Forward) {
        result = Min(buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
                     buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Min(result, buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos));
    } else {
        result = Max(buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
                     buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Max(result, buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos));
    }

    return result;
}

internal i64 vim_boundary_word_end(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    i64 result = 0;
    if (direction == Scan_Forward) {
        result = Min(buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
                     buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Min(result, buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_whitespace, direction, pos));
    } else {
        result = vim_boundary_word(app, buffer, side, direction, pos);
    }

    return result;
}

internal i64 vim_boundary_whitespace(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_whitespace));
}

internal Vim_Motion_Result vim_motion_word_internal(Application_Links* app, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_motion(start_pos);

    Scratch_Block scratch(app);

    i64 end_pos = start_pos;

    if (dir == Scan_Forward) {
        if (line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, end_pos))) {
            i64 next_line = get_line_number_from_pos(app, buffer, end_pos) + 1;
            i64 next_line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, next_line);

            end_pos = next_line_start;
        } else {
            end_pos = scan(app, push_boundary_list(scratch, vim_boundary_word), buffer, Scan_Forward, end_pos);

            u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
            if (character_is_whitespace(c_at_cursor)) {
                end_pos = scan(app, push_boundary_list(scratch, vim_boundary_whitespace, boundary_line), buffer, Scan_Forward, end_pos);
            }
        }
    } else {
        if (line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, end_pos))) {
            i64 next_line = get_line_number_from_pos(app, buffer, end_pos) - 1;
            i64 next_line_end = get_line_end_pos(app, buffer, next_line);

            end_pos = next_line_end;
        } else {
            end_pos = scan(app, push_boundary_list(scratch, vim_boundary_word, boundary_line), buffer, Scan_Backward, end_pos);

            u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
            if (!vim_character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                end_pos = scan(app, push_boundary_list(scratch, vim_boundary_word, boundary_line), buffer, Scan_Backward, end_pos);
            }
        }
    }

    result.seek_pos = end_pos;
    result.flags |= VimMotionFlag_SetPreferredX;

#if 0
    // @TODO: What did this do?
    if (dir == Scan_Backward && range_size(result.range) > 0) {
        result.range.max -= 1;
    }
#endif

    return result;
}

internal VIM_MOTION(vim_motion_word) {
    return vim_motion_word_internal(app, buffer, Scan_Forward, start_pos);
}

internal VIM_MOTION(vim_motion_word_backward) {
    return vim_motion_word_internal(app, buffer, Scan_Backward, start_pos);
}

internal i64 vim_boundary_big_word(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    i64 result = buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos);
    return result;
}

internal VIM_MOTION(vim_motion_big_word) {
    Vim_Motion_Result result = vim_motion(start_pos);
    Scratch_Block scratch(app);
    result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_big_word), buffer, Scan_Forward, result.seek_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_big_word_backward) {
    Vim_Motion_Result result = vim_motion(start_pos);
    Scratch_Block scratch(app);
    result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_big_word), buffer, Scan_Backward, result.seek_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_word_end) {
    Scratch_Block scratch(app);
    Vim_Motion_Result result = vim_motion(start_pos);
    result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_word_end), buffer, Scan_Forward, start_pos + 1) - 1;
    if (character_is_whitespace(buffer_get_char(app, buffer, result.seek_pos))) {
        result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_word_end), buffer, Scan_Forward, result.seek_pos + 1) - 1;
    }
    result.flags |= VimMotionFlag_SetPreferredX;
    result.style  = VimRangeStyle_Inclusive;
    return result;
}

internal VIM_MOTION(vim_motion_buffer_start_or_goto_line) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);

    if (motion_count_was_set) {
        Buffer_Cursor start_cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
        result.seek_pos = buffer_compute_cursor(app, buffer, seek_line_col(motion_count, start_cursor.col)).pos;
    } else {
        result.seek_pos = 0;
    }

    result.flags |= VimMotionFlag_IsJump|VimMotionFlag_SetPreferredX;

    return result;
}

internal VIM_MOTION(vim_motion_buffer_end_or_goto_line) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);

    if (motion_count_was_set) {
        Buffer_Cursor start_cursor = buffer_compute_cursor(app, buffer, seek_pos(start_pos));
        result.seek_pos = buffer_compute_cursor(app, buffer, seek_line_col(motion_count, start_cursor.col)).pos;
    } else {
        result.seek_pos = buffer_get_size(app, buffer);
    }

    result.flags |= VimMotionFlag_IsJump|VimMotionFlag_SetPreferredX;

    return result;
}

internal Vim_Motion_Result vim_motion_line_side_textual(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_motion(start_pos);

    i64 line_side = get_line_side_pos_from_pos(app, buffer, start_pos, dir == Scan_Forward ? Side_Max : Side_Min);

    result.seek_pos = line_side;
    result.flags |= VimMotionFlag_SetPreferredX;

    return result;
}

internal VIM_MOTION(vim_motion_line_start_textual) {
    return vim_motion_line_side_textual(app, view, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_line_end_textual) {
    Vim_Motion_Result result = vim_motion_line_side_textual(app, view, buffer, Scan_Forward, start_pos);
    result.flags |= VimMotionFlag_VisualBlockForceToLineEnd;
    return result;
}

internal Vim_Motion_Result vim_motion_scope_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, i32 motion_count, b32 motion_count_was_set, b32 unlimited_lookahead) {
    // NOTE: One minor disappointment regarding this function is that because it uses tokens, it doesn't work inside comments.
    //       That's lame. I want it to work inside comments.
    Vim_Motion_Result result = vim_motion_inclusive(start_pos);
    result.flags |= VimMotionFlag_IsJump|
                    VimMotionFlag_SetPreferredX;

    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.count > 0) {
        Token_Base_Kind opening_token = TokenBaseKind_EOF;
        Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, start_pos));
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, start_pos);

        b32 scan_forward = true;
        while (opening_token == TokenBaseKind_EOF) {
            Token* token = token_it_read(&it);

            if (!token) {
                break;
            }

            if (unlimited_lookahead || range_contains_inclusive(line_range, token->pos + token->size)) {
                if (token->kind == TokenBaseKind_ScopeOpen          ||
                    token->kind == TokenBaseKind_ParentheticalOpen  ||
                    token->kind == TokenBaseKind_ScopeOpen          ||
                    token->kind == TokenBaseKind_ParentheticalClose ||
                    token->kind == TokenBaseKind_ScopeClose)
                {
                    opening_token = token->kind;
                } else {
                    scan_forward ? token_it_inc(&it) : token_it_dec(&it);
                }
            } else {
                if (opening_token == TokenBaseKind_EOF && scan_forward) {
                    scan_forward = false;
                    token_it_dec(&it);
                } else {
                    break;
                }
            }
        }

        if (opening_token != TokenBaseKind_EOF) {
            scan_forward = (opening_token == TokenBaseKind_ScopeOpen || opening_token == TokenBaseKind_ParentheticalOpen);

            Token_Base_Kind closing_token = TokenBaseKind_EOF;
            if (opening_token == TokenBaseKind_ParentheticalOpen)  closing_token = TokenBaseKind_ParentheticalClose;
            if (opening_token == TokenBaseKind_ParentheticalClose) closing_token = TokenBaseKind_ParentheticalOpen;
            if (opening_token == TokenBaseKind_ScopeOpen)          closing_token = TokenBaseKind_ScopeClose;
            if (opening_token == TokenBaseKind_ScopeClose)         closing_token = TokenBaseKind_ScopeOpen;

            i32 scope_depth = 1;
            for (;;) {
                b32 tokens_left = scan_forward ? token_it_inc(&it) : token_it_dec(&it);
                if (tokens_left) {
                    Token* token = token_it_read(&it);

                    if (token->kind == opening_token) {
                        scope_depth++;
                    } else if (token->kind == closing_token) {
                        scope_depth--;
                    }

                    if (scope_depth == 0) {
                        i64 end_pos = token->pos;
                        if (scan_forward) {
                            end_pos += token->size - 1;
                        }

                        result.seek_pos = end_pos;

                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }

    return result;
}

internal VIM_MOTION(vim_motion_scope) {
    return vim_motion_scope_internal(app, view, buffer, start_pos, motion_count, motion_count_was_set, false);
}

internal VIM_MOTION(vim_motion_scope_unlimited_lookahead) {
    return vim_motion_scope_internal(app, view, buffer, start_pos, motion_count, motion_count_was_set, true);
}

internal VIM_TEXT_OBJECT(vim_text_object_line) {
    Vim_Text_Object_Result result = vim_text_object_linewise(start_pos);
    return result;
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_line) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);
    result.range.min = get_pos_past_lead_whitespace(app, buffer, start_pos);
    result.range.max = get_line_end_pos_from_pos(app, buffer, start_pos);
    return result;
}

function b32 vim_find_surrounding_nest(Application_Links *app, Buffer_ID buffer, i64 pos, Find_Nest_Flag flags, Range_i64 *out, b32 inner) {
    b32 result = false;
    Range_i64 range = {};

    u32 min_flags = flags|FindNest_Balanced;
    u32 max_flags = flags|FindNest_Balanced|FindNest_EndOfToken;
    if (inner) {
        Swap(u32, min_flags, max_flags);
    }

    if (find_nest_side(app, buffer, pos, min_flags, Scan_Backward, NestDelim_Open, &range.start) &&
        find_nest_side(app, buffer, range.start, max_flags, Scan_Forward, NestDelim_Close, &range.end))
    {
        if (inner && range_size(range) > 0) {
            range.max--;
        }
        *out = range;
        result = true;
    }

    return result;
}

internal Vim_Text_Object_Result vim_text_object_inner_nest_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, Find_Nest_Flag flags, b32 leave_inner_line, b32 select_inner_block) {
    // @TODO: Turn this function into not a total hot mess
    Vim_Text_Object_Result result = vim_text_object(start_pos);

    if (find_surrounding_nest(app, buffer, start_pos, flags, &result.range)) {
        i64 min = result.range.min;
        i64 max = result.range.max;
        i64 min_inner = result.range.min + 1; // @Incomplete: This must be the opening token's size!!!
        i64 max_inner = result.range.max - 1;
        i64 min_line = get_line_number_from_pos(app, buffer, min);
        i64 max_line = get_line_number_from_pos(app, buffer, max);
        result.range = Ii64(min_inner, max_inner);
        if (min_line != max_line) {
            Scratch_Block scratch(app);
            String_Const_u8 left_of_max = push_buffer_range(app, scratch, buffer, Ii64(get_line_start_pos(app, buffer, max_line), max_inner));
            if (!leave_inner_line && (!left_of_max.size || string_find_first_non_whitespace(left_of_max) == left_of_max.size)) {
                result.range.max = get_line_start_pos(app, buffer, max_line);
            }
            if ((max_line - 1) > min_line) {
                if (select_inner_block) {
                    result.range.min = get_line_start_pos(app, buffer, min_line + 1);
                } else if (leave_inner_line) {
                    result.range.min = get_pos_past_lead_whitespace_from_line_number(app, buffer, min_line + 1);
                }
            }
        }
    }

    return result;
}

// internal VIM_TEXT_OBJECT(vim_text_object_inner_scope_change) {
//     return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Scope, true, false);
// }

internal VIM_TEXT_OBJECT(vim_text_object_inner_scope_delete) {
    return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Scope, false, false);
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_scope) {
    return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Scope, false, true);
}

// internal VIM_TEXT_OBJECT(vim_text_object_inner_paren_change) {
//     return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Paren, true, false);
// }

internal VIM_TEXT_OBJECT(vim_text_object_inner_paren_delete) {
    return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Paren, true, false);
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_paren) {
    return vim_text_object_inner_nest_internal(app, view, buffer, start_pos, FindNest_Paren, false, true);
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_double_quotes) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);
    result.range = enclose_pos_inside_quotes(app, buffer, start_pos);
    return result;
}

internal i64 vim_boundary_inside_single_quotes(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    internal Character_Predicate predicate = {};
    internal b32 first_call = true;
    if (first_call) {
        first_call = false;
        predicate = character_predicate_from_character(cast(u8) '\'');
        predicate = character_predicate_not(&predicate);
    }
    return boundary_predicate(app, buffer, side, direction, pos, &predicate);
}

internal Range_i64 enclose_pos_inside_single_quotes(Application_Links *app, Buffer_ID buffer, i64 pos) {
    return enclose_boundary(app, buffer, Ii64(pos), vim_boundary_inside_single_quotes);
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_single_quotes) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);
    result.range = enclose_pos_inside_single_quotes(app, buffer, start_pos);
    return result;
}

internal VIM_TEXT_OBJECT(vim_text_object_inner_word) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);
    result.range = enclose_boundary(app, buffer, Ii64(start_pos + 1), vim_boundary_word);
    return result;
}

internal Vim_Motion_Result vim_motion_character_seek_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, String_Const_u8 target, Scan_Direction dir, b32 till) {
    Vim_Motion_Result result = vim_motion(start_pos);
    b32 inclusive = (dir == Scan_Forward);
    result.style  = inclusive ? VimRangeStyle_Inclusive : VimRangeStyle_Exclusive;
    result.flags |= VimMotionFlag_SetPreferredX;

    b32 new_input = true;
    if (!target.size) {
        new_input = false;
        target = vim_state.most_recent_character_seek.string;
    }

    if (target.size) {
        i64 seek_pos = start_pos;
        for (;;) {
            String_Match match = buffer_seek_string(app, buffer, target, dir, seek_pos + (till ? dir : 0));
            if (match.buffer != buffer) {
                break;
            }
            // @TODO: Make this work as a separate command instead
#if VIM_CASE_SENSITIVE_CHARACTER_SEEK
            if (HasFlag(match.flags, StringMatch_CaseSensitive))
#endif
            {
                if (dir == Scan_Forward) {
                    result.seek_pos = match.range.min + (till ? -1 : 0);
                } else {
                    result.seek_pos = match.range.max - 1 + (till ? target.size : 0);
                }
                break;
            }
#if VIM_CASE_SENSITIVE_CHARACTER_SEEK
            seek_pos = match.range.min;
#endif
        }

        if (new_input) {
            if (!vim_state.most_recent_character_seek.cap) {
                vim_state.most_recent_character_seek = Su8(vim_state.most_recent_character_seek_storage, 0, ArrayCount(vim_state.most_recent_character_seek_storage));
            }
            block_copy(vim_state.most_recent_character_seek.str, target.str, Min(vim_state.most_recent_character_seek.cap, target.size));
            vim_state.most_recent_character_seek.size = target.size;

            vim_state.character_seek_highlight_dir = dir;
            vim_state.most_recent_character_seek_dir = dir;
            vim_state.most_recent_character_seek_till = till;
            vim_state.most_recent_character_seek_inclusive = inclusive;
        }
    }

    return result;
}

internal VIM_MOTION(vim_motion_repeat_character_seek_same_direction);
internal VIM_MOTION(vim_motion_repeat_character_seek_reverse_direction);

internal VIM_MOTION(vim_motion_find_character) {
    String_Const_u8 target = vim_get_next_writable(app);
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target, Scan_Forward, false);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_to_character) {
    String_Const_u8 target = vim_get_next_writable(app);
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target, Scan_Forward, true);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_backward) {
    String_Const_u8 target = vim_get_next_writable(app);
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target, Scan_Backward, false);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_to_character_backward) {
    String_Const_u8 target = vim_get_next_writable(app);
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target, Scan_Backward, true);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_pair) {
    u8 target_storage[8];
    String_u8 target = Su8(target_storage, 0, ArrayCount(target_storage));
    string_append(&target, vim_get_next_writable(app));
    string_append(&target, vim_get_next_writable(app));
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target.string, Scan_Forward, false);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_pair_backward) {
    u8 target_storage[8];
    String_u8 target = Su8(target_storage, 0, ArrayCount(target_storage));
    string_append(&target, vim_get_next_writable(app));
    string_append(&target, vim_get_next_writable(app));
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view, buffer, start_pos, target.string, Scan_Backward, false);
    for (i32 i = 1; i < motion_count; ++i) {
        mr = vim_motion_repeat_character_seek_same_direction(app, view, buffer, mr.seek_pos, 1, false);
    }
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek|VimMotionFlag_IgnoreMotionCount;
    return mr;
}

internal VIM_MOTION(vim_motion_repeat_character_seek_same_direction) {
    Scan_Direction dir = vim_state.most_recent_character_seek_dir;
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    vim_state.character_seek_highlight_dir = dir;
    return vim_motion_character_seek_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_till);
}

internal VIM_MOTION(vim_motion_repeat_character_seek_reverse_direction) {
    Scan_Direction dir = -vim_state.most_recent_character_seek_dir;
    vim_state.character_seek_show_highlight = !vim_state.executing_queried_motion;
    vim_state.character_seek_highlight_dir = dir;
    return vim_motion_character_seek_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_till);
}

internal i64 vim_seek_blank_line(Application_Links *app, Scan_Direction direction, Position_Within_Line position) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 new_pos = get_pos_of_blank_line_grouped(app, buffer, direction, pos);
    switch (position) {
        case PositionWithinLine_SkipLeadingWhitespace: {
            new_pos = get_pos_past_lead_whitespace(app, buffer, new_pos);
        } break;
        case PositionWithinLine_End: {
            new_pos = get_line_side_pos_from_pos(app, buffer, new_pos, Side_Max);
        } break;
    }
    new_pos = view_get_character_legal_pos_from_pos(app, view, new_pos);
    return new_pos;
}

internal VIM_MOTION(vim_motion_to_empty_line_up) {
    // @TODO: Are you sure these shouldn't just be linewise?
    Vim_Motion_Result result = vim_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Backward, PositionWithinLine_Start);
    return result;
}

internal VIM_MOTION(vim_motion_to_empty_line_down) {
    Vim_Motion_Result result = vim_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Forward, PositionWithinLine_Start);
    return result;
}

internal VIM_MOTION(vim_motion_prior_to_empty_line_up) {
    Vim_Motion_Result result = vim_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Backward, PositionWithinLine_Start);

    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, Ii64(start_pos, result.seek_pos));
    if (line_range.min != line_range.max) {
        result.seek_pos = get_line_start_pos(app, buffer, line_range.min + 1);
    }

    return result;
}

internal VIM_MOTION(vim_motion_prior_to_empty_line_down) {
    Vim_Motion_Result result = vim_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Forward, PositionWithinLine_Start);

    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, Ii64(start_pos, result.seek_pos));
    if (line_range.min != line_range.max) {
        result.seek_pos = get_line_end_pos(app, buffer, line_range.max - 1);
    }

    return result;
}

internal Range_i64 vim_search_once_internal(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction direction, i64 pos, String_Const_u8 query, u32 flags, b32 recursed = false) {
    Range_i64 result = Ii64();

    b32 found_match = false;
    i64 buffer_size = buffer_get_size(app, buffer);
    switch (direction) {
        case Scan_Forward: {
            i64 new_pos = 0;
            if (HasFlag(flags, VimSearchFlag_CaseSensitive)) {
                seek_string_forward(app, buffer, pos - 1, 0, query, &new_pos);
            } else {
                seek_string_insensitive_forward(app, buffer, pos - 1, 0, query, &new_pos);
            }
            if (new_pos < buffer_size) {
                result = Ii64(new_pos, new_pos + query.size);
                found_match = true;
            }
        } break;

        case Scan_Backward: {
            i64 new_pos = 0;
            if (HasFlag(flags, VimSearchFlag_CaseSensitive)) {
                seek_string_backward(app, buffer, pos + 1, 0, query, &new_pos);
            } else {
                seek_string_insensitive_backward(app, buffer, pos + 1, 0, query, &new_pos);
            }
            if (new_pos >= 0) {
                result = Ii64(new_pos, new_pos + query.size);
                found_match = true;
            }
        } break;
    }

    if (found_match && HasFlag(flags, VimSearchFlag_WholeWord)) {
        Range_i64 enclosing_word = enclose_alpha_numeric_underscore_utf8(app, buffer, result);
        if (enclosing_word.min != result.min || enclosing_word.max != result.max) {
            found_match = false;
            result = Ii64();
        }
    }

    if (!recursed && !found_match) {
        switch (direction) {
            case Scan_Forward:  { result = vim_search_once_internal(app, view, buffer, direction, 0, query, flags, true); } break;
            case Scan_Backward: { result = vim_search_once_internal(app, view, buffer, direction, buffer_size, query, flags, true); } break;
        }
    }

    return result;
}

internal void vim_search_once(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction direction, i64 pos, String_Const_u8 query, u32 flags, b32 log_jump, b32 always_jump) {
    Range_i64 range = vim_search_once_internal(app, view, buffer, direction, pos, query, flags);
    i64 jump_target = pos;
    if (range_size(range) > 0) {
        jump_target = range.min;
        if (log_jump) {
            vim_log_jump_history(app);
            if (!always_jump) {
                view_set_cursor_and_preferred_x(app, view, seek_pos(jump_target));
            }
        }
    }
    if (always_jump) {
        view_set_cursor_and_preferred_x(app, view, seek_pos(jump_target));
    }
}

internal void vim_isearch_internal(Application_Links *app, View_ID view, Buffer_ID buffer, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 search_immediately, u32 mode_index) {
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;

    Assert(mode_index < ArrayCount(vim_search_mode_cycle));
    vim_state.search_mode_index = mode_index;
    vim_state.search_flags = vim_search_mode_cycle[vim_state.search_mode_index];

    if (search_immediately) {
        vim_write_register(app, &vim_state.last_search_register, query_init);
        vim_search_once(app, view, buffer, scan, pos, query_init, vim_state.search_flags, true, false);
    } else {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        if (!start_query_bar(app, &bar, 0)) {
            return;
        }

        u8 bar_string_space[256];
        bar.string = SCu8(bar_string_space, query_init.size);
        block_copy(bar.string.str, query_init.str, query_init.size);

        User_Input in = {};
        for (;;) {
            if (scan == Scan_Forward) {
                bar.prompt = vim_search_mode_prompt_forward[vim_state.search_mode_index];
            } else if (scan == Scan_Backward) {
                bar.prompt = vim_search_mode_prompt_reverse[vim_state.search_mode_index];
            }

            in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
            if (in.abort) {
                break;
            }

            String_Const_u8 string = to_writable(&in);

            b32 string_change = false;
            if (match_key_code(&in, KeyCode_Tab)) {
                vim_state.search_mode_index++;
                if (vim_state.search_mode_index >= ArrayCount(vim_search_mode_cycle)) {
                    vim_state.search_mode_index = 0;
                }
                vim_state.search_flags = vim_search_mode_cycle[vim_state.search_mode_index];
                continue;
            } else if (match_key_code(&in, KeyCode_Return)) {
                Input_Modifier_Set* mods = &in.event.key.modifiers;
                if (has_modifier(mods, KeyCode_Control)) {
                    bar.string = vim_read_register(app, &vim_state.last_search_register);
                    string_change = true;
                } else {
                    Managed_Scope scope = view_get_managed_scope(app, view);
                    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);
                    vim_log_jump_history_internal(app, view, buffer, vim_view, first_pos);
                    vim_write_register(app, &vim_state.last_search_register, bar.string);
                    break;
                }
            } else if (string.str != 0 && string.size > 0) {
                String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
                string_append(&bar_string, string);
                bar.string = bar_string.string;
                string_change = true;
            } else if (match_key_code(&in, KeyCode_Backspace)) {
                if (is_unmodified_key(&in.event)) {
                    u64 old_bar_string_size = bar.string.size;
                    bar.string = backspace_utf8(bar.string);
                    string_change = (bar.string.size < old_bar_string_size);
                } else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)) {
                    if (bar.string.size > 0) {
                        string_change = true;
                        bar.string.size = 0;
                    }
                }
            }

            if (string_change) {
                vim_write_register(app, &vim_state.last_search_register, bar.string);
            }

            // TODO(allen): how to detect if the input corresponds to
            // a search or rsearch command, a scroll wheel command?

            b32 do_scan_action = false;
            b32 do_scroll_wheel = false;
            Scan_Direction change_scan = scan;
            if (!string_change) {
                if (match_key_code(&in, KeyCode_PageDown) || match_key_code(&in, KeyCode_Down)) {
                    change_scan = Scan_Forward;
                    do_scan_action = true;
                }
                if (match_key_code(&in, KeyCode_PageUp) || match_key_code(&in, KeyCode_Up)) {
                    change_scan = Scan_Backward;
                    do_scan_action = true;
                }
            }

            if (string_change) {
                vim_search_once(app, view, buffer, scan, pos, bar.string, vim_state.search_flags, false, true);
            } else if (do_scan_action) {
                vim_search_once(app, view, buffer, change_scan, pos, bar.string, vim_state.search_flags, false, true);
            } else if (do_scroll_wheel) {
                mouse_wheel_scroll(app);
            } else {
                leave_current_input_unhandled(app);
            }
        }

        if (in.abort) {
            vim_write_register(app, &vim_state.last_search_register, SCu8());
            view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
        }
    }
}

CUSTOM_COMMAND_SIG(vim_isearch) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    vim_state.search_direction = Scan_Forward;
    vim_isearch_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view) + vim_state.search_direction, SCu8(), false, 0);
}

CUSTOM_COMMAND_SIG(vim_isearch_backward) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    vim_state.search_direction = Scan_Backward;
    vim_isearch_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view) - vim_state.search_direction, SCu8(), false, 0);
}

internal void vim_isearch_repeat_internal(Application_Links* app, Scan_Direction repeat_direction, b32 skip_at_cursor) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    Scan_Direction search_direction = vim_state.search_direction*repeat_direction;
    i64 start_pos = view_get_cursor_pos(app, view);
    if (skip_at_cursor) {
        start_pos += search_direction;
    }

    String_Const_u8 last_search = vim_read_register(app, &vim_state.last_search_register);
    Range_i64 range = vim_search_once_internal(app, view, buffer, search_direction, start_pos, last_search, vim_state.search_flags);

    if (range_size(range) > 0) {
        vim_log_jump_history(app);
        view_set_cursor_and_preferred_x(app, view, seek_pos(range.min));
        vim_state.search_show_highlight = true;
    }
}

CUSTOM_COMMAND_SIG(vim_isearch_repeat_forward) {
    vim_isearch_repeat_internal(app, Scan_Forward, true);
}

CUSTOM_COMMAND_SIG(vim_isearch_repeat_backward) {
    vim_isearch_repeat_internal(app, Scan_Backward, true);
}

internal void vim_isearch_selection_internal(Application_Links* app, Scan_Direction dir) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    Range_i64 range;
    Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);
    if (selection.kind != VimSelectionKind_Block && vim_selection_consume_line(app, buffer, &selection, &range, true)) {
        Scratch_Block scratch(app);
        String_Const_u8 query_init = push_buffer_range(app, scratch, buffer, range);

        vim_state.search_direction = dir;
        vim_isearch_internal(app, view, buffer, vim_state.search_direction, range.min, query_init, true, 3);
    }

    vim_enter_normal_mode(app);
}

CUSTOM_COMMAND_SIG(vim_isearch_selection) {
    vim_isearch_selection_internal(app, Scan_Forward);
}

CUSTOM_COMMAND_SIG(vim_reverse_isearch_selection) {
    vim_isearch_selection_internal(app, Scan_Backward);
}

CUSTOM_COMMAND_SIG(vim_isearch_word_under_cursor) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    Range_i64 range_under_cursor = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, view_get_cursor_pos(app, view));

    Scratch_Block scratch(app);
    String_Const_u8 under_cursor = push_buffer_range(app, scratch, buffer, range_under_cursor);

    vim_state.search_direction = Scan_Forward;
    if (under_cursor.size) {
        vim_isearch_internal(app, view, buffer, vim_state.search_direction, range_under_cursor.min + vim_state.search_direction, under_cursor, true, 3);
    }
}

CUSTOM_COMMAND_SIG(vim_reverse_isearch_word_under_cursor) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    Range_i64 range_under_cursor = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, view_get_cursor_pos(app, view));

    Scratch_Block scratch(app);
    String_Const_u8 under_cursor = push_buffer_range(app, scratch, buffer, range_under_cursor);

    vim_state.search_direction = Scan_Backward;
    if (under_cursor.size) {
        vim_isearch_internal(app, view, buffer, vim_state.search_direction, range_under_cursor.min + vim_state.search_direction, under_cursor, true, true);
    }
}

// @TODO: gd, search for the first occurence in the scope, walking up scopes until a match is found.

CUSTOM_COMMAND_SIG(noh) {
    vim_state.character_seek_show_highlight = false;
    vim_state.search_show_highlight = false;
}

enum Vim_Comment_Style {
    VimCommentStyle_NoIndent,
    VimCommentStyle_RangeIndent,
    VimCommentStyle_FullIndent,
};

enum Vim_Toggle_Comment_Mode {
    VimToggleComment_Auto,
    VimToggleComment_Comment,
    VimToggleComment_Uncomment,
};

global String_Const_u8 vim_cpp_line_comment = string_u8_litexpr("//");

internal b32 vim_line_comment_starts_at_position(Application_Links *app, Buffer_ID buffer, i64 pos, String_Const_u8 token) {
    b32 result = false;

    Scratch_Block scratch(app);
    String_Const_u8 text = push_buffer_range(app, scratch, buffer, Ii64(pos, pos + token.size));
    if (string_match(token, text)) {
        result = true;
    }

    return result;
}

internal void vim_toggle_line_comment_range(Application_Links* app, Buffer_ID buffer, Range_i64 line_range, String_Const_u8 comment_token, b32 comment_empty_lines, Vim_Comment_Style style, Vim_Toggle_Comment_Mode mode) {
    Scratch_Block scratch(app);
    String_Const_u8 comment_token_with_space = push_u8_stringf(scratch, "%.*s ", string_expand(comment_token));

    if (range_size(line_range) == 0) {
        line_range.one_past_last++;
    }

    i64 lowest_comment_col = max_i64;
    Vim_Toggle_Comment_Mode detected_mode = VimToggleComment_Comment;
    for (i64 line = line_range.first; line < line_range.one_past_last; line++) {
        i64 line_start_pos   = get_line_start_pos(app, buffer, line);
        i64 line_comment_pos = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
        i64 line_comment_col = (line_comment_pos - line_start_pos);

        if (!line_is_blank(app, buffer, line)) {
            lowest_comment_col = Min(lowest_comment_col, line_comment_col);
        }

        if (vim_line_comment_starts_at_position(app, buffer, line_comment_pos, comment_token)) {
            detected_mode = VimToggleComment_Uncomment;
        }
    }

    if (mode == VimToggleComment_Auto) {
        mode = detected_mode;
    }

    for (i64 line = line_range.first; line < line_range.one_past_last; line++) {
        i64 line_start_pos   = get_line_start_pos(app, buffer, line);
        i64 line_comment_pos = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
        i64 line_comment_col = (line_comment_pos - line_start_pos);

        String_Const_u8 line_string = push_buffer_line(app, scratch, buffer, line);
        b32 line_is_empty = (string_find_first_non_whitespace(line_string) == line_string.size);

        if (mode == VimToggleComment_Comment) {
            if (comment_empty_lines || !line_is_empty) {
                i64 insert_col = 1; // NOTE: columns start at 1, but the way I calculate cols up above starts at 0.
                switch (style) {
                    case VimCommentStyle_NoIndent: {
                        insert_col += 0;
                    } break;

                    case VimCommentStyle_RangeIndent: {
                        insert_col += lowest_comment_col;
                    } break;

                    case VimCommentStyle_FullIndent: {
                        insert_col += line_comment_col;
                    } break;

                    default: {
                        InvalidPath;
                    } break;
                }
                Buffer_Cursor insert_cursor = buffer_compute_cursor(app, buffer, seek_line_col(line, insert_col));
                buffer_replace_range(app, buffer, Ii64(insert_cursor.pos), comment_token_with_space);
            }
        } else {
            Assert(mode == VimToggleComment_Uncomment);
            if (!line_is_empty && vim_line_comment_starts_at_position(app, buffer, line_comment_pos, comment_token)) {
                String_Const_u8 post_comment_string = string_skip(line_string, line_comment_col + comment_token.size);
                u64 spaces_post_comment = string_find_first_non_whitespace(post_comment_string);
                buffer_replace_range(app, buffer, Ii64_size(line_comment_pos, comment_token.size + Min(1, spaces_post_comment)), SCu8());
            }
        }
    }
}

internal VIM_OPERATOR(vim_toggle_line_comment_no_indent_style) {
    Vim_Comment_Style style       = VimCommentStyle_NoIndent;
    Vim_Toggle_Comment_Mode mode  = VimToggleComment_Auto;

    b32 comment_empty_lines = false; // @TODO: Expose to user

    if (selection.kind) {
        Range_i64 line_range = Ii64(selection.range.first.line, selection.range.one_past_last.line);
        vim_toggle_line_comment_range(app, buffer, line_range, vim_cpp_line_comment, comment_empty_lines, style, mode);
    } else {
        i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
        vim_toggle_line_comment_range(app, buffer, Ii64(line), vim_cpp_line_comment, comment_empty_lines, style, mode);
    }
}

internal VIM_OPERATOR(vim_toggle_line_comment_range_indent_style) {
    Vim_Comment_Style style       = VimCommentStyle_RangeIndent;
    Vim_Toggle_Comment_Mode mode  = VimToggleComment_Auto;

    b32 comment_empty_lines = false; // @TODO: Expose to user

    if (selection.kind) {
        Range_i64 line_range = Ii64(selection.range.first.line, selection.range.one_past_last.line);
        vim_toggle_line_comment_range(app, buffer, line_range, vim_cpp_line_comment, comment_empty_lines, style, mode);
    } else {
        i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
        vim_toggle_line_comment_range(app, buffer, Ii64(line), vim_cpp_line_comment, comment_empty_lines, style, mode);
    }
}

internal VIM_OPERATOR(vim_toggle_line_comment_full_indent_style) {
    Vim_Comment_Style style       = VimCommentStyle_FullIndent;
    Vim_Toggle_Comment_Mode mode  = VimToggleComment_Auto;

    b32 comment_empty_lines = false; // @TODO: Expose to user

    if (selection.kind) {
        Range_i64 line_range = Ii64(selection.range.first.line, selection.range.one_past_last.line);
        vim_toggle_line_comment_range(app, buffer, line_range, vim_cpp_line_comment, comment_empty_lines, style, mode);
    } else {
        i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
        vim_toggle_line_comment_range(app, buffer, Ii64(line), vim_cpp_line_comment, comment_empty_lines, style, mode);
    }
}

function void vim_write_text(Application_Links *app, String_Const_u8 insert) {
    // NOTE: This function exists because I actually don't want it to do record merging
    if (insert.str != 0 && insert.size > 0){
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        if_view_has_highlighted_range_delete_range(app, view);
        
        i64 pos = view_get_cursor_pos(app, view);
        pos = view_get_character_legal_pos_from_pos(app, view, pos);
        
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

        if (buffer_replace_range(app, buffer, Ii64(pos), insert)) {
            view_set_cursor_and_preferred_x(app, view, seek_pos(pos + insert.size));
        }
    }
}

internal void vim_write_text_and_auto_indent_internal(Application_Links* app, String_Const_u8 insert, i64 reference_line = -1) {
    // TODO: This function is really a hack, and what I actually should do is edit the auto indentation stuff properly to do what I want.
    //       But that's a lot of work. And this at least kind of works with non-cpp files (if I actually used it for mapid_file).
    // NOTE: reference_line is a thing because mixed line endings are ruining my life.
    if (insert.str != 0 && insert.size > 0) {
        b32 do_auto_indent = false;
        b32 preprocessor = false;
        b32 open_new_line = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1) {
            switch (insert.str[i]) {
                case '}':
                case ':': {
                    do_auto_indent = true;
                } break;

                case '\n': {
                    do_auto_indent = true;
                    open_new_line = true;
                } break;

                case '#': {
                    do_auto_indent = true;
                    preprocessor = true;
                } break;
            }
        }
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        if (do_auto_indent) {
            Range_i64 pos = {};
            pos.min = view_get_cursor_pos(app, view);
            i64 line = reference_line;
            if (line < 1) {
                line = get_line_number_from_pos(app, buffer, pos.min);
            }
            vim_write_text(app, insert);
            pos.max = view_get_cursor_pos(app, view);

            if (open_new_line || preprocessor) {
                Scratch_Block scratch(app);
                String_Const_u8 line_string = push_buffer_line(app, scratch, buffer, line);
                i64 first_non_whitespace = string_find_first_non_whitespace(line_string);
                String_Const_u8 indent_string = string_prefix(line_string, first_non_whitespace);
                String_Const_u8 remainder_string = string_postfix(line_string, line_string.size - first_non_whitespace);
                if (open_new_line) {
                    b32 full_auto_indent = false;
                    if (!line_string.size || indent_string.size == line_string.size || line_string.str[0] == '#') {
                        full_auto_indent = true;
                    } else {
                        b32 line_contains_open_scope = false;
                        switch (line_string.str[line_string.size - 1]) {
                            case '{':
                            case '[':
                            case '(': {
                                line_contains_open_scope = true;
                            } break;
                        }
                        if (line_contains_open_scope) {
                            full_auto_indent = true;
                        }
                    }

                    if (full_auto_indent) {
                        auto_indent_buffer(app, buffer, pos);
                    } else {
                        i64 new_line = get_line_number_from_pos(app, buffer, pos.max);
                        i64 new_line_start_pos = get_line_start_pos(app, buffer, new_line);
                        buffer_replace_range(app, buffer, Ii64(new_line_start_pos), indent_string);
#if VIM_AUTO_LINE_COMMENTS
                        if (string_match(vim_cpp_line_comment, string_prefix(remainder_string, vim_cpp_line_comment.size))) {
                            vim_toggle_line_comment_range(app, buffer, Ii64(new_line), vim_cpp_line_comment, true, VimCommentStyle_FullIndent, VimToggleComment_Comment);
                            seek_end_of_textual_line(app);
                        }
#endif
                    }
                } else {
                    Assert(preprocessor);
                    i64 line_start = get_line_start_pos(app, buffer, line);
                    buffer_replace_range(app, buffer, Ii64_size(line_start, indent_string.size), SCu8());
                }
            } else {
                auto_indent_buffer(app, buffer, pos);
            }
            move_past_lead_whitespace(app, view, buffer);
        } else {
            vim_write_text(app, insert);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_move_line_down) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    move_line(app, buffer, line, Scan_Forward);
    move_down_textual(app);
}

CUSTOM_COMMAND_SIG(vim_move_up_textual)
CUSTOM_DOC("[vim] Moves up to the next line of actual text, regardless of line wrapping.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    // @TODO: Does view_compute_cursor deal in visual lines? Does that make sense? What's a line col anyway
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    i64 next_line = cursor.line - 1;
    view_set_cursor_and_preferred_x(app, view, seek_line_col(next_line, 1));
}

CUSTOM_COMMAND_SIG(vim_move_line_up) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    move_line(app, buffer, line, Scan_Backward);
    vim_move_up_textual(app);
}

internal VIM_TEXT_OBJECT(vim_text_object_isearch_repeat_forward) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);

    String_Const_u8 last_search = vim_read_register(app, &vim_state.last_search_register);
    result.range = vim_search_once_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view), last_search, vim_state.search_flags);
    
    vim_state.search_show_highlight = true;

    return result;
}

internal VIM_TEXT_OBJECT(vim_text_object_isearch_repeat_backward) {
    Vim_Text_Object_Result result = vim_text_object(start_pos);

    String_Const_u8 last_search = vim_read_register(app, &vim_state.last_search_register);
    result.range = vim_search_once_internal(app, view, buffer, -vim_state.search_direction, view_get_cursor_pos(app, view), last_search, vim_state.search_flags);
    
    vim_state.search_show_highlight = true;
    
    return result;
}

internal VIM_MOTION(vim_motion_page_top) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor start_cursor = view_compute_cursor(app, view, seek_pos(pos));

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);

    i64 line = scroll.position.line_number;
    Buffer_Cursor target_cursor = view_compute_cursor(app, view, seek_line_col(line, start_cursor.col));

    result.seek_pos = target_cursor.pos;
    return result;
}
                   
internal VIM_MOTION(vim_motion_page_mid) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));

    Rect_f32 region = view_get_buffer_region(app, view);
    Vec2_f32 p = (region.p1 - region.p0) * .5f;

    i64 target_pos = view_pos_from_xy(app, view, p);
    Buffer_Cursor target_cursor = view_compute_cursor(app, view, seek_pos(target_pos));
    target_cursor = view_compute_cursor(app, view, seek_line_col(target_cursor.line, cursor.col));

    result.seek_pos = target_cursor.pos;
    return result;
}

internal VIM_MOTION(vim_motion_page_bottom) {
    Vim_Motion_Result result = vim_motion_linewise(start_pos);

    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));

    Rect_f32 region = view_get_buffer_region(app, view);
    Vec2_f32 p = region.p1 - region.p0;

    i64 target_pos = view_pos_from_xy(app, view, p);
    Buffer_Cursor target_cursor = view_compute_cursor(app, view, seek_pos(target_pos));
    target_cursor = view_compute_cursor(app, view, seek_line_col(target_cursor.line, cursor.col));

    result.seek_pos = target_cursor.pos;
    return result;
}

internal b32 vim_get_operator_range(Vim_Operator_State* state, Range_i64* out_range, u32 flags = 0) {
    Application_Links* app = state->app;
    View_ID view = state->view;
    Buffer_ID buffer = state->buffer;

    b32 result = false;
    Range_i64 range = Ii64();

    if (state->op_count > 0) {
        result = true;
        if (state->selection.kind) {
            result = vim_selection_consume_line(app, buffer, &state->selection, &range, true);
            b32 trim_newlines = false;
            if (state->selection.kind == VimSelectionKind_Line) {
                trim_newlines = HasFlag(flags, VimOpFlag_ChangeBehaviour);
            } else if (state->selection.kind == VimSelectionKind_Block) {
                trim_newlines = true;
                if (result) {
                    state->op_count++;
                }
            }
            if (trim_newlines) {
                b32 did_trim = false;
                while (vim_character_is_newline(buffer_get_char(app, buffer, range.one_past_last - 1))) {
                    range.one_past_last = view_set_pos_by_character_delta(app, view, range.one_past_last, -1);
                    did_trim = true;
                }
                if (did_trim) {
                    ++range.one_past_last; // NOTE: Because we want to point one past the last included byte
                }
            }
        } else if (HasFlag(flags, VimOpFlag_QueryMotion)) {
            if (!state->motion && !state->text_object) {
                i64 count_query = vim_query_number(app);
                state->motion_count = cast(i32) clamp(1, count_query, VIM_MAXIMUM_OP_COUNT);
                state->motion_count_was_set = (count_query != 0);

                Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, true);
                if (query) {
                    if (query->kind == VimBindingKind_Motion) {
                        state->motion = query->motion;
                    } else if (query->kind == VimBindingKind_TextObject) {
                        state->text_object = query->text_object;
                    }
                }

                Vim_Command_Rep* rep = &vim_state.next_command_rep;
                if (rep->kind == VimCommandRep_Operator) {
                    rep->motion_count = state->motion_count;
                    rep->motion_count_was_set = state->motion_count_was_set;
                    rep->motion = state->motion;
                    rep->text_object = state->text_object;
                }
            }

            if (state->motion) {
                if (HasFlag(flags, VimOpFlag_ChangeBehaviour)) {
                    // Note: Vim's behaviour is a little inconsistent with some motions wnen using the change operator for the sake of intuitiveness.
                    if      (state->motion == vim_motion_word)               state->motion = vim_motion_word_end;
                    else if (state->motion == vim_motion_to_empty_line_up)   state->motion = vim_motion_prior_to_empty_line_up;
                    else if (state->motion == vim_motion_to_empty_line_down) state->motion = vim_motion_prior_to_empty_line_down;
                    else if (state->motion == vim_motion_to_empty_line_down) state->motion = vim_motion_prior_to_empty_line_down;
                }

                i64 pos = view_get_cursor_pos(app, view);

                vim_state.executing_queried_motion = true;
                Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, state->motion, pos, state->motion_count, state->motion_count_was_set);
                vim_state.executing_queried_motion = false;
                range = mr.range_;

                if (HasFlag(mr.flags, VimMotionFlag_InvalidMotion)) {
                    result = false;
                }
                if (HasFlag(mr.flags, VimMotionFlag_AlwaysSeek)) {
                    vim_seek_motion_result(app, view, buffer, mr);
                }
            } else if (state->text_object) {
                i64 pos = view_get_cursor_pos(app, view);

                vim_state.executing_queried_motion = true;
                Vim_Text_Object_Result tor = vim_execute_text_object(app, view, buffer, state->text_object, pos, state->motion_count, state->motion_count_was_set);
                vim_state.executing_queried_motion = false;

                range = tor.range;
            } else {
                result = false;
            }
        } else {
            i64 pos = view_get_cursor_pos(app, view);
            range = Ii64(pos, view_set_pos_by_character_delta(app, view, pos, 1));
        }

        if (result) {
            state->total_range = range_union(state->total_range, range);
        }

        state->op_count--;
    }

    if (out_range) *out_range = range;
    return result;
}

internal String_Const_u8 vim_cut_range(Application_Links* app, Arena* arena, View_ID view, Buffer_ID buffer, Range_i64 range) {
    String_Const_u8 result = push_buffer_range(app, arena, buffer, range);
    buffer_replace_range(app, buffer, range, SCu8());
    return result;
}

CUSTOM_COMMAND_SIG(vim_new_line_below) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);

    seek_end_of_textual_line(app);

    vim_write_text_and_auto_indent_internal(app, string_u8_litexpr("\n"), line);
    vim_enter_insert_mode(app);
}

CUSTOM_COMMAND_SIG(vim_new_line_above) {
    vim_move_up_textual(app);
    vim_new_line_below(app);
}

enum Vim_DCY {
    VimDCY_Delete,
    VimDCY_Change,
    VimDCY_Yank,
};

internal void vim_delete_change_or_yank(Application_Links* app, Vim_Operator_State* state, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, Vim_DCY mode, Vim_Operator* op, b32 query_motion, Vim_Motion* motion = 0) {
    // @TODO: This whole function is weird and stupid...
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    String_Const_u8 eol = (*eol_setting == LineEndingKind_CRLF) ? SCu8("\r\n") : SCu8("\n");

    b32 insert_final_newline = false;
    i64 prev_line = 0;

    b32 did_act = false;
    Range_i64 range = {};

    u32 op_flags = query_motion ? VimOpFlag_QueryMotion : 0;
    if (mode == VimDCY_Change) op_flags |= VimOpFlag_ChangeBehaviour;

    Scratch_Block scratch(app);
    List_String_Const_u8 yank_string_list = {};

    if (motion) state->motion = motion;
    while (vim_get_operator_range(state, &range, op_flags)) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);

        if (!did_act && range_size(range) > 0) {
            did_act = true;
        }

        i64 line = line_range.min;
        if (prev_line && prev_line != line) {
            insert_final_newline = true;
            string_list_push(scratch, &yank_string_list, eol);
        }

        String_Const_u8 yank_string = SCu8();

        if (mode == VimDCY_Delete || mode == VimDCY_Change) {
            yank_string = vim_cut_range(app, scratch, view, buffer, range);
        } else {
            yank_string = push_buffer_range(app, scratch, buffer, range);
        }

        string_list_push(scratch, &yank_string_list, yank_string);

        prev_line = line;
    }

    if (insert_final_newline) {
        string_list_push(scratch, &yank_string_list, eol);
    }

    String_Const_u8 yank_string = string_list_flatten(scratch, yank_string_list, StringFill_NoTerminate);
    if (yank_string.size) {
        b32 from_block_copy = (selection.kind == VimSelectionKind_Block);
        vim_write_register(app, vim_state.active_register, yank_string, from_block_copy);
    }

    b32 set_cursor = true;
    if (mode == VimDCY_Change && did_act) {
        if (selection.kind == VimSelectionKind_Line || selection.kind == VimSelectionKind_Block) {
            vim_enter_visual_insert_mode(app);
        } else {
            // @TODO: This still seems like a gross solution
            if (state->text_object == vim_text_object_inner_scope ||
                state->text_object == vim_text_object_inner_paren ||
                state->text_object == vim_text_object_line)
            {
                set_cursor = false;
                vim_new_line_above(app);
            }
            vim_enter_insert_mode(app);
        }
    }

    if (set_cursor && did_act && mode != VimDCY_Yank) {
        view_set_cursor_and_preferred_x(app, view, seek_pos(state->total_range.min));
    }
}

internal VIM_OPERATOR(vim_delete) {
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Delete, vim_delete, true);
}

internal VIM_OPERATOR(vim_delete_character) {
    // @TODO: At the end of the line, this will delete the newline rather than step back and delete the last character on the line, contrary to vim's behaviour.
    //        It's a little annoying, so I want to make sure to get it consistent with vim.
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Delete, vim_delete_character, false);
}

internal VIM_OPERATOR(vim_change) {
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Change, vim_change, true);
}

internal VIM_OPERATOR(vim_yank) {
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Yank, vim_yank, true);
}

internal VIM_OPERATOR(vim_delete_eol) {
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Delete, vim_delete, false, vim_motion_line_end_textual);
}

internal VIM_OPERATOR(vim_change_eol) {
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Change, vim_change, false, vim_motion_line_end_textual);
}

internal VIM_OPERATOR(vim_yank_eol) {
    // NOTE: This is vim's default behaviour (cuz it's vi compatible)
    // vim_delete_change_or_yank(app, state, view, buffer, selection, count, vim_yank, false, vim_motion_enclose_line_textual);
    vim_delete_change_or_yank(app, state, view, buffer, selection, count, VimDCY_Yank, vim_yank, false, vim_motion_line_end_textual);
}

internal VIM_OPERATOR(vim_replace) {
    String_Const_u8 replace_char = vim_get_next_writable(app);

    Range_i64 range = {};
    while (vim_get_operator_range(state, &range)) {
        for (i64 replace_cursor = range.min; replace_cursor < range.max; replace_cursor++) {
            buffer_replace_range(app, buffer, Ii64(replace_cursor, view_set_pos_by_character_delta(app, view, replace_cursor, 1)), replace_char);
        }
    }
}

internal void vim_paste_internal(Application_Links* app, b32 post_cursor) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);

    String_Const_u8 paste_string = vim_read_register(app, vim_state.active_register);

    if (paste_string.size > 0) {
        Scratch_Block scratch(app);

        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        History_Group history = history_group_begin(app, buffer);

        Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);

        b32 do_block_paste = HasFlag(vim_state.active_register->flags, VimRegisterFlag_FromBlockCopy) || (selection.kind == VimSelectionKind_Block);

        if (do_block_paste) {
            Range_i64 replace_range = {};

            // @TODO: This shouldn't really try to guess, instead it should just split on both \r\n and \n
            Line_Ending_Kind eol_kind = string_guess_line_ending_kind(paste_string);
            String_Const_u8 newline_needle = SCu8("\n");
            switch (eol_kind) {
                case LineEndingKind_CRLF: { newline_needle = string_u8_litexpr("\r\n"); } break;
                case LineEndingKind_LF:   { newline_needle = string_u8_litexpr("\n"); } break;
            }

            i64 visual_block_line_count = (selection.range.one_past_last.line - selection.range.first.line);

            List_String_Const_u8 paste_lines = string_split_needle(scratch, paste_string, newline_needle);
            List_String_Const_u8 replaced_string_list = {};

            u64 longest_line_size = 0;
            u32 newline_count = 0;
            for (Node_String_Const_u8* line = paste_lines.first; line; line = line->next) {
                longest_line_size = Max(longest_line_size, line->string.size);
                if (string_match(line->string, newline_needle)) {
                    newline_count++;
                }
            }

            if (selection.kind == VimSelectionKind_None) {
                Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
                selection.kind                     = VimSelectionKind_Block;
                selection.range.first.line         = cursor.line;
                selection.range.one_past_last.line = cursor.line + Max(1, newline_count);
                selection.range.first.col          = selection.range.one_past_last.col = cursor.col + (post_cursor ? 1 : 0);
            }
            
            i64 post_paste_pos = buffer_compute_cursor(app, buffer, seek_line_col(selection.range.first.line, selection.range.first.col)).pos;
            b32 replaces_something = selection.range.first.col != selection.range.one_past_last.col;

            Node_String_Const_u8* line = paste_lines.first;
            while (vim_selection_consume_line(app, buffer, &selection, &replace_range, true)) {
                if (replaces_something) {
                    String_Const_u8 replaced_string = push_buffer_range(app, scratch, buffer, replace_range);
                    string_list_push(scratch, &replaced_string_list, replaced_string);
                    if (visual_block_line_count > 0) {
                        string_list_push(scratch, &replaced_string_list, newline_needle);
                    }
                }

                while (line && string_match(line->string, newline_needle)) {
                    line = line->next;
                }

                Range_i64 tail_range = Ii64(replace_range.min, get_line_end_pos_from_pos(app, buffer, replace_range.max));
                String_Const_u8 tail = push_buffer_range(app, scratch, buffer, tail_range);

                b32 tail_contains_non_whitespace_characters = false;
                for (u64 character_index = 0; character_index < tail.size; character_index++) {
                    u8 c = tail.str[character_index];
                    if (!character_is_whitespace(c)) {
                        tail_contains_non_whitespace_characters = true;
                        break;
                    }
                }

                String_Const_u8 line_string = SCu8();

                if (line) {
                    line_string = line->string;
                    line = line->next;

                    if (tail_contains_non_whitespace_characters) {
                        Assert(line_string.size <= longest_line_size);
                        i32 pad_spaces = cast(i32) (longest_line_size - line_string.size);
                        line_string = push_u8_stringf(scratch, "%.*s%*s", string_expand(line_string), pad_spaces, "");
                    }
                } else {
                    if (tail_contains_non_whitespace_characters) {
                        i32 pad_spaces = cast(i32) longest_line_size;
                        line_string = push_u8_stringf(scratch, "%*s", pad_spaces, "");
                    }
                }

                buffer_replace_range(app, buffer, replace_range, line_string);

                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                // @TODO: Figure out why these fades end up wrong sometimes
                buffer_post_fade(app, buffer, 0.667f, Ii64_size(replace_range.min, paste_string.size), argb);
            }
            view_set_cursor_and_preferred_x(app, view, seek_pos(post_paste_pos));
            String_Const_u8 replaced_string = string_list_flatten(scratch, replaced_string_list, StringFill_NoTerminate);
            if (replaced_string.size) {
                vim_write_register(app, vim_state.active_register, replaced_string);
            }
        } else {
            b32 contains_newlines = false;
            for (u64 character_index = 0; character_index < paste_string.size; character_index++) {
                if (vim_character_is_newline(paste_string.str[character_index])) {
                    contains_newlines = true;
                    break;
                }
            }

            Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
            if (post_cursor && contains_newlines) {
                view_set_cursor(app, view, seek_line_col(cursor.line + 1, 1));
            }

            cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
            if (post_cursor && !contains_newlines) {
                cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, cursor.col + 1));
            }

            Range_i64 replace_range = Ii64(cursor.pos);
            if (selection.kind) {
                vim_selection_consume_line(app, buffer, &selection, &replace_range, true);
            }

            String_Const_u8 replaced_string = push_buffer_range(app, scratch, buffer, replace_range);
            buffer_replace_range(app, buffer, replace_range, paste_string);
            if (contains_newlines) {
                i64 new_pos = get_pos_past_lead_whitespace(app, buffer, replace_range.min);
                view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
            } else {
                view_set_cursor_and_preferred_x(app, view, seek_pos(replace_range.min + paste_string.size - 1)); // @Unicode
            }
#if VIM_AUTO_INDENT_ON_PASTE
            auto_indent_buffer(app, buffer, Ii64_size(cursor.pos, paste_string.size - (contains_newlines ? 1 : 0)));
#endif
            
            if (replaced_string.size) {
                vim_write_register(app, vim_state.active_register, replaced_string);
            }

            ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
            // @TODO: Figure out why these fades end up wrong sometimes
            buffer_post_fade(app, buffer, 0.667f, Ii64_size(view_get_cursor_pos(app, view), paste_string.size), argb);
        }

        history_group_end(history);
    }
}

internal VIM_OPERATOR(vim_paste) {
    vim_paste_internal(app, true);
}

internal VIM_OPERATOR(vim_paste_pre_cursor) {
    vim_paste_internal(app, false);
}

internal VIM_OPERATOR(vim_lowercase) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
        Scratch_Block scratch(app);
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);

        string_mod_lower(string);

        buffer_replace_range(app, buffer, range, string);
    }
}

internal VIM_OPERATOR(vim_uppercase) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
        Scratch_Block scratch(app);
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);

        string_mod_upper(string);

        buffer_replace_range(app, buffer, range, string);
    }
}

internal VIM_OPERATOR(vim_auto_indent) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
        auto_indent_buffer(app, buffer, range);
    }
}

internal void vim_shift_indentation_of_line(Application_Links* app, Buffer_ID buffer, i64 line, Scan_Direction dir) {
    // @TODO: Support adding/removing indentation in the middle of lines (using visual block and whatnot)
    i32 indent_width = global_config.indent_width;

    Scratch_Block scratch(app);
    String_Const_u8 line_text = push_buffer_line(app, scratch, buffer, line);

    i64 line_start = get_line_start_pos(app, buffer, line);

    if (dir == Scan_Backward) {
        i32 characters_to_remove = 0;
        if (string_get_character(line_text, 0) == '\t') {
            characters_to_remove = 1;
        } else {
            while (characters_to_remove < indent_width && string_get_character(line_text, characters_to_remove) == ' ') {
                characters_to_remove++;
            }
        }
        buffer_replace_range(app, buffer, Ii64_size(line_start, characters_to_remove), SCu8());
    } else if (dir == Scan_Forward) {
        if (global_config.indent_with_tabs){
            buffer_replace_range(app, buffer, Ii64(line_start), string_u8_litexpr("\t"));
        } else {
            u8* str = push_array(scratch, u8, indent_width);
            block_fill_u8(str, indent_width, ' ');
            buffer_replace_range(app, buffer, Ii64(line_start), SCu8(str, indent_width));
        }
    } else {
        InvalidPath;
    }
}

internal VIM_OPERATOR(vim_indent) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
        if (range_size(line_range) == 0) {
            line_range.max++;
        }
        for (i64 line = line_range.min; line < line_range.max; line++) {
            vim_shift_indentation_of_line(app, buffer, line, Scan_Forward);
        }
    }
}

internal VIM_OPERATOR(vim_outdent) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
        if (range_size(line_range) == 0) {
            line_range.max++;
        }
        for (i64 line = line_range.min; line < line_range.max; line++) {
            vim_shift_indentation_of_line(app, buffer, line, Scan_Backward);
        }
    }
}

internal b32 vim_align_range(Application_Links* app, Buffer_ID buffer, Range_i64 line_range, Range_i64 col_range, String_Const_u8 align_target, b32 align_after_target) {
    // @TODO: If you want, you could rewrite this to use Batch_Edit and such or something. I didn't know it existed. Also just push a buffer line instead of buffer_seek
    if (col_range.min == 0 && col_range.max == 0) {
        col_range = Ii64(0, max_i64);
    }

    if (string_match(align_target, string_u8_litexpr(" "))) {
        align_after_target = true;
    }

    i64 line_range_size = range_size_inclusive(line_range);

    Scratch_Block scratch(app);
    Buffer_Cursor* align_cursors = push_array(scratch, Buffer_Cursor, line_range_size);
    Buffer_Cursor furthest_align_cursor = { -1, -1, -1 };

    b32 found_align_target = false;
    b32 did_align = false;

    for (i64 line_index = 0; line_index < line_range_size; line_index++) {
        i64 line_number = line_range.min + line_index;

        Range_i64 col_pos_range = Ii64(buffer_compute_cursor(app, buffer, seek_line_col(line_number, col_range.min)).pos,
                                       buffer_compute_cursor(app, buffer, seek_line_col(line_number, col_range.max)).pos);
        Range_i64 seek_range = range_intersect(get_line_pos_range(app, buffer, line_number), col_pos_range);

        i64 align_pos = -1;
        String_Match match = buffer_seek_string(app, buffer, align_target, Scan_Forward, seek_range.min);
        if (match.buffer == buffer && HasFlag(match.flags, StringMatch_CaseSensitive)) {
            if (align_after_target) {
                // NOTE: Offset the seek position to skip over the range of the matched string, just in case for some reason
                // the user asked to align a string with spaces in it, I guess.
                i64 range_delta = range_size(match.range) - 1;
                match.range.min += range_delta;
                match.range.max += range_delta;
                match = buffer_seek_character_class(app, buffer, &character_predicate_non_whitespace, Scan_Forward, match.range.first);
            }

            if (match.range.first < seek_range.max) {
                align_pos = match.range.first;
            }
        }

        if (align_pos != -1) {
            Buffer_Cursor align_cursor = buffer_compute_cursor(app, buffer, seek_pos(align_pos));
            align_cursors[line_index] = align_cursor;

            found_align_target = true;

            if (furthest_align_cursor.col != -1) {
                if (align_cursor.col != furthest_align_cursor.col) {
                    // NOTE: Well, we didn't align yet. But we will.
                    did_align = true;
                }
            }

            if (align_cursor.col > furthest_align_cursor.col) {
                furthest_align_cursor = align_cursor;
            }
        } else {
            align_cursors[line_index] = { -1, -1, -1 };
        }
    }

    if (did_align) {
        History_Group history = history_group_begin(app, buffer);
        for (i64 line_index = 0; line_index < line_range_size; line_index++) {
            Buffer_Cursor align_cursor = align_cursors[line_index];

            if (align_cursor.col != -1) {
                // Note: We have to recompute the cursor because by inserting indentation we adjust the absolute pos of the alignment point
                align_cursor = buffer_compute_cursor(app, buffer, seek_line_col(align_cursor.line, align_cursor.col));

                if (!align_after_target) {
                    i64 post_target = align_cursor.pos + align_target.size;
                    String_Match match = buffer_seek_character_class(app, buffer, &character_predicate_non_whitespace, Scan_Forward, post_target);
                    if (match.buffer == buffer) {
                        buffer_replace_range(app, buffer, Ii64(post_target + 1, match.range.min), SCu8());
                    }
                }

                i64 col_delta = furthest_align_cursor.col - align_cursor.col;
                if (col_delta > 0) {
                    String_Const_u8 align_string = push_u8_stringf(scratch, "%*s", col_delta, "");
                    buffer_replace_range(app, buffer, Ii64(align_cursor.pos), align_string);
                }
            }
        }
        history_group_end(history);
    } else if (found_align_target) {
        did_align |= vim_align_range(app, buffer, line_range, Ii64(furthest_align_cursor.col + 1, col_range.max), align_target, align_after_target);
    }

    return did_align;
}

internal void vim_align_internal(Application_Links* app, Vim_Operator_State* state, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, b32 align_right) {
    if (selection.kind == VimSelectionKind_Block || selection.kind == VimSelectionKind_Line) {
        String_Const_u8 align_target = vim_get_next_writable(app);
        Range_i64 line_range = Ii64(selection.range.first.line, selection.range.one_past_last.line);
        Range_i64 col_range  = Ii64(selection.range.first.col , selection.range.one_past_last.col );
        for (i32 i = 0; i < count; i++) {
            vim_align_range(app, buffer, line_range, col_range, align_target, align_right);
        }
    } else {
        String_Const_u8 align_target = SCu8();
        Range_i64 range = {};
        while (vim_get_operator_range(state, &range, VimOpFlag_QueryMotion)) {
            if (!align_target.size) {
                align_target = vim_get_next_writable(app);
            }
            Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
            Range_i64 col_range  = Ii64();
            vim_align_range(app, buffer, line_range, col_range, align_target, align_right);
        }
    }
}

internal VIM_OPERATOR(vim_align) {
    vim_align_internal(app, state, view, buffer, selection, count, false);
}

internal VIM_OPERATOR(vim_align_right) {
    vim_align_internal(app, state, view, buffer, selection, count, true);
}

internal void vim_join_line_internal(Application_Links* app, Buffer_ID buffer, i64 line) {
    i64 next_line = line + 1;
    if (is_valid_line(app, buffer, next_line)) {
        vim_toggle_line_comment_range(app, buffer, Ii64(next_line), vim_cpp_line_comment, false, VimCommentStyle_RangeIndent, VimToggleComment_Uncomment);
        i64 line_end_pos = get_line_end_pos(app, buffer, line);
        i64 join_pos = get_pos_past_lead_whitespace_from_line_number(app, buffer, next_line);
        buffer_replace_range(app, buffer, Ii64(line_end_pos, join_pos), SCu8(" "));
    }
}

internal VIM_OPERATOR(vim_join_line) {
    if (selection.kind) {
        Range_i64 range = {};
        while (vim_get_operator_range(state, &range)) {
            Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
            for (i64 line_index = 0; line_index < Max(1, range_size(line_range) - 1); line_index++) {
                vim_join_line_internal(app, buffer, line_range.min);
            }
        }
    } else {
        i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
        vim_join_line_internal(app, buffer, line);
    }
}

internal void vim_miblo_internal(Application_Links* app, Buffer_ID buffer, Range_i64 range, i64 delta, i64* rolling_delta = 0) {
    // @Incomplete: This doesn't handle negative numbers.
    for (i64 scan_pos = range.min; scan_pos <= range.max;) {
        Miblo_Number_Info number = {};
        if (get_numeric_at_cursor(app, buffer, scan_pos, &number)) {
            Scratch_Block scratch(app);
            if (rolling_delta) *rolling_delta += delta;
            String_Const_u8 str = push_u8_stringf(scratch, "%lld", number.x + (rolling_delta ? *rolling_delta : delta));
            buffer_replace_range(app, buffer, number.range, str);
            scan_pos = number.range.max;
        } else {
            scan_pos++;
        }
    }
}

internal VIM_OPERATOR(vim_miblo_increment) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range)) {
        vim_miblo_internal(app, buffer, range, 1);
    }
}

internal VIM_OPERATOR(vim_miblo_decrement) {
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range)) {
        vim_miblo_internal(app, buffer, range, -1);
    }
}

internal VIM_OPERATOR(vim_miblo_increment_sequence) {
    i64 rolling_delta = 0;
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range)) {
        vim_miblo_internal(app, buffer, range, 1, &rolling_delta);
    }
}

internal VIM_OPERATOR(vim_miblo_decrement_sequence) {
    i64 rolling_delta = 0;
    Range_i64 range = {};
    while (vim_get_operator_range(state, &range)) {
        vim_miblo_internal(app, buffer, range, -1, &rolling_delta);
    }
}

CUSTOM_COMMAND_SIG(vim_repeat_command) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    View_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    History_Group rep_history = history_group_begin(app, buffer);
    
    if (!buffer_exists(app, buffer)) return;
    
    vim_state.playing_back_command = true;

#if VIM_PRINT_COMMANDS
    print_message(app, string_u8_litexpr("Repeating Command.\n"));
#endif

    Vim_Visual_Selection selection = {};
    Vim_Command_Rep* rep = &vim_state.command_rep;
    switch (rep->kind) {
        case VimCommandRep_Operator: {
            selection = rep->selection;

            Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
            selection.range.first.line         += cursor.line;
            selection.range.first.col          += cursor.col;
            selection.range.one_past_last.line += cursor.line;
            selection.range.one_past_last.col  += cursor.col;
            
            // @TODO: I'm pretty sure nobody uses the selection range .pos, they shouldn't (should probably stop using Buffer_Range...),
            // but if they did it'd be wrong cuz I'm not recomputing it.

            Vim_Operator_State op_state = {};
            op_state.app          = app;
            op_state.view         = view;
            op_state.buffer       = buffer;
            op_state.op_count     = rep->count;
            op_state.op           = rep->op;
            op_state.motion_count = rep->motion_count;
            op_state.motion       = rep->motion;
            op_state.text_object  = rep->text_object;
            op_state.selection    = selection;
            op_state.total_range  = Ii64_neg_inf;

            vim_set_selection(app, view, selection);

            vim_state.active_register = rep->reg;
            rep->op(app, &op_state, view, buffer, selection, rep->count, rep->count_was_set);
            vim_state.active_register = &vim_state.VIM_DEFAULT_REGISTER;
        } break;
        
        case VimCommandRep_4CoderCommand: {
            for (i32 i = 0; i < Min(rep->count, VIM_MAXIMUM_OP_COUNT); i++) {
                rep->fcoder_command(app);
            }
        } break;
    }

    if (is_vim_insert_mode(vim_state.mode)) {
        Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
        i64 end_pos = cursor.pos;
        for (Vim_Insert_Node* node = vim_state.first_insert_node; node; node = node->next) {
            i64 col = cursor.col + node->rel_col;
            Buffer_Cursor insert_cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, col));
            Range_i64 replace_range = Ii64_size(insert_cursor.pos, node->text_backward.size);
            buffer_replace_range(app, buffer, replace_range, node->text_forward);
            end_pos = insert_cursor.pos + node->text_forward.size;
        }
        view_set_cursor_and_preferred_x(app, view, seek_pos(end_pos));
    }
    
    if (is_vim_visual_mode(vim_state.mode)) {
        view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.range.first.line, selection.range.first.col));
    }
    
    vim_enter_normal_mode(app);
    
    vim_state.playing_back_command = false;

    history_group_end(rep_history);
}

internal b32 vim_split_window_internal(Application_Links* app, View_ID view, Dimension dim) {
    b32 result = false;

    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Panel_ID panel = view_get_panel(app, view);
    if (panel) {
        panel_split(app, panel, dim);
        Panel_ID new_panel = panel_get_child(app, panel, Side_Max);
        if (new_panel) {
            View_ID new_panel_view = panel_get_view(app, new_panel, Access_ReadVisible);
            view_set_buffer(app, new_panel_view, buffer, 0);
            result = true;
        }
        Panel_ID old_panel = panel_get_child(app, panel, Side_Min);
        if (old_panel) {
            View_ID old_panel_view = panel_get_view(app, old_panel, Access_ReadVisible);
            view_set_active(app, old_panel_view);
        }
    }

    return result;
}

CUSTOM_COMMAND_SIG(vim_split_window_horizontal) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    vim_split_window_internal(app, view, Dimension_Y);
}

CUSTOM_COMMAND_SIG(vim_split_window_vertical) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    vim_split_window_internal(app, view, Dimension_X);
}

CUSTOM_COMMAND_SIG(vim_open_file_in_quotes_in_same_window)
CUSTOM_DOC("[vim] Reads a filename from surrounding '\"' characters and attempts to open the corresponding file in the same window.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer_exists(app, buffer)){
        Scratch_Block scratch(app);

        i64 pos = view_get_cursor_pos(app, view);

        Range_i64 range = enclose_pos_inside_quotes(app, buffer, pos);

        String_Const_u8 quoted_name = push_buffer_range(app, scratch, buffer, range);

        String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
        String_Const_u8 path = string_remove_last_folder(file_name);

        if (character_is_slash(string_get_character(path, path.size - 1))) {
            path = string_chop(path, 1);
        }

        String_Const_u8 new_file_name = push_u8_stringf(scratch, "%.*s/%.*s", string_expand(path), string_expand(quoted_name));

        if (view_open_file(app, view, new_file_name, true)){
            view_set_active(app, view);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_undo)
CUSTOM_DOC("[vim] Advances backwards through the undo history of the current buffer.")
{
    // @TODO: vim_echo some stuff
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    if (current > 0) {
        Record_Info record = buffer_history_get_record_info(app, buffer, current);
        i64 new_position = record.single_first;
        if (record.kind == RecordKind_Group) {
            Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, current, 0);
            new_position = sub_record.single_first;
        }
        buffer_history_set_current_state_index(app, buffer, current - 1);
        view_set_cursor_and_preferred_x(app, view, seek_pos(new_position));
    }
}

CUSTOM_COMMAND_SIG(vim_redo)
CUSTOM_DOC("[vim] Advances forwards through the undo history of the current buffer.")
{
    // @TODO: vim_echo some stuff
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    History_Record_Index current = buffer_history_get_current_state_index(app, buffer);
    History_Record_Index max_index = buffer_history_get_max_record_index(app, buffer);
    if (current < max_index) {
        History_Record_Index index = current + 1;
        Record_Info record = buffer_history_get_record_info(app, buffer, index);
        i64 new_position = record.single_first;
        if (record.kind == RecordKind_Group) {
            Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, index, 0);
            new_position = sub_record.single_first;
        }
        buffer_history_set_current_state_index(app, buffer, index);
        view_set_cursor_and_preferred_x(app, view, seek_pos(new_position));
    }
}

CUSTOM_COMMAND_SIG(vim_page_up) {
#if VIM_PAGE_MOVE_LOGS_JUMP_HISTORY
    vim_log_jump_history(app);
#endif
    page_up(app);
}

CUSTOM_COMMAND_SIG(vim_page_down) {
#if VIM_PAGE_MOVE_LOGS_JUMP_HISTORY
    vim_log_jump_history(app);
#endif
    page_down(app);
}

CUSTOM_COMMAND_SIG(vim_half_page_up) {
    vim_log_jump_history(app);
    View_ID view = get_active_view(app, Access_ReadVisible);
    f32 page_jump = 0.5f*get_page_jump(app, view);
    move_vertical_pixels(app, -page_jump);
}

CUSTOM_COMMAND_SIG(vim_half_page_down) {
    vim_log_jump_history(app);
    View_ID view = get_active_view(app, Access_ReadVisible);
    f32 page_jump = 0.5f*get_page_jump(app, view);
    move_vertical_pixels(app, page_jump);
}

CUSTOM_COMMAND_SIG(vim_previous_buffer) {
    View_ID view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);

    Buffer_ID buffer = vim_view->previous_buffer;

    if (buffer_exists(app, buffer)) {
        i64 pos = vim_view->pos_in_previous_buffer;

        Swap(Buffer_ID, vim_view->previous_buffer, vim_view->most_recent_known_buffer);
        Swap(i64, vim_view->pos_in_previous_buffer, vim_view->most_recent_known_pos);

        jump_to_location(app, view, buffer, pos);
    }
}

CUSTOM_COMMAND_SIG(vim_jump_to_definition_under_cursor) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (buffer_exists(app, buffer)) {
        vim_state.definition_stack_count = 0;

        Range_i64 under_cursor = enclose_pos_alpha_numeric_underscore_utf8(app, buffer, view_get_cursor_pos(app, view));

        Scratch_Block scratch(app);
        String_Const_u8 name = push_buffer_range(app, scratch, buffer, under_cursor);

        for (Buffer_ID search_buffer = get_buffer_next(app, 0, Access_Always);
             search_buffer;
             search_buffer = get_buffer_next(app, search_buffer, Access_Always))
        {
            Code_Index_File* file = code_index_get_file(search_buffer);
            if (file) {
                for (i32 i = 0; i < file->note_array.count; i++) {
                    Code_Index_Note* note = file->note_array.ptrs[i];
                    if (string_match(note->text, name)) {
                        Tiny_Jump jump;
                        jump.buffer = search_buffer;
                        jump.pos = note->pos.first;
                        vim_state.definition_stack[vim_state.definition_stack_count++] = jump;
                        if (vim_state.definition_stack_count >= ArrayCount(vim_state.definition_stack)) {
                            break;
                        }
                    }
                }
            }
        }

        if (vim_state.definition_stack_count > 0) {
            vim_state.definition_stack_cursor = 0;
            Tiny_Jump first_jump = vim_state.definition_stack[vim_state.definition_stack_cursor];
            vim_log_jump_history(app);
            jump_to_location(app, view, first_jump.buffer, first_jump.pos);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_cycle_definitions_forward)
CUSTOM_DOC("Jump to next definition")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Tiny_Jump jump = vim_state.definition_stack[++vim_state.definition_stack_cursor % vim_state.definition_stack_count];
    vim_log_jump_history(app);
    jump_to_location(app, view, jump.buffer, jump.pos);
}

CUSTOM_COMMAND_SIG(vim_cycle_definitions_backward)
CUSTOM_DOC("Jump to previous definition")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Tiny_Jump jump = vim_state.definition_stack[--vim_state.definition_stack_cursor % vim_state.definition_stack_count];
    vim_log_jump_history(app);
    jump_to_location(app, view, jump.buffer, jump.pos);
}

CUSTOM_COMMAND_SIG(q)
CUSTOM_DOC("[vim] Closes the current panel, or 4coder if this is the last panel.")
{
    View_ID view = get_active_view(app, Access_Always);
    View_ID first_view = get_view_next(app, 0, Access_Always);
    if (get_view_next(app, first_view, Access_Always)) {
        view_close(app, view);
    } else {
        exit_4coder(app);
    }
}

CUSTOM_COMMAND_SIG(w)
CUSTOM_DOC("[vim] Saves the current buffer.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
    vim_echo(app, "Wrote file '%.*s' to disk.", string_expand(file_name));

    save(app);
}

CUSTOM_COMMAND_SIG(wq)
CUSTOM_DOC("[vim] Saves the current buffer and closes the current panel, or 4coder if this is the last panel.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
    vim_echo(app, "Wrote file '%.*s' to disk.", string_expand(file_name));

    save(app);
    q(app);
}

CUSTOM_COMMAND_SIG(qa)
CUSTOM_DOC("[vim] Closes 4coder.")
{
    exit_4coder(app);
}

CUSTOM_COMMAND_SIG(wqa)
CUSTOM_DOC("[vim] Saves all buffers and closes 4coder.")
{
    save_all_dirty_buffers(app);
    qa(app);
}

CUSTOM_COMMAND_SIG(e)
CUSTOM_DOC("[vim] Interactively open or create a new file.")
{
    interactive_open_or_new(app);
}

CUSTOM_COMMAND_SIG(b)
CUSTOM_DOC("[vim] Interactively switch to an open buffer.")
{
    interactive_switch_buffer(app);
}

CUSTOM_COMMAND_SIG(ta)
CUSTOM_DOC("[vim] Open jump-to-definition lister.")
{
    jump_to_definition(app);
}

CUSTOM_COMMAND_SIG(vim_start_mouse_select)
CUSTOM_DOC("[vim] Sets the cursor position and mark to the mouse position and enters normal mode.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);

    Mouse_State mouse = get_mouse_state(app);
    i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
#if VIM_FILE_BAR_ON_BOTTOM
    // @Hack
    // Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    // pos = view_compute_cursor(app, view, seek_line_col(cursor.line + 1, cursor.col)).pos;
#endif
    
    vim_enter_normal_mode(app);

    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    view_set_mark(app, view, seek_pos(pos));
}

CUSTOM_COMMAND_SIG(vim_mouse_drag)
CUSTOM_DOC("[vim] If the mouse left button is pressed, sets the cursor position to the mouse position, and enables the preferred visual mode.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);

    Mouse_State mouse = get_mouse_state(app);
    if (mouse.l) {
        i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
#if VIM_FILE_BAR_ON_BOTTOM
        // @Hack
        // Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
        // pos = view_compute_cursor(app, view, seek_line_col(cursor.line + 1, cursor.col)).pos;
#endif
        i64 mark_pos = view_get_mark_pos(app, view);
        if (pos != mark_pos) {
            // @TODO: I shouldn't really need to check this, I should just add a parameter to not toggle the mode
            if (!is_vim_visual_mode(vim_state.mode)) {
                Assert(is_vim_visual_mode(glue(VimMode_, VIM_MOUSE_SELECT_MODE)));
                vim_enter_mode(app, glue(VimMode_, VIM_MOUSE_SELECT_MODE));
            }
            view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
        }
    }

    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(vim_write_text_abbrev_and_auto_indent)
CUSTOM_DOC("[vim] Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines, and substitutes words according to the abbreviations added with the vim_add_abbreviation function.")
{
    ProfileScope(app, "[vim] write, abbrev, and auto indent");

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    // Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    i64 pos = view_get_cursor_pos(app, view);

    if (vim_state.insert_sequence) {
        if (pos != vim_state.insert_sequence_pos) {
            vim_state.insert_sequence = false;
        }
    }

    User_Input in = get_current_input(app);
    String_Const_u8 insert = to_writable(&in);

    if (sizeof(VIM_ESCAPE_SEQUENCE) >= 3) {
        u8 this_char = insert.size > 0 ? insert.str[0] : 0;
        u8 prev_char = vim_state.prev_insert_char;
        vim_state.prev_insert_char = this_char;
        if (this_char &&
            prev_char == VIM_ESCAPE_SEQUENCE[0] &&
            this_char == VIM_ESCAPE_SEQUENCE[1])
        {
            backspace_char(app);
            vim_enter_normal_mode_escape(app);
            return;
        }
    }

    if (insert.str != 0 && insert.size > 0) {
        b32 do_abbrev = false;
        for (u64 i = 0; i < insert.size; i += 1) {
            if (!character_is_alpha_numeric(insert.str[i])) {
                do_abbrev = true;
                break;
            }
        }

        if (do_abbrev) {
            if (vim_state.insert_sequence) {
                vim_apply_abbreviations_for_range(app, view, Ii64(vim_state.insert_sequence_start_pos, vim_state.insert_sequence_pos));
                vim_state.insert_sequence = false;
            }
        } else if (!vim_state.insert_sequence) {
            vim_state.insert_sequence = true;
            vim_state.insert_sequence_pos = vim_state.insert_sequence_start_pos = pos;
        }
    }

    vim_write_text_and_auto_indent_internal(app, insert);
    vim_state.insert_sequence_pos = view_get_cursor_pos(app, view);
}

CUSTOM_COMMAND_SIG(vim_backspace_char) {
    backspace_char(app);

    View_ID view = get_active_view(app, Access_ReadWriteVisible);

    vim_state.insert_sequence_pos = view_get_cursor_pos(app, view);
}

internal Vim_Key_Binding* vim_add_binding(Vim_Binding_Map* map, Vim_Key_Sequence binding_sequence, b32 clear = true) {
    Vim_Key_Binding* result = 0;

    if (binding_sequence.count > 0) {
        Vim_Key key1 = binding_sequence.keys[0];
        result = vim_make_or_retrieve_binding(map, key1);

        for (i32 key_index = 1; key_index < binding_sequence.count; key_index++) {
            Vim_Key next_key = binding_sequence.keys[key_index];
            if (result->kind != VimBindingKind_Map) {
                result->kind = VimBindingKind_Map;
                result->map = vim_new_binding_map(VIM_DEFAULT_NESTED_BINDING_MAP_SLOT_COUNT);
            }

            result = vim_make_or_retrieve_binding(result->map, next_key);
        }
    }

    if (result && clear) {
        block_zero_struct(result);
    }

    return result;
}

#define vim_bind_motion(map, motion, key1, ...) vim_bind_motion_(map, motion, string_u8_litexpr(#motion), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_motion_(Vim_Binding_Map* map, Vim_Motion* motion, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_add_binding(map, sequence);
    bind->kind = VimBindingKind_Motion;
    bind->description = description;
    bind->motion = motion;
    return bind;
}

#define vim_bind_text_object(map, text_object, key1, ...) vim_bind_text_object_(map, text_object, string_u8_litexpr(#text_object), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_text_object_(Vim_Binding_Map* map, Vim_Text_Object* text_object, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_add_binding(map, sequence);
    bind->kind = VimBindingKind_TextObject;
    bind->description = description;
    bind->text_object = text_object;
    return bind;
}

#define vim_bind_operator(map, op, key1, ...) vim_bind_operator_(map, op, string_u8_litexpr(#op), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_operator_(Vim_Binding_Map* map, Vim_Operator* op, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_add_binding(map, sequence);
    bind->kind = VimBindingKind_Operator;
    bind->description = description;
    bind->flags |= VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly;
    bind->op = op;
    return bind;
}

#define vim_bind_fcoder_command(map, cmd, key1, ...) vim_bind_fcoder_command_(map, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_fcoder_command_(Vim_Binding_Map* map, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_add_binding(map, sequence);
    bind->kind = VimBindingKind_4CoderCommand;
    bind->description = description;
    bind->fcoder_command = fcoder_command;
    return bind;
}

#define vim_bind_text_command(map, cmd, key1, ...) vim_bind_text_command_(map, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_text_command_(Vim_Binding_Map* map, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_bind_fcoder_command_(map, fcoder_command, description, sequence);
    bind->flags |= VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly;
    return bind;
}

#define vim_bind(map, cmd, key1, ...) vim_bind_(map, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_bind_(Vim_Binding_Map* map, Vim_Motion* motion, String_Const_u8 description, Vim_Key_Sequence sequence) {
    return vim_bind_motion_(map, motion, description, sequence);
}

internal Vim_Key_Binding* vim_bind_(Vim_Binding_Map* map, Vim_Text_Object* text_object, String_Const_u8 description, Vim_Key_Sequence sequence) {
    return vim_bind_text_object_(map, text_object, description, sequence);
}

internal Vim_Key_Binding* vim_bind_(Vim_Binding_Map* map, Vim_Operator* op, String_Const_u8 description, Vim_Key_Sequence sequence) {
    return vim_bind_operator_(map, op, description, sequence);
}

internal Vim_Key_Binding* vim_bind_(Vim_Binding_Map* map, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key_Sequence sequence) {
    return vim_bind_fcoder_command_(map, fcoder_command, description, sequence);
}

#define vim_name_bind(map, description, key1, ...) vim_name_bind_(map, description, vim_key_sequence(key1, ##__VA_ARGS__))
internal Vim_Key_Binding* vim_name_bind_(Vim_Binding_Map* map, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_add_binding(map, sequence, false);
    bind->description = description;
    return bind;
}

#define vim_unbind(map, key1, ...) vim_unbind_(map, vim_key_sequence(key1, ##__VA_ARGS__))
internal void vim_unbind_(Vim_Binding_Map* map, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = 0;
    for (i32 key_index = 0; key_index < sequence.count; key_index++) {
        Vim_Key key = sequence.keys[key_index];
        bind = vim_retrieve_binding(map, key);
#if 1
        Assert(bind);
#endif
        if (bind && bind->kind == VimBindingKind_Map) {
            map = bind->map;
        } else {
            break;
        }
    }
    if (bind && bind->kind) {
        block_zero_struct(bind);
    }
}

internal Vim_Binding_Map* vim_select_map_(Application_Links* app, Vim_Binding_Map* map) {
    if (!map->allocator) {
        *map = vim_make_binding_map(VIM_DEFAULT_BINDING_MAP_SLOT_COUNT);
    }
    return map;
}

#define VimMappingScope() Vim_Binding_Map* vim_map = 0
#define VimSelectMap(map) vim_map = vim_select_map_(app, &map)
#define VimAddParentMap(parent) vim_add_parent_binding_map(vim_map, &(parent))
#define VimBind(cmd, key1, ...) vim_bind_(vim_map, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, ##__VA_ARGS__))
#define VimUnbind(key1, ...) vim_unbind_(vim_map, vim_key_sequence(key1, ##__VA_ARGS__))
#define VimNameBind(description, key1, ...) vim_name_bind_(vim_map, description, vim_key_sequence(key1, ##__VA_ARGS__))

//
// Hooks
//

function void vim_draw_character_block_selection(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, Range_i64 range, f32 roundness, ARGB_Color color) {
    if (range.first < range.one_past_last) {
        i64 i = range.first;
        Rect_f32 first_rect = text_layout_character_on_screen(app, layout, i);
        first_rect.y0 -= 1.0f;
        first_rect.y1 += 1.0f;
        i += 1;
        Range_f32 y = rect_range_y(first_rect);
        Range_f32 x = rect_range_x(first_rect);
        for (; i < range.one_past_last; i += 1) {
            i64 line = get_line_number_from_pos(app, buffer, i);
            if (!line_is_blank(app, buffer, line) && vim_character_is_newline(buffer_get_char(app, buffer, i))) {
                continue;
            }
            Rect_f32 rect = text_layout_character_on_screen(app, layout, i);
            rect.y0 -= 1.0f;
            rect.y1 += 1.0f;
            if (rect.x0 < rect.x1 && rect.y0 < rect.y1){
                Range_f32 new_y = rect_range_y(rect);
                Range_f32 new_x = rect_range_x(rect);
                b32 joinable = false;
                if (new_y == y && (range_overlap(x, new_x) || x.max == new_x.min || new_x.max == x.min)) {
                    joinable = true;
                }

                if (!joinable) {
                    Rect_f32 rect_to_draw = Rf32(x, y);
                    draw_rectangle(app, rect_to_draw, roundness, color);
                    y = new_y;
                    x = new_x;
                } else {
                    x = range_union(x, new_x);
                }
            }
        }
        draw_rectangle(app, Rf32(x, y), roundness, color);
    }
}

function void vim_draw_character_block_selection(Application_Links *app, Buffer_ID buffer, Text_Layout_ID layout, Range_i64 range, f32 roundness, FColor color) {
    ARGB_Color argb = fcolor_resolve(color);
    vim_draw_character_block_selection(app, buffer, layout, range, roundness, argb);
}

function void vim_draw_cursor(Application_Links *app, View_ID view, b32 is_active_view, Buffer_ID buffer, Text_Layout_ID text_layout_id, f32 roundness, f32 outline_thickness, Vim_Mode mode) {
    Managed_ID cursor_color = defcolor_cursor;

#if VIM_USE_CUSTOM_COLORS
    switch (mode) {
        case VimMode_Normal: {
            cursor_color = defcolor_vim_cursor_normal;
        } break;

        case VimMode_VisualInsert:
        case VimMode_Insert: {
            cursor_color = defcolor_vim_cursor_insert;
        } break;

        case VimMode_Visual:
        case VimMode_VisualLine:
        case VimMode_VisualBlock: {
            cursor_color = defcolor_vim_cursor_visual;
        } break;
    }
#endif

    i32 cursor_sub_id = default_cursor_sub_id();

    i64 cursor_pos = view_get_cursor_pos(app, view);
    if (is_active_view) {
        if (is_vim_insert_mode(mode)) {
            draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(cursor_color, cursor_sub_id));
        } else {
            Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
            Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);

            if (selection.kind) {
                Range_i64 range = {};
                while (vim_selection_consume_line(app, buffer, &selection, &range, true)) {
                    // @TODO: Even if I limit the drawing to the visible range, this slows 4coder to a crawl if you select a big amount of text...
                    range = range_intersect(range, visible_range);
                    vim_draw_character_block_selection(app, buffer, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
                    paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
                }
            }

            vim_draw_character_block_selection(app, buffer, text_layout_id, Ii64(cursor_pos, cursor_pos + 1), roundness, fcolor_id(cursor_color, cursor_sub_id));
            paint_text_color_pos(app, text_layout_id, cursor_pos, fcolor_id(defcolor_at_cursor));
        }
    } else {
        /* Draw nothing at all... */
    }
}

function void vim_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer, Text_Layout_ID text_layout_id, Rect_f32 rect) {
    ProfileScope(app, "render buffer");

    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);

    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        draw_cpp_token_colors(app, text_layout_id, &token_array);

        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                { string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0) },
                { string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1) },
            };
            draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
        }
    }
    else{
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }

    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);

    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }

    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer, fcolor_id(defcolor_highlight_junk));
        }

        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
            }
        }
    }

    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }

    // NOTE(allen): Line highlight
    if (!is_vim_visual_mode(vim_state.mode) && global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
    }

    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (metrics.normal_advance*0.5f)*VIM_CURSOR_ROUNDNESS;
    f32 mark_thickness = 2.0f;

    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);

    if (vim_state.search_show_highlight) {
        // NOTE: Vim Search highlight
        i64 pos = visible_range.min;
        while (pos < visible_range.max) {
            Range_i64 highlight_range = vim_search_once_internal(app, view_id, buffer, Scan_Forward, pos, vim_state.last_search_register.string.string, vim_state.search_flags, true);
            if (!range_size(highlight_range)) {
                break;
            }
            vim_draw_character_block_selection(app, buffer, text_layout_id, highlight_range, cursor_roundness, fcolor_id(defcolor_highlight_white));
            pos = highlight_range.max;
        }
    }

#if VIM_USE_CHARACTER_SEEK_HIGHLIGHTS
    if (is_active_view && vim_state.character_seek_show_highlight) {
        // NOTE: Vim Character Seek highlight
        i64 pos = view_get_cursor_pos(app, view_id);
        while (range_contains(visible_range, pos)) {
            Vim_Motion_Result mr = vim_motion_character_seek_internal(app, view_id, buffer, pos, SCu8(), vim_state.character_seek_highlight_dir, vim_state.most_recent_character_seek_till);
            if (mr.seek_pos == pos) {
                break;
            }
            Range_i64 range = Ii64_size(mr.seek_pos, vim_state.most_recent_character_seek.size);
#if VIM_USE_CUSTOM_COLORS
            FColor highlight_color = fcolor_id(defcolor_vim_character_highlight);
#else
            FColor highlight_color = fcolor_id(defcolor_highlight);
#endif
            vim_draw_character_block_selection(app, buffer, text_layout_id, range, cursor_roundness, highlight_color);
            paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
            pos = mr.seek_pos;
        }
    }
#endif

    // NOTE(allen): Cursor
    vim_draw_cursor(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, vim_state.mode);

    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer, view_id);

    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);

    draw_set_clip(app, prev_clip);
}

function void vim_draw_echo_bar(Application_Links *app, Face_ID face_id, Rect_f32 bar) {
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 digit_advance = face_metrics.decimal_digit_advance;

    draw_rectangle(app, bar, 0.0f, fcolor_resolve(fcolor_id(defcolor_back)));
    draw_string_oriented(app, face_id, fcolor_resolve(vim_state.echo_color), vim_state.echo_string.string, bar.p0 + V2f32(2.0f, 2.0f), 0, V2f32(1, 0));

    // if (vim_state.recording_macro) {
    //     push_fancy_stringf(scratch, &list, pop2_color, "   recording @%c", vim_state.most_recent_macro_register);
    // }

    f32 step_back = 16*digit_advance;
    draw_string_oriented(app, face_id, fcolor_resolve(fcolor_id(defcolor_text_default)), vim_state.chord_bar.string, V2f32(bar.x1 - step_back, bar.y0 + 2.0f), 0, V2f32(1, 0));
}

function void vim_whole_screen_render_caller(Application_Links *app, Frame_Info frame_info) {
#if VIM_USE_ECHO_BAR
    Rect_f32 region = global_get_screen_rectangle(app);
    Vec2_f32 center = rect_center(region);
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    
    Rect_f32_Pair bar_pair = rect_split_top_bottom_neg(region, line_height + 2.0f);
    vim_draw_echo_bar(app, face_id, bar_pair.max);
#endif
}

function Rect_f32 vim_draw_background_and_margin(Application_Links *app, View_ID view, ARGB_Color margin, ARGB_Color back, f32 width, f32 echo_bar_height) {
    Rect_f32 region = global_get_screen_rectangle(app);
    Rect_f32 view_rect = view_get_screen_rect(app, view);
#if VIM_USE_ECHO_BAR
    view_rect.y1 = Min(view_rect.y1, region.y1 - echo_bar_height);
#endif
    Rect_f32 inner = rect_inner(view_rect, width);
#if VIM_DRAW_PANEL_MARGINS == 1
    inner.x1 = view_rect.x1;
    inner.y0 = view_rect.y0;
    inner.y1 = view_rect.y1;
    if (region.x0 == view_rect.x0) {
        inner.x0 = view_rect.x0;
    }
#endif
#if VIM_DRAW_PANEL_MARGINS
    draw_rectangle(app, inner, 0.f, back);
    if (width > 0.f) {
        draw_margin(app, view_rect, inner, margin);
    }
    return inner;
#else
    draw_rectangle(app, view_rect, 0.f, back);
    return view_rect;
#endif
}

function Rect_f32 vim_draw_background_and_margin(Application_Links *app, View_ID view, b32 is_active_view, f32 echo_bar_height) {
#if VIM_DRAW_PANEL_MARGINS == 1
    FColor margin_color = get_panel_margin_color(UIHighlight_Active);
#else
    FColor margin_color = get_panel_margin_color(is_active_view ? UIHighlight_Active : UIHighlight_None);
#endif
    return vim_draw_background_and_margin(app, view, fcolor_resolve(margin_color), fcolor_resolve(fcolor_id(defcolor_back)), VIM_PANEL_MARGIN_THICKNESS, echo_bar_height);
}

function void vim_draw_file_bar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar) {
    Scratch_Block scratch(app);

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);

    Managed_ID bar_color = defcolor_bar;

#if VIM_USE_CUSTOM_COLORS
    if (vim_state.recording_macro) {
        bar_color = defcolor_vim_bar_recording_macro;
    } else {
        switch (vim_state.mode) {
            case VimMode_Normal: {
                bar_color = defcolor_vim_bar_normal;
            } break;

            case VimMode_VisualInsert:
            case VimMode_Insert: {
                bar_color = defcolor_vim_bar_insert;
            } break;

            case VimMode_Visual:
            case VimMode_VisualLine:
            case VimMode_VisualBlock: {
                bar_color = defcolor_vim_bar_visual;
            } break;
        }
    }
#endif

    draw_rectangle(app, bar, 0.f, fcolor_resolve(fcolor_id(bar_color)));

    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);

    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);

    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;

        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;

        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }

    u8 space[3];
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }

#if !VIM_USE_ECHO_BAR
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);

    if (is_active_view) {
        if (vim_state.echo_string.size) {
            push_fancy_string(scratch, &list, vim_state.echo_color, string_u8_litexpr("   << "));
            push_fancy_string(scratch, &list, vim_state.echo_color, vim_state.echo_string.string);
            push_fancy_string(scratch, &list, vim_state.echo_color, string_u8_litexpr(" >> "));
        }

        if (vim_state.recording_macro) {
            push_fancy_stringf(scratch, &list, pop2_color, "   recording @%c", vim_state.most_recent_macro_register);
        }
        push_fancy_stringf(scratch, &list, base_color, "   %.*s", string_expand(vim_state.chord_bar));
    }
#endif

    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

function void vim_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id) {
    ProfileScope(app, "default render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);

    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;

    Rect_f32 region = vim_draw_background_and_margin(app, view_id, is_active_view, line_height + 2.0f);
    Rect_f32 prev_clip = draw_set_clip(app, region);

    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
#if VIM_FILE_BAR_ON_BOTTOM
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        vim_draw_file_bar(app, view_id, buffer, face_id, pair.max);
        region = pair.min;
#else
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        vim_draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
#endif
    }

    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);

    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }

    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            for (i32 i = 0; i < query_bars.count; i += 1){
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
            }
        }
    }

    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }

    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }

    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);

    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }

    // NOTE(allen): draw the buffer
    vim_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);

    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

BUFFER_HOOK_SIG(vim_begin_buffer) {
    default_begin_buffer(app, buffer_id);

    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    Command_Map_ID* insert_map_id_ptr = scope_attachment(app, scope, vim_buffer_insert_map_id, Command_Map_ID);
    *insert_map_id_ptr = *map_id_ptr;
    *map_id_ptr = vim_mapid_normal;

    local_persist b32 warned_virtual_whitespace = false;
    if (!warned_virtual_whitespace && global_config.enable_virtual_whitespace) {
        vim_echo_alert(app, "Warning: Virtual Whitespace is not yet properly supported by 4coder_vimmish.");
        warned_virtual_whitespace = true;
    }

    return 0;
}

function void vim_tick(Application_Links *app, Frame_Info frame_info) {
    default_tick(app, frame_info);

    ProfileScope(app, "[vim] tick");

    if (vim_state.played_macro) {
        vim_state.played_macro = false;
        history_group_end(vim_state.macro_history);
    }

    View_ID active_view = get_active_view(app, Access_Always);
    View_ID active_buffer = view_get_buffer(app, active_view, Access_Always);
    u32 active_access_flags = buffer_get_access_flags(app, active_buffer);
    if (!HasFlag(active_access_flags, Access_Write) && is_vim_insert_mode(vim_state.mode)) {
        vim_enter_normal_mode_escape(app);
    }

    for (View_ID view = get_view_next(app, 0, Access_ReadVisible); view; view = get_view_next(app, view, Access_ReadVisible)) {
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
        if (buffer_exists(app, buffer)) {
            Buffer_ID scratch_buffer = get_buffer_by_name(app, string_u8_litexpr("*scratch*"), Access_Always);
            Managed_Scope view_scope = view_get_managed_scope(app, view);
            Vim_View_Attachment* vim_view = scope_attachment(app, view_scope, vim_view_attachment, Vim_View_Attachment);
            
            vim_select_mapid_for_mode(app, buffer, vim_state.mode);

            Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));

            if (vim_state.mode == VimMode_VisualBlock && vim_state.visual_block_force_to_line_end) {
                view_set_cursor_and_preferred_x(app, view, seek_line_col(cursor.line, max_i64));
            } else if (!is_vim_insert_mode(vim_state.mode)) {
                while (cursor.col > 1 && vim_character_is_newline(buffer_get_char(app, buffer, cursor.pos))) {
                    cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, cursor.col - 1));
                    view_set_cursor(app, view, seek_pos(cursor.pos));
                }
            }
            
            if (vim_view->most_recent_known_buffer &&
                vim_view->most_recent_known_buffer != buffer &&
                vim_view->most_recent_known_buffer != scratch_buffer)
            {
                vim_state.insert_sequence = false;
                if (vim_view->dont_log_this_buffer_jump) {
                    vim_view->dont_log_this_buffer_jump = false;
                } else {
                    vim_log_jump_history_internal(app, view, vim_view->most_recent_known_buffer, vim_view, vim_view->most_recent_known_pos);
                }
                vim_view->previous_buffer = vim_view->most_recent_known_buffer;
                vim_view->pos_in_previous_buffer = vim_view->most_recent_known_pos;
            }

            vim_view->most_recent_known_buffer = buffer;
            vim_view->most_recent_known_pos = cursor.pos;
        }
    }
}

CUSTOM_COMMAND_SIG(vim_default_view_handler)
CUSTOM_DOC("[vim] Input consumption loop for view behavior")
{
    Thread_Context *tctx = get_thread_context(app);
    Scratch_Block scratch(tctx);

    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        String_Const_u8 name = push_u8_stringf(scratch, "view %d", view);

        Profile_Global_List *list = get_core_profile_list(app);
        ProfileThreadName(tctx, list, name);

        View_Context ctx = view_current_context(app, view);
        ctx.mapping = &framework_mapping;
        ctx.map_id = mapid_global;
        view_alter_context(app, view, &ctx);
    }

    for (;;) {
        // NOTE: This is to make it easy for macros to know exactly where to end
        {
            Buffer_ID keyboard_log_buffer = get_keyboard_log_buffer(app);
            Buffer_Cursor cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_pos(buffer_get_size(app, keyboard_log_buffer)));
            vim_state.command_start_pos = cursor.pos;
        }

        // NOTE(allen): Get the binding from the buffer's current map
        User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
        ProfileScopeNamed(app, "[vim] before view input", view_input_profile);
        if (input.abort) {
            break;
        }

        Event_Property event_properties = get_event_properties(&input.event);

        if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0){
            continue;
        }

        View_ID view = get_this_ctx_view(app, Access_Always);
        Managed_Scope view_scope = view_get_managed_scope(app, view);

        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);

        Command_Map_ID* map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
        Command_Map_ID map_id = *map_id_ptr;

        Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, &input.event);
        
        i32 op_count = 1;
        b32 op_count_was_set = false;
        Vim_Key_Binding* vim_bind = 0;
        if (!is_vim_insert_mode(vim_state.mode)) {
            Vim_Binding_Map* map = vim_get_map_for_mode(vim_state.mode);

            if (map && map->allocator) {
                vim_begin_query(app, VimQuery_CurrentInput);

                vim_query_and_set_register(app, map);

                op_count = 1;
                op_count_was_set = false;
                if (!is_vim_insert_mode(vim_state.mode)) {
                    i64 queried_number = vim_query_number(app);
                    op_count = cast(i32) clamp(1, queried_number, max_i32);
                    op_count_was_set = (queried_number != 0);
                }

                vim_query_and_set_register(app, map);

                vim_bind = vim_query_binding(app, map, false);
            }
        }

        if (!binding.custom && !vim_bind) {
            // NOTE(allen): we don't have anything to do with this input,
            // leave it marked unhandled so that if there's a follow up
            // event it is not blocked.
            leave_current_input_unhandled(app);
        } else {
            // NOTE(allen): before the command is called do some book keeping
            Rewrite_Type *next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
            *next_rewrite = Rewrite_None;
            if (fcoder_mode == FCoderMode_NotepadLike) {
                for (View_ID view_it = get_view_next(app, 0, Access_Always);
                     view_it != 0;
                     view_it = get_view_next(app, view_it, Access_Always))
                {
                    Managed_Scope scope_it = view_get_managed_scope(app, view_it);
                    b32 *snap_mark_to_cursor = scope_attachment(app, scope_it, view_snap_mark_to_cursor, b32);
                    *snap_mark_to_cursor = true;
                }
            }

            ProfileCloseNow(view_input_profile);

            if (vim_bind) {
                Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);

                if (HasFlag(vim_bind->flags, VimBindingFlag_WriteOnly)) {
                    u32 buffer_flags = buffer_get_access_flags(app, buffer);
                    if (!HasFlag(buffer_flags, Access_Write)) {
                        return;
                    }
                }

                vim_begin_command(app, buffer, vim_bind, op_count, op_count_was_set, selection);

                switch (vim_bind->kind) {
                    case VimBindingKind_Motion: {
                        vim_state.character_seek_show_highlight = false;
                        
                        Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, vim_bind->motion, view_get_cursor_pos(app, view), op_count, op_count_was_set);
                        vim_seek_motion_result(app, view, buffer, mr);

                        if (HasFlag(mr.flags, VimMotionFlag_SetPreferredX)) {
                            vim_state.visual_block_force_to_line_end = HasFlag(mr.flags, VimMotionFlag_VisualBlockForceToLineEnd);
                        }
                    } break;

                    case VimBindingKind_TextObject: {
                        vim_state.character_seek_show_highlight = false;

                        Vim_Text_Object_Result tor = vim_execute_text_object(app, view, buffer, vim_bind->text_object, view_get_cursor_pos(app, view), op_count, op_count_was_set);

                        Vim_Mode new_mode = VimMode_Visual;
                        if (tor.style == VimRangeStyle_Linewise) {
                            new_mode = VimMode_VisualLine;
                        }

                        if (vim_state.mode != new_mode) {
                            vim_enter_mode(app, new_mode);
                        }

                        vim_select_range(app, view, tor.range);
                    } break;

                    case VimBindingKind_Operator: {
                        Vim_Operator_State op_state = {};
                        op_state.app          = app;
                        op_state.view         = view;
                        op_state.buffer       = buffer;
                        op_state.op_count     = op_count;
                        op_state.op           = vim_bind->op;
                        op_state.motion_count = 1;
                        op_state.selection    = selection;
                        op_state.total_range  = Ii64_neg_inf;

                        vim_bind->op(app, &op_state, view, buffer, selection, op_count, op_count_was_set);
                    } break;

                    case VimBindingKind_4CoderCommand: {
                        for (i32 i = 0; i < Min(op_count, VIM_MAXIMUM_OP_COUNT); i++) {
                            vim_bind->fcoder_command(app);
                        }
                    } break;
                }

                view = get_active_view(app, Access_ReadVisible);
                buffer = view_get_buffer(app, view, Access_ReadVisible);

                if (vim_bind->kind == VimBindingKind_Operator) {
                    if (is_vim_visual_mode(vim_state.mode)) {
                        view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.range.first.line, selection.range.first.col));
                        vim_enter_normal_mode(app);
                    }
                }

                if (vim_state.mode == VimMode_Normal) {
                    vim_end_command(app, true);
                }

                vim_state.active_register = &vim_state.VIM_DEFAULT_REGISTER;
            } else {
                // NOTE(allen): call the command
                binding.custom(app);
            }

            // NOTE(allen): after the command is called do some book keeping
            ProfileScope(app, "after view input");

            next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
            if (next_rewrite != 0) {
                Rewrite_Type *rewrite = scope_attachment(app, view_scope, view_rewrite_loc, Rewrite_Type);
                *rewrite = *next_rewrite;
                if (fcoder_mode == FCoderMode_NotepadLike) {
                    for (View_ID view_it = get_view_next(app, 0, Access_Always);
                         view_it != 0;
                         view_it = get_view_next(app, view_it, Access_Always))
                    {
                        Managed_Scope scope_it = view_get_managed_scope(app, view_it);
                        b32 *snap_mark_to_cursor = scope_attachment(app, scope_it, view_snap_mark_to_cursor, b32);
                        if (*snap_mark_to_cursor) {
                            i64 pos = view_get_cursor_pos(app, view_it);
                            view_set_mark(app, view_it, seek_pos(pos));
                        }
                    }
                }
            }
        }

        vim_end_query();
    }
}

BUFFER_EDIT_RANGE_SIG(vim_default_buffer_edit_range) {
    default_buffer_edit_range(app, buffer_id, new_range, original_size);

    ProfileScope(app, "[vim] buffer edit range");
    
    Range_i64 old_range = Ii64(new_range.first, new_range.first + original_size);
    // Range_i64 old_line_range = get_line_range_from_pos_range(app, buffer_id, old_range);

    i64 insert_size = range_size(new_range);
    i64 text_shift = replace_range_shift(old_range, insert_size);

    for (u32 mark_index = 0; mark_index < ArrayCount(vim_state.all_marks); mark_index++) {
        Tiny_Jump* mark = vim_state.all_marks + mark_index;
        if (mark->buffer == buffer_id) {
            // NOTE: I'm not bothering to delete lower case marks if you delete the line they're on, like happens in vim,
            //       because that behaviour seems totally pointless AND WHY IS IT ONLY FOR LOWER CASE MARKS? Vim's weird, dude.
            if (new_range.min < mark->pos) {
                mark->pos += text_shift;
            }
        }
    }

    for (View_ID view = get_view_next(app, 0, Access_Read); view; view = get_view_next(app, view, Access_Read)) {
        Managed_Scope scope = view_get_managed_scope(app, view);
        Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);
        for (i32 jump_index = vim_view->jump_history_first; jump_index < vim_view->jump_history_one_past_last; jump_index++) {
            Tiny_Jump* jump = vim_view->jump_history + (jump_index % ArrayCount(vim_view->jump_history));
            if (jump->buffer == buffer_id) {
                // @TODO: Make it look at the line instead
                // i64 line = get_line_number_from_pos(app, buffer_id, jump->pos);
                // if (text_shift < 0 && range_contains(old_line_range, line)) {
                if (text_shift < 0 && range_contains(old_range, jump->pos)) {
                    vim_delete_jump_history_at_index(app, vim_view, jump_index);
                } else if (new_range.min < jump->pos) {
                    jump->pos += text_shift;
                }
            }
        }
    }

    return 0;
}

function void vim_set_default_colors(Application_Links *app) {
    // NOTE: If this assert fires, you haven't called set_default_color_scheme() yet.
    Assert(global_theme_arena.base_allocator);

    Arena* arena = &global_theme_arena;

    active_color_table.arrays[defcolor_vim_bar_normal] = make_colors(arena, 0xFF888888);
    active_color_table.arrays[defcolor_vim_bar_insert] = make_colors(arena, 0xFF888888);
    active_color_table.arrays[defcolor_vim_bar_visual] = make_colors(arena, 0xFF888888);
    active_color_table.arrays[defcolor_vim_bar_recording_macro] = make_colors(arena, 0xFF888888);
    active_color_table.arrays[defcolor_vim_cursor_normal] = make_colors(arena, 0xFF00EE00, 0xFFEE7700);
    active_color_table.arrays[defcolor_vim_cursor_insert] = make_colors(arena, 0xFF00EE00, 0xFFEE7700);
    active_color_table.arrays[defcolor_vim_cursor_visual] = make_colors(arena, 0xFF00EE00, 0xFFEE7700);
    active_color_table.arrays[defcolor_vim_character_highlight] = make_colors(arena, 0xFFEE7700);
}

//
//
//

function void vim_setup_default_mapping(Application_Links* app, Mapping *mapping, Vim_Key vim_leader) {
    MappingScope();
    SelectMapping(mapping);

    //
    // Global Map
    //

    SelectMap(mapid_global);

    BindCore(default_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(project_go_to_root_directory,      KeyCode_H, KeyCode_Control);
    Bind(save_all_dirty_buffers,            KeyCode_S, KeyCode_Control, KeyCode_Shift);
    Bind(change_to_build_panel,             KeyCode_Period, KeyCode_Alt);
    Bind(close_build_panel,                 KeyCode_Comma, KeyCode_Alt);
    Bind(goto_next_jump,                    KeyCode_N, KeyCode_Control);
    Bind(goto_prev_jump,                    KeyCode_P, KeyCode_Control);
    Bind(build_in_build_panel,              KeyCode_M, KeyCode_Alt);
    Bind(goto_first_jump,                   KeyCode_M, KeyCode_Alt, KeyCode_Shift);
    Bind(toggle_filebar,                    KeyCode_B, KeyCode_Alt);
    Bind(execute_any_cli,                   KeyCode_Z, KeyCode_Alt);
    Bind(execute_previous_cli,              KeyCode_Z, KeyCode_Alt, KeyCode_Shift);
    Bind(command_lister,                    KeyCode_X, KeyCode_Alt);
    Bind(project_command_lister,            KeyCode_X, KeyCode_Alt, KeyCode_Shift);
    Bind(list_all_functions_current_buffer, KeyCode_I, KeyCode_Control, KeyCode_Shift);
    Bind(project_fkey_command,              KeyCode_F1);
    Bind(project_fkey_command,              KeyCode_F2);
    Bind(project_fkey_command,              KeyCode_F3);
    Bind(project_fkey_command,              KeyCode_F4);
    Bind(project_fkey_command,              KeyCode_F5);
    Bind(project_fkey_command,              KeyCode_F6);
    Bind(project_fkey_command,              KeyCode_F7);
    Bind(project_fkey_command,              KeyCode_F8);
    Bind(project_fkey_command,              KeyCode_F9);
    Bind(project_fkey_command,              KeyCode_F10);
    Bind(project_fkey_command,              KeyCode_F11);
    Bind(project_fkey_command,              KeyCode_F12);
    Bind(project_fkey_command,              KeyCode_F13);
    Bind(project_fkey_command,              KeyCode_F14);
    Bind(project_fkey_command,              KeyCode_F15);
    Bind(project_fkey_command,              KeyCode_F16);
    Bind(exit_4coder,                       KeyCode_F4, KeyCode_Alt);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);

    //
    // File Map
    //

    SelectMap(mapid_file);
    ParentMap(mapid_global);

    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);

    BindTextInput(write_text_input);

    Bind(delete_char,                                 KeyCode_Delete);
    Bind(backspace_char,                              KeyCode_Backspace);
    Bind(move_up,                                     KeyCode_Up);
    Bind(move_down,                                   KeyCode_Down);
    Bind(move_left,                                   KeyCode_Left);
    Bind(move_right,                                  KeyCode_Right);
    Bind(seek_end_of_line,                            KeyCode_End);
    Bind(seek_beginning_of_line,                      KeyCode_Home);
    Bind(page_up,                                     KeyCode_PageUp);
    Bind(page_down,                                   KeyCode_PageDown);
    Bind(goto_beginning_of_file,                      KeyCode_PageUp, KeyCode_Control);
    Bind(goto_end_of_file,                            KeyCode_PageDown, KeyCode_Control);
    Bind(move_up_to_blank_line_end,                   KeyCode_Up, KeyCode_Control);
    Bind(move_down_to_blank_line_end,                 KeyCode_Down, KeyCode_Control);
    Bind(move_left_whitespace_boundary,               KeyCode_Left, KeyCode_Control);
    Bind(move_right_whitespace_boundary,              KeyCode_Right, KeyCode_Control);
    Bind(move_line_up,                                KeyCode_Up, KeyCode_Alt);
    Bind(move_line_down,                              KeyCode_Down, KeyCode_Alt);
    Bind(backspace_alpha_numeric_boundary,            KeyCode_Backspace, KeyCode_Control);
    Bind(delete_alpha_numeric_boundary,               KeyCode_Delete, KeyCode_Control);
    Bind(snipe_backward_whitespace_or_token_boundary, KeyCode_Backspace, KeyCode_Alt);
    Bind(snipe_forward_whitespace_or_token_boundary,  KeyCode_Delete, KeyCode_Alt);
    Bind(set_mark,                                    KeyCode_Space, KeyCode_Control);
    Bind(copy,                                        KeyCode_C, KeyCode_Control);
    Bind(delete_range,                                KeyCode_D, KeyCode_Control);
    Bind(delete_line,                                 KeyCode_D, KeyCode_Control, KeyCode_Shift);
    Bind(goto_line,                                   KeyCode_G, KeyCode_Control);
    Bind(paste_and_indent,                            KeyCode_V, KeyCode_Control);
    Bind(paste_next_and_indent,                       KeyCode_V, KeyCode_Control, KeyCode_Shift);
    Bind(cut,                                         KeyCode_X, KeyCode_Control);
    Bind(vim_redo,                                    KeyCode_Y, KeyCode_Control);
    Bind(vim_undo,                                    KeyCode_Z, KeyCode_Control);
    Bind(if_read_only_goto_position,                  KeyCode_Return);
    Bind(if_read_only_goto_position_same_panel,       KeyCode_Return, KeyCode_Shift);
    Bind(view_jump_list_with_lister,                  KeyCode_Period, KeyCode_Control, KeyCode_Shift);
    Bind(vim_enter_normal_mode_escape,                KeyCode_Escape);

    //
    // Code Map
    //

    SelectMap(mapid_code);
    ParentMap(mapid_file);

    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);

    BindTextInput(vim_write_text_abbrev_and_auto_indent);

    Bind(move_left_alpha_numeric_boundary,                    KeyCode_Left, KeyCode_Control);
    Bind(move_right_alpha_numeric_boundary,                   KeyCode_Right, KeyCode_Control);
    Bind(move_left_alpha_numeric_or_camel_boundary,           KeyCode_Left, KeyCode_Alt);
    Bind(move_right_alpha_numeric_or_camel_boundary,          KeyCode_Right, KeyCode_Alt);
    Bind(vim_backspace_char,                                  KeyCode_Backspace);
    Bind(comment_line_toggle,                                 KeyCode_Semicolon, KeyCode_Control);
    Bind(word_complete,                                       KeyCode_Tab);
    // Bind(word_complete_drop_down,                             KeyCode_N, KeyCode_Control);
    Bind(auto_indent_range,                                   KeyCode_Tab, KeyCode_Control);
    Bind(auto_indent_line_at_cursor,                          KeyCode_Tab, KeyCode_Shift);
    Bind(word_complete_drop_down,                             KeyCode_Tab, KeyCode_Shift, KeyCode_Control);
    Bind(write_block,                                         KeyCode_R, KeyCode_Alt);
    Bind(write_todo,                                          KeyCode_T, KeyCode_Alt);
    Bind(write_note,                                          KeyCode_Y, KeyCode_Alt);
    Bind(list_all_locations_of_type_definition,               KeyCode_D, KeyCode_Alt);
    Bind(list_all_locations_of_type_definition_of_identifier, KeyCode_T, KeyCode_Alt, KeyCode_Shift);
    Bind(open_long_braces,                                    KeyCode_LeftBracket, KeyCode_Control);
    Bind(open_long_braces_semicolon,                          KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
    Bind(open_long_braces_break,                              KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
    Bind(select_surrounding_scope,                            KeyCode_LeftBracket, KeyCode_Alt);
    Bind(select_surrounding_scope_maximal,                    KeyCode_LeftBracket, KeyCode_Alt, KeyCode_Shift);
    Bind(select_prev_scope_absolute,                          KeyCode_RightBracket, KeyCode_Alt);
    Bind(select_prev_top_most_scope,                          KeyCode_RightBracket, KeyCode_Alt, KeyCode_Shift);
    Bind(select_next_scope_absolute,                          KeyCode_Quote, KeyCode_Alt);
    Bind(select_next_scope_after_current,                     KeyCode_Quote, KeyCode_Alt, KeyCode_Shift);
    Bind(place_in_scope,                                      KeyCode_ForwardSlash, KeyCode_Alt);
    Bind(delete_current_scope,                                KeyCode_Minus, KeyCode_Alt);
    Bind(if0_off,                                             KeyCode_I, KeyCode_Alt);
    Bind(open_file_in_quotes,                                 KeyCode_1, KeyCode_Alt);
    Bind(open_matching_file_cpp,                              KeyCode_2, KeyCode_Alt);
    Bind(write_zero_struct,                                   KeyCode_0, KeyCode_Control);

    //
    // Normal Map
    //

    SelectMap(vim_mapid_normal);
    ParentMap(mapid_global);

    BindMouse(vim_start_mouse_select, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(vim_mouse_drag);

    //
    // Visual Map
    //

    SelectMap(vim_mapid_visual);
    ParentMap(vim_mapid_normal);

    //
    // ...
    //

    //
    // Vim Maps
    //

    VimMappingScope();

    //
    // Text Object Vim Map
    //

    VimSelectMap(vim_map_text_objects);

    VimBind(vim_text_object_inner_scope,                         vim_char('i'), vim_char('{'));
    VimBind(vim_text_object_inner_scope,                         vim_char('i'), vim_char('}'));
    VimBind(vim_text_object_inner_paren,                         vim_char('i'), vim_char('('));
    VimBind(vim_text_object_inner_paren,                         vim_char('i'), vim_char(')'));
    VimBind(vim_text_object_inner_single_quotes,                 vim_char('i'), vim_char('\''));
    VimBind(vim_text_object_inner_double_quotes,                 vim_char('i'), vim_char('"'));
    VimBind(vim_text_object_inner_word,                          vim_char('i'), vim_char('w'));
    VimBind(vim_text_object_isearch_repeat_forward,              vim_char('g'), vim_char('n'));
    VimBind(vim_text_object_isearch_repeat_backward,             vim_char('g'), vim_char('N'));

    //
    // Operator Pending Vim Map
    //

    VimSelectMap(vim_map_operator_pending);
    VimAddParentMap(vim_map_text_objects);

    VimBind(vim_motion_left,                                     vim_char('h'));
    VimBind(vim_motion_down,                                     vim_char('j'));
    VimBind(vim_motion_up,                                       vim_char('k'));
    VimBind(vim_motion_right,                                    vim_char('l'));
    VimBind(vim_motion_left,                                     vim_key(KeyCode_Left));
    VimBind(vim_motion_down,                                     vim_key(KeyCode_Down));
    VimBind(vim_motion_up,                                       vim_key(KeyCode_Up));
    VimBind(vim_motion_right,                                    vim_key(KeyCode_Right));
    VimBind(vim_motion_to_empty_line_down,                       vim_char('}'));
    VimBind(vim_motion_to_empty_line_up,                         vim_char('{'));
    VimBind(vim_motion_word,                                     vim_char('w'));
    VimBind(vim_motion_big_word,                                 vim_char('W'));
    VimBind(vim_motion_word_end,                                 vim_char('e'));
    VimBind(vim_motion_word_backward,                            vim_char('b'));
    VimBind(vim_motion_big_word_backward,                        vim_char('B'));
    VimBind(vim_motion_line_start_textual,                       vim_char('0'));
    VimBind(vim_motion_line_end_textual,                         vim_char('$'));
    VimBind(vim_motion_scope,                                    vim_char('%'));
    VimBind(vim_motion_buffer_start_or_goto_line,                vim_char('g'), vim_char('g'));
    VimBind(vim_motion_buffer_end_or_goto_line,                  vim_char('G'));
    VimBind(vim_motion_page_top,                                 vim_char('H'));
    VimBind(vim_motion_page_mid,                                 vim_char('M'));
    VimBind(vim_motion_page_bottom,                              vim_char('L'));
    VimBind(vim_motion_find_character,                           vim_char('f'));
    VimBind(vim_motion_find_character_backward,                  vim_char('F'));
    VimBind(vim_motion_to_character,                             vim_char('t'));
    VimBind(vim_motion_to_character_backward,                    vim_char('T'));
    VimBind(vim_motion_find_character_pair,                      vim_char('s'));
    VimBind(vim_motion_find_character_pair_backward,             vim_char('S'));
    VimBind(vim_motion_repeat_character_seek_same_direction,     vim_char(';'));
    VimBind(vim_motion_repeat_character_seek_reverse_direction,  vim_char(','));

    //
    // Normal Vim Map
    //

    VimSelectMap(vim_map_normal);
    VimAddParentMap(vim_map_operator_pending);

    VimBind(vim_register,                                        vim_char('"'));
    VimBind(vim_change,                                          vim_char('c'));
    VimBind(vim_change_eol,                                      vim_char('C'));
    VimBind(vim_delete,                                          vim_char('d'));
    VimBind(vim_delete_eol,                                      vim_char('D'));
    VimBind(vim_delete_character,                                vim_char('x'));
    VimBind(vim_yank,                                            vim_char('y'));
    VimBind(vim_yank_eol,                                        vim_char('Y'));
    VimBind(vim_paste,                                           vim_char('p'));
    VimBind(vim_paste_pre_cursor,                                vim_char('P'));
    VimBind(vim_auto_indent,                                     vim_char('='));
    VimBind(vim_indent,                                          vim_char('>'));
    VimBind(vim_outdent,                                         vim_char('<'));
    VimBind(vim_replace,                                         vim_char('r'));
    VimBind(vim_new_line_below,                                  vim_char('o'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_new_line_above,                                  vim_char('O'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_join_line,                                       vim_char('J'));
    VimBind(vim_align,                                           vim_char('g'), vim_char('l'));
    VimBind(vim_align_right,                                     vim_char('g'), vim_char('L'));
    VimBind(vim_lowercase,                                       vim_char('g'), vim_char('u'));
    VimBind(vim_uppercase,                                       vim_char('g'), vim_char('U'));
    VimBind(vim_miblo_increment,                                 vim_char('a', KeyCode_Control));
    VimBind(vim_miblo_decrement,                                 vim_char('x', KeyCode_Control));
    VimBind(vim_miblo_increment_sequence,                        vim_char('g'), vim_char('a', KeyCode_Control));
    VimBind(vim_miblo_decrement_sequence,                        vim_char('g'), vim_char('x', KeyCode_Control));

    // @TODO: Doing that ->flags thing is a bit weird...
    VimBind(vim_enter_insert_mode,                               vim_char('i'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_enter_insert_sol_mode,                           vim_char('I'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_enter_append_mode,                               vim_char('a'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_enter_append_eol_mode,                           vim_char('A'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_toggle_visual_mode,                              vim_char('v'));
    VimBind(vim_toggle_visual_line_mode,                         vim_char('V'));
    VimBind(vim_toggle_visual_block_mode,                        vim_char('v', KeyCode_Control));

    VimBind(change_active_panel,                                 vim_char('w', KeyCode_Control), vim_char('w'));
    VimBind(change_active_panel,                                 vim_char('w', KeyCode_Control), vim_char('w', KeyCode_Control));
    VimBind(swap_panels,                                         vim_char('w', KeyCode_Control), vim_char('x'));
    VimBind(swap_panels,                                         vim_char('w', KeyCode_Control), vim_char('x', KeyCode_Control));
    VimBind(windmove_panel_left,                                 vim_char('w', KeyCode_Control), vim_char('h'));
    VimBind(windmove_panel_left,                                 vim_char('w', KeyCode_Control), vim_char('h', KeyCode_Control));
    VimBind(windmove_panel_down,                                 vim_char('w', KeyCode_Control), vim_char('j'));
    VimBind(windmove_panel_down,                                 vim_char('w', KeyCode_Control), vim_char('j', KeyCode_Control));
    VimBind(windmove_panel_up,                                   vim_char('w', KeyCode_Control), vim_char('k'));
    VimBind(windmove_panel_up,                                   vim_char('w', KeyCode_Control), vim_char('k', KeyCode_Control));
    VimBind(windmove_panel_right,                                vim_char('w', KeyCode_Control), vim_char('l'));
    VimBind(windmove_panel_right,                                vim_char('w', KeyCode_Control), vim_char('l', KeyCode_Control));
    VimBind(windmove_panel_swap_left,                            vim_char('w', KeyCode_Control), vim_char('H'));
    VimBind(windmove_panel_swap_left,                            vim_char('w', KeyCode_Control), vim_char('H', KeyCode_Control));
    VimBind(windmove_panel_swap_down,                            vim_char('w', KeyCode_Control), vim_char('J'));
    VimBind(windmove_panel_swap_down,                            vim_char('w', KeyCode_Control), vim_char('J', KeyCode_Control));
    VimBind(windmove_panel_swap_up,                              vim_char('w', KeyCode_Control), vim_char('K'));
    VimBind(windmove_panel_swap_up,                              vim_char('w', KeyCode_Control), vim_char('K', KeyCode_Control));
    VimBind(windmove_panel_swap_right,                           vim_char('w', KeyCode_Control), vim_char('L'));
    VimBind(windmove_panel_swap_right,                           vim_char('w', KeyCode_Control), vim_char('L', KeyCode_Control));
    VimBind(vim_split_window_vertical,                           vim_char('w', KeyCode_Control), vim_char('v'));
    VimBind(vim_split_window_vertical,                           vim_char('w', KeyCode_Control), vim_char('v', KeyCode_Control));
    VimBind(vim_split_window_horizontal,                         vim_char('w', KeyCode_Control), vim_char('s'));
    VimBind(vim_split_window_horizontal,                         vim_char('w', KeyCode_Control), vim_char('s', KeyCode_Control));

    VimBind(center_view,                                         vim_char('z'), vim_char('z'));
    VimBind(vim_view_move_line_to_top,                           vim_char('z'), vim_char('t'));
    VimBind(vim_view_move_line_to_bottom,                        vim_char('z'), vim_char('b'));

    VimBind(vim_page_up,                                         vim_char('b', KeyCode_Control));
    VimBind(vim_page_down,                                       vim_char('f', KeyCode_Control));
    VimBind(vim_half_page_up,                                    vim_char('u', KeyCode_Control));
    VimBind(vim_half_page_down,                                  vim_char('d', KeyCode_Control));

    VimBind(vim_step_back_jump_history,                          vim_char('o', KeyCode_Control));
    VimBind(vim_step_forward_jump_history,                       vim_char('i', KeyCode_Control));
    VimBind(vim_previous_buffer,                                 vim_char('6', KeyCode_Control));

    VimBind(vim_record_macro,                                    vim_char('q'));
    VimBind(vim_replay_macro,                                    vim_char('@'))->flags |= VimBindingFlag_TextCommand;
    VimBind(vim_set_mark,                                        vim_char('m'));
    VimBind(vim_go_to_mark,                                      vim_char('\''));
    VimBind(vim_go_to_mark,                                      vim_char('`'));
    VimBind(vim_go_to_mark_less_history,                         vim_char('g'), vim_char('\''));
    VimBind(vim_go_to_mark_less_history,                         vim_char('g'), vim_char('`'));
    VimBind(vim_open_file_in_quotes_in_same_window,              vim_char('g'), vim_char('f'));
    VimBind(vim_jump_to_definition_under_cursor,                 vim_char(']', KeyCode_Control));

    VimNameBind(string_u8_litexpr("Files"),                      vim_leader, vim_char('f'));
    VimBind(interactive_open_or_new,                             vim_leader, vim_char('f'), vim_char('e'));
    VimBind(interactive_switch_buffer,                           vim_leader, vim_char('f'), vim_char('b'));
    VimBind(interactive_kill_buffer,                             vim_leader, vim_char('f'), vim_char('k'));
    VimBind(kill_buffer,                                         vim_leader, vim_char('f'), vim_char('d'));
    VimBind(q,                                                   vim_leader, vim_char('f'), vim_char('q'));
    VimBind(qa,                                                  vim_leader, vim_char('f'), vim_char('Q'));
    VimBind(w,                                                   vim_leader, vim_char('f'), vim_char('w'));
    
    VimNameBind(string_u8_litexpr("Search"),                     vim_leader, vim_char('s'));
    VimBind(list_all_substring_locations_case_insensitive,       vim_leader, vim_char('s'), vim_char('s'));
    
    VimNameBind(string_u8_litexpr("Tags"),                       vim_leader, vim_char('t'));
    VimBind(jump_to_definition,                                  vim_leader, vim_char('t'), vim_char('a'));
    
    VimBind(vim_toggle_line_comment_range_indent_style,          vim_leader, vim_char('c'), vim_key(KeyCode_Space));

    VimBind(vim_enter_normal_mode_escape,                        vim_key(KeyCode_Escape));
    VimBind(vim_isearch_word_under_cursor,                       vim_char('*'));
    VimBind(vim_reverse_isearch_word_under_cursor,               vim_char('#'));
    VimBind(vim_repeat_command,                                  vim_char('.'));
    VimBind(vim_move_line_up,                                    vim_char('k', KeyCode_Alt));
    VimBind(vim_move_line_down,                                  vim_char('j', KeyCode_Alt));
    VimBind(vim_isearch,                                         vim_char('/'));
    VimBind(vim_isearch_backward,                                vim_char('?'));
    VimBind(vim_isearch_repeat_forward,                          vim_char('n'));
    VimBind(vim_isearch_repeat_backward,                         vim_char('N'));
    VimBind(noh,                                                 vim_leader, vim_char('n'));
    VimBind(goto_next_jump,                                      vim_char('n', KeyCode_Control));
    VimBind(goto_prev_jump,                                      vim_char('p', KeyCode_Control));
    VimBind(vim_undo,                                            vim_char('u'));
    VimBind(vim_redo,                                            vim_char('r', KeyCode_Control));
    VimBind(command_lister,                                      vim_char(':'));
    VimBind(if_read_only_goto_position,                          vim_key(KeyCode_Return));
    VimBind(if_read_only_goto_position_same_panel,               vim_key(KeyCode_Return, KeyCode_Shift));

    //
    // Visual Vim Map
    //

    VimSelectMap(vim_map_visual);
    VimAddParentMap(vim_map_normal);
    VimAddParentMap(vim_map_text_objects);
    
    VimBind(vim_enter_visual_insert_mode,                        vim_char('I'));
    VimBind(vim_enter_visual_append_mode,                        vim_char('A'));

    VimBind(vim_lowercase,                                       vim_char('u'));
    VimBind(vim_uppercase,                                       vim_char('U'));

    VimBind(vim_isearch_selection,                               vim_char('/'));
    VimBind(vim_reverse_isearch_selection,                       vim_char('?'));
}

internal void vim_init(Application_Links* app) {
    vim_state.arena = make_arena_system();
    heap_init(&vim_state.heap, &vim_state.arena);
    vim_state.alloc = base_allocator_on_heap(&vim_state.heap);

    vim_state.active_register = &vim_state.VIM_DEFAULT_REGISTER;
    vim_state.search_direction = Scan_Forward;
    vim_state.chord_bar = Su8(vim_state.chord_bar_storage, 0, ArrayCount(vim_state.chord_bar_storage));
    vim_state.echo_string = Su8(vim_state.echo_string_storage, 0, ArrayCount(vim_state.echo_string_storage));
    vim_state.echo_color = fcolor_id(defcolor_text_default);
    vim_state.persistent_echo_string = Su8(vim_state.persistent_echo_string_storage, 0, ArrayCount(vim_state.persistent_echo_string_storage));
    vim_state.persistent_echo_color = fcolor_resolve(fcolor_id(defcolor_text_default));

    if (sizeof(VIM_ESCAPE_SEQUENCE) != 0 && sizeof(VIM_ESCAPE_SEQUENCE) != 3) {
        vim_echo_alert(app, "Warning: Only two-character escape sequences are supported.");
    }

    vim_set_default_colors(app);
}

internal void vim_set_default_hooks(Application_Links* app) {
    set_custom_hook(app, HookID_ViewEventHandler, vim_default_view_handler);
    set_custom_hook(app, HookID_RenderCaller, vim_render_caller);
    set_custom_hook(app, HookID_WholeScreenRenderCaller, vim_whole_screen_render_caller);
    set_custom_hook(app, HookID_BeginBuffer, vim_begin_buffer);
    set_custom_hook(app, HookID_Tick, vim_tick);
    set_custom_hook(app, HookID_BufferEditRange, vim_default_buffer_edit_range);
}

#undef cast
#endif // FCODER_VIMMISH_CPP
