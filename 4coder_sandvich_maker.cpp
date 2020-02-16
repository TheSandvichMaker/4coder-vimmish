#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"
#include "clearfeld_windmove.cpp"

CUSTOM_ID(command_map, vim_mapid_shared);
CUSTOM_ID(command_map, vim_mapid_normal);
CUSTOM_ID(command_map, vim_mapid_visual);
CUSTOM_ID(attachment, vim_buffer_attachment);

#include "generated/managed_id_metadata.cpp"

// @TODO: What is ivy/helm?
// @TODO: Clean up input querying

#define cast(...) (__VA_ARGS__)

internal i64 get_vertical_pixel_move_pos(Application_Links *app, View_ID view, i64 pos, f32 pixels) {
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

internal i64 get_pos_of_visual_line_side(Application_Links *app, View_ID view, i64 pos, Side side) {
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    Vec2_f32 p = view_relative_xy_of_pos(app, view, cursor.line, pos);
    p.x = (side == Side_Min)?(0.f):(max_f32);
    i64 new_pos = view_pos_at_relative_xy(app, view, cursor.line, p);
    return new_pos;
}

internal Range_i64 maximize_visual_line_range(Application_Links* app, View_ID view, Range_i64 range, b32 inclusive) {
    Range_i64 result = Ii64(get_pos_of_visual_line_side(app, view, range.min, Side_Min),
                            get_pos_of_visual_line_side(app, view, range.max, Side_Max));
    if (!inclusive) {
        result.max = view_set_pos_by_character_delta(app, view, result.max, -1);
    }
    return result;
}

enum Vim_Mode {
    VimMode_Normal,
    VimMode_Insert,
    VimMode_Append,
    VimMode_Visual,
    VimMode_VisualLine,
    VimMode_VisualBlock,
};

inline b32 is_vim_insert_mode(Vim_Mode mode) {
    b32 result = ((mode == VimMode_Insert) ||
                  (mode == VimMode_Append));
    return result;
}

inline b32 is_vim_visual_mode(Vim_Mode mode) {
    b32 result = ((mode == VimMode_Visual)     ||
                  (mode == VimMode_VisualLine) ||
                  (mode == VimMode_VisualBlock));
    return result;
}

struct Vim_Motion_Result {
    Range_i64 range;
    i64 seek_pos;
    b32 inclusive;
    b32 set_preferred_x;
};

inline Vim_Motion_Result vim_null_motion(i64 pos) {
    Vim_Motion_Result result = {};
    result.range = Ii64(pos, pos);
    result.seek_pos = pos;
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
    Range_i64 range;
};

#define VIM_MOTION(name) Vim_Motion_Result name(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos)
typedef VIM_MOTION(Vim_Motion);

#define VIM_OPERATOR(name) void name(Application_Links* app, struct Vim_Binding_Handler* handler, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count)
typedef VIM_OPERATOR(Vim_Operator);

enum Vim_Buffer_Flag {
    VimBufferFlag_TreatAsCode = 0x1,
    VimBufferFlag_AllowSittingOnLineEnd = 0x2,
};

struct Vim_Buffer_Attachment {
    u32 flags;
    Vim_Mode mode;
};

enum Vim_Modifier {
    VimModifier_Control = 0x1,
    VimModifier_Alt     = 0x2,
    VimModifier_Shift   = 0x4,
    VimModifier_Command = 0x8,
};

struct Vim_Key {
    u32 kc;
    u32 mods;
};

inline u32 key_code_to_vim_modifier(Key_Code mod) {
    switch (mod) {
        case KeyCode_Control: return VimModifier_Control;
        case KeyCode_Alt:     return VimModifier_Alt;
        case KeyCode_Shift:   return VimModifier_Shift;
        case KeyCode_Command: return VimModifier_Command;
    }
    return 0;
}

inline Key_Code vim_modifier_to_key_code(u32 mod) {
    switch (mod) {
        case VimModifier_Control: return KeyCode_Control;
        case VimModifier_Alt:     return KeyCode_Alt;
        case VimModifier_Shift:   return KeyCode_Shift;
        case VimModifier_Command: return KeyCode_Command;
    }
    return 0;
}

inline Input_Modifier_Set_Fixed vim_modifiers_to_input_modifier_set_fixed(u32 mods) {
    Input_Modifier_Set_Fixed result = {};
    while (mods) {
        Key_Code kc = vim_modifier_to_key_code(mods & 1);
        if (kc) {
            result.mods[result.count++] = kc;
        }
        mods = mods >> 1;
    }
    return result;
}

inline u32 input_modifier_set_to_vim_modifiers_internal(i32 count, Key_Code* mods) {
    u32 result = 0;
    for (i32 mod_index = 0; mod_index < count; mod_index++) {
        u32 mod = key_code_to_vim_modifier(mods[mod_index]);
        if (mod) {
            result |= mod;
        }
    }
    return result;
}

inline u32 input_modifier_set_to_vim_modifiers(Input_Modifier_Set mods) {
    u32 result = input_modifier_set_to_vim_modifiers_internal(mods.count, mods.mods);
    return result;
}

inline u32 input_modifier_set_fixed_to_vim_modifiers(Input_Modifier_Set_Fixed mods) {
    u32 result = input_modifier_set_to_vim_modifiers_internal(mods.count, mods.mods);
    return result;
}

internal Vim_Key vim_key_v(Key_Code kc, va_list args) {
    Vim_Key result;
    result.kc = kc;
	
    Input_Modifier_Set_Fixed mods = {};
    while (mods.count < Input_MaxModifierCount) {
        i32 v = va_arg(args, i32);
        if (v <= 0) {
            break;
        }
        mods.mods[mods.count++] = v;
    }
	
    result.mods = input_modifier_set_fixed_to_vim_modifiers(mods);
    return result;
}

#define vim_key(kc, ...) vim_key_(kc, __VA_ARGS__, 0)
internal Vim_Key vim_key_(Key_Code kc, ...) {
    va_list args;
    va_start(args, kc);
    Vim_Key result = vim_key_v(kc, args);
    va_end(args);
    return result;
}

inline Vim_Key get_vim_key_from_input(User_Input in) {
    Vim_Key key = {};
    key.kc = in.event.key.code;
    key.mods = input_modifier_set_to_vim_modifiers(in.event.key.modifiers);
    return key;
}

inline b32 vim_keys_match(Vim_Key a, Vim_Key b) {
    b32 result = (a.kc == b.kc) && (a.mods == b.mods);
    return result;
}

inline u64 vim_key_as_u64(Vim_Key key) {
    u64 result = (cast(u64) key.kc) | ((cast(u64) key.mods) << 32);
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

struct Vim_Global_State {
    b32 started_command;
    i64 keyboard_buffer_cursor_on_most_recent_input;
    Range_i64 recorded_command_macro_range;
    History_Group command_history;
	
    u8              most_recent_character_seek_storage[4];
    String_u8       most_recent_character_seek;
    Scan_Direction  most_recent_character_seek_dir;
    b32             most_recent_character_seek_inclusive;
	
    u8        chord_bar_storage[32];
    String_u8 chord_bar;
};

global Vim_Global_State vim_state;

inline void vim_clear_chord_bar() {
    vim_state.chord_bar.size = 0;
}

struct Vim_Binding_Handler {
    Mapping* mapping;
    Command_Map* command_map;
	
    Arena node_arena;
    Heap heap;
    Base_Allocator allocator;
	
    Table_u64_u64 keybind_table;
};

global Vim_Binding_Handler vim_map_normal;
global Vim_Binding_Handler vim_map_visual;
global Vim_Binding_Handler vim_map_operator_pending;

CUSTOM_COMMAND_SIG(vim_handle_input);

#define InitializeVimBindingHandler(handler, app, ...) initialize_vim_binding_handler(handler, app, m, map, __VA_ARGS__)
internal void initialize_vim_binding_handler(Vim_Binding_Handler* handler, Application_Links* app, Mapping* mapping, Command_Map* command_map, Vim_Binding_Handler* parent = 0) {
    block_zero_struct(handler);
	
    handler->mapping = mapping;
    handler->command_map = command_map;
    handler->node_arena = make_arena_system(KB(2));
    heap_init(&handler->heap, &handler->node_arena);
    handler->allocator = base_allocator_on_heap(&handler->heap);
	
    if (parent) {
        handler->keybind_table = make_table_u64_u64(&handler->allocator, Max(32, parent->keybind_table.slot_count));
        block_copy_dynamic_array(handler->keybind_table.keys, parent->keybind_table.keys, parent->keybind_table.slot_count);
        block_copy_dynamic_array(handler->keybind_table.vals, parent->keybind_table.vals, parent->keybind_table.slot_count);
        handler->keybind_table.used_count  = parent->keybind_table.used_count;
        handler->keybind_table.dirty_count = parent->keybind_table.dirty_count;
        
        for (u32 slot_index = 0; slot_index < handler->keybind_table.slot_count; slot_index++) {
            u64 value = handler->keybind_table.vals[slot_index];
            if (value) {
                Vim_Key_Binding* source_bind = cast(Vim_Key_Binding*) IntAsPtr(value);
                Vim_Key_Binding* dest_bind   = push_array_zero(&handler->node_arena, Vim_Key_Binding, 1);
                block_copy_struct(dest_bind, source_bind);
                handler->keybind_table.vals[slot_index] = PtrAsInt(dest_bind);
            }
        }
    } else {
        handler->keybind_table = make_table_u64_u64(&handler->allocator, 32);
    }
	
    // Note: These are for when you want to enter a count for a command
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_1, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_2, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_3, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_4, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_5, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_6, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_7, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_8, 0);
    map_set_binding_l(mapping, command_map, vim_handle_input, InputEventKind_KeyStroke, KeyCode_9, 0);
}

internal Vim_Key_Binding* make_or_retrieve_vim_binding(Vim_Binding_Handler* handler, Table_u64_u64* table, Vim_Key key, b32 make_if_doesnt_exist) {
    Vim_Key_Binding* result = 0;
	
    u64 location = 0;
    if (table_read(table, vim_key_as_u64(key), &location)) {
        result = cast(Vim_Key_Binding*) IntAsPtr(location);
    } else if (make_if_doesnt_exist) {
        result = push_array_zero(&handler->node_arena, Vim_Key_Binding, 1);
        table_insert(table, vim_key_as_u64(key), PtrAsInt(result));
    }
	
    return result;
}

internal i32 input_to_digit(User_Input in) {
    i32 result = -1;
    Input_Event* event = &in.event;
    // @TODO: This crashed here once, something about event->modifiers being all busted up. Why??
    if (event->kind == InputEventKind_KeyStroke && is_unmodified_key(event)) {
        if (event->key.code >= KeyCode_0 && event->key.code <= KeyCode_9) {
            result = cast(i32) (event->key.code - KeyCode_0);
        }
    }
    return result;
}

internal i64 vim_query_number(Application_Links* app, b32 handle_sign, User_Input* user_in = 0) {
    i64 result = 0;
	
    User_Input in = user_in ? *user_in : get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
    if (in.abort) {
        return result;
    }
	
    i64 sign = 1;
    if (handle_sign && match_key_code(&in, KeyCode_Minus)) {
        sign = -1;
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
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
			
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
            if (in.abort) {
                break;
            }
        }
    }
	
    if (user_in) *user_in = in;
	
    return result;
}

internal User_Input vim_get_next_key(Application_Links* app) {
    User_Input in = {};
    do {
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
        if (in.abort) break;
    } while (key_code_to_vim_modifier(in.event.key.code));
    return in;
}

internal String_Const_u8 vim_get_next_writable(Application_Links* app) {
    User_Input in = {};
    do {
        in = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape|EventProperty_ViewActivation);
        if (in.abort) break;
    } while (key_code_to_vim_modifier(in.event.key.code));
    String_Const_u8 result = to_writable(&in);
    return result;
}

internal Vim_Key_Binding* vim_query_binding(Application_Links* app, Vim_Binding_Handler* handler, Vim_Binding_Handler* fallback_handler = 0, i32* out_count = 0, User_Input* user_in = 0) {
    Vim_Key_Binding* result = 0;
	
    User_Input in = user_in ? *user_in : vim_get_next_key(app);
    while (key_code_to_vim_modifier(in.event.key.code)) {
        in = vim_get_next_key(app);
    }
	
    i64 count_query = vim_query_number(app, false, &in);
    i32 count = cast(i32) clamp(1, count_query, 256);
	
    Vim_Key_Binding* bind = 0;
    Vim_Key_Binding* fallback_bind = 0;
	
    for (;;) {
        Vim_Key key = get_vim_key_from_input(in);
        if (bind && bind->kind == VimBindingKind_Nested) {
            bind = make_or_retrieve_vim_binding(handler, bind->nested_keybind_table, key, false);
        } else {
            bind = make_or_retrieve_vim_binding(handler, &handler->keybind_table, key, false);
        }
		
        if (fallback_handler) {
            if (fallback_bind && fallback_bind->kind == VimBindingKind_Nested) {
                fallback_bind = make_or_retrieve_vim_binding(fallback_handler, fallback_bind->nested_keybind_table, key, false);
            } else {
                fallback_bind = make_or_retrieve_vim_binding(fallback_handler, &fallback_handler->keybind_table, key, false);
            }
        }
		
        if (bind && bind->description.size) {
            print_message(app, bind->description);
            if (bind->kind == VimBindingKind_Nested) {
                print_message(app, string_u8_litexpr(" -> "));
            } else {
                print_message(app, string_u8_litexpr("\n"));
            }
        }
		
        if (bind && bind->kind != VimBindingKind_Nested) {
            result = bind;
            break;
        }
		
        if (!bind) {
            if (fallback_bind) {
                if (fallback_bind->kind == VimBindingKind_Nested) {
                    bind = fallback_bind;
                    handler = fallback_handler;
                    fallback_bind = 0;
                    fallback_handler = 0;
                } else {
                    result = fallback_bind;
                    break;
                }
            } else {
                break;
            }
        }
		
        in = vim_get_next_key(app);
		
        if (in.abort) {
            break;
        }
    }
	
    if (out_count) *out_count = count;
    if (user_in) *user_in = in;
	
    return result;
}

CUSTOM_COMMAND_SIG(snd_select_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
	
    i64 start_of_line = get_line_start_pos_from_pos(app, buffer, view_get_cursor_pos(app, view));
	
    view_set_mark(app, view, seek_pos(start_of_line));
    seek_end_of_line(app);
}

CUSTOM_COMMAND_SIG(snd_move_left_on_visual_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
	
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_start_pos = get_pos_of_visual_line_side(app, view, cursor_pos, Side_Min);
    i64 legal_line_start_pos = view_get_character_legal_pos_from_pos(app, view, line_start_pos);
	
    if (cursor_pos > legal_line_start_pos) {
        move_left(app);
    }
}

CUSTOM_COMMAND_SIG(snd_move_right_on_visual_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
	
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_end_pos = get_pos_of_visual_line_side(app, view, cursor_pos, Side_Max);
    i64 legal_line_end_pos = view_get_character_legal_pos_from_pos(app, view, line_end_pos);
	
    if (cursor_pos < legal_line_end_pos) {
        move_right(app);
    }
}

internal void vim_command_begin(Application_Links* app, Buffer_ID buffer) {
    if (vim_state.started_command) {
        return;
    }
	
    vim_state.started_command = true;
    vim_state.command_history = history_group_begin(app, buffer);
	
    if (get_current_input_is_virtual(app)) {
        return;
    }
	
    vim_state.recorded_command_macro_range.min = vim_state.keyboard_buffer_cursor_on_most_recent_input;
}

internal void vim_command_end(Application_Links* app) {
    if (!vim_state.started_command) {
        return;
    }
	
    vim_state.started_command = false;
    history_group_end(vim_state.command_history);
	
    if (get_current_input_is_virtual(app)) {
        return;
    }
	
    Buffer_ID buffer = get_keyboard_log_buffer(app);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));
    vim_state.recorded_command_macro_range.max = cursor.pos;
}

internal void vim_enter_mode(Application_Links* app, Vim_Mode mode) {
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
	
    vim_buffer->flags &= ~VimBufferFlag_AllowSittingOnLineEnd;
	
    switch (mode) {
        case VimMode_Normal: {
            *map_id = vim_mapid_normal;
            if (is_vim_insert_mode(vim_buffer->mode)) {
                snd_move_left_on_visual_line(app);
            }
            vim_command_end(app);
            vim_state.most_recent_character_seek.size = 0;
        } break;
		
        case VimMode_Append:
        case VimMode_Insert: {
            vim_buffer->flags |= VimBufferFlag_AllowSittingOnLineEnd;
            if (vim_buffer->flags & VimBufferFlag_TreatAsCode) {
                *map_id = mapid_code;
            } else {
                *map_id = mapid_file;
            }
        } break;
		
        case VimMode_Visual:
        case VimMode_VisualLine: {
            *map_id = vim_mapid_visual;
            /* ... */
        } break;
		
        case VimMode_VisualBlock: {
            /* ... */
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

CUSTOM_COMMAND_SIG(vim_enter_insert_sol_mode) {
    vim_enter_mode(app, VimMode_Insert);
    seek_beginning_of_line(app);
}

CUSTOM_COMMAND_SIG(vim_enter_append_mode) {
    vim_enter_mode(app, VimMode_Append);
    snd_move_right_on_visual_line(app);
}

CUSTOM_COMMAND_SIG(vim_enter_append_eol_mode) {
    vim_enter_mode(app, VimMode_Append);
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
	
    if (vim_buffer->mode == VimMode_Visual) {
        vim_enter_mode(app, VimMode_Normal);
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
	
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    if (!is_vim_visual_mode(vim_buffer->mode)) {
        i64 pos = view_get_cursor_pos(app, view);
        view_set_mark(app, view, seek_pos(pos));
    }
	
    if (vim_buffer->mode == VimMode_VisualLine) {
        vim_enter_mode(app, VimMode_Normal);
    } else {
        vim_enter_mode(app, VimMode_VisualLine);
    }
}

internal Vim_Visual_Selection vim_get_selection(Application_Links* app, View_ID view, Buffer_ID buffer) {
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    Vim_Visual_Selection selection = {};
    if (vim_buffer->mode == VimMode_Visual || vim_buffer->mode == VimMode_VisualLine) {
        selection.kind = vim_buffer->mode == VimMode_VisualLine ? VimSelectionKind_Line : VimSelectionKind_Range;
        i64 cursor_pos = view_get_cursor_pos(app, view);
        i64 mark_pos = view_get_mark_pos(app, view);
        selection.range = Ii64(cursor_pos, mark_pos);
        if (vim_buffer->mode == VimMode_VisualLine) {
            selection.range = maximize_visual_line_range(app, view, selection.range, false);
        }
    }
    return selection;
}

internal void vim_isearch_selection_internal(Application_Links* app, Scan_Direction dir) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    Vim_Visual_Selection selection = vim_get_selection(app, view, buffer);
    selection.range.max = view_set_pos_by_character_delta(app, view, selection.range.max, 1);
	
    if (range_size(selection.range) > 0) {
        Scratch_Block scratch(app);
        String_Const_u8 query_init = push_buffer_range(app, scratch, buffer, selection.range);
		
        vim_enter_normal_mode(app);
		
        i64 pos = view_get_cursor_pos(app, view);
        isearch(app, dir, pos, query_init);
    }
}

CUSTOM_COMMAND_SIG(vim_isearch_selection) {
    vim_isearch_selection_internal(app, Scan_Forward);
}

CUSTOM_COMMAND_SIG(vim_reverse_isearch_selection) {
    vim_isearch_selection_internal(app, Scan_Backward);
}

internal void snd_write_text_and_auto_indent(Application_Links* app, String_Const_u8 insert) {
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

CUSTOM_COMMAND_SIG(snd_move_line_down) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
	
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    move_line(app, buffer, line, Scan_Forward);
    move_down_textual(app);
}

CUSTOM_COMMAND_SIG(snd_move_up_textual)
CUSTOM_DOC("Moves up to the next line of actual text, regardless of line wrapping.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    i64 next_line = cursor.line - 1;
    view_set_cursor_and_preferred_x(app, view, seek_line_col(next_line, 1));
}

CUSTOM_COMMAND_SIG(snd_move_line_up) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
	
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    i64 line = get_line_number_from_pos(app, buffer, view_get_cursor_pos(app, view));
    move_line(app, buffer, line, Scan_Backward);
    snd_move_up_textual(app);
}

internal b32 character_is_newline(char c) {
    return c == '\r' || c == '\n';
}

internal i64 boundary_whitespace(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_whitespace));
}

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

internal void vim_cut_range(Application_Links* app, View_ID view, Buffer_ID buffer, Range_i64 range, b32 inclusive) {
    view_set_cursor(app, view, seek_pos(range.min));
    i64 mark_pos = range.max;
    if (inclusive) {
        mark_pos = view_set_pos_by_character_delta(app, view, mark_pos, 1);
    }
    view_set_mark(app, view, seek_pos(mark_pos));
	
    cut(app);
}

CUSTOM_COMMAND_SIG(vim_repeat_most_recent_command) {
    if (vim_state.started_command || get_current_input_is_virtual(app)) {
        return;
    }
	
    Buffer_ID buffer = view_get_buffer(app, get_active_view(app, Access_ReadWriteVisible), Access_ReadWriteVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }
	
    Buffer_ID keyboard_buffer = get_keyboard_log_buffer(app);
    if (!buffer_exists(app, keyboard_buffer)) {
        return;
    }
	
    Scratch_Block scratch(app, Scratch_Share);
    String_Const_u8 macro = push_buffer_range(app, scratch, keyboard_buffer, vim_state.recorded_command_macro_range);
	
    keyboard_macro_play(app, macro);
}

internal Vim_Motion_Result vim_execute_motion(Application_Links* app, View_ID view, Buffer_ID buffer, Vim_Motion* motion, i64 start_pos, i32 count) {
    if (count < 0) count = 1;
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64_neg_inf;
    for (i32 i = 0; i < count; i++) {
        Vim_Motion_Result inner = motion(app, view, buffer, result.seek_pos);
        result.range = range_union(inner.range, result.range);
        result.seek_pos = inner.seek_pos;
        result.inclusive = inner.inclusive;
        result.set_preferred_x = inner.set_preferred_x;
    }
    return result;
}

CUSTOM_COMMAND_SIG(vim_handle_input) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    Vim_Binding_Handler* handler = 0;
    if (is_vim_visual_mode(vim_buffer->mode)) {
        handler = &vim_map_visual;
    } else if (vim_buffer->mode == VimMode_Normal) {
        handler = &vim_map_normal;
    } else {
        InvalidPath;
    }
	
    User_Input in = get_current_input(app);
	
    if (!vim_state.started_command && !get_current_input_is_virtual(app)) {
        Buffer_ID keyboard_log_buffer = get_keyboard_log_buffer(app);
        Buffer_Cursor cursor      = buffer_compute_cursor(app, keyboard_log_buffer, seek_pos(buffer_get_size(app, keyboard_log_buffer)));
        Buffer_Cursor back_cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_line_col(cursor.line - 1, 1));
        vim_state.keyboard_buffer_cursor_on_most_recent_input = back_cursor.pos;
    }
	
    i32 count = 1;
    if (input_to_digit(in) > 0) {
        i64 queried_number = vim_query_number(app, false, &in);
        count = cast(i32) clamp(1, queried_number, 256);
    }
	
    Vim_Key key = get_vim_key_from_input(in);
    Vim_Key_Binding* bind = make_or_retrieve_vim_binding(handler, &handler->keybind_table, key, false);
	
    if (bind && bind->description.size) {
        print_message(app, bind->description);
        if (bind->kind == VimBindingKind_Nested) {
            print_message(app, string_u8_litexpr(" -> "));
        } else {
            print_message(app, string_u8_litexpr("\n"));
        }
    }
	
	
    if (!bind) {
        return;
    }
	
    while (bind && bind->kind == VimBindingKind_Nested) {
        in   = vim_get_next_key(app);
        key  = get_vim_key_from_input(in);
        bind = make_or_retrieve_vim_binding(handler, bind->nested_keybind_table, key, false);
		
        if (bind && bind->description.size) {
            print_message(app, bind->description);
            if (bind->kind == VimBindingKind_Nested) {
                print_message(app, string_u8_litexpr(" -> "));
            } else {
                print_message(app, string_u8_litexpr("\n"));
            }
        }
    }
	
    if (!bind) {
        return;
    }
	
    if (bind->flags & VimBindingFlag_WriteOnly) {
        u32 buffer_flags = buffer_get_access_flags(app, buffer);
        if (!HasFlag(buffer_flags, Access_Write)) {
            return;
        }
    }
	
    if (bind->flags & VimBindingFlag_IsRepeatable) {
        vim_command_begin(app, buffer);
    }
	
#if 0
    if (bind->description.size) {
        print_message(app, bind->description);
        print_message(app, string_u8_litexpr("\n"));
    }
#endif
	
    switch (bind->kind) {
        case VimBindingKind_Motion: {
            Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, bind->motion, view_get_cursor_pos(app, view), count);
            Buffer_Seek new_pos = seek_pos(mr.seek_pos);
            if (mr.set_preferred_x) {
                view_set_cursor_and_preferred_x(app, view, new_pos);
            } else {
                view_set_cursor(app, view, new_pos);
            }
        } break;
		
        case VimBindingKind_Operator: {
            Vim_Visual_Selection selection = {};
            if (vim_buffer->mode == VimMode_Visual || vim_buffer->mode == VimMode_VisualLine) {
                selection.kind = vim_buffer->mode == VimMode_VisualLine ? VimSelectionKind_Line : VimSelectionKind_Range;
                i64 cursor_pos = view_get_cursor_pos(app, view);
                i64 mark_pos = view_get_mark_pos(app, view);
                selection.range = Ii64(cursor_pos, mark_pos);
                // if (vim_buffer->mode == VimMode_VisualLine) {
                //     selection.range = maximize_visual_line_range(app, view, selection.range, false);
                // }
            }
            
            bind->op(app, handler, view, buffer, selection, count);
        } break;
		
        case VimBindingKind_4CoderCommand: {
            for (i32 i =0; i < count; i++) {
                bind->fcoder_command(app);
            }
        } break;
    }
	
    View_ID view_post_command = get_active_view(app, Access_ReadVisible);
    View_ID buffer_post_command = view_get_buffer(app, view_post_command, Access_ReadVisible);
    Managed_Scope scope_post_command = buffer_get_managed_scope(app, buffer_post_command);
    Vim_Buffer_Attachment* vim_buffer_post_command = scope_attachment(app, scope_post_command, vim_buffer_attachment, Vim_Buffer_Attachment);
	
    if (bind->kind == VimBindingKind_Operator) {
        if (vim_buffer_post_command->mode == VimMode_Visual || vim_buffer_post_command->mode == VimMode_VisualLine) {
            vim_enter_normal_mode(app);
        }
    }
	
    if (bind->flags & VimBindingFlag_IsRepeatable) {
        if (vim_buffer->mode == VimMode_Normal) {
            vim_command_end(app);
        }
    }
}

struct Vim_Binding_Sequence {
    Vim_Key keys[8];
    i32 count;
};

inline Vim_Binding_Sequence vim_bind(Vim_Key key1,      Vim_Key key2 = {}, Vim_Key key3 = {}, Vim_Key key4 = {}, 
									 Vim_Key key5 = {}, Vim_Key key6 = {}, Vim_Key key7 = {}, Vim_Key key8 = {}) {
    Vim_Binding_Sequence result = {};
    result.keys[result.count++] = key1;
    if (key2.kc) result.keys[result.count++] = key2;
    if (key3.kc) result.keys[result.count++] = key3;
    if (key4.kc) result.keys[result.count++] = key4;
    if (key5.kc) result.keys[result.count++] = key5;
    if (key6.kc) result.keys[result.count++] = key6;
    if (key7.kc) result.keys[result.count++] = key7;
    if (key8.kc) result.keys[result.count++] = key8;
    return result;
}

internal Vim_Key_Binding* add_vim_binding(Vim_Binding_Handler* handler, Vim_Binding_Sequence binding_sequence, u32 flags) {
    Vim_Key_Binding* result = 0;
	
    if (binding_sequence.count > 0) {
        Command_Binding binding;
        binding.custom = vim_handle_input;
		
        Vim_Key key1 = binding_sequence.keys[0];
		
        Input_Modifier_Set_Fixed fixed_modifiers = vim_modifiers_to_input_modifier_set_fixed(key1.mods);
        Input_Modifier_Set modifiers = { fixed_modifiers.mods, fixed_modifiers.count };
        map_set_binding_key(handler->mapping, handler->command_map, binding, key1.kc, &modifiers);
		
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

#define vim_bind_motion(handler, motion, key1, ...) vim_bind_motion_(handler, motion, string_u8_litexpr(#motion), key1, __VA_ARGS__)
internal void vim_bind_motion_(Vim_Binding_Handler* handler, Vim_Motion* motion, String_Const_u8 description, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0, u32 inverse_flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, vim_bind(key1, key2), flags);
    bind->kind = VimBindingKind_Motion;
    bind->description = description;
    bind->flags |= flags;
    bind->flags &= ~inverse_flags;
    bind->motion = motion;
}

#define vim_bind_operator(handler, op, key1, ...) vim_bind_operator_(handler, op, string_u8_litexpr(#op), key1, __VA_ARGS__)
internal void vim_bind_operator_(Vim_Binding_Handler* handler, Vim_Operator* op, String_Const_u8 description, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0, u32 inverse_flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, vim_bind(key1, key2), flags);
    bind->kind = VimBindingKind_Operator;
    bind->description = description;
    bind->flags |= flags|VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly;
    bind->flags &= ~inverse_flags;
    bind->op = op;
}

#define vim_bind_generic_command(handler, cmd, seq, ...) vim_bind_generic_command_(handler, cmd, string_u8_litexpr(#cmd), seq, __VA_ARGS__)
internal void vim_bind_generic_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Binding_Sequence sequence, u32 flags = 0, u32 inverse_flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence, flags);
    bind->kind = VimBindingKind_4CoderCommand;
    bind->description = description;
    bind->flags |= flags;
    bind->flags &= ~inverse_flags;
    bind->fcoder_command = fcoder_command;
}

internal void vim_bind_generic_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0, u32 inverse_flags = 0) {
    vim_bind_generic_command_(handler, fcoder_command, description, vim_bind(key1, key2), flags, inverse_flags);
}

#define vim_bind_text_command(handler, cmd, seq, ...) vim_bind_text_command_(handler, cmd, string_u8_litexpr(#cmd), seq, __VA_ARGS__)
internal void vim_bind_text_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Binding_Sequence sequence, u32 flags = 0, u32 inverse_flags = 0) {
    vim_bind_generic_command_(handler, fcoder_command, description, sequence, flags|VimBindingFlag_IsRepeatable|VimBindingFlag_WriteOnly, inverse_flags);
}

internal void vim_bind_text_command_(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, String_Const_u8 description, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0, u32 inverse_flags = 0) {
    vim_bind_text_command_(handler, fcoder_command, description, vim_bind(key1, key2), flags, inverse_flags);
}

internal void vim_named_bind(Vim_Binding_Handler* handler, Vim_Binding_Sequence sequence, String_Const_u8 description) {
    Vim_Key_Binding* bind = add_vim_binding(handler, sequence, 0);
    bind->description = description;
}

internal VIM_MOTION(vim_motion_left) {
    Vim_Motion_Result result = {};
    result.seek_pos = view_set_pos_by_character_delta(app, view, start_pos, -1);
    result.range = Ii64(start_pos, result.seek_pos);
    result.set_preferred_x = true;
    return result;
}

internal VIM_MOTION(vim_motion_right) {
    Vim_Motion_Result result = {};
    result.seek_pos = view_set_pos_by_character_delta(app, view, start_pos,  1);
#if 0
    if (buffer_get_char(app, buffer, result.seek_pos) == '\r') {
        result.seek_pos++;
    }
    if (buffer_get_char(app, buffer, result.seek_pos) == '\n') {
        result.seek_pos++;
    }
#endif
    result.range = Ii64(start_pos, result.seek_pos);
    result.set_preferred_x = true;
    return result;
}

internal VIM_MOTION(vim_motion_down) {
    Vim_Motion_Result result = {};
    result.seek_pos = get_vertical_pixel_move_pos(app, view, start_pos, 1.0f);
    result.range = Ii64(start_pos, result.seek_pos);
    result.range.min = get_pos_of_visual_line_side(app, view, result.range.min, Side_Min);
    result.range.max = get_pos_of_visual_line_side(app, view, result.range.max, Side_Max);
    return result;
}

internal VIM_MOTION(vim_motion_up) {
    Vim_Motion_Result result = {};
    result.seek_pos = get_vertical_pixel_move_pos(app, view, start_pos, -1.0f);
    result.range = Ii64(start_pos, result.seek_pos);
    result.range.min = get_pos_of_visual_line_side(app, view, result.range.min, Side_Min);
    result.range.max = get_pos_of_visual_line_side(app, view, result.range.max, Side_Max);
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
    result.set_preferred_x = true;
	
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
    // @TODO: Use consistent word definition
    Vim_Motion_Result result;
    result.seek_pos = scan(app, push_boundary_list(scratch, boundary_token), buffer, Scan_Forward, start_pos + 1) - 1; // @Unicode
    result.range = Ii64(start_pos, result.seek_pos);
    result.inclusive = true;
    return result;
}

internal VIM_MOTION(vim_motion_buffer_start) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    result.range = Ii64(0, start_pos);
    result.seek_pos = 0;
    result.set_preferred_x = true;
	
    return result;
}

internal VIM_MOTION(vim_motion_buffer_end) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    result.range = Ii64(start_pos, buffer_get_size(app, buffer));
    result.seek_pos = result.range.max;
    result.set_preferred_x = true;
	
    return result;
}

internal Vim_Motion_Result vim_motion_line_side_visual(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    i64 line_side = get_pos_of_visual_line_side(app, view, start_pos, dir == Scan_Forward ? Side_Max : Side_Min);
	
    result.seek_pos = line_side;
    result.range = Ii64(start_pos, line_side);
    result.inclusive = true;
    result.set_preferred_x = true;
	
    return result;
}

internal Vim_Motion_Result vim_motion_line_side_textual(Application_Links* app, View_ID view, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
	
    i64 line_side = get_line_side_pos_from_pos(app, buffer, start_pos, dir == Scan_Forward ? Side_Max : Side_Min);
	
    result.seek_pos = line_side;
    result.range = Ii64(start_pos, result.seek_pos);
    result.inclusive = false;
    result.set_preferred_x = true;
	
    return result;
}

internal VIM_MOTION(vim_motion_line_start_visual) {
    return vim_motion_line_side_visual(app, view, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_line_end_visual) {
    Vim_Motion_Result result = vim_motion_line_side_visual(app, view, buffer, Scan_Forward, start_pos);
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
    if (result.seek_pos > result.range.min) {
        result.seek_pos = view_set_pos_by_character_delta(app, view, result.seek_pos, -1);
    }
    return result;
}

internal VIM_MOTION(vim_motion_enclose_line_visual) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64(get_pos_of_visual_line_side(app, view, start_pos, Side_Min),
                        get_pos_of_visual_line_side(app, view, start_pos, Side_Max));
    result.seek_pos = result.range.max;
    result.inclusive = true;
    return result;
}

internal VIM_MOTION(vim_motion_enclose_line_textual) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64(get_line_side_pos_from_pos(app, buffer, start_pos, Side_Min),
                        get_line_side_pos_from_pos(app, buffer, start_pos, Side_Max));
    result.seek_pos = result.range.max;
    result.inclusive = true;
    return result;
}

internal VIM_MOTION(vim_motion_scope) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.inclusive = true;
    result.set_preferred_x = true;
	
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.count > 0) {
        Token_Base_Kind opening_token = TokenBaseKind_EOF;
        Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, start_pos));
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, start_pos);
		
        b32 loop = true;
        while (loop && opening_token == TokenBaseKind_EOF) {
            Token* token = token_it_read(&it);
            if (token && range_contains_inclusive(line_range, token->pos)) {
                switch (token->kind) {
                    case TokenBaseKind_ParentheticalOpen:
                    case TokenBaseKind_ScopeOpen:
                    case TokenBaseKind_ParentheticalClose:
                    case TokenBaseKind_ScopeClose: {
                        opening_token = token->kind;
                    } break;
					
                    default: {
                        loop = token_it_inc(&it);
                    } break;
                }
            } else {
                break;
            }
        }
		
        if (opening_token != TokenBaseKind_EOF) {
            b32 scan_forward = (opening_token == TokenBaseKind_ScopeOpen || opening_token == TokenBaseKind_ParentheticalOpen);
			
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
	
    if (find_nest_side(app, buffer, pos - 1, min_flags, Scan_Backward, NestDelim_Open, &range.start) &&
        find_nest_side(app, buffer, pos, max_flags, Scan_Forward, NestDelim_Close, &range.end))
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
    result.inclusive = true;
    result.set_preferred_x = true;
	
    if (vim_find_surrounding_nest(app, buffer, start_pos, flags, &result.range, true)) {
        i64 min = result.range.min;
        i64 max = result.range.max;
        i64 min_line = get_line_number_from_pos(app, buffer, min);
        i64 max_line = get_line_number_from_pos(app, buffer, max);
        if (min_line != max_line) {
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

internal VIM_MOTION(vim_motion_inner_word) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.inclusive = false;
    result.set_preferred_x = true;
	
    Range_i64 range = enclose_boundary(app, buffer, Ii64(start_pos + 1), vim_boundary_word);
    if (range_size(range) > 0) {
        result.range = range;
        result.seek_pos = range.max;
    }
	
    return result;
}

internal Vim_Motion_Result vim_motion_seek_character_internal(Application_Links* app, View_ID view, Buffer_ID buffer, i64 start_pos, String_Const_u8 target, Scan_Direction dir, b32 inclusive) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.inclusive = inclusive;
    result.set_preferred_x = true;
	
    b32 new_input = true;
    if (!target.size) {
        new_input = false;
        target = vim_state.most_recent_character_seek.string;
    }
	
    if (target.size) {
        i64 seek_pos = start_pos;
        for (;;) {
            String_Match match = buffer_seek_string(app, buffer, target, dir, seek_pos + (inclusive ? 0 : dir));
            if (match.buffer != buffer) {
                break;
            }
            if (HasFlag(match.flags, StringMatch_CaseSensitive)) {
                result.range = Ii64(start_pos, match.range.min);
                if (dir == Scan_Forward) {
                    result.seek_pos = result.range.max - (inclusive ? 0 : 1);
                } else {
                    result.seek_pos = result.range.min + (inclusive ? 0 : target.size);
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
			
            vim_state.most_recent_character_seek_dir = dir;
            vim_state.most_recent_character_seek_inclusive = inclusive;
        }
    }
	
    return result;
}

internal VIM_MOTION(vim_motion_find_character) {
    String_Const_u8 target = vim_get_next_writable(app);
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Forward, true);
}

internal VIM_MOTION(vim_motion_to_character) {
    String_Const_u8 target = vim_get_next_writable(app);
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Forward, false);
}

internal VIM_MOTION(vim_motion_find_character_backward) {
    String_Const_u8 target = vim_get_next_writable(app);
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Backward, true);
}

internal VIM_MOTION(vim_motion_to_character_backward) {
    String_Const_u8 target = vim_get_next_writable(app);
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, target, Scan_Backward, false);
}

internal VIM_MOTION(vim_motion_repeat_character_seek_forward) {
    Scan_Direction dir = vim_state.most_recent_character_seek_dir;
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_inclusive);
}

internal VIM_MOTION(vim_motion_repeat_character_seek_backward) {
    Scan_Direction dir = -vim_state.most_recent_character_seek_dir;
    return vim_motion_seek_character_internal(app, view, buffer, start_pos, SCu8(), dir, vim_state.most_recent_character_seek_inclusive);
}

inline Vim_Motion* vim_unwrap_motion_query(Vim_Key_Binding* query, Vim_Operator* op = 0) {
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

internal VIM_OPERATOR(vim_delete) {
    Range_i64 range = {};
    b32 inclusive = false;
	
    i32 motion_count = 1;
    Vim_Motion* motion = 0;
	
    if (selection.kind == VimSelectionKind_Range) {
        range = selection.range;
        inclusive = true;
    } else if (selection.kind == VimSelectionKind_Line) {
        range = maximize_visual_line_range(app, view, selection.range, true);
        inclusive = true;
    } else {
        Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, handler, &motion_count);
        motion = vim_unwrap_motion_query(query, vim_delete);
    }
	
    for (i32 i = 0; i < count; i++) {
        if (motion) {
            Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, motion, view_get_cursor_pos(app, view), motion_count);
            range = mr.range;
            inclusive = mr.inclusive;
        }
		
        if (range_size(range) + inclusive > 0) {
            vim_cut_range(app, view, buffer, range, inclusive);
        }
    }
}

internal VIM_OPERATOR(vim_change) {
    Range_i64 range = {};
    b32 inclusive = false;
	
    i32 motion_count = 1;
    Vim_Motion* motion = 0;
	
    if (selection.kind == VimSelectionKind_Range) {
        range = selection.range;
        inclusive = true;
    } else if (selection.kind == VimSelectionKind_Line) {
        range = maximize_visual_line_range(app, view, selection.range, false);
        inclusive = true;
    } else {
        Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, handler, &motion_count);
        motion = vim_unwrap_motion_query(query, vim_change);
		
        // Note: Vim's behaviour is a little inconsistent with some motions here for the sake of intuitiveness.
        if      (motion == vim_motion_word)        motion = vim_motion_word_end;
        else if (motion == vim_motion_inner_scope) motion = vim_motion_inner_scope_leave_inner_line;
        else if (motion == vim_motion_inner_paren) motion = vim_motion_inner_paren_leave_inner_line;
    }
	
    for (i32 i = 0; i < count; i++) {
        if (motion) {
            Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, motion, view_get_cursor_pos(app, view), motion_count);
            range = mr.range;
            inclusive = mr.inclusive;
        }
		
        if (range_size(range) + inclusive > 0) {
            vim_cut_range(app, view, buffer, range, inclusive);
        }
    }
	
    vim_enter_insert_mode(app);
}

internal VIM_OPERATOR(vim_replace) {
	i64 pos = view_get_cursor_pos(app, view);
	Range_i64 selection_range = Ii64(pos);
	
	if (selection.kind == VimSelectionKind_Range) {
		selection_range = selection.range;
	} else if (selection.kind == VimSelectionKind_Line) {
		selection_range = maximize_visual_line_range(app, view, selection.range, false);
	}
	
	String_Const_u8 replace_char = vim_get_next_writable(app);
	
	for (i64 replace_cursor = selection_range.min; replace_cursor <= selection_range.max; replace_cursor++) {
		Range_i64 replace_range = Ii64(replace_cursor, view_set_pos_by_character_delta(app, view, replace_cursor, 1));
		buffer_replace_range(app, buffer, replace_range, replace_char);
	}
	
    view_set_cursor_and_preferred_x(app, view, seek_pos(selection_range.min));
}

internal VIM_OPERATOR(vim_yank) {
    Range_i64 range = {};
    b32 inclusive = false;
	
    if (selection.kind == VimSelectionKind_Range) {
        range = selection.range;
        inclusive = true;
    } else if (selection.kind == VimSelectionKind_Line) {
        range = maximize_visual_line_range(app, view, selection.range, true);
        inclusive = true;
    } else {
        i32 motion_count;
        Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, handler, &motion_count);
        Vim_Motion* motion = vim_unwrap_motion_query(query, vim_yank);
		
        // Note: Vim's behaviour is a little inconsistent with some motions here for the sake of intuitiveness.
        if (motion == vim_motion_word) {
            motion = vim_motion_word_end;
        }
		
        if (motion) {
            Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, motion, view_get_cursor_pos(app, view), motion_count);
            range = mr.range;
            inclusive = mr.inclusive;
        }
    }
	
    if (range_size(range) + inclusive > 0) {
        if (inclusive) {
            range.max += 1;
        }
        clipboard_post_buffer_range(app, 0, buffer, range);
    }
}

internal void vim_paste_internal(Application_Links* app, b32 post_cursor) {
    i32 clip_count = clipboard_count(app, 0);
    if (clip_count > 0) {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        Managed_Scope scope = view_get_managed_scope(app, view);
        Rewrite_Type* next_rewrite = scope_attachment(app, scope, view_next_rewrite_loc, Rewrite_Type);
        if (next_rewrite != 0) {
            *next_rewrite = Rewrite_Paste;
            i32* paste_index = scope_attachment(app, scope, view_paste_index_loc, i32);
            *paste_index = 0;
            
            Scratch_Block scratch(app);
            String_Const_u8 string = push_clipboard_index(app, scratch, 0, *paste_index);

            b32 contains_newlines = false;
            for (u64 string_index = 0; string_index < string.size; string_index++) {
                if (character_is_newline(string.str[string_index])) {
                    contains_newlines = true;
                    break;
                }
            }

            if (string.size > 0) {
                Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
                
                if (post_cursor && contains_newlines) {
                    move_down(app);
                    seek_beginning_of_line(app);
                }

                i64 pos = view_get_cursor_pos(app, view);
                if (post_cursor && !contains_newlines) {
                    Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, pos));
                    pos = view_set_pos_by_character_delta(app, view, pos, 1);
                    pos = clamp(line_range.min, pos, line_range.max);
                }

                buffer_replace_range(app, buffer, Ii64(pos), string);
                auto_indent_buffer(app, buffer, Ii64(pos, pos + string.size));
                move_past_lead_whitespace(app, view, buffer);
                
                ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
                buffer_post_fade(app, buffer, 0.667f, Ii64_size(pos, string.size), argb);
            }
        }
    }
}

internal CUSTOM_COMMAND_SIG(vim_paste) {
    vim_paste_internal(app, true);
}

internal CUSTOM_COMMAND_SIG(vim_paste_pre_cursor) {
    vim_paste_internal(app, false);
}

internal b32 vim_align_range(Application_Links* app, Buffer_ID buffer, Range_i64 col_range, Range_i64 line_range, String_Const_u8 align_target, b32 align_after_target) {
    if (col_range.min == 0 && col_range.max == 0) {
        col_range = Ii64(0, max_i64);
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
                // @Note: Offset the seek position to skip over the range of the matched string, just in case for some reason
                // the user asked to align a string with spaces in it, I guess.
                i64 range_delta = range_size(match.range);
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
                    // @Note: Well, we didn't align yet. But we will.
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
                // @Note: We have to recompute the cursor because by inserting indentation we adjust the absolute pos of the alignment point
                align_cursor = buffer_compute_cursor(app, buffer, seek_line_col(align_cursor.line, align_cursor.col));
				
                i64 col_delta = furthest_align_cursor.col - align_cursor.col;
                if (col_delta > 0) {
                    String_Const_u8 align_string = push_u8_stringf(scratch, "%*s", col_delta, "");
                    buffer_replace_range(app, buffer, Ii64(align_cursor.pos), align_string);
                }
            }
        }
        history_group_end(history);
    } else if (found_align_target) {
        did_align |= vim_align_range(app, buffer, Ii64(furthest_align_cursor.col + 1, col_range.max), line_range, align_target, align_after_target);
    }
	
    return did_align;
}

internal void vim_align_internal(Application_Links* app, Vim_Binding_Handler* handler, View_ID view, Buffer_ID buffer, Vim_Visual_Selection selection, i32 count, b32 align_right) {
    Range_i64 range = {};
    b32 inclusive = false;
	
    i32 motion_count = 1;
    Vim_Motion* motion = 0;
	
    if (selection.kind == VimSelectionKind_Range) {
        range = selection.range;
        inclusive = true;
    } else if (selection.kind == VimSelectionKind_Line) {
        range = maximize_visual_line_range(app, view, selection.range, true);
        inclusive = true;
    } else {
        Vim_Key_Binding* query = vim_query_binding(app, &vim_map_operator_pending, handler, &motion_count);
        motion = vim_unwrap_motion_query(query);
    }
	
    String_Const_u8 align_target = vim_get_next_writable(app);
    
    for (i32 i = 0; i < count; i++) {
        if (motion) {
            Vim_Motion_Result mr = vim_execute_motion(app, view, buffer, motion, view_get_cursor_pos(app, view), motion_count);
            range = mr.range;
            inclusive = mr.inclusive;
        }
		
        if (range_size(range) + inclusive > 0) {
            Range_i64 line_range = Ii64(get_line_number_from_pos(app, buffer, range.min), get_line_number_from_pos(app, buffer, range.max));
            Range_i64  col_range = Ii64(); // Ii64(buffer_compute_cursor(app, buffer, seek_pos(range.min)).col, buffer_compute_cursor(app, buffer, seek_pos(range.max)).col);
            
#if 0
            if (inclusive) {
                col_range.max += 1;
            }
#endif
            
            vim_align_range(app, buffer, col_range, line_range, align_target, align_right);
            
            Managed_Scope scope = buffer_get_managed_scope(app, buffer);
            Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
            if (vim_buffer->mode == VimMode_Visual || vim_buffer->mode == VimMode_VisualLine) {
                i64 mark_pos = view_get_mark_pos(app, view);
                view_set_cursor_and_preferred_x(app, view, seek_pos(mark_pos));
            }
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
    snd_write_text_and_auto_indent(app, string_u8_litexpr("\n"));
    vim_enter_insert_mode(app);
}

internal VIM_OPERATOR(vim_new_line_above) {
    seek_beginning_of_line(app);
    move_left(app);
    snd_write_text_and_auto_indent(app, string_u8_litexpr("\n"));
    vim_enter_insert_mode(app);
}

internal VIM_OPERATOR(vim_join_line) {
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
    i64 line_end = get_line_end_pos(app, buffer, line);
    i64 next_line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line + 1);
	
    // @TODO: Do language aware comments
    if (c_line_comment_starts_at_position(app, buffer, line_start) && c_line_comment_starts_at_position(app, buffer, next_line_start)) {
        next_line_start += 2;
        while (character_is_whitespace(buffer_get_char(app, buffer, next_line_start))) {
            next_line_start++;
        }
    }
	
    i64 new_pos = view_get_character_legal_pos_from_pos(app, view, line_end);
    view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
	
    buffer_replace_range(app, buffer, Ii64(line_end, next_line_start), string_u8_litexpr(" "));
}

function void snd_setup_mapping(Application_Links* app, Mapping *mapping, i64 global_id, i64 shared_id, i64 file_id, i64 code_id, i64 normal_id, i64 visual_id) {
    MappingScope();
    SelectMapping(mapping);
	
    SelectMap(global_id);
    {
        BindCore(default_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
        Bind(keyboard_macro_start_recording,    KeyCode_U, KeyCode_Control);
        Bind(keyboard_macro_finish_recording,   KeyCode_U, KeyCode_Control, KeyCode_Shift);
        Bind(keyboard_macro_replay,             KeyCode_U, KeyCode_Alt);
        Bind(change_active_panel,               KeyCode_W, KeyCode_Control);
        Bind(change_active_panel_backwards,     KeyCode_Comma, KeyCode_Control, KeyCode_Shift);
        Bind(interactive_new,                   KeyCode_N, KeyCode_Control);
        Bind(interactive_open_or_new,           KeyCode_O, KeyCode_Control);
        Bind(open_in_other,                     KeyCode_O, KeyCode_Alt);
        Bind(interactive_kill_buffer,           KeyCode_K, KeyCode_Control);
        Bind(interactive_switch_buffer,         KeyCode_I, KeyCode_Control);
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
	
    SelectMap(shared_id);
    {
        ParentMap(global_id);
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
	
    SelectMap(file_id);
    {
        ParentMap(shared_id);
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
        Bind(redo,                                        KeyCode_Y, KeyCode_Control);
        Bind(undo,                                        KeyCode_Z, KeyCode_Control);
        Bind(if_read_only_goto_position,                  KeyCode_Return);
        Bind(if_read_only_goto_position_same_panel,       KeyCode_Return, KeyCode_Shift);
        Bind(view_jump_list_with_lister,                  KeyCode_Period, KeyCode_Control, KeyCode_Shift);
        Bind(vim_enter_normal_mode,                       KeyCode_Escape);
    }
	
    SelectMap(code_id);
    {
        ParentMap(file_id);
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
	
    SelectMap(normal_id);
    {
        ParentMap(shared_id);
		
        InitializeVimBindingHandler(&vim_map_operator_pending, app);
		
        vim_bind_motion(&vim_map_operator_pending, vim_motion_left,                           vim_key(KeyCode_H));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_down,                           vim_key(KeyCode_J));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_up,                             vim_key(KeyCode_K));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_right,                          vim_key(KeyCode_L));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_left,                           vim_key(KeyCode_Left));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_down,                           vim_key(KeyCode_Down));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_up,                             vim_key(KeyCode_Up));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_right,                          vim_key(KeyCode_Right));
        
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word,                           vim_key(KeyCode_W));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word_end,                       vim_key(KeyCode_E));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_word_backward,                  vim_key(KeyCode_B));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_line_start_textual,             vim_key(KeyCode_0));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_line_end_textual,               vim_key(KeyCode_4, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_scope,                          vim_key(KeyCode_5, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_buffer_start,                   vim_key(KeyCode_G), vim_key(KeyCode_G));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_buffer_end,                     vim_key(KeyCode_G, KeyCode_Shift));
		
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character,                 vim_key(KeyCode_F));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_character,                   vim_key(KeyCode_T));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_find_character_backward,        vim_key(KeyCode_F, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_to_character_backward,          vim_key(KeyCode_T, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_repeat_character_seek_forward,  vim_key(KeyCode_Semicolon));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_repeat_character_seek_backward, vim_key(KeyCode_Comma));
        
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_scope, vim_key(KeyCode_I), vim_key(KeyCode_LeftBracket,  KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_scope, vim_key(KeyCode_I), vim_key(KeyCode_RightBracket, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_paren, vim_key(KeyCode_I), vim_key(KeyCode_9, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_paren, vim_key(KeyCode_I), vim_key(KeyCode_0, KeyCode_Shift));
        vim_bind_motion(&vim_map_operator_pending, vim_motion_inner_word,  vim_key(KeyCode_I), vim_key(KeyCode_W));
		
        InitializeVimBindingHandler(&vim_map_normal, app, &vim_map_operator_pending);
#if 0
        Bind(move_up_to_blank_line_end,                     KeyCode_K, KeyCode_Control);
        Bind(move_down_to_blank_line_end,                   KeyCode_J, KeyCode_Control);
#endif
		
		vim_bind_operator(&vim_map_normal, vim_change,                       vim_key(KeyCode_C));
		vim_bind_operator(&vim_map_normal, vim_replace,                      vim_key(KeyCode_R));
        vim_bind_operator(&vim_map_normal, vim_delete,                       vim_key(KeyCode_D));
        vim_bind_operator(&vim_map_normal, vim_yank,                         vim_key(KeyCode_Y));
        vim_bind_operator(&vim_map_normal, vim_new_line_below,               vim_key(KeyCode_O));
        vim_bind_operator(&vim_map_normal, vim_new_line_above,               vim_key(KeyCode_O, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_join_line,                    vim_key(KeyCode_J, KeyCode_Shift));
        vim_bind_operator(&vim_map_normal, vim_align,                        vim_key(KeyCode_G), vim_key(KeyCode_L));
        vim_bind_operator(&vim_map_normal, vim_align_right,                  vim_key(KeyCode_G), vim_key(KeyCode_L, KeyCode_Shift));
		
        vim_bind_text_command(&vim_map_normal, vim_paste,                    vim_key(KeyCode_P));
        vim_bind_text_command(&vim_map_normal, vim_paste_pre_cursor,         vim_key(KeyCode_P, KeyCode_Shift));
		
        vim_bind_text_command(&vim_map_normal, delete_char,                  vim_key(KeyCode_X));
        vim_bind_text_command(&vim_map_normal, vim_enter_insert_mode,        vim_key(KeyCode_I));
        vim_bind_text_command(&vim_map_normal, vim_enter_insert_sol_mode,    vim_key(KeyCode_I, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, vim_enter_append_mode,        vim_key(KeyCode_A));
        vim_bind_text_command(&vim_map_normal, vim_enter_append_eol_mode,    vim_key(KeyCode_A, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, vim_toggle_visual_mode,       vim_key(KeyCode_V));
        vim_bind_text_command(&vim_map_normal, vim_toggle_visual_line_mode,  vim_key(KeyCode_V, KeyCode_Shift));
        vim_bind_text_command(&vim_map_normal, miblo_increment_basic,        vim_key(KeyCode_A, KeyCode_Control));
        vim_bind_text_command(&vim_map_normal, miblo_decrement_basic,        vim_key(KeyCode_X, KeyCode_Control));
		
        vim_bind_generic_command(&vim_map_normal, center_view,               vim_key(KeyCode_Z), vim_key(KeyCode_Z));
		
        vim_bind_generic_command(&vim_map_normal, change_active_panel,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_W));
        vim_bind_generic_command(&vim_map_normal, change_active_panel,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_W, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, swap_panels,               vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_X));
        vim_bind_generic_command(&vim_map_normal, swap_panels,               vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_X, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_left,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_down,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_up,         vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_right,      vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_left,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_down,       vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_up,         vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Control));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_right,      vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Control));
		
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_left,  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_down,  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_up,    vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_right, vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_left,  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_H, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_down,  vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_J, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_up,    vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_K, KeyCode_Control, KeyCode_Shift));
        vim_bind_generic_command(&vim_map_normal, windmove_panel_swap_right, vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_L, KeyCode_Control, KeyCode_Shift));
		
        vim_bind_generic_command(&vim_map_normal, open_file_in_quotes,       vim_key(KeyCode_G), vim_key(KeyCode_F));
		
        vim_named_bind(&vim_map_normal, vim_bind(vim_leader, vim_key(KeyCode_F)), string_u8_litexpr("Files"));
        vim_bind_generic_command(&vim_map_normal, interactive_open_or_new,   vim_bind(vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_E)));
        vim_bind_generic_command(&vim_map_normal, interactive_switch_buffer, vim_bind(vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_B)));
        vim_bind_generic_command(&vim_map_normal, interactive_kill_buffer,   vim_bind(vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_K)));
        vim_bind_generic_command(&vim_map_normal, kill_buffer,               vim_bind(vim_leader, vim_key(KeyCode_F), vim_key(KeyCode_D)));
		
        Bind(vim_repeat_most_recent_command, KeyCode_Period);
        Bind(vim_enter_normal_mode,          KeyCode_Escape);
		
        Bind(snd_move_line_up,               KeyCode_K, KeyCode_Alt);
        Bind(snd_move_line_down,             KeyCode_J, KeyCode_Alt);
		
        // Bind(interactive_open_or_new,        KeyCode_O, KeyCode_Control);
        Bind(goto_next_jump,                 KeyCode_N, KeyCode_Control);
        Bind(goto_prev_jump,                 KeyCode_P, KeyCode_Control);
        Bind(search,                         KeyCode_ForwardSlash);
        Bind(reverse_search,                 KeyCode_ForwardSlash, KeyCode_Shift);
        Bind(undo,                           KeyCode_U);
        Bind(redo,                           KeyCode_R, KeyCode_Control);
        Bind(command_lister,                 KeyCode_Semicolon, KeyCode_Shift);
        Bind(page_up,                        KeyCode_B, KeyCode_Control);
        Bind(page_down,                      KeyCode_F, KeyCode_Control);
    }
	
    SelectMap(visual_id);
    {
        ParentMap(normal_id);
        InitializeVimBindingHandler(&vim_map_visual, app, &vim_map_normal);
		
        vim_bind_generic_command(&vim_map_visual, vim_isearch_selection, vim_key(KeyCode_ForwardSlash));
        vim_bind_generic_command(&vim_map_visual, vim_reverse_isearch_selection, vim_key(KeyCode_ForwardSlash, KeyCode_Shift));
    }
}

#include "4coder_sandvich_hooks.cpp"

void custom_layer_init(Application_Links *app) {
    Thread_Context *tctx = get_thread_context(app);
	
    // NOTE(allen): setup for default framework
    async_task_handler_init(app, &global_async_system);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app);
	
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
	
    set_custom_hook(app, HookID_RenderCaller, vim_render_caller);
    set_custom_hook(app, HookID_BeginBuffer, vim_begin_buffer);
    set_custom_hook(app, HookID_Tick, vim_tick);
    // set_custom_hook(app, HookID_ViewEventHandler, snd_view_input_handler);
	
    vim_state.chord_bar = Su8(vim_state.chord_bar_storage, 0, ArrayCount(vim_state.chord_bar_storage));
	
    mapping_init(tctx, &framework_mapping);
    // setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
    snd_setup_mapping(app, &framework_mapping, mapid_global, vim_mapid_shared, mapid_file, mapid_code, vim_mapid_normal, vim_mapid_visual);
}

#endif //FCODER_DEFAULT_BINDINGS
