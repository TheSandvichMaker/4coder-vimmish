#ifndef FCODER_VIMMISH_CPP
#define FCODER_VIMMISH_CPP

/* USAGE:
    #include "4coder_default_include.cpp"

    // You could override some defines here:
    // #define VIM_USE_CHARACTER_SEEK_HIGHLIGHTS 0 / 1                                   [default: 1]
    // #define VIM_AUTO_INDENT_ON_PASTE 0 / 1                                            [default: 1]
    // #define VIM_DEFAULT_REGISTER unnamed_register / clipboard_register                [default: unnamed_register]
    // #define VIM_NORMAL_COLOR [insert expression that evaluates to an ARGB_Color]      [default: fcolor_resolve(fcolor_id(defcolor_bar))]
    // #define VIM_INSERT_COLOR [insert expression that evaluates to an ARGB_Color]      [default: fcolor_resolve(fcolor_id(defcolor_bar))]
    // #define VIM_VISUAL_COLOR [insert expression that evaluates to an ARGB_Color]      [default: fcolor_resolve(fcolor_id(defcolor_bar))]
    // #define VIM_CURSOR_ROUNDNESS [insert expression that evaluates to a float]        [default: (metrics.normal_advance*0.5f)*0.9f]

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
        vim_setup_default_mapping(app, &framework_mapping, mapid_global, vim_mapid_shared, mapid_file, mapid_code, vim_mapid_normal, vim_mapid_visual);
    }
*/

#include "clearfeld_windmove.cpp"

//
//
//

#ifndef VIM_USE_CHARACTER_SEEK_HIGHLIGHTS
#define VIM_USE_CHARACTER_SEEK_HIGHLIGHTS 1
#endif

#ifndef VIM_AUTO_INDENT_ON_PASTE
#define VIM_AUTO_INDENT_ON_PASTE 1            // @TODO: Maybe just make this a separate command
#endif

#ifndef VIM_DEFAULT_REGISTER
#define VIM_DEFAULT_REGISTER unnamed_register // change this to clipboard_register to use the clipboard by default
#endif

#ifndef VIM_NORMAL_COLOR
#define VIM_NORMAL_COLOR fcolor_resolve(fcolor_id(defcolor_bar))
#endif

#ifndef VIM_INSERT_COLOR
#define VIM_INSERT_COLOR fcolor_resolve(fcolor_id(defcolor_bar))
#endif

#ifndef VIM_VISUAL_COLOR
#define VIM_VISUAL_COLOR fcolor_resolve(fcolor_id(defcolor_bar))
#endif

#ifndef VIM_CURSOR_ROUNDNESS
#define VIM_CURSOR_ROUNDNESS (metrics.normal_advance*0.5f)*0.9f
#endif

//
//
//

#define VIM_DYNAMIC_STRINGS_ARE_TOO_CLEVER_FOR_THEIR_OWN_GOOD 0 // Note: I don't really think this is good for my use, but it's here if you like things that complicate your life
#define VIM_PRINT_COMMANDS 0

CUSTOM_ID(command_map, vim_mapid_shared);
CUSTOM_ID(command_map, vim_mapid_normal);
CUSTOM_ID(command_map, vim_mapid_visual);
CUSTOM_ID(attachment, vim_buffer_attachment);
CUSTOM_ID(attachment, vim_view_attachment);

#define cast(type) (type)

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

internal void string_free(Base_Allocator* alloc, String_u8* string) {
    if (string->str) {
        base_free(alloc, string->str);
    }
    block_zero_struct(string);
}

internal i64 vim_get_vertical_pixel_move_pos(Application_Links *app, View_ID view, i64 pos, f32 pixels) {
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    Rect_f32 r = view_padded_box_of_pos(app, view, cursor.line, pos);
    Vec2_f32 p = {};
    p.x = view_get_preferred_x(app, view);
    if (pixels > 0.f) {
        p.y = r.y1 + pixels;
    } else {
        p.y = r.y0 + pixels;
    }
    i64 new_pos = view_pos_at_relative_xy(app, view, cursor.line, p);
    return new_pos;
}

internal i64 vim_get_pos_of_visual_line_side(Application_Links *app, View_ID view, i64 pos, Side side) {
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    Vec2_f32 p = view_relative_xy_of_pos(app, view, cursor.line, pos);
    p.x = (side == Side_Min)?(0.f):(max_f32);
    i64 new_pos = view_pos_at_relative_xy(app, view, cursor.line, p);
    return new_pos;
}

internal Range_i64 vim_maximize_visual_line_range(Application_Links* app, View_ID view, Range_i64 range, b32 inclusive) {
    Range_i64 result = Ii64(vim_get_pos_of_visual_line_side(app, view, range.min, Side_Min),
                            vim_get_pos_of_visual_line_side(app, view, range.max, Side_Max));
    if (!inclusive) {
        result.max = view_set_pos_by_character_delta(app, view, result.max, -1);
    }
    return result;
}

internal Range_i64 vim_maximize_textual_line_range(Application_Links* app, View_ID view, Buffer_ID buffer, Range_i64 range, b32 inclusive) {
    Range_i64 result = Ii64(get_line_side_pos_from_pos(app, buffer, range.min, Side_Min),
                            get_line_side_pos_from_pos(app, buffer, range.max, Side_Max));
    if (!inclusive) {
        result.max = view_set_pos_by_character_delta(app, view, result.max, -1);
    }
    return result;
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
    union {
        Range_i64 range;
        struct /* VisualSelectionKind_Block */ {
            i64 first_line;
            i64 first_col;
            i64 one_past_last_line;
            i64 one_past_last_col;
        };
    };
};

enum VimRegisterFlag {
    VimRegisterFlag_FromBlockCopy = 0x1,
    VimRegisterFlag_Append        = 0x2,
    VimRegisterFlag_ReadOnly      = 0x4,
};

struct Vim_Register {
    u32 flags;
    String_u8 string;
};

struct Vim_Global_State {
    Arena                arena;
    Heap                 heap;
    Base_Allocator       alloc;

    Vim_Register         registers[36];        // "a-z + "0-9
    Vim_Register         unnamed_register;     // ""
    Vim_Register         clipboard_register;   // "+
    Vim_Register         last_search_register; // "/
    Vim_Register         command_register;     // for command repetition - not exposed to the user
    Vim_Register*        active_register;

    Vim_Register*        most_recent_macro;
    b32                  recording_macro;
    b32                  played_macro;
    i64                  current_macro_start_pos;
    History_Group        macro_history;

    b32                  recording_command;
    b32                  playing_command;
    u32                  command_flags;
    i64                  command_start_pos;
    History_Group        command_history;
    Vim_Visual_Selection command_selection;
    
    b32                  search_show_highlight;
    b32                  search_case_sensitive;
    Scan_Direction       search_direction;
	
    b32                  character_seek_show_highlight;
    Scan_Direction       character_seek_highlight_dir;
    u8                   most_recent_character_seek_storage[8];
    String_u8            most_recent_character_seek;
    Scan_Direction       most_recent_character_seek_dir;
    b32                  most_recent_character_seek_till;
    b32                  most_recent_character_seek_inclusive;
	
    u8                   chord_bar_storage[32];
    String_u8            chord_bar;

    i32                  definition_stack_count;
    i32                  definition_stack_cursor;
    Tiny_Jump            definition_stack[16];
};

global Vim_Global_State vim_state;

internal Vim_Register* vim_get_register(u8 register_char) {
    Vim_Register* result = 0;

    b32 append = false;
    if (character_is_lower(register_char)) {
        result = vim_state.registers + (register_char - 'a');
    } else if (character_is_upper(register_char)) {
        result = vim_state.registers + (register_char - 'A');
        append = true;
    } else if (register_char >= '0' && register_char <= '9') {
        result = vim_state.registers + 26 + (register_char - '0');
    } else if (register_char == '*' || register_char == '+') {
        // @TODO: I don't know if on Linux it's worth it to make a distinction between "* and "+
        result = &vim_state.clipboard_register;
    } else if (register_char == '/') {
        result = &vim_state.last_search_register;
    }

    if (result) {
        if (append) result->flags |= VimRegisterFlag_Append;
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
            String_Const_u8 clipboard = push_clipboard_index(scratch, 0, 0);
            // vim_write_register(app, reg, clipboard);
        }

        result = reg->string.string;
    }

    return result;
}

struct Vim_Buffer_Attachment {
    Vim_Mode      mode;
    b32           treat_as_code;
    
    Buffer_Cursor insert_cursor;
    i32           insert_history_index;
    
    b32           visual_block_force_to_line_end;
    b32           right_justify_visual_insert;
    Range_i64     visual_insert_line_range;
};

#define VIM_JUMP_HISTORY_SIZE 256
struct Vim_View_Attachment {
    Buffer_ID most_recent_known_buffer;
    i64       most_recent_known_pos;
	
    Buffer_ID previous_buffer;
    i64       pos_in_previous_buffer;

    b32       dont_log_this_buffer_jump;
	
    i32       jump_history_min;
    i32       jump_history_one_past_max;
    i32       jump_history_cursor;
    Tiny_Jump jump_history[VIM_JUMP_HISTORY_SIZE];
};

internal void vim_log_jump_history_internal(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_View_Attachment* vim_view, i64 pos) {
    i64 line = get_line_number_from_pos(app, buffer, pos);

    Scratch_Block scratch(app);
    for (i32 jump_index = vim_view->jump_history_min; jump_index < vim_view->jump_history_one_past_max; jump_index++) {
        i32 jumps_left = vim_view->jump_history_one_past_max - jump_index;
        Tiny_Jump test_jump = vim_view->jump_history[jump_index];
        if (test_jump.buffer == buffer) {
            i64 test_line = get_line_number_from_pos(app, buffer, test_jump.pos); // @Speed
            if (test_line == line) {
                vim_view->jump_history_one_past_max--;
                vim_view->jump_history_cursor = Min(vim_view->jump_history_cursor, vim_view->jump_history_one_past_max);
                
                if (jumps_left > 1) {
                    // @Speed
                    for (i32 shift = 0; shift < jumps_left - 1; shift++) {
                        i32 shift_index = jump_index + shift;
                        vim_view->jump_history[shift_index] = vim_view->jump_history[shift_index + 1];
                    }
                }
            
                break;
            }
        }
    }
	
    Tiny_Jump jump;
    jump.buffer = buffer;
    jump.pos = pos;

    vim_view->jump_history[(vim_view->jump_history_cursor++) % ArrayCount(vim_view->jump_history)] = jump;
	
    vim_view->jump_history_one_past_max = vim_view->jump_history_cursor;
    i32 unsafe_index = vim_view->jump_history_one_past_max - ArrayCount(vim_view->jump_history);
    vim_view->jump_history_min = Max(0, Max(vim_view->jump_history_min, unsafe_index));
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
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);
	
    b32 log_jump = false;
    if (vim_view->jump_history_cursor == vim_view->jump_history_one_past_max) {
        log_jump = true;
        vim_log_jump_history(app);
        vim_view->jump_history_cursor--;
    }
	
    if (vim_view->jump_history_cursor > vim_view->jump_history_min) {
        Tiny_Jump jump = vim_view->jump_history[(--vim_view->jump_history_cursor) % ArrayCount(vim_view->jump_history)];
        jump_to_location(app, view, jump.buffer, jump.pos);
        vim_view->dont_log_this_buffer_jump = true;
    }
}

CUSTOM_COMMAND_SIG(vim_step_forward_jump_history) {
    View_ID view = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view);
    Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);
	
    if (vim_view->jump_history_cursor + 1 < vim_view->jump_history_one_past_max) {
        Tiny_Jump jump = vim_view->jump_history[(++vim_view->jump_history_cursor) % ArrayCount(vim_view->jump_history)];
        jump_to_location(app, view, jump.buffer, jump.pos);
        vim_view->dont_log_this_buffer_jump = true;
    }
}

enum Vim_Motion_Flag {
    VimMotionFlag_Inclusive                 = 0x1,
    VimMotionFlag_SetPreferredX             = 0x2,
    VimMotionFlag_IsJump                    = 0x4,
    VimMotionFlag_VisualBlockForceToLineEnd = 0x8,
    VimMotionFlag_AlwaysSeek                = 0x10,
    VimMotionFlag_LogJumpPostSeek           = 0x20,
};

struct Vim_Motion_Result {
    Range_i64 range;
    i64 seek_pos;
    u32 flags;
};

internal Vim_Motion_Result vim_null_motion(i64 pos) {
    Vim_Motion_Result result = {};
    result.range = Ii64(pos, pos);
    result.seek_pos = pos;
    return result;
}

#define VIM_MOTION(name) Vim_Motion_Result name(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos)
typedef VIM_MOTION(Vim_Motion);

#define VIM_OPERATOR(name) void name(Application_Links* app, struct Vim_Binding_Handler* handler, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count)
typedef VIM_OPERATOR(Vim_Operator);

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

internal Vim_Key vim_char(u32 codepoint) {
    Vim_Key result = {};
    result.codepoint = codepoint;
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

internal b32 vim_keys_match(Vim_Key a, Vim_Key b) {
    b32 result = (a.kc == b.kc) && (a.mods == b.mods);
    return result;
}

internal u64 vim_key_code_hash(Vim_Key key) {
    u64 result = (cast(u64) key.kc) | ((cast(u64) key.mods) << 16); 
    return result;
}

internal u64 vim_codepoint_hash(Vim_Key key) {
    u64 result = (cast(u64) key.codepoint) << 32;
    return result;
}

enum Vim_Binding_Kind {
    VimBindingKind_None,
	
    VimBindingKind_Nested,
    VimBindingKind_Motion,
    VimBindingKind_Operator,
    VimBindingKind_4CoderCommand,
};

enum Vim_Binding_Flag {
    VimBindingFlag_IsRepeatable = 0x1,
    VimBindingFlag_WriteOnly    = 0x2,
};

struct Vim_Key_Binding {
    Vim_Binding_Kind kind;
    String_Const_u8 description;
    u32 flags;
    union {
        Table_u64_u64* nested_keybind_table;
        Vim_Motion* motion;
        Vim_Operator* op;
        Custom_Command_Function* fcoder_command;
    };
};

internal void vim_clear_chord_bar() {
    vim_state.chord_bar.size = 0;
}

struct Vim_Binding_Handler {
    b32 initialized;

    Arena node_arena;
    Heap heap;
    Base_Allocator allocator;
	
    Table_u64_u64 keybind_table;
};

global Vim_Binding_Handler vim_map_normal;
global Vim_Binding_Handler vim_map_visual;
global Vim_Binding_Handler vim_map_operator_pending;

internal void deep_copy_vim_keybind_table(Vim_Binding_Handler* handler, Table_u64_u64* dest, Table_u64_u64* source) {
    *dest = make_table_u64_u64(&handler->allocator, source->slot_count);

    dest->used_count = source->used_count;
    dest->dirty_count = source->dirty_count;

    for (u32 slot_index = 0; slot_index < source->slot_count; slot_index++) {
        dest->keys[slot_index] = source->keys[slot_index];
        u64 value = source->vals[slot_index];
        if (value) {
            Vim_Key_Binding* source_bind = cast(Vim_Key_Binding*) IntAsPtr(value);
            Vim_Key_Binding* dest_bind = push_array(&handler->node_arena, Vim_Key_Binding, 1);

            block_copy_struct(dest_bind, source_bind);

            if (dest_bind->kind == VimBindingKind_Nested) {
                dest_bind->nested_keybind_table = push_array(&handler->node_arena, Table_u64_u64, 1);
                deep_copy_vim_keybind_table(handler, dest_bind->nested_keybind_table, source_bind->nested_keybind_table);
            }

            dest->vals[slot_index] = PtrAsInt(dest_bind);
        } else {
            dest->vals[slot_index] = 0;
        }
    }
}

internal void initialize_vim_binding_handler(Application_Links* app, Vim_Binding_Handler* handler, Vim_Binding_Handler* parent = 0) {
    block_zero_struct(handler);
	
    handler->initialized = true;
    handler->node_arena = make_arena_system(KB(2));
    heap_init(&handler->heap, &handler->node_arena);
    handler->allocator = base_allocator_on_heap(&handler->heap);
	
    if (parent) {
        deep_copy_vim_keybind_table(handler, &handler->keybind_table, &parent->keybind_table);
    } else {
        handler->keybind_table = make_table_u64_u64(&handler->allocator, 32);
    }
}

internal Vim_Key_Binding* make_or_retrieve_vim_binding(Vim_Binding_Handler* handler, Table_u64_u64* table, Vim_Key key, b32 make_if_doesnt_exist) {
    Assert(handler);
    Assert(table);

    Vim_Key_Binding* result = 0;

    u64 hash_hi = vim_codepoint_hash(key);
    u64 hash_lo = vim_key_code_hash(key);

    {
        u64 location = 0;
        if (table_read(table, hash_hi, &location)) {
            result = cast(Vim_Key_Binding*) IntAsPtr(location);
        }
    }
	
    if (!result) {
        u64 location = 0;
        if (table_read(table, hash_lo, &location)) {
            result = cast(Vim_Key_Binding*) IntAsPtr(location);
        }
    }

    if (!result && make_if_doesnt_exist) {
        if (hash_hi) {
            result = push_array_zero(&handler->node_arena, Vim_Key_Binding, 1);
            table_insert(table, hash_hi, PtrAsInt(result));
        }
        if (hash_lo) {
            result = push_array_zero(&handler->node_arena, Vim_Key_Binding, 1);
            table_insert(table, hash_lo, PtrAsInt(result));
        }
    }

    return result;
}

internal i32 input_to_digit(User_Input in) {
    i32 result = -1;
    Input_Event* event = &in.event;
    if (event->kind == InputEventKind_KeyStroke && is_unmodified_key(event)) {
        if (event->key.code >= KeyCode_0 && event->key.code <= KeyCode_9) {
            result = cast(i32) (event->key.code - KeyCode_0);
        }
    }
    return result;
}

enum VimQueryMode {
    VimQuery_CurrentInput,
    VimQuery_NextInput,
};

internal i64 vim_query_number(Application_Links* app, VimQueryMode mode, b32 handle_sign = false) {
    i64 result = 0;
	
    User_Input in = mode == VimQuery_CurrentInput ?
                    get_current_input(app) :
                    get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);

    if (in.abort) {
        return result;
    }

    i64 sign = 1;
    if (handle_sign && match_key_code(&in, KeyCode_Minus)) {
        sign = -1;
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
    }
	
    if (!match_key_code(&in, KeyCode_0)) {
        for (;;) {
            i32 digit = input_to_digit(in);
			
            if (digit >= 0) {
                // @TODO: Check if these cases are actually correct
                if (result > (max_i64 / 10 - digit)) {
                    result = max_i64;
                } else if (result < (min_i64 / 10 + digit)) {
                    result = min_i64;
                } else {
                    result = 10*result + sign*digit;
                }
            } else {
                break;
            }
			
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
            if (in.abort) {
                break;
            }
        }
    }
	
    return result;
}

internal User_Input vim_get_next_key(Application_Links* app) {
    User_Input in = {};
    do {
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
        if (in.abort) break;
    } while (key_code_to_vim_modifier(in.event.key.code));
    return in;
}

internal String_Const_u8 vim_get_next_writable(Application_Links* app) {
    User_Input in = {};
    do {
        in = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
        if (in.abort) break;
    } while (key_code_to_vim_modifier(in.event.key.code));
    String_Const_u8 result = to_writable(&in);
    return result;
}

internal Vim_Key vim_get_key_from_current_input(Application_Links* app) {
    Vim_Key result = {};

    User_Input in = get_current_input(app);

    if (in.event.kind == InputEventKind_KeyStroke) {
        result.kc = cast(u16) in.event.key.code;
        result.mods = input_modifier_set_to_vim_modifiers(in.event.key.modifiers);

        leave_current_input_unhandled(app);

        in = get_next_input(app, EventProperty_TextInsert, EventProperty_ViewActivation|EventProperty_Exit);
    }

    if (in.event.kind == InputEventKind_TextInsert) {
        String_Const_u8 writable = to_writable(&in);

        u32 codepoint = 0;
        if (writable.size) {
            codepoint = utf8_consume(writable.str, writable.size).codepoint;
        }

        result.codepoint = codepoint;
    }

    return result;
}

internal Vim_Key vim_get_key_from_next_input(Application_Links* app) {
    Vim_Key result = {};

    User_Input in = vim_get_next_key(app);

    if (in.event.kind == InputEventKind_KeyStroke) {
        result.kc = cast(u16) in.event.key.code;
        result.mods = input_modifier_set_to_vim_modifiers(in.event.key.modifiers);

        leave_current_input_unhandled(app);

        in = get_next_input(app, EventProperty_TextInsert, EventProperty_ViewActivation|EventProperty_Exit);
    }

    if (in.event.kind == InputEventKind_TextInsert) {
        String_Const_u8 writable = to_writable(&in);

        u32 codepoint = 0;
        if (writable.size) {
            codepoint = utf8_consume(writable.str, writable.size).codepoint;
        }

        result.codepoint = codepoint;
    }

    return result;
}

internal void vim_print_bind(Application_Links* app, Vim_Key_Binding* bind) {
    if (bind && bind->description.size) {
        if (get_current_input_is_virtual(app)) {
            print_message(app, string_u8_litexpr("    "));
        }
        print_message(app, bind->description);
        if (bind->kind == VimBindingKind_Nested) {
            print_message(app, string_u8_litexpr(" -> "));
        } else {
            print_message(app, string_u8_litexpr("\n"));
        }
    }
}

internal Vim_Key_Binding* vim_query_binding(Application_Links* app, Vim_Binding_Handler* handler, Vim_Binding_Handler* fallback, VimQueryMode mode) {
    Vim_Key key = (mode == VimQuery_CurrentInput) ? vim_get_key_from_current_input(app) : vim_get_key_from_next_input(app);
    Vim_Key_Binding* bind = make_or_retrieve_vim_binding(handler, &handler->keybind_table, key, false);
    Vim_Key_Binding* fallback_bind = 0;
    if (fallback) {
        fallback_bind = make_or_retrieve_vim_binding(handler, &fallback->keybind_table, key, false);
    }

    if (!bind) {
        bind = fallback_bind;
        fallback_bind = 0;
    }

#if VIM_PRINT_COMMANDS
    vim_print_bind(app, bind);
#endif
	
    while (bind && bind->kind == VimBindingKind_Nested) {
        key  = vim_get_key_from_next_input(app);
        bind = make_or_retrieve_vim_binding(handler, bind->nested_keybind_table, key, false);

        if (fallback_bind && fallback_bind->kind == VimBindingKind_Nested) {
            fallback_bind = make_or_retrieve_vim_binding(handler, fallback_bind->nested_keybind_table, key, false);
        }

        if (!bind) {
            bind = fallback_bind;
            fallback_bind = 0;
        }

#if VIM_PRINT_COMMANDS
        vim_print_bind(app, bind);
#endif
    }
	
    return bind;
}

CUSTOM_COMMAND_SIG(vim_move_left_on_visual_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
	
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_start_pos = vim_get_pos_of_visual_line_side(app, view, cursor_pos, Side_Min);
    i64 legal_line_start_pos = view_get_character_legal_pos_from_pos(app, view, line_start_pos);
	
    if (cursor_pos > legal_line_start_pos) {
        move_left(app);
    }
}

CUSTOM_COMMAND_SIG(vim_move_right_on_visual_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
	
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_end_pos = vim_get_pos_of_visual_line_side(app, view, cursor_pos, Side_Max);
    i64 legal_line_end_pos = view_get_character_legal_pos_from_pos(app, view, line_end_pos);
	
    if (cursor_pos < legal_line_end_pos) {
        move_right(app);
    }
}

internal void vim_macro_begin(Application_Links* app, Vim_Register* reg) {
    if (vim_state.recording_macro || get_current_input_is_virtual(app)) {
        return;
    }

    vim_state.recording_macro = true;
    vim_state.most_recent_macro = reg;

    Buffer_ID buffer = get_keyboard_log_buffer(app);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));
    vim_state.current_macro_start_pos = cursor.pos;
}

internal void vim_macro_end(Application_Links* app, Vim_Register* reg) {
    if (!vim_state.recording_macro || get_current_input_is_virtual(app)) {
        return;
    }
	
    vim_state.recording_macro = false;
    vim_state.most_recent_macro = reg;

    Buffer_ID buffer = get_keyboard_log_buffer(app);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));
    // @Robustness: We step back by two lines because we want to exclude both the key stroke and text insert event.
    // But this seems kind of dodgy, because it assumes the keyboard macro was ended with a bind that has an
    // associated text insert event.
    Buffer_Cursor back_cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line - 2, 1));

    Range_i64 range = Ii64(vim_state.current_macro_start_pos, back_cursor.pos);
    
    Scratch_Block scratch(app);
    String_Const_u8 macro_string = push_buffer_range(app, scratch, buffer, range);

    vim_write_register(app, reg, macro_string);
}

CUSTOM_COMMAND_SIG(vim_record_macro) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) return;

    if (vim_state.recording_macro && vim_state.most_recent_macro) {
        vim_macro_end(app, vim_state.most_recent_macro);
    } else {
        String_Const_u8 reg_str = vim_get_next_writable(app);
        if (reg_str.size) {
            char reg_char = reg_str.str[0];
            Vim_Register* reg = vim_get_register(reg_char);
            if (reg) {
                vim_macro_begin(app, reg);
            }
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
        Vim_Register* reg = 0;
        if (reg_char == '@') {
            reg = vim_state.most_recent_macro;
        } else {
            reg = vim_get_register(reg_char);
        }

        if (!vim_state.recording_macro && reg) {
            String_Const_u8 macro_string = vim_read_register(app, reg);
            keyboard_macro_play(app, macro_string);

            vim_state.most_recent_macro = reg;
            vim_state.played_macro = true;
            vim_state.macro_history = history_group_begin(app, buffer);
        }
    }
}

internal void vim_command_begin(Application_Links* app, Buffer_ID buffer, u32 flags) {
    if (vim_state.recording_command) {
        return;
    }
	
    vim_state.recording_command = true;
    vim_state.command_history = history_group_begin(app, buffer);
    vim_state.command_flags = flags;
	
    if (get_current_input_is_virtual(app)) {
        return;
    }
}

internal void vim_command_end(Application_Links* app, b32 completed) {
    if (!vim_state.recording_command) {
        return;
    }
	
    vim_state.recording_command = false;
    history_group_end(vim_state.command_history);
	
    if (get_current_input_is_virtual(app)) {
        return;
    }
	
    if (HasFlag(vim_state.command_flags, VimBindingFlag_IsRepeatable) && completed) {
        Buffer_ID buffer = get_keyboard_log_buffer(app);
        Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));

        Range_i64 range = Ii64(vim_state.command_start_pos, cursor.pos);
        
        Scratch_Block scratch(app);
        String_Const_u8 macro_string = push_buffer_range(app, scratch, buffer, range);

        vim_write_register(app, &vim_state.command_register, macro_string);
    }
}

internal Vim_Visual_Selection vim_get_selection(Application_Links* app, View_ID view, Buffer_ID buffer, b32 inclusive) {
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);

    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos = view_get_mark_pos(app, view);
	
    Vim_Visual_Selection selection = {};
    if (vim_buffer->mode == VimMode_Visual || vim_buffer->mode == VimMode_VisualLine) {
        selection.kind = vim_buffer->mode == VimMode_VisualLine ? VimSelectionKind_Line : VimSelectionKind_Range;
        selection.range = Ii64(cursor_pos, mark_pos);
        if (vim_buffer->mode == VimMode_VisualLine) {
            selection.range = vim_maximize_textual_line_range(app, view, buffer, selection.range, true);
        }

        if (inclusive) {
            selection.range.max = view_set_pos_by_character_delta(app, view, selection.range.max, 1);
        }
    } else if (vim_buffer->mode == VimMode_VisualBlock) {
        selection.kind = VimSelectionKind_Block;
        Buffer_Cursor cursor_a = buffer_compute_cursor(app, buffer, seek_pos(cursor_pos));
        Buffer_Cursor cursor_b = buffer_compute_cursor(app, buffer, seek_pos(mark_pos));
        selection.first_line = Min(cursor_a.line, cursor_b.line);
        selection.first_col  = Min(cursor_a.col, cursor_b.col);
        selection.one_past_last_line = Max(cursor_a.line, cursor_b.line) + 1;
        selection.one_past_last_col  = Max(cursor_a.col, cursor_b.col) + 1;
        if (vim_buffer->visual_block_force_to_line_end) {
            i64 furthest_column = min_i64;
            for (i64 line = selection.first_line; line < selection.one_past_last_line; line++) {
                furthest_column = Max(furthest_column, get_line_end(app, buffer, line).col);
            }
            selection.one_past_last_col = furthest_column + 1;
            view_set_preferred_x(app, view, max_f32);
        }
    }

    return selection;
}

internal b32 vim_selection_consume_line(Application_Links* app, Buffer_ID buffer, Vim_Visual_Selection* selection, Range_i64* out_range, b32 inclusive) {
    b32 result = false;
    if (selection->kind == VimSelectionKind_Block) {
        if (selection->first_line < selection->one_past_last_line) {
            *out_range = Ii64(buffer_compute_cursor(app, buffer, seek_line_col(selection->first_line, selection->first_col)).pos,
                              buffer_compute_cursor(app, buffer, seek_line_col(selection->first_line, selection->one_past_last_col - (inclusive ? 0 : 1))).pos);
            selection->first_line++;
            result = true;
        }
    } else {
        if (range_size(selection->range) > 0) {
            *out_range = selection->range;
            selection->range = {};
            result = true;
        }
    }

    return result;
}

internal void vim_apply_undo_history_to_line(Application_Links* app, Buffer_ID buffer, Vim_Buffer_Attachment* vim_buffer, i64 line, i64 col_delta, History_Record_Index history_index, Record_Info record, Buffer_Cursor record_cursor) {
    Buffer_Cursor insert_cursor = buffer_compute_cursor(app, buffer, seek_line_col(line, record_cursor.col));

    if (vim_buffer->right_justify_visual_insert) {
        i64 new_col   = Max(0, record_cursor.col + col_delta);
        insert_cursor = buffer_compute_cursor(app, buffer, seek_line_col(insert_cursor.line, new_col));
    }

    String_Const_u8 insert_string = record.single_string_forward;
    Buffer_Cursor line_end = get_line_end(app, buffer, line);

    if (line_end.col >= record_cursor.col) {
        Range_i64 replace_range = Ii64_size(insert_cursor.pos, record.single_string_backward.size);
        buffer_replace_range(app, buffer, replace_range, insert_string);
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

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
	
    Command_Map_ID* map_id = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);

    if (vim_buffer->mode == mode) {
        mode = VimMode_Normal;
    }
	
    switch (mode) {
        case VimMode_Normal: {
            *map_id = vim_mapid_normal;

            if (is_vim_insert_mode(vim_buffer->mode)) {
                vim_move_left_on_visual_line(app);
            }

            Buffer_Cursor end_cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));

            if (vim_buffer->mode == VimMode_VisualInsert && end_cursor.line == vim_buffer->insert_cursor.line) {
                // @TODO: I could investigate if Batch_Edit could simplify this hot mess, but I'm not sure it could.
                // The undo history stuff isn't order independent, after all.

                History_Record_Index insert_history_index  = vim_buffer->insert_history_index;
                History_Record_Index current_history_index = buffer_history_get_current_state_index(app, buffer);

                u32 history_record_count = 0;
                for (i32 history_index = insert_history_index; history_index <= current_history_index; history_index++) {
                    Record_Info record = buffer_history_get_record_info(app, buffer, history_index);
                    if (record.kind == RecordKind_Group) {
                        history_record_count += record.group_count;
                    } else {
                        history_record_count++;
                    }
                }

                Scratch_Block scratch(app);
                Buffer_Cursor* record_cursors_array = push_array(scratch, Buffer_Cursor, history_record_count);

                {
                    u32 record_array_index = 0;
                    for (i32 history_index = insert_history_index; history_index <= current_history_index; history_index++) {
                        Record_Info record = buffer_history_get_record_info(app, buffer, history_index);
                        if (record.kind == RecordKind_Group) {
                            for (i32 sub_index = 0; sub_index < record.group_count; sub_index++) {
                                Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, history_index, sub_index);
                                Assert(sub_record.kind == RecordKind_Single);
                                record_cursors_array[record_array_index++] = buffer_compute_cursor(app, buffer, seek_pos(sub_record.single_first));
                            }
                        } else {
                            record_cursors_array[record_array_index++] = buffer_compute_cursor(app, buffer, seek_pos(record.single_first));
                        }
                    }
                }

                Range_i64 line_range = vim_buffer->visual_insert_line_range;
                for (i64 line = line_range.first + 1; line < line_range.one_past_last; line++) {
                    Buffer_Cursor line_end_cursor = buffer_compute_cursor(app, buffer, seek_line_col(line, max_i64));
                    i64 col_delta = line_end_cursor.col - vim_buffer->insert_cursor.col;

                    u32 record_array_index = 0;

                    for (i32 history_index = insert_history_index; history_index <= current_history_index; history_index++) {
                        Record_Info record = buffer_history_get_record_info(app, buffer, history_index);
                        if (record.kind == RecordKind_Group) {
                            for (i32 sub_index = 0; sub_index < record.group_count; sub_index++) {
                                Record_Info sub_record = buffer_history_get_group_sub_record(app, buffer, history_index, sub_index);
                                Assert(sub_record.kind == RecordKind_Single);

                                Buffer_Cursor record_cursor = record_cursors_array[record_array_index++];
                                vim_apply_undo_history_to_line(app, buffer, vim_buffer, line, col_delta, history_index, sub_record, record_cursor);
                            }
                        } else {
                            Buffer_Cursor record_cursor = record_cursors_array[record_array_index++];
                            vim_apply_undo_history_to_line(app, buffer, vim_buffer, line, col_delta, history_index, record, record_cursor);
                        }
                    }
                }
            }

            vim_state.most_recent_character_seek.size = 0;

            // @TODO: I don't love this, it's a bit obtuse
            User_Input in = get_current_input(app);
            b32 command_was_complete = (vim_buffer->mode == VimMode_Insert || !match_key_code(&in, KeyCode_Escape));
            vim_command_end(app, command_was_complete);
        } break;
		
        case VimMode_VisualInsert: {
            Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, false);
            if (selection.kind == VimSelectionKind_Block) {
                vim_buffer->visual_insert_line_range = Ii64(selection.first_line, selection.one_past_last_line);
            } else if (selection.kind == VimSelectionKind_Line) {
                vim_buffer->visual_insert_line_range = Ii64(get_line_number_from_pos(app, buffer, selection.range.min),
                                                            get_line_number_from_pos(app, buffer, selection.range.max) + 1);
            } else {
                InvalidPath;
            }

            if (append) {
                if (selection.kind == VimSelectionKind_Block) {
                    vim_buffer->right_justify_visual_insert = vim_buffer->visual_block_force_to_line_end;
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.first_line, selection.one_past_last_col));
                } else {
                    vim_buffer->right_justify_visual_insert = true;
                    i64 line = get_line_number_from_pos(app, buffer, selection.range.min);
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(line, max_i64));
                }
            } else {
                vim_buffer->right_justify_visual_insert = false;
                if (selection.kind == VimSelectionKind_Block) {
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.first_line, selection.first_col));
                } else {
                    view_set_cursor_and_preferred_x(app, view, seek_pos(selection.range.min));
                }
            }
        } case VimMode_Insert: {
            if (vim_buffer->treat_as_code) {
                *map_id = mapid_code;
            } else {
                *map_id = mapid_file;
            }

            vim_buffer->insert_history_index = buffer_history_get_current_state_index(app, buffer) + 1;
            vim_buffer->insert_cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
        } break;
		
        case VimMode_Visual:
        case VimMode_VisualLine:
        case VimMode_VisualBlock: {
            *map_id = vim_mapid_visual;
            vim_command_begin(app, buffer, VimBindingFlag_IsRepeatable);
        } break;
    }
	
    vim_buffer->mode = mode;
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

    Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, false);
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
    vim_move_right_on_visual_line(app);
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
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    if (!is_vim_visual_mode(vim_buffer->mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }
	
    vim_enter_mode(app, VimMode_Visual);
}

CUSTOM_COMMAND_SIG(vim_toggle_visual_line_mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    if (!is_vim_visual_mode(vim_buffer->mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }
	
    vim_enter_mode(app, VimMode_VisualLine);
}

CUSTOM_COMMAND_SIG(vim_toggle_visual_block_mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    if (!is_vim_visual_mode(vim_buffer->mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
        vim_buffer->visual_block_force_to_line_end = false;
    }
	
    vim_enter_mode(app, VimMode_VisualBlock);
}

internal void vim_seek_motion_result(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Motion_Result result) {
    i64 old_line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    i64 new_line = get_line_number_from_pos(app, buffer, result.seek_pos);

    if (HasFlag(result.flags, VimMotionFlag_IsJump) && !HasFlag(result.flags, VimMotionFlag_LogJumpPostSeek)) {
        if (old_line != new_line) {
            vim_log_jump_history(app);
        }
    }

    Buffer_Seek new_pos = seek_pos(result.seek_pos);
    if (HasFlag(result.flags, VimMotionFlag_SetPreferredX)) {
        view_set_cursor_and_preferred_x(app, view, new_pos);
    } else {
        view_set_cursor(app, view, new_pos);
    }

    if (HasFlag(result.flags, VimMotionFlag_IsJump) && HasFlag(result.flags, VimMotionFlag_LogJumpPostSeek)) {
        vim_log_jump_history(app);
    }
}

internal Vim_Motion_Result vim_execute_motion(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Motion* motion, i64 start_pos, i32 count) {
    if (count < 0) count = 1;
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64_neg_inf;
    for (i32 i = 0; i < count; i++) {
        Vim_Motion_Result inner = motion(app, view, buffer, result.seek_pos);
        Range_i64 new_range = range_union(inner.range, result.range);
        result = inner;
        result.range = new_range;
    }
    return result;
}

internal Vim_Binding_Handler* vim_get_handler_for_mode(Vim_Mode mode) {
    Vim_Binding_Handler* handler = 0;
    if (is_vim_visual_mode(mode)) {
        handler = &vim_map_visual;
    } else if (mode == VimMode_Normal) {
        handler = &vim_map_normal;
    }
    return handler;
}

CUSTOM_COMMAND_SIG(vim_handle_input) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    Vim_Binding_Handler* handler = vim_get_handler_for_mode(vim_buffer->mode);
    Assert(handler);

    if (!vim_state.recording_command && !get_current_input_is_virtual(app)) {
        Buffer_ID keyboard_log_buffer = get_keyboard_log_buffer(app);
        Buffer_Cursor cursor      = buffer_compute_cursor(app, keyboard_log_buffer, seek_pos(buffer_get_size(app, keyboard_log_buffer)));
        Buffer_Cursor back_cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_line_col(cursor.line - 1, 1));
        vim_state.command_start_pos = back_cursor.pos;
    }

    vim_state.character_seek_show_highlight = false;
	
    User_Input in = get_current_input(app);

    i64 queried_number = vim_query_number(app, VimQuery_CurrentInput);
    i32 op_count = cast(i32) clamp(1, queried_number, 256);
    
    Vim_Key_Binding* bind = vim_query_binding(app, handler, 0, VimQuery_CurrentInput);
	
    if (bind) {
        Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, false);

        if (bind->flags & VimBindingFlag_WriteOnly) {
            u32 buffer_flags = buffer_get_access_flags(app, buffer);
            if (!HasFlag(buffer_flags, Access_Write)) {
                return;
            }
        }
        
        vim_command_begin(app, buffer, bind->flags);

        if (vim_state.playing_command) {
            Vim_Visual_Selection repeat_selection = vim_state.command_selection;
            if (repeat_selection.kind == VimSelectionKind_Block) {
                selection.one_past_last_line = selection.first_line + (repeat_selection.one_past_last_line - repeat_selection.first_line);
                selection.one_past_last_col  = selection.first_col  + (repeat_selection.one_past_last_col  - repeat_selection.first_col );
            } else {
                selection.range.max = selection.range.min + range_size(repeat_selection.range);
            }
        } else if (vim_state.recording_command) {
            vim_state.command_selection = selection;
        }
        
        switch (bind->kind) {
            case VimBindingKind_Motion: {
                i64 old_pos = view_get_cursor_pos(app, view);
                Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, bind->motion, old_pos, op_count);
                vim_seek_motion_result(app, view, buffer, mr);
                if (HasFlag(mr.flags, VimMotionFlag_SetPreferredX)) {
                    vim_buffer->visual_block_force_to_line_end = HasFlag(mr.flags, VimMotionFlag_VisualBlockForceToLineEnd);
                }
            } break;
            
            case VimBindingKind_Operator: {
                bind->op(app, handler, view, buffer, selection, op_count);
            } break;
            
            case VimBindingKind_4CoderCommand: {
                for (i32 i = 0; i < op_count; i++) {
                    bind->fcoder_command(app);
                }
            } break;
        }
        
        view = get_active_view(app, Access_ReadVisible);
        buffer = view_get_buffer(app, view, Access_ReadVisible);
        scope = buffer_get_managed_scope(app, buffer);
        vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
        
        if (bind->kind == VimBindingKind_Operator) {
            if (is_vim_visual_mode(vim_buffer->mode)) {
                if (vim_buffer->mode == VimMode_VisualBlock) {
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(selection.first_line, selection.first_col));
                } else {
                    view_set_cursor_and_preferred_x(app, view, seek_pos(selection.range.min));
                }
                vim_enter_normal_mode(app);
            }
        }
        
        if (vim_buffer->mode == VimMode_Normal) {
            vim_command_end(app, true);
        }
    }

    vim_state.active_register = &vim_state.VIM_DEFAULT_REGISTER;
}

internal Vim_Key_Binding* add_vim_binding(Vim_Binding_Handler* handler, Vim_Key_Sequence binding_sequence) {
    Vim_Key_Binding* result = 0;

    Assert(handler->initialized);
	
    if (binding_sequence.count > 0) {
        Command_Binding binding;
        binding.custom = vim_handle_input;
		
        Vim_Key key1 = binding_sequence.keys[0];
        result = make_or_retrieve_vim_binding(handler, &handler->keybind_table, key1, true);
		
        for (i32 key_index = 1; key_index < binding_sequence.count; key_index++) {
            Vim_Key next_key = binding_sequence.keys[key_index];
            if (result->kind != VimBindingKind_Nested) {
                result->kind = VimBindingKind_Nested;
				
                Table_u64_u64* inner_table = push_array(&handler->node_arena, Table_u64_u64, 1);
                *inner_table = make_table_u64_u64(&handler->allocator, 8);
				
                result->nested_keybind_table = inner_table;
            }
			
            result = make_or_retrieve_vim_binding(handler, result->nested_keybind_table, next_key, true);
        }
    }
	
    if (result) {
        block_zero_struct(result);
    }
	
    return result;
}

#define vim_bind_motion(handler, motion, key1, ...) vim_bind_motion_(handler, motion, string_u8_litexpr(#motion), vim_key_sequence(key1, __VA_ARGS__))
internal Vim_Key_Binding* vim_bind_motion_(Vim_Binding_Handler* handler, Vim_Motion* motion, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence);
    bind->kind = VimBindingKind_Motion;
    bind->description = description;
    bind->motion = motion;
    return bind;
}

#define vim_bind_operator(handler, op, key1, ...) vim_bind_operator_(handler, op, string_u8_litexpr(#op), vim_key_sequence(key1, __VA_ARGS__))
internal Vim_Key_Binding* vim_bind_operator_(Vim_Binding_Handler* handler, Vim_Operator* op, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence);
    bind->kind = VimBindingKind_Operator;
    bind->description = description;
    bind->flags |= VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly;
    bind->op = op;
    return bind;
}

#define vim_bind_generic_command(handler, cmd, key1, ...) vim_bind_generic_command_(handler, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, __VA_ARGS__))
internal Vim_Key_Binding* vim_bind_generic_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence);
    bind->kind = VimBindingKind_4CoderCommand;
    bind->description = description;
    bind->fcoder_command = fcoder_command;
    return bind;
}

#define vim_bind_text_command(handler, cmd, key1, ...) vim_bind_text_command_(handler, cmd, string_u8_litexpr(#cmd), vim_key_sequence(key1, __VA_ARGS__))
internal Vim_Key_Binding* vim_bind_text_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = vim_bind_generic_command_(handler, fcoder_command, description, sequence);
    bind->flags |= VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly;
    return bind;
}

#define vim_named_bind(handler, description, key1, ...) vim_named_bind_(handler, description, vim_key_sequence(key1, __VA_ARGS__))
internal Vim_Key_Binding* vim_named_bind_(Vim_Binding_Handler* handler, String_Const_u8 description, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence);
    bind->description = description;
    return bind;
}

#define vim_unbind(handler, key1, ...) vim_unbind_(handler, vim_key_sequence(key1, __VA_ARGS__))
internal void vim_unbind_(Vim_Binding_Handler* handler, Vim_Key_Sequence sequence) {
    Vim_Key_Binding* bind = 0;
    Table_u64_u64* keybind_table = &handler->keybind_table;
    for (i32 key_index = 0; key_index < sequence.count; key_index++) {
        Vim_Key key = sequence.keys[key_index];
        bind = make_or_retrieve_vim_binding(handler, keybind_table, key, false);
        if (bind->kind == VimBindingKind_Nested) {
            keybind_table = bind->nested_keybind_table;
        } else {
            break;
        }
    }
    if (bind && bind->kind) {
        block_zero_struct(bind);
    }
}

internal b32 character_is_newline(char c) {
    return c == '\r' || c == '\n';
}

internal i64 boundary_whitespace(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_whitespace));
}

CUSTOM_COMMAND_SIG(vim_repeat_most_recent_command) {
    if (vim_state.recording_command || vim_state.playing_command) {
        return;
    }

    vim_state.playing_command = true;
	
    String_Const_u8 macro_string = vim_read_register(app, &vim_state.command_register);
    keyboard_macro_play(app, macro_string);
}
internal VIM_MOTION(vim_motion_left) {
    Vim_Motion_Result result = {};
    result.seek_pos = view_set_pos_by_character_delta(app, view, start_pos, -1);
    result.range = Ii64(start_pos, result.seek_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_right) {
    Vim_Motion_Result result = {};
    result.seek_pos = view_set_pos_by_character_delta(app, view, start_pos,  1);
    result.range = Ii64(start_pos, result.seek_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_down) {
    Vim_Motion_Result result = {};
    result.seek_pos = vim_get_vertical_pixel_move_pos(app, view, start_pos, 1.0f);
    result.range = Ii64(start_pos, result.seek_pos);
    result.range.min = vim_get_pos_of_visual_line_side(app, view, result.range.min, Side_Min);
    result.range.max = vim_get_pos_of_visual_line_side(app, view, result.range.max, Side_Max);
    return result;
}

internal VIM_MOTION(vim_motion_up) {
    Vim_Motion_Result result = {};
    result.seek_pos = vim_get_vertical_pixel_move_pos(app, view, start_pos, -1.0f);
    result.range = Ii64(start_pos, result.seek_pos);
    result.range.min = vim_get_pos_of_visual_line_side(app, view, result.range.min, Side_Min);
    result.range.max = vim_get_pos_of_visual_line_side(app, view, result.range.max, Side_Max);
    return result;
}

// @TODO: My word motions seem kind of stupidly complicated, but I keep running into little limitations of the built in helper boundary functions and such,
// so at some point I should put some effort into going a bit lower level and simplifying these. But until then, they work.

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

internal Vim_Motion_Result vim_motion_word_internal(Application_Links* app, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
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
                end_pos = scan(app, push_boundary_list(scratch, boundary_whitespace, boundary_line), buffer, Scan_Forward, end_pos);
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
            if (!character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                end_pos = scan(app, push_boundary_list(scratch, vim_boundary_word, boundary_line), buffer, Scan_Backward, end_pos);
            }
        }
    }
	
    result.seek_pos = end_pos;
    result.range = Ii64(start_pos, end_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
	
    if (dir == Scan_Backward && range_size(result.range) > 0) {
        result.range.max -= 1;
    }
	
    return result;
}

internal VIM_MOTION(vim_motion_word) {
    return vim_motion_word_internal(app, buffer, Scan_Forward, start_pos);
}

internal VIM_MOTION(vim_motion_word_backward) {
    return vim_motion_word_internal(app, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_word_end) {
    Scratch_Block scratch(app);
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_word_end), buffer, Scan_Forward, start_pos + 1) - 1;
    if (character_is_whitespace(buffer_get_char(app, buffer, result.seek_pos))) {
        result.seek_pos = scan(app, push_boundary_list(scratch, vim_boundary_word_end), buffer, Scan_Forward, result.seek_pos + 1) - 1;
    }
    result.range = Ii64(start_pos, result.seek_pos);
    result.flags |= VimMotionFlag_Inclusive|VimMotionFlag_SetPreferredX;
    return result;
}

internal VIM_MOTION(vim_motion_buffer_start) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    result.range = Ii64(0, start_pos);
    result.seek_pos = 0;
    result.flags |= VimMotionFlag_IsJump|VimMotionFlag_SetPreferredX;
	
    return result;
}

internal VIM_MOTION(vim_motion_buffer_end) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    result.range = Ii64(start_pos, buffer_get_size(app, buffer));
    result.seek_pos = result.range.max;
    result.flags |= VimMotionFlag_IsJump|VimMotionFlag_SetPreferredX;
	
    return result;
}

internal Vim_Motion_Result vim_motion_line_side_visual(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    i64 line_side = vim_get_pos_of_visual_line_side(app, view, start_pos, dir == Scan_Forward ? Side_Max : Side_Min);
	
    result.seek_pos = line_side;
    result.range = Ii64(start_pos, line_side);
    result.flags |= VimMotionFlag_Inclusive|VimMotionFlag_SetPreferredX;
	
    return result;
}

internal Vim_Motion_Result vim_motion_line_side_textual(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    i64 line_side = get_line_side_pos_from_pos(app, buffer, start_pos, dir == Scan_Forward ? Side_Max : Side_Min);
	
    result.seek_pos = line_side;
    result.range = Ii64(start_pos, result.seek_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
	
    return result;
}

internal VIM_MOTION(vim_motion_line_start_visual) {
    return vim_motion_line_side_visual(app, view, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_line_end_visual) {
    Vim_Motion_Result result = vim_motion_line_side_visual(app, view, buffer, Scan_Forward, start_pos);
    result.flags |= VimMotionFlag_VisualBlockForceToLineEnd;
    if (result.seek_pos > result.range.min) {
        result.seek_pos = view_set_pos_by_character_delta(app, view, result.seek_pos, -1);
    }
    return result;
}

internal VIM_MOTION(vim_motion_line_start_textual) {
    return vim_motion_line_side_textual(app, view, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_line_end_textual) {
    Vim_Motion_Result result = vim_motion_line_side_textual(app, view, buffer, Scan_Forward, start_pos);
    result.flags |= VimMotionFlag_VisualBlockForceToLineEnd;
    if (result.seek_pos > result.range.min) {
        result.seek_pos = view_set_pos_by_character_delta(app, view, result.seek_pos, -1);
    }
    return result;
}

internal VIM_MOTION(vim_motion_enclose_line_visual) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64(vim_get_pos_of_visual_line_side(app, view, start_pos, Side_Min),
                        vim_get_pos_of_visual_line_side(app, view, start_pos, Side_Max));
    result.seek_pos = result.range.max;
    result.flags |= VimMotionFlag_Inclusive;
    return result;
}

internal VIM_MOTION(vim_motion_enclose_line_textual) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64(get_line_side_pos_from_pos(app, buffer, start_pos, Side_Min),
                        get_line_side_pos_from_pos(app, buffer, start_pos, Side_Max));
    result.seek_pos = result.range.max;
    result.flags |= VimMotionFlag_Inclusive;
    return result;
}

internal VIM_MOTION(vim_motion_scope) {
	Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_Inclusive|
                    VimMotionFlag_IsJump|
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

            if (range_contains_inclusive(line_range, token->pos)) {
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
                        result.range = Ii64(start_pos, end_pos);
						
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

internal Vim_Motion_Result vim_motion_inner_nest_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, Find_Nest_Flag flags, b32 leave_inner_line) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
	
    if (vim_find_surrounding_nest(app, buffer, start_pos, flags, &result.range, true)) {
        result.flags |= VimMotionFlag_Inclusive;
        i64 min = result.range.min;
        i64 max = result.range.max;
        i64 min_line = get_line_number_from_pos(app, buffer, min);
        i64 max_line = get_line_number_from_pos(app, buffer, max);
        if (min_line != max_line) {
            // @Speed
            while (character_is_whitespace(buffer_get_char(app, buffer, min)) && !character_is_newline(buffer_get_char(app, buffer, min))) {
                min = view_set_pos_by_character_delta(app, view, min, 1);
            }
            while (character_is_newline(buffer_get_char(app, buffer, min))) {
                min = view_set_pos_by_character_delta(app, view, min, 1);
            }
            if (leave_inner_line) {
                while (character_is_whitespace(buffer_get_char(app, buffer, max)) && !character_is_newline(buffer_get_char(app, buffer, max))) {
                    max = view_set_pos_by_character_delta(app, view, max, -1);
                }
                while (character_is_newline(buffer_get_char(app, buffer, max))) {
                    max = view_set_pos_by_character_delta(app, view, max, -1);
                }
            }
        }
        result.range = Ii64(min, max);
        result.seek_pos = max;
    }
	
    return result;
}

internal VIM_MOTION(vim_motion_inner_scope_leave_inner_line) {
    return vim_motion_inner_nest_internal(app, view, buffer, start_pos, FindNest_Scope, true);
}

internal VIM_MOTION(vim_motion_inner_scope) {
    return vim_motion_inner_nest_internal(app, view, buffer, start_pos, FindNest_Scope, false);
}

internal VIM_MOTION(vim_motion_inner_paren_leave_inner_line) {
    return vim_motion_inner_nest_internal(app, view, buffer, start_pos, FindNest_Paren, true);
}

internal VIM_MOTION(vim_motion_inner_paren) {
    return vim_motion_inner_nest_internal(app, view, buffer, start_pos, FindNest_Paren, false);
}

internal VIM_MOTION(vim_motion_inner_double_quotes) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX;

    Range_i64 range = enclose_pos_inside_quotes(app, buffer, start_pos);
    if (range_size(range) > 0) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
        if (line_range.min == line_range.max) {
            result.range = range;
            result.seek_pos = range.max;
        }
    }

    return result;
}

internal i64 boundary_inside_single_quotes(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
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
    return enclose_boundary(app, buffer, Ii64(pos), boundary_inside_single_quotes);
}

internal VIM_MOTION(vim_motion_inner_single_quotes) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX;

    Range_i64 range = enclose_pos_inside_single_quotes(app, buffer, start_pos);
    if (range_size(range) > 0) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);
        if (line_range.min == line_range.max) {
            result.range = range;
            result.seek_pos = range.max;
        }
    }

    return result;
}

internal VIM_MOTION(vim_motion_inner_word) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX;
	
    Range_i64 range = enclose_boundary(app, buffer, Ii64(start_pos + 1), vim_boundary_word);
    if (range_size(range) > 0) {
        result.range = range;
        result.seek_pos = range.max;
    }
	
    return result;
}

internal Vim_Motion_Result vim_motion_seek_character_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, String_Const_u8 target, Scan_Direction dir, b32 till, b32 inclusive) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= (inclusive ? VimMotionFlag_Inclusive : 0)|
                     VimMotionFlag_SetPreferredX;
	
    b32 new_input = true;
    if (!target.size) {
        new_input = false;
        target = vim_state.most_recent_character_seek.string;
    }
	
    if (target.size) {
        i64 seek_pos = start_pos;
        for (;;) {
            String_Match match = buffer_seek_string(app, buffer, target, dir, seek_pos + (till ? 0 : dir));
            if (match.buffer != buffer) {
                break;
            }
            if (HasFlag(match.flags, StringMatch_CaseSensitive)) {
                result.range = Ii64(start_pos, match.range.min);
                if (dir == Scan_Forward) {
                    result.seek_pos = result.range.max - (till ? 0 : 1);
                } else {
                    result.seek_pos = result.range.min + (till ? 0 : target.size);
                }
                break;
            }
            seek_pos = match.range.min;
        }
		
        if (new_input) {
            // @TODO: This seems rather laboured just to store a unicode character.
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

internal VIM_MOTION(vim_motion_find_character) {
    vim_state.character_seek_show_highlight = true;
    String_Const_u8 target = vim_get_next_writable(app);
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Forward, true, true);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_to_character) {
    vim_state.character_seek_show_highlight = true;
    String_Const_u8 target = vim_get_next_writable(app);
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Forward, false, false);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_backward) {
    vim_state.character_seek_show_highlight = true;
    String_Const_u8 target = vim_get_next_writable(app);
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Backward, true, false);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_to_character_backward) {
    vim_state.character_seek_show_highlight = true;
    String_Const_u8 target = vim_get_next_writable(app);
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Backward, false, false);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_pair) {
    vim_state.character_seek_show_highlight = true;
    u8 target_storage[8];
    String_u8 target = Su8(target_storage, 0, ArrayCount(target_storage));
    string_append(&target, vim_get_next_writable(app));
    string_append(&target, vim_get_next_writable(app));
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target.string, Scan_Forward, true, true);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_find_character_pair_backward) {
    vim_state.character_seek_show_highlight = true;
    u8 target_storage[8];
    String_u8 target = Su8(target_storage, 0, ArrayCount(target_storage));
    string_append(&target, vim_get_next_writable(app));
    string_append(&target, vim_get_next_writable(app));
    Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view, buffer, start_pos, target.string, Scan_Backward, true, true);
    mr.flags |= VimMotionFlag_IsJump|VimMotionFlag_LogJumpPostSeek;
    return mr;
}

internal VIM_MOTION(vim_motion_repeat_character_seek_forward) {
    Scan_Direction dir = vim_state.most_recent_character_seek_dir;
    vim_state.character_seek_show_highlight = true;
    vim_state.character_seek_highlight_dir = dir;
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_inclusive, vim_state.most_recent_character_seek_till);
}

internal VIM_MOTION(vim_motion_repeat_character_seek_backward) {
    Scan_Direction dir = -vim_state.most_recent_character_seek_dir;
    vim_state.character_seek_show_highlight = true;
    vim_state.character_seek_highlight_dir = dir;
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_inclusive, vim_state.most_recent_character_seek_till);
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
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Backward, PositionWithinLine_Start);
    result.range = Ii64(start_pos, result.seek_pos);
    return result;
}

internal VIM_MOTION(vim_motion_to_empty_line_down) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Forward, PositionWithinLine_Start);
    result.range = Ii64(start_pos, result.seek_pos);
    return result;
}

internal VIM_MOTION(vim_motion_prior_to_empty_line_up) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Backward, PositionWithinLine_Start);
    result.range = Ii64(start_pos, result.seek_pos);

    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, result.range);
    if (line_range.min != line_range.max) {
        result.range.min = get_line_start_pos(app, buffer, line_range.min + 1);
    }

    return result;
}

internal VIM_MOTION(vim_motion_prior_to_empty_line_down) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.seek_pos = vim_seek_blank_line(app, Scan_Forward, PositionWithinLine_Start);
    result.range = Ii64(start_pos, result.seek_pos);

    Range_i64 line_range = get_line_range_from_pos_range(app, buffer, result.range);
    if (line_range.min != line_range.max) {
        result.range.max = get_line_end_pos(app, buffer, line_range.max - 1);
    }

    return result;
}

internal Range_i64 vim_search_once_internal(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction direction, i64 pos, String_Const_u8 query, b32 case_sensitive, b32 recursed = false) {
    Range_i64 result = {};

    b32 found_match = false;
    i64 buffer_size = buffer_get_size(app, buffer);
    switch (direction) {
        case Scan_Forward: {
            i64 new_pos = 0;
            if (case_sensitive) {
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
            if (case_sensitive) {
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

    if (!recursed && !found_match) {
        switch (direction) {
            case Scan_Forward:  { result = vim_search_once_internal(app, view, buffer, direction, 0, query, case_sensitive, true); } break;
            case Scan_Backward: { result = vim_search_once_internal(app, view, buffer, direction, buffer_size, query, case_sensitive, true); } break;
        }
    }

    return result;
}

internal void vim_search_once(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction direction, i64 pos, String_Const_u8 query, b32 case_sensitive) {
    Range_i64 range = vim_search_once_internal(app, view, buffer, direction, pos, query, case_sensitive);
    if (range_size(range) > 0) {
        vim_log_jump_history(app);
    }
    isearch__update_highlight(app, view, range);
}

internal void vim_isearch_internal(Application_Links *app, View_ID view, Buffer_ID buffer, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 search_immediately, b32 case_sensitive) {
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if (!start_query_bar(app, &bar, 0)) {
        return;
    }
    
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    
    String_Const_u8 isearch_str = string_u8_litexpr("I-Search: ");
    String_Const_u8 rsearch_str = string_u8_litexpr("Reverse-I-Search: ");

    vim_state.search_case_sensitive = case_sensitive;
    
    User_Input in = {};
    for (;;) {
        if (search_immediately) {
            vim_write_register(app, &vim_state.last_search_register, bar.string);
            vim_search_once(app, view, buffer, scan, pos, bar.string, case_sensitive);
            break;
        }

        if (scan == Scan_Forward) {
            bar.prompt = isearch_str;
        } else if (scan == Scan_Backward) {
            bar.prompt = rsearch_str;
        }
        
        in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
        if (in.abort) {
            break;
        }
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (match_key_code(&in, KeyCode_Return) || match_key_code(&in, KeyCode_Tab)) {
            Input_Modifier_Set* mods = &in.event.key.modifiers;
            if (has_modifier(mods, KeyCode_Control)) {
                bar.string = vim_read_register(app, &vim_state.last_search_register);
            } else {
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
            vim_search_once(app, view, buffer, scan, pos, bar.string, case_sensitive);
        } else if (do_scan_action) {
            vim_search_once(app, view, buffer, change_scan, pos, bar.string, case_sensitive);
        } else if (do_scroll_wheel) {
            mouse_wheel_scroll(app);
        } else {
            leave_current_input_unhandled(app);
        }
    }
    
    if (in.abort) {
        vim_write_register(app, &vim_state.last_search_register, bar.string);
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
    }
}

CUSTOM_COMMAND_SIG(vim_isearch) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    vim_state.search_direction = Scan_Forward;
    vim_isearch_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view) + vim_state.search_direction, SCu8(), false, false);
}

CUSTOM_COMMAND_SIG(vim_isearch_backward) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }

    vim_state.search_direction = Scan_Backward;
    vim_isearch_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view) - vim_state.search_direction, SCu8(), false, false);
}

internal void vim_select_range(Application_Links* app, View_ID view, Range_i64 range) {
    view_set_mark(app, view, seek_pos(range.min));
    view_set_cursor(app, view, seek_pos(range.max - 1));
    vim_enter_mode(app, VimMode_Visual);
}

internal void vim_isearch_repeat_internal(Application_Links* app, Scan_Direction repeat_direction, b32 select, b32 skip_at_cursor) {
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
    Range_i64 range = vim_search_once_internal(app, view, buffer, search_direction, start_pos, last_search, vim_state.search_case_sensitive);

    if (select) {
        vim_select_range(app, view, range);
    } else {
        isearch__update_highlight(app, view, range);
    }
}


CUSTOM_COMMAND_SIG(vim_isearch_repeat_forward) {
    vim_isearch_repeat_internal(app, Scan_Forward, false, true);
}

CUSTOM_COMMAND_SIG(vim_isearch_repeat_select_forward) {
    vim_isearch_repeat_internal(app, Scan_Forward, true, false);
}

CUSTOM_COMMAND_SIG(vim_isearch_repeat_backward) {
    vim_isearch_repeat_internal(app, Scan_Backward, false, true);
}

CUSTOM_COMMAND_SIG(vim_isearch_repeat_select_backward) {
    vim_isearch_repeat_internal(app, Scan_Backward, true, false);
}

internal void vim_isearch_selection_internal(Application_Links* app, Scan_Direction dir) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, true);
    if (selection.kind != VimSelectionKind_Block && range_size(selection.range) > 0) {
        Scratch_Block scratch(app);
        String_Const_u8 query_init = push_buffer_range(app, scratch, buffer, selection.range);
		
        vim_state.search_direction = dir;
        vim_isearch_internal(app, view, buffer, vim_state.search_direction, selection.range.min, query_init, true, true);
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
        vim_isearch_internal(app, view, buffer, vim_state.search_direction, range_under_cursor.min + vim_state.search_direction, under_cursor, true, true);
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
    vim_state.search_show_highlight = false;
}

internal void vim_write_text_and_auto_indent_internal(Application_Links* app, String_Const_u8 insert) {
    if (insert.str != 0 && insert.size > 0){
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1){
            switch (insert.str[i]){
                case ';': case ':':
                case '{': case '}':
                case '(': case ')':
                case '[': case ']':
                case '#':
                case '\n': case '\t':
                {
                    do_auto_indent = true;
                }break;
            }
        }
        if (do_auto_indent){
            View_ID view = get_active_view(app, Access_ReadWriteVisible);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            Range_i64 pos = {};
            pos.min = view_get_cursor_pos(app, view);
            write_text(app, insert);
            pos.max= view_get_cursor_pos(app, view);
            auto_indent_buffer(app, buffer, pos, 0);
            move_past_lead_whitespace(app, view, buffer);
        }
        else{
            write_text(app, insert);
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
CUSTOM_DOC("Moves up to the next line of actual text, regardless of line wrapping.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
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

internal VIM_MOTION(vim_motion_isearch_repeat_forward) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX|
                    VimMotionFlag_AlwaysSeek;

    String_Const_u8 last_search = vim_read_register(app, &vim_state.last_search_register);
    Range_i64 range = vim_search_once_internal(app, view, buffer, vim_state.search_direction, view_get_cursor_pos(app, view), last_search, vim_state.search_case_sensitive);

    result.seek_pos = range.min;
    result.range = range;

    return result;
}

internal VIM_MOTION(vim_motion_isearch_repeat_backward) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.flags |= VimMotionFlag_SetPreferredX|
                    VimMotionFlag_AlwaysSeek;

    String_Const_u8 last_search = vim_read_register(app, &vim_state.last_search_register);
    Range_i64 range = vim_search_once_internal(app, view, buffer, -vim_state.search_direction, view_get_cursor_pos(app, view), last_search, vim_state.search_case_sensitive);

    result.seek_pos = range.min;
    result.range = range;

    return result;
}

internal Vim_Motion* vim_unwrap_motion_query(Vim_Key_Binding* query, Vim_Operator* op = 0) {
    Vim_Motion* result = 0;
    if (query) {
        if (query->kind == VimBindingKind_Motion) {
            result = query->motion;
        } else if (query->kind == VimBindingKind_Operator && query->op == op) {
            result = vim_motion_enclose_line_textual;
        }
    }
    return result;
}

enum Vim_Operator_State_Flag {
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
    Vim_Motion* motion;

    Vim_Visual_Selection selection;
    Range_i64 total_range;

    u32 flags;
};

#define vim_build_op_state(op, flags) vim_build_op_state_(app, view, buffer, count, op, selection, flags)
internal Vim_Operator_State vim_build_op_state_(Application_Links* app, View_ID view, Buffer_ID buffer, i32 op_count, Vim_Operator* op, Vim_Visual_Selection selection, u32 flags) {
    Vim_Operator_State result = {};
    result.app          = app;
    result.view         = view;
    result.buffer       = buffer;
    result.op_count     = op_count;
    result.op           = op;
    result.motion_count = 1;
    result.selection    = selection;
    result.total_range  = Ii64_neg_inf;
    result.flags        = flags;
    return result;
}

internal b32 vim_get_operator_range(Vim_Operator_State* state, Range_i64* out_range) {
    Application_Links* app = state->app;
    View_ID view = state->view;
    Buffer_ID buffer = state->buffer;

    b32 result = false;
    Range_i64 range = Ii64();

    if (state->op_count > 0) {
        result = true;
        b32 inclusive = true;
        if (state->selection.kind == VimSelectionKind_Range) {
            range = state->selection.range;
        } else if (state->selection.kind == VimSelectionKind_Line) {
            range = state->selection.range;
            if (HasFlag(state->flags, VimOpFlag_ChangeBehaviour)) {
                inclusive = false;
            }
        } else if (state->selection.kind == VimSelectionKind_Block) {
            result = vim_selection_consume_line(app, buffer, &state->selection, &range, false);
            while (character_is_newline(buffer_get_char(app, buffer, range.max))) {
                range.max = view_set_pos_by_character_delta(app, view, range.max, -1);
            }
            if (result) {
                state->op_count++;
            }
        } else if (HasFlag(state->flags, VimOpFlag_QueryMotion)) {
            if (!state->motion) {
                i64 count_query = vim_query_number(app, VimQuery_NextInput);
                state->motion_count = cast(i32) clamp(1, count_query, 256);

                Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, &vim_map_normal, VimQuery_CurrentInput);
                state->motion = vim_unwrap_motion_query(query, state->op);
            }

            if (state->motion) {
                if (HasFlag(state->flags, VimOpFlag_ChangeBehaviour)) {
                    // Note: Vim's behaviour is a little inconsistent with some motions in change for the sake of intuitiveness.
                    if      (state->motion == vim_motion_word)               state->motion = vim_motion_word_end;
                    else if (state->motion == vim_motion_inner_scope)        state->motion = vim_motion_inner_scope_leave_inner_line;
                    else if (state->motion == vim_motion_inner_paren)        state->motion = vim_motion_inner_paren_leave_inner_line;
                    else if (state->motion == vim_motion_to_empty_line_up)   state->motion = vim_motion_prior_to_empty_line_up;
                    else if (state->motion == vim_motion_to_empty_line_down) state->motion = vim_motion_prior_to_empty_line_down;
                }

                i64 pos = view_get_cursor_pos(app, view);

                Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, state->motion, pos, state->motion_count);
                range = mr.range;
                inclusive = HasFlag(mr.flags, VimMotionFlag_Inclusive);

                if (HasFlag(mr.flags, VimMotionFlag_AlwaysSeek)) {
                    vim_seek_motion_result(app, view, buffer, mr);
                }
            } else {
                result = false;
            }
        } else {
            range = Ii64(view_get_cursor_pos(app, view));
        }

        if (inclusive) {
            // range.max = Min(view_set_pos_by_character_delta(app, view, range.max, 1), get_line_end_pos_from_pos(app, buffer, range.max));
            range.max = view_set_pos_by_character_delta(app, view, range.max, 1);
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

#if 1
enum Vim_DCY {
    VimDCY_Delete,
    VimDCY_Change,
    VimDCY_Yank,
};

internal void vim_delete_change_or_yank(Application_Links* app, Vim_Binding_Handler* handler, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, Vim_DCY mode, Vim_Operator* op, b32 query_motion, Vim_Motion* motion = 0) {
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

    Vim_Operator_State state = vim_build_op_state(op, op_flags);
    state.motion = motion;
    while (vim_get_operator_range(&state, &range)) {
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
            if (line_range.min != line_range.max) {
                if (mode == VimDCY_Change) {
                    auto_indent_line_at_cursor(app);
                }
            }
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

    if (mode == VimDCY_Change && did_act) {
        if (selection.kind == VimSelectionKind_Line || selection.kind == VimSelectionKind_Block) {
            vim_enter_visual_insert_mode(app);
        } else {
            vim_enter_insert_mode(app);
        }
    }
}

internal VIM_OPERATOR(vim_delete) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Delete, vim_delete, true);
}

internal VIM_OPERATOR(vim_delete_character) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Delete, vim_delete_character, false);
}

internal VIM_OPERATOR(vim_change) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Change, vim_change, true);
}

internal VIM_OPERATOR(vim_yank) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Yank, vim_yank, true);
}

internal VIM_OPERATOR(vim_delete_eol) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Delete, vim_delete, false, vim_motion_line_end_textual);
}

internal VIM_OPERATOR(vim_change_eol) {
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Change, vim_change, false, vim_motion_line_end_textual);
}

internal VIM_OPERATOR(vim_yank_eol) {
    // Note: This is vim's default behaviour (cuz it's vi compatible)
    // vim_delete_change_or_yank(app, handler, view, buffer, selection, count, vim_yank, false, vim_motion_enclose_line_textual);
    vim_delete_change_or_yank(app, handler, view, buffer, selection, count, VimDCY_Yank, vim_yank, false, vim_motion_line_end_textual);
}

#else

internal VIM_OPERATOR(vim_delete) {
    Scratch_Block scratch(app);
    List_String_Const_u8 list = {};

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    String_Const_u8 eol = (*eol_setting == LineEndingKind_CRLF) ? SCu8("\r\n") : SCu8("\n");

    b32 insert_final_newline = false;
    i64 prev_line = 0;

    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_delete, VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);

        i64 line = line_range.min;
        if (prev_line && prev_line != line) {
            insert_final_newline = true;
            string_list_push(scratch, &list, eol);
        }

        String_Const_u8 cut_string = vim_cut_range(app, scratch, view, buffer, range);
        string_list_push(scratch, &list, cut_string);
        if (line_range.min != line_range.max) {
            auto_indent_buffer(app, buffer, Ii64(range.min, range.min + 1));
        }

        prev_line = line;
    }

    if (insert_final_newline) {
        string_list_push(scratch, &list, eol);
    }

    String_Const_u8 cut_string = string_list_flatten(scratch, list, StringFill_NoTerminate);
    if (cut_string.size) {
        b32 from_block_copy = (selection.kind == VimSelectionKind_Block);
        vim_write_register(app, vim_state.active_register, cut_string, from_block_copy);
    }
}

internal VIM_OPERATOR(vim_change) {
    Scratch_Block scratch(app);
    List_String_Const_u8 list = {};

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    String_Const_u8 eol = (*eol_setting == LineEndingKind_CRLF) ? SCu8("\r\n") : SCu8("\n");

    b32 insert_final_newline = false;
    i64 prev_line = 0;

    b32 did_act = false;
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_change, VimOpFlag_ChangeBehaviour|VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer, range);

        did_act = true;
        i64 line = line_range.min;
        if (prev_line && prev_line != line) {
            insert_final_newline = true;
            string_list_push(scratch, &list, eol);
        }

        String_Const_u8 cut_string = vim_cut_range(app, scratch, view, buffer, range);
        string_list_push(scratch, &list, cut_string);
        if (line_range.min != line_range.max) {
            auto_indent_line_at_cursor(app);
        }

        prev_line = line;
    }

    if (insert_final_newline) {
        string_list_push(scratch, &list, eol);
    }

    String_Const_u8 cut_string = string_list_flatten(scratch, list, StringFill_NoTerminate);
    if (cut_string.size) {
        b32 from_block_copy = (selection.kind == VimSelectionKind_Block);
        vim_write_register(app, vim_state.active_register, cut_string, from_block_copy);
    }

    if (did_act) {
        if (selection.kind == VimSelectionKind_Line || selection.kind == VimSelectionKind_Block) {
            History_Record_Index current_history_index = buffer_history_get_current_state_index(app, buffer);
            Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
            vim_buffer->history_index_post_change = current_history_index;
            vim_enter_visual_insert_mode(app);
        } else {
            vim_enter_insert_mode(app);
        }
    }
}

internal VIM_OPERATOR(vim_yank) {
    Scratch_Block scratch(app);
    List_String_Const_u8 list = {};

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind* eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    String_Const_u8 eol = (*eol_setting == LineEndingKind_CRLF) ? SCu8("\r\n") : SCu8("\n");

    b32 insert_final_newline = false;
    i64 prev_line = 0;

    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_yank, VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        i64 line = get_line_number_from_pos(app, buffer, range.min);
        if (prev_line && prev_line != line) {
            insert_final_newline = true;
            string_list_push(scratch, &list, eol);
        }
        prev_line = line;
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);
        string_list_push(scratch, &list, string);
    }

    if (insert_final_newline) {
        string_list_push(scratch, &list, eol);
    }

    String_Const_u8 final_string = string_list_flatten(scratch, list, StringFill_NoTerminate);
    if (final_string.size) {
        b32 from_block_copy = (selection.kind == VimSelectionKind_Block);
        vim_write_register(app, vim_state.active_register, final_string, from_block_copy);
        vim_write_register(app, vim_get_register('0'), final_string, from_block_copy);
    }
}
#endif

internal VIM_OPERATOR(vim_replace) {
	String_Const_u8 replace_char = vim_get_next_writable(app);

    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_replace, 0);
    while (vim_get_operator_range(&state, &range)) {
        for (i64 replace_cursor = range.min; replace_cursor < range.max; replace_cursor++) {
            buffer_replace_range(app, buffer, Ii64(replace_cursor, view_set_pos_by_character_delta(app, view, replace_cursor, 1)), replace_char);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_register) {
    String_Const_u8 reg_str = vim_get_next_writable(app);
    if (reg_str.size) {
        u8 reg_char = reg_str.str[0];
        Vim_Register* reg = vim_get_register(reg_char);
        if (reg) {
            vim_state.active_register = reg;

            User_Input dummy = vim_get_next_key(app); // Note: Because vim_handle_input checks current input
            vim_handle_input(app);
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

        Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, true);

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

            i64 visual_block_line_count = (selection.one_past_last_line - selection.first_line);

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
                selection.kind               = VimSelectionKind_Block;
                selection.first_line         = cursor.line;
                selection.one_past_last_line = cursor.line + Max(1, newline_count);
                selection.first_col          = selection.one_past_last_col = cursor.col + (post_cursor ? 1 : 0);
            }

            Node_String_Const_u8* line = paste_lines.first;
            while (vim_selection_consume_line(app, buffer, &selection, &replace_range, true)) {
                String_Const_u8 replaced_string = push_buffer_range(app, scratch, buffer, replace_range);
                string_list_push(scratch, &replaced_string_list, replaced_string);
                if (visual_block_line_count > 0) {
                    string_list_push(scratch, &replaced_string_list, newline_needle);
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
                buffer_post_fade(app, buffer, 0.667f, Ii64_size(replace_range.min, paste_string.size), argb);
            }
            String_Const_u8 replaced_string = string_list_flatten(scratch, replaced_string_list, StringFill_NoTerminate);
            if (replaced_string.size) {
                vim_write_register(app, vim_state.active_register, replaced_string);
            }
        } else {
            b32 contains_newlines = false;
            for (u64 character_index = 0; character_index < paste_string.size; character_index++) {
                if (character_is_newline(paste_string.str[character_index])) {
                    contains_newlines = true;
                    break;
                }
            }
    
            if (post_cursor && contains_newlines) {
                move_down(app);
                seek_beginning_of_line(app);
            }
            
            Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(view_get_cursor_pos(app, view)));
            if (post_cursor && !contains_newlines) {
                cursor = buffer_compute_cursor(app, buffer, seek_line_col(cursor.line, cursor.col + 1));
            }
            
            Range_i64 replace_range = Ii64(cursor.pos);
            if (selection.kind == VimSelectionKind_Range || selection.kind == VimSelectionKind_Line) {
                replace_range = selection.range;
            }
            
            String_Const_u8 replaced_string = push_buffer_range(app, scratch, buffer, replace_range);
            buffer_replace_range(app, buffer, replace_range, paste_string);
#if VIM_AUTO_INDENT_ON_PASTE
            auto_indent_buffer(app, buffer, Ii64_size(cursor.pos, paste_string.size));
#endif
            move_past_lead_whitespace(app, view, buffer);
            
            if (replaced_string.size) {
                vim_write_register(app, vim_state.active_register, replaced_string);
            }
            
            ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
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
    Vim_Operator_State state = vim_build_op_state(vim_lowercase, VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        Scratch_Block scratch(app);
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);

        string_mod_lower(string);

        buffer_replace_range(app, buffer, range, string);
    }
}

internal VIM_OPERATOR(vim_uppercase) {
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_uppercase, VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        Scratch_Block scratch(app);
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);

        string_mod_upper(string);

        buffer_replace_range(app, buffer, range, string);
    }
}

internal VIM_OPERATOR(vim_auto_indent) {
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(vim_auto_indent, VimOpFlag_QueryMotion);
    while (vim_get_operator_range(&state, &range)) {
        auto_indent_buffer(app, buffer, range);
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
                // Note: Offset the seek position to skip over the range of the matched string, just in case for some reason
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
                    // Note: Well, we didn't align yet. But we will.
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

internal void vim_align_internal(Application_Links* app, Vim_Binding_Handler* handler, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, b32 align_right) {
    if (selection.kind == VimSelectionKind_Block) {
        String_Const_u8 align_target = vim_get_next_writable(app);
        Range_i64 line_range = Ii64(selection.first_line, selection.one_past_last_line);
        Range_i64 col_range  = Ii64(selection.first_col , selection.one_past_last_col );
        for (i32 i = 0; i < count; i++) {
            vim_align_range(app, buffer, line_range, col_range, align_target, align_right);
        }
    } else {
        String_Const_u8 align_target = SCu8();
        Range_i64 range = {};
        Vim_Operator_State state = vim_build_op_state(vim_lowercase, VimOpFlag_QueryMotion);
        while (vim_get_operator_range(&state, &range)) {
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
    vim_align_internal(app, handler, view, buffer, selection, count, false);
}

internal VIM_OPERATOR(vim_align_right) {
    vim_align_internal(app, handler, view, buffer, selection, count, true);
}

internal VIM_OPERATOR(vim_new_line_below) {
    seek_end_of_line(app);
    vim_write_text_and_auto_indent_internal(app, string_u8_litexpr("\n"));
    vim_enter_insert_mode(app);
}

internal VIM_OPERATOR(vim_new_line_above) {
    seek_beginning_of_line(app);
    move_left(app);
    vim_write_text_and_auto_indent_internal(app, string_u8_litexpr("\n"));
    vim_enter_insert_mode(app);
}

global String_Const_u8 vim_recognised_line_comment_tokens[] = {
    string_u8_litexpr("//"),
    string_u8_litexpr("--"),
};

internal b32 vim_line_comment_starts_at_position(Application_Links *app, Buffer_ID buffer, i64 pos) {
    b32 result = false;

    for (u32 token_index = 0; token_index < ArrayCount(vim_recognised_line_comment_tokens); token_index++) {
        Scratch_Block scratch(app);

        String_Const_u8 token = vim_recognised_line_comment_tokens[token_index];
        String_Const_u8 text = push_buffer_range(app, scratch, buffer, Ii64(pos, pos + token.size));
        if (string_match(token, text)) {
            result = true;
            break;
        }
    }

    return result;
}

internal VIM_OPERATOR(vim_join_line) {
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    i64 line_end = get_line_end_pos(app, buffer, line);
    i64 next_line = line + 1;
    i64 next_line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, next_line);
	
    if (vim_line_comment_starts_at_position(app, buffer, next_line_start)) {
        next_line_start += 2;
        for (;;) {
            u8 c = buffer_get_char(app, buffer, next_line_start);
            if (!character_is_newline(c) && character_is_whitespace(c)) {
                next_line_start++;
            } else {
                break;
            }
        }
    }
	
    i64 new_pos = view_get_character_legal_pos_from_pos(app, view, line_end);
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
	
    buffer_replace_range(app, buffer, Ii64(line_end, next_line_start), string_u8_litexpr(" "));
}

internal void vim_miblo_internal(Application_Links* app, Buffer_ID buffer, Range_i64 range, i64 delta) {
    for (i64 scan_pos = range.min; scan_pos <= range.max;) {
        Miblo_Number_Info number = {};
        if (get_numeric_at_cursor(app, buffer, scan_pos, &number)) {
            Scratch_Block scratch(app);
            String_Const_u8 str = push_u8_stringf(scratch, "%lld", number.x + delta);
            buffer_replace_range(app, buffer, number.range, str);
            scan_pos = number.range.max;
        } else {
            scan_pos++;
        }
    }
}

internal VIM_OPERATOR(vim_miblo_increment) {
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(0, 0);
    while (vim_get_operator_range(&state, &range)) {
        vim_miblo_internal(app, buffer, range, 1);
    }
}

internal VIM_OPERATOR(vim_miblo_decrement) {
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(0, 0);
    while (vim_get_operator_range(&state, &range)) {
        vim_miblo_internal(app, buffer, range, -1);
    }
}

internal VIM_OPERATOR(vim_miblo_increment_sequence) {
    i64 delta = 1;
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(0, 0);
    while (vim_get_operator_range(&state, &range)) {
        vim_miblo_internal(app, buffer, range, delta++);
    }
}

internal VIM_OPERATOR(vim_miblo_decrement_sequence) {
    i64 delta = -1;
    Range_i64 range = {};
    Vim_Operator_State state = vim_build_op_state(0, 0);
    while (vim_get_operator_range(&state, &range)) {
        vim_miblo_internal(app, buffer, range, delta--);
    }
}

CUSTOM_COMMAND_SIG(vim_split_window_horizontal) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Panel_ID panel = view_get_panel(app, view);
    if (panel) {
        panel_split(app, panel, Dimension_Y);
    }
}

CUSTOM_COMMAND_SIG(vim_split_window_vertical) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Panel_ID panel = view_get_panel(app, view);
    if (panel) {
        panel_split(app, panel, Dimension_X);
    }
}

CUSTOM_COMMAND_SIG(vim_open_file_in_quotes_in_same_window)
CUSTOM_DOC("Reads a filename from surrounding '\"' characters and attempts to open the corresponding file in the same window.")
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
        
        if (character_is_slash(string_get_character(path, path.size - 1))){
            path = string_chop(path, 1);
        }
        
        String_Const_u8 new_file_name = push_u8_stringf(scratch, "%.*s/%.*s", string_expand(path), string_expand(quoted_name));
        
        if (view_open_file(app, view, new_file_name, true)){
            view_set_active(app, view);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_undo)
CUSTOM_DOC("Advances backwards through the undo history of the current buffer.")
{
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
CUSTOM_DOC("Advances forwards through the undo history of the current buffer.")
{
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
    vim_log_jump_history(app);
    page_up(app);
}

CUSTOM_COMMAND_SIG(vim_page_down) {
    vim_log_jump_history(app);
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
            jump_to_location(app, view, first_jump.buffer, first_jump.pos);
        }
    }
}

CUSTOM_COMMAND_SIG(vim_cycle_definitions_forward) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Tiny_Jump jump = vim_state.definition_stack[++vim_state.definition_stack_cursor % vim_state.definition_stack_count];
    jump_to_location(app, view, jump.buffer, jump.pos);
}

CUSTOM_COMMAND_SIG(vim_cycle_definitions_backward) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Tiny_Jump jump = vim_state.definition_stack[--vim_state.definition_stack_cursor % vim_state.definition_stack_count];
    jump_to_location(app, view, jump.buffer, jump.pos);
}

CUSTOM_COMMAND_SIG(q)
CUSTOM_DOC("Closes the current panel, or 4coder if this is the last panel.")
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
CUSTOM_DOC("Saves the current buffer.")
{
    save(app);
}

CUSTOM_COMMAND_SIG(wq)
CUSTOM_DOC("Saves the current buffer and closes the current panel, or 4coder if this is the last panel.")
{
    save(app);
    q(app);
}

CUSTOM_COMMAND_SIG(qa)
CUSTOM_DOC("Closes 4coder.")
{
    exit_4coder(app);
}

CUSTOM_COMMAND_SIG(wqa)
CUSTOM_DOC("Saves all buffers and closes 4coder.")
{
    save_all_dirty_buffers(app);
    qa(app);
}

CUSTOM_COMMAND_SIG(e)
CUSTOM_DOC("Interactively open or create a new file.")
{
    interactive_open_or_new(app);
}

CUSTOM_COMMAND_SIG(b)
CUSTOM_DOC("Interactively switch to an open buffer.")
{
    interactive_switch_buffer(app);
}

CUSTOM_COMMAND_SIG(ta)
CUSTOM_DOC("Öpen jump-to-definition lister.")
{
    jump_to_definition(app);
}

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
            if (character_is_newline(buffer_get_char(app, buffer, i))) {
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
    // draw_highlight_range(app, view, buffer, text_layout_id, roundness);

    i32 cursor_sub_id = default_cursor_sub_id();
    
    i64 cursor_pos = view_get_cursor_pos(app, view);
    if (is_active_view) {
        if (is_vim_insert_mode(mode)) {
            draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(defcolor_cursor, cursor_sub_id));
        } else {
            Vim_Visual_Selection selection = vim_get_selection(app, view, buffer, true);

            if (selection.kind) {
                Range_i64 range = {};
                while (vim_selection_consume_line(app, buffer, &selection, &range, true)) {
                    vim_draw_character_block_selection(app, buffer, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
                    paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
                }
            }

            vim_draw_character_block_selection(app, buffer, text_layout_id, Ii64(cursor_pos, cursor_pos + 1), roundness, fcolor_id(defcolor_cursor, cursor_sub_id));
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
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        draw_cpp_token_colors(app, text_layout_id, &token_array);
		
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                { string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0) },
                { string_u8_litexpr("Note"), finalize_color(defcolor_comment_pop, 0) },
                { string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1) },
                { string_u8_litexpr("Todo"), finalize_color(defcolor_comment_pop, 1) },
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
    if (!is_vim_visual_mode(vim_buffer->mode) && global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
    }
	
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = VIM_CURSOR_ROUNDNESS;
    f32 mark_thickness = 2.f;
	
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);

    if (vim_state.search_show_highlight) {
        // Note: Vim Search highlight
        i64 pos = visible_range.min;
        while (pos < visible_range.max) {
            Range_i64 highlight_range = vim_search_once_internal(app, view_id, buffer, Scan_Forward, pos, vim_state.last_search_register.string.string, vim_state.search_case_sensitive, true);
            if (!range_size(highlight_range)) {
                break;
            }
            vim_draw_character_block_selection(app, buffer, text_layout_id, highlight_range, cursor_roundness, fcolor_id(defcolor_highlight_white));
            pos = highlight_range.max;
        }
    }

#if VIM_USE_CHARACTER_SEEK_HIGHLIGHTS
    if (is_active_view && vim_state.character_seek_show_highlight) {
        // Note: Vim Character Seek highlight
        i64 pos = view_get_cursor_pos(app, view_id);
        while (range_contains(visible_range, pos)) {
            Vim_Motion_Result mr = vim_motion_seek_character_internal(app, view_id, buffer, pos, SCu8(), vim_state.character_seek_highlight_dir, vim_state.most_recent_character_seek_inclusive, true);

            if (mr.seek_pos == pos) {
                break;
            }

            pos = mr.seek_pos;

            vim_draw_character_block_selection(app, buffer, text_layout_id, Ii64_size(mr.seek_pos, vim_state.most_recent_character_seek.size), cursor_roundness, fcolor_id(defcolor_highlight));
        }
    }
#endif
	
    // NOTE(allen): Cursor
    vim_draw_cursor(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, vim_buffer->mode);
	
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer, view_id);
	
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
	
    draw_set_clip(app, prev_clip);
}

function void vim_draw_file_bar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar) {
    Scratch_Block scratch(app);
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    ARGB_Color bar_color = fcolor_resolve(fcolor_id(defcolor_bar));

    switch (vim_buffer->mode) {
        case VimMode_Normal: {
            bar_color = VIM_NORMAL_COLOR;
        } break;

        case VimMode_VisualInsert:
        case VimMode_Insert: {
            bar_color = VIM_INSERT_COLOR;
        } break;

        case VimMode_Visual:
        case VimMode_VisualLine:
        case VimMode_VisualBlock: {
            bar_color = VIM_VISUAL_COLOR;
        } break;
    }
	
    draw_rectangle(app, bar, 0.f, bar_color);
	
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
	
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

function void vim_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id) {
    ProfileScope(app, "default render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
	
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
	
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
	
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        vim_draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
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
    ProfileScope(app, "begin buffer");
	
    Scratch_Block scratch(app);
	
    b32 treat_as_code = false;
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    if (file_name.size > 0){
        String_Const_u8_Array extensions = global_config.code_exts;
        String_Const_u8 ext = string_file_extension(file_name);
        for (i32 i = 0; i < extensions.count; ++i){
            if (string_match(ext, extensions.strings[i])){
                if (string_match(ext, string_u8_litexpr("cpp")) ||
                    string_match(ext, string_u8_litexpr("h"))   ||
                    string_match(ext, string_u8_litexpr("c"))   ||
                    string_match(ext, string_u8_litexpr("hpp")) ||
                    string_match(ext, string_u8_litexpr("cc")))
                {
                    treat_as_code = true;
                }
				
                break;
            }
        }
    }
	
    Command_Map_ID map_id = vim_mapid_normal;
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = map_id;
	
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
    vim_buffer->treat_as_code = treat_as_code;
	
    Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = setting;
	
    // NOTE(allen): Decide buffer settings
    b32 wrap_lines = true;
    b32 use_virtual_whitespace = false;
    b32 use_lexer = false;
    if (treat_as_code){
        wrap_lines = global_config.enable_code_wrapping;
        use_virtual_whitespace = global_config.enable_virtual_whitespace;
        use_lexer = true;
    }
	
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    if (string_match(buffer_name, string_u8_litexpr("*compilation*"))){
        wrap_lines = false;
    }
	
    if (use_lexer){
        ProfileBlock(app, "begin buffer kick off lexer");
        Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
        *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async, make_data_struct(&buffer_id));
    }
	
    {
        b32 *wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
        *wrap_lines_ptr = wrap_lines;
    }
	
    if (use_virtual_whitespace){
        if (use_lexer){
            buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
        }
        else{
            buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
        }
    }
    else{
        buffer_set_layout(app, buffer_id, layout_generic);
    }
	
    // no meaning for return
    return(0);
}

function void vim_tick(Application_Links *app, Frame_Info frame_info) {
    default_tick(app, frame_info);

    vim_state.playing_command = false;
    if (vim_state.played_macro) {
        vim_state.played_macro = false;
        history_group_end(vim_state.macro_history);
    }
	
    // @TODO: Run this on all views/buffers
    View_ID view = get_active_view(app, Access_Visible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Visible);
    if (buffer_exists(app, buffer)) {
        Buffer_ID scratch_buffer = get_buffer_by_name(app, string_u8_litexpr("*scratch*"), Access_Always);
        Managed_Scope scope = view_get_managed_scope(app, view);
        Vim_View_Attachment* vim_view = scope_attachment(app, scope, vim_view_attachment, Vim_View_Attachment);

        if (vim_view->most_recent_known_buffer &&
            vim_view->most_recent_known_buffer != buffer &&
            vim_view->most_recent_known_buffer != scratch_buffer)
        {
            if (vim_view->dont_log_this_buffer_jump) {
                vim_view->dont_log_this_buffer_jump = false;
            } else {
                vim_log_jump_history_internal(app, view, vim_view->most_recent_known_buffer, vim_view, vim_view->most_recent_known_pos);
            }
            vim_view->previous_buffer = vim_view->most_recent_known_buffer;
            vim_view->pos_in_previous_buffer = vim_view->most_recent_known_pos;
        }
		
        vim_view->most_recent_known_buffer = buffer;
        vim_view->most_recent_known_pos = view_get_cursor_pos(app, view);
    }
}

CUSTOM_COMMAND_SIG(vim_view_input_handler)
CUSTOM_DOC("Input consumption loop for vim view behavior")
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
        // NOTE(allen): Get the binding from the buffer's current map
        User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
        ProfileScopeNamed(app, "before view input", view_input_profile);
        if (input.abort) {
            break;
        }
        
        Event_Property event_properties = get_event_properties(&input.event);
        
        if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0){
            continue;
        }
        
        View_ID view = get_this_ctx_view(app, Access_Always);
        
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
        Command_Map_ID *map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
        if (*map_id_ptr == 0) {
            *map_id_ptr = mapid_file;
        }
        Command_Map_ID map_id = *map_id_ptr;
        
        Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, &input.event);
        
        Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, buffer_scope, vim_buffer_attachment, Vim_Buffer_Attachment);
        Vim_Binding_Handler* handler = vim_get_handler_for_mode(vim_buffer->mode);

        Vim_Key_Binding* vim_bind = 0;
        if (handler && handler->initialized) {
            Vim_Key vim_key = vim_get_key_from_input(input);
            vim_bind = make_or_retrieve_vim_binding(handler, &handler->keybind_table, vim_key, false);
        }

        Managed_Scope scope = view_get_managed_scope(app, view);
        
        if (!binding.custom && !vim_bind) {
            // NOTE(allen): we don't have anything to do with this input,
            // leave it marked unhandled so that if there's a follow up
            // event it is not blocked.
            leave_current_input_unhandled(app);
        } else {
            // NOTE(allen): before the command is called do some book keeping
            Rewrite_Type *next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
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
                vim_handle_input(app);
            } else {
                // NOTE(allen): call the command
                binding.custom(app);
            }
            
            // NOTE(allen): after the command is called do some book keeping
            ProfileScope(app, "after view input");
            
            next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
            if (next_rewrite != 0) {
                Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
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
    }
}

//
//
//

function void vim_setup_default_mapping(Application_Links* app, Mapping *mapping) {
    MappingScope();
    SelectMapping(mapping);
	
    SelectMap(mapid_global);
    {
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
    }
	
    SelectMap(vim_mapid_shared);
    {
        ParentMap(mapid_global);
        BindMouse(click_set_cursor_and_mark, MouseCode_Left);
        BindMouseRelease(click_set_cursor, MouseCode_Left);
        BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
        BindMouseMove(click_set_cursor_if_lbutton);
        Bind(center_view,                                   KeyCode_E, KeyCode_Control);
        Bind(left_adjust_view,                              KeyCode_E, KeyCode_Control, KeyCode_Shift);
        Bind(search,                                        KeyCode_F, KeyCode_Control);
        Bind(list_all_locations,                            KeyCode_F, KeyCode_Control, KeyCode_Shift);
        Bind(list_all_substring_locations_case_insensitive, KeyCode_F, KeyCode_Alt);
        Bind(list_all_locations_of_selection,               KeyCode_G, KeyCode_Control, KeyCode_Shift);
        Bind(snippet_lister,                                KeyCode_J, KeyCode_Control);
        Bind(kill_buffer,                                   KeyCode_K, KeyCode_Control, KeyCode_Shift);
        Bind(duplicate_line,                                KeyCode_L, KeyCode_Control);
        Bind(cursor_mark_swap,                              KeyCode_M, KeyCode_Control);
        Bind(reopen,                                        KeyCode_O, KeyCode_Control, KeyCode_Shift);
        Bind(query_replace,                                 KeyCode_Q, KeyCode_Control);
        Bind(query_replace_identifier,                      KeyCode_Q, KeyCode_Control, KeyCode_Shift);
        Bind(query_replace_selection,                       KeyCode_Q, KeyCode_Alt);
        Bind(reverse_search,                                KeyCode_R, KeyCode_Control);
        Bind(save,                                          KeyCode_S, KeyCode_Control);
        Bind(save_all_dirty_buffers,                        KeyCode_S, KeyCode_Control, KeyCode_Shift);
        Bind(search_identifier,                             KeyCode_T, KeyCode_Control);
        Bind(list_all_locations_of_identifier,              KeyCode_T, KeyCode_Control, KeyCode_Shift);
        Bind(view_buffer_other_panel,                       KeyCode_1, KeyCode_Control);
        Bind(swap_panels,                                   KeyCode_2, KeyCode_Control);
    }
	
    SelectMap(mapid_file);
    {
        ParentMap(vim_mapid_shared);
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
        Bind(replace_in_range,                            KeyCode_A, KeyCode_Control);
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
        Bind(vim_enter_normal_mode,                       KeyCode_Escape);
    }
	
    SelectMap(mapid_code);
    {
        ParentMap(mapid_file);
        BindTextInput(write_text_and_auto_indent);
        Bind(move_left_alpha_numeric_boundary,                    KeyCode_Left, KeyCode_Control);
        Bind(move_right_alpha_numeric_boundary,                   KeyCode_Right, KeyCode_Control);
        Bind(move_left_alpha_numeric_or_camel_boundary,           KeyCode_Left, KeyCode_Alt);
        Bind(move_right_alpha_numeric_or_camel_boundary,          KeyCode_Right, KeyCode_Alt);
        Bind(comment_line_toggle,                                 KeyCode_Semicolon, KeyCode_Control);
        Bind(word_complete,                                       KeyCode_Tab);
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
    }
	
    Vim_Key vim_leader = vim_key(KeyCode_Space);
	
    SelectMap(vim_mapid_normal);
    {
        ParentMap(vim_mapid_shared);
		
        initialize_vim_binding_handler(app, &vim_map_operator_pending);
		
        vim_bind_motion(&vim_map_operator_pending, vim_motion_left,                           vim_key(KeyCode_H));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_down,                           vim_key(KeyCode_J));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_up,                             vim_key(KeyCode_K));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_right,                          vim_key(KeyCode_L));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_left,                           vim_key(KeyCode_Left));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_down,                           vim_key(KeyCode_Down));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_up,                             vim_key(KeyCode_Up));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_right,                          vim_key(KeyCode_Right));

        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_empty_line_down,             vim_key(KeyCode_LeftBracket, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_empty_line_up,               vim_key(KeyCode_RightBracket, KeyCode_Shift));
        
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word,                           vim_key(KeyCode_W));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word_end,                       vim_key(KeyCode_E));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word_backward,                  vim_key(KeyCode_B));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_line_start_textual,             vim_key(KeyCode_0));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_line_end_textual,               vim_char('$'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_scope,                          vim_char('%'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_buffer_start,                   vim_key(KeyCode_G), vim_key(KeyCode_G));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_buffer_end,                     vim_key(KeyCode_G, KeyCode_Shift));
		
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character,                 vim_key(KeyCode_F));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character_pair,            vim_key(KeyCode_S));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_character,                   vim_key(KeyCode_T));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character_backward,        vim_key(KeyCode_F, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character_pair_backward,   vim_key(KeyCode_S, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_character_backward,          vim_key(KeyCode_T, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_repeat_character_seek_forward,  vim_key(KeyCode_Semicolon));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_repeat_character_seek_backward, vim_key(KeyCode_Comma));

        vim_bind_motion(&vim_map_operator_pending, vim_motion_isearch_repeat_forward,         vim_key(KeyCode_G), vim_key(KeyCode_N));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_isearch_repeat_backward,        vim_key(KeyCode_G), vim_key(KeyCode_N, KeyCode_Shift));
        
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_scope,                    vim_key(KeyCode_I), vim_char('{'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_scope,                    vim_key(KeyCode_I), vim_char('}'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_paren,                    vim_key(KeyCode_I), vim_char('('));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_paren,                    vim_key(KeyCode_I), vim_char(')'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_single_quotes,            vim_key(KeyCode_I), vim_char('\''));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_double_quotes,            vim_key(KeyCode_I), vim_char('"'));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_word,                     vim_key(KeyCode_I), vim_key(KeyCode_W));

        initialize_vim_binding_handler(app, &vim_map_normal, &vim_map_operator_pending);

        // Note: These are here (maybe temporarily, maybe not) to make entering a count for an operator possible.
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_1));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_2));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_3));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_4));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_5));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_6));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_7));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_8));
		vim_bind_generic_command(&vim_map_normal, vim_handle_input,                           vim_key(KeyCode_9));

        vim_bind_generic_command(&vim_map_normal, vim_register,                               vim_char('"'));

		vim_bind_operator(&vim_map_normal, vim_change,                                        vim_key(KeyCode_C));
        vim_bind_operator(&vim_map_normal, vim_delete,                                        vim_key(KeyCode_D));
        vim_bind_operator(&vim_map_normal, vim_delete_character,                              vim_key(KeyCode_X));
        vim_bind_operator(&vim_map_normal, vim_yank,                                          vim_key(KeyCode_Y));
		vim_bind_operator(&vim_map_normal, vim_change_eol,                                    vim_key(KeyCode_C, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_delete_eol,                                    vim_key(KeyCode_D, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_yank_eol,                                      vim_key(KeyCode_Y, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_auto_indent,                                   vim_key(KeyCode_Equal));
		vim_bind_operator(&vim_map_normal, vim_replace,                                       vim_key(KeyCode_R));
        vim_bind_operator(&vim_map_normal, vim_new_line_below,                                vim_key(KeyCode_O));
        vim_bind_operator(&vim_map_normal, vim_new_line_above,                                vim_key(KeyCode_O, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_join_line,                                     vim_key(KeyCode_J, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_align,                                         vim_key(KeyCode_G), vim_key(KeyCode_L));
        vim_bind_operator(&vim_map_normal, vim_align_right,                                   vim_key(KeyCode_G), vim_key(KeyCode_L, KeyCode_Shift));
		
        vim_bind_operator(&vim_map_normal, vim_paste,                                         vim_key(KeyCode_P));
        vim_bind_operator(&vim_map_normal, vim_paste_pre_cursor,                              vim_key(KeyCode_P, KeyCode_Shift));
        
        vim_bind_operator(&vim_map_normal, vim_lowercase, vim_key(KeyCode_G),                 vim_key(KeyCode_U));
        vim_bind_operator(&vim_map_normal, vim_uppercase, vim_key(KeyCode_G),                 vim_key(KeyCode_U, KeyCode_Shift));

        vim_bind_operator(&vim_map_normal, vim_miblo_increment,                               vim_key(KeyCode_A, KeyCode_Control));
        vim_bind_operator(&vim_map_normal, vim_miblo_decrement,                               vim_key(KeyCode_X, KeyCode_Control));
        vim_bind_operator(&vim_map_normal, vim_miblo_increment_sequence,                      vim_key(KeyCode_G), vim_key(KeyCode_A, KeyCode_Control));
        vim_bind_operator(&vim_map_normal, vim_miblo_decrement_sequence,                      vim_key(KeyCode_G), vim_key(KeyCode_X, KeyCode_Control));
        
        vim_bind_text_command(&vim_map_normal, vim_enter_insert_mode,                         vim_key(KeyCode_I));
        vim_bind_text_command(&vim_map_normal, vim_enter_insert_sol_mode,                     vim_key(KeyCode_I, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, vim_enter_append_mode,                         vim_key(KeyCode_A));
        vim_bind_text_command(&vim_map_normal, vim_enter_append_eol_mode,                     vim_key(KeyCode_A, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, vim_toggle_visual_mode,                        vim_key(KeyCode_V));
        vim_bind_text_command(&vim_map_normal, vim_toggle_visual_line_mode,                   vim_key(KeyCode_V, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, vim_toggle_visual_block_mode,                  vim_key(KeyCode_V, KeyCode_Control));

        vim_bind_generic_command(&vim_map_normal, vim_record_macro,                           vim_key(KeyCode_Q));
        vim_bind_text_command(&vim_map_normal,    vim_replay_macro,                           vim_key(KeyCode_2, KeyCode_Shift));
        
        vim_bind_generic_command(&vim_map_normal, center_view,                                vim_key(KeyCode_Z), vim_key(KeyCode_Z));
		
        vim_bind_generic_command(&vim_map_normal, change_active_panel,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_W));
        vim_bind_generic_command(&vim_map_normal, change_active_panel,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_W, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, swap_panels,                                vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_X));
        vim_bind_generic_command(&vim_map_normal, swap_panels,                                vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_X, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_left,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_left,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_down,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_down,                        vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_up,                          vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_up,                          vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_right,                       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_right,                       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_left,                   vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_left,                   vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_down,                   vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_down,                   vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_up,                     vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_up,                     vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_right,                  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_right,                  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, vim_split_window_vertical,                  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_V));
        vim_bind_generic_command(&vim_map_normal, vim_split_window_vertical,                  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_V, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_split_window_horizontal,                vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_S));
        vim_bind_generic_command(&vim_map_normal, vim_split_window_horizontal,                vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_S, KeyCode_Control));
		
        vim_bind_generic_command(&vim_map_normal, vim_page_up,                                vim_key(KeyCode_B, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_page_down,                              vim_key(KeyCode_F, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_half_page_up,                           vim_key(KeyCode_U, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_half_page_down,                         vim_key(KeyCode_D, KeyCode_Control));
		
        vim_bind_generic_command(&vim_map_normal, vim_step_back_jump_history,                 vim_key(KeyCode_O, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_step_forward_jump_history,              vim_key(KeyCode_I, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, vim_previous_buffer,                        vim_key(KeyCode_6, KeyCode_Control));

        vim_bind_generic_command(&vim_map_normal, vim_open_file_in_quotes_in_same_window,     vim_key(KeyCode_G), vim_key(KeyCode_F));
        vim_bind_generic_command(&vim_map_normal, vim_jump_to_definition_under_cursor,        vim_key(KeyCode_RightBracket, KeyCode_Control));

        vim_named_bind(&vim_map_normal, string_u8_litexpr("Files"), vim_leader, vim_key(KeyCode_F));
        vim_bind_generic_command(&vim_map_normal, interactive_open_or_new,                    vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_E));
        vim_bind_generic_command(&vim_map_normal, interactive_switch_buffer,                  vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_B));
        vim_bind_generic_command(&vim_map_normal, interactive_kill_buffer,                    vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_K));
        vim_bind_generic_command(&vim_map_normal, kill_buffer,                                vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_D));
        vim_bind_generic_command(&vim_map_normal, q,                                          vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_Q));
        vim_bind_generic_command(&vim_map_normal, qa,                                         vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_Q, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, save,                                       vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_W));

        vim_bind_generic_command(&vim_map_normal, noh,                                        vim_leader, vim_key(KeyCode_N));

        vim_bind_generic_command(&vim_map_normal, vim_isearch_repeat_select_forward,          vim_key(KeyCode_G), vim_key(KeyCode_N));
        vim_bind_generic_command(&vim_map_normal, vim_isearch_repeat_select_backward,         vim_key(KeyCode_G), vim_key(KeyCode_N, KeyCode_Shift));

        //
        
        Bind(vim_enter_normal_mode,                 KeyCode_Escape);
        Bind(vim_isearch_word_under_cursor,         KeyCode_8, KeyCode_Shift);
        Bind(vim_reverse_isearch_word_under_cursor, KeyCode_3, KeyCode_Shift);
        Bind(vim_repeat_most_recent_command,        KeyCode_Period);
        Bind(vim_move_line_up,                      KeyCode_K, KeyCode_Alt);
        Bind(vim_move_line_down,                    KeyCode_J, KeyCode_Alt);
        Bind(vim_isearch,                           KeyCode_ForwardSlash);
        Bind(vim_isearch_backward,                  KeyCode_ForwardSlash, KeyCode_Shift);
        Bind(vim_isearch_repeat_forward,            KeyCode_N);
        Bind(vim_isearch_repeat_backward,           KeyCode_N, KeyCode_Shift);
        Bind(goto_next_jump,                        KeyCode_N, KeyCode_Control);
        Bind(goto_prev_jump,                        KeyCode_P, KeyCode_Control);
        Bind(vim_undo,                              KeyCode_U);
        Bind(vim_redo,                              KeyCode_R, KeyCode_Control);
        Bind(command_lister,                        KeyCode_Semicolon, KeyCode_Shift);
    }
	
    SelectMap(vim_mapid_visual);
    {
        ParentMap(vim_mapid_normal);
        initialize_vim_binding_handler(app, &vim_map_visual, &vim_map_normal);

        vim_unbind(&vim_map_visual, vim_key(KeyCode_I));
        vim_unbind(&vim_map_visual, vim_key(KeyCode_A));
        
        vim_bind_text_command(&vim_map_visual, vim_enter_visual_insert_mode, vim_key(KeyCode_I, KeyCode_Shift));
        vim_bind_text_command(&vim_map_visual, vim_enter_visual_append_mode, vim_key(KeyCode_A, KeyCode_Shift));
        
        vim_bind_operator(&vim_map_visual, vim_lowercase, vim_key(KeyCode_U));
        vim_bind_operator(&vim_map_visual, vim_uppercase, vim_key(KeyCode_U, KeyCode_Shift));

        vim_bind_generic_command(&vim_map_visual, vim_isearch_selection,         vim_key(KeyCode_ForwardSlash));
        vim_bind_generic_command(&vim_map_visual, vim_reverse_isearch_selection, vim_key(KeyCode_ForwardSlash, KeyCode_Shift));
    }
}

internal void vim_init(Application_Links* app) {
    vim_state.arena = make_arena_system();
    heap_init(&vim_state.heap, &vim_state.arena);
    vim_state.alloc = base_allocator_on_heap(&vim_state.heap);

    vim_state.active_register = &vim_state.VIM_DEFAULT_REGISTER;
    vim_state.search_direction = Scan_Forward;
    vim_state.chord_bar = Su8(vim_state.chord_bar_storage, 0, ArrayCount(vim_state.chord_bar_storage));
}

internal void vim_set_default_hooks(Application_Links* app) {
    set_custom_hook(app, HookID_ViewEventHandler, vim_view_input_handler);
    set_custom_hook(app, HookID_RenderCaller, vim_render_caller);
    set_custom_hook(app, HookID_BeginBuffer, vim_begin_buffer);
    set_custom_hook(app, HookID_Tick, vim_tick);
}

#undef cast
#endif // FCODER_VIMMISH_CPP
