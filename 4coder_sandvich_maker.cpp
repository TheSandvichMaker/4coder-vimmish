// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

CUSTOM_ID(command_map, vim_mapid_shared);
CUSTOM_ID(command_map, vim_mapid_normal);
CUSTOM_ID(attachment, vim_buffer_attachment);

#include "generated/managed_id_metadata.cpp"

#define cast(...) (__VA_ARGS__)

enum Vim_Mode {
    VimMode_Normal,
    VimMode_Insert,
    VimMode_Append,
    VimMode_Visual,
    VimMode_VisualBlock,
};

struct Vim_Motion_Result {
    Range_i64 range;
    i64 seek_pos;
};

inline Vim_Motion_Result vim_null_motion(i64 pos) {
    Vim_Motion_Result result = { Ii64(pos, pos), pos };
    return result;
}

#define VIM_MOTION(name) Vim_Motion_Result name(Application_Links* app, Buffer_ID buffer, i64 start_pos)
typedef VIM_MOTION(Vim_Motion);

#define VIM_ACTION(name) void name(Application_Links* app, View_ID view, Buffer_ID buffer, i32 count)
typedef VIM_ACTION(Vim_Action);

enum Vim_Buffer_Flag {
    VimBufferFlag_TreatAsCode = 0x1,
};

struct Vim_Buffer_Attachment {
    u32 flags;
    Vim_Mode mode;
};

enum Vim_Modifier {
    VimModifier_Control = 0x1,
    VimModifier_Alt     = 0x2,
    VimModifier_Shift   = 0x4,
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
    }
    return 0;
}

inline Key_Code vim_modifier_to_key_code(u32 mod) {
    switch (mod) {
        case VimModifier_Control: return KeyCode_Control;
        case VimModifier_Alt:     return KeyCode_Alt;
        case VimModifier_Shift:   return KeyCode_Shift;
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
    VimBindingKind_Action,
    VimBindingKind_4CoderCommand,
};

enum Vim_Binding_Flag {
    VimBindingFlag_IsRepeatable = 0x1,
};

struct Vim_Key_Binding {
    Vim_Binding_Kind kind;
    u32 flags;
    union {
        Table_u64_u64* nested_keybind_table;
        Vim_Motion* motion;
        Vim_Action* action;
        Custom_Command_Function* fcoder_command;
    };
};

struct Vim_Binding_Handler {
    // @TODO: This doesn't really need to be here, except for print_message.
    Application_Links* app;

    Mapping* mapping;
    Command_Map* command_map;

    Arena node_arena;
    Heap heap;
    Base_Allocator allocator;

    Table_u64_u64 keybind_table;

    b32 recording_command;
    Range_i64 recorded_command_macro_range;
    History_Group command_history;

    i32 chord_bar_count;
    u8 chord_bar[8];
};

global Vim_Binding_Handler vim_binds;

CUSTOM_COMMAND_SIG(vim_handle_input);
#define InitializeVimBindingHandler(handler, app) initialize_vim_binding_handler(handler, app, m, map)
internal void initialize_vim_binding_handler(Vim_Binding_Handler* handler, Application_Links* app, Mapping* mapping, Command_Map* command_map) {
    block_zero_struct(handler);

    handler->app = app;
    handler->mapping = mapping;
    handler->command_map = command_map;
    handler->node_arena = make_arena_system(KB(2));
    heap_init(&handler->heap, &handler->node_arena);
    handler->allocator = base_allocator_on_heap(&handler->heap);
    handler->keybind_table = make_table_u64_u64(&handler->allocator, 32);

    // @Note: These are for when you want to enter a count for an action
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

internal Vim_Motion* vim_query_motion(Application_Links* app, i32* out_motion_count = 0, User_Input* user_in = 0) {
    Vim_Motion* result = 0;

    User_Input in = user_in ? *user_in : get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
    while (key_code_to_vim_modifier(in.event.key.code)) {
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
    }

    i64 motion_count_query = vim_query_number(app, false, &in);
    i32 motion_count = cast(i32) clamp(1, motion_count_query, 256);

    Vim_Key_Binding* bind = 0;

    for (;;) {
        Vim_Key key = get_vim_key_from_input(in);
        if (bind && bind->kind == VimBindingKind_Nested) {
            bind = make_or_retrieve_vim_binding(&vim_binds, bind->nested_keybind_table, key, false);
        } else {
            bind = make_or_retrieve_vim_binding(&vim_binds, &vim_binds.keybind_table, key, false);
        }

        if (bind && bind->kind != VimBindingKind_Nested) {
            if (bind->kind == VimBindingKind_Motion) {
                result = bind->motion;
            }
            break;
        }

        if (!bind) {
            break;
        }

        in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);

        if (in.abort) {
            break;
        }
    }

    if (out_motion_count) *out_motion_count = motion_count;
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

CUSTOM_COMMAND_SIG(snd_move_left_on_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_start_pos = get_line_start_pos_from_pos(app, buffer, cursor_pos);
    i64 legal_line_start_pos = view_get_character_legal_pos_from_pos(app, view, line_start_pos);

    if (cursor_pos != legal_line_start_pos) {
        move_left(app);
    }
}

CUSTOM_COMMAND_SIG(snd_move_right_on_line) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 line_end_pos = get_line_end_pos_from_pos(app, buffer, cursor_pos);
    i64 legal_line_end_pos = view_get_character_legal_pos_from_pos(app, view, line_end_pos);

    if (cursor_pos != legal_line_end_pos) {
        move_right(app);
    }
}

internal void vim_command_begin(Application_Links* app, Buffer_ID buffer) {
    if (vim_binds.recording_command || get_current_input_is_virtual(app)) {
        return;
    }

    vim_binds.recording_command = true;
    vim_binds.command_history = history_group_begin(app, buffer);

    print_message(app, string_u8_litexpr("Began command\n"));
}

internal void vim_command_end(Application_Links* app) {
    if (!vim_binds.recording_command || get_current_input_is_virtual(app)) {
        return;
    }

    vim_binds.recording_command = false;

    Buffer_ID buffer = get_keyboard_log_buffer(app);
    Buffer_Cursor cursor = buffer_compute_cursor(app, buffer, seek_pos(buffer_get_size(app, buffer)));
    vim_binds.recorded_command_macro_range.max = cursor.pos;

    history_group_end(vim_binds.command_history);

    print_message(app, string_u8_litexpr("Ended command\n"));
}

internal void vim_enter_mode(Application_Links* app, Vim_Mode mode) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);

    Command_Map_ID* map_id = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);

    switch (mode) {
        case VimMode_Normal: {
            *map_id = vim_mapid_normal;
            snd_move_left_on_line(app);
            vim_command_end(app);
        } break;

        case VimMode_Append:
        case VimMode_Insert: {
            if (vim_buffer->flags & VimBufferFlag_TreatAsCode) {
                *map_id = mapid_code;
            } else {
                *map_id = mapid_file;
            }
        } break;

        case VimMode_Visual: {
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

CUSTOM_COMMAND_SIG(vim_enter_append_mode) {
    snd_move_right_on_line(app);
    vim_enter_mode(app, VimMode_Append);
}

CUSTOM_COMMAND_SIG(vim_enter_append_eol_mode) {
    seek_end_of_line(app);
    vim_enter_mode(app, VimMode_Append);
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

#if 0
CUSTOM_COMMAND_SIG(snd_new_line_below) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    snd_begin_history_group_for_buffer(app, buffer);

    seek_end_of_line(app);
    snd_write_text_and_auto_indent(app, string_u8_litexpr("\n"));

    snd_exit_command_mode(app);
}

CUSTOM_COMMAND_SIG(snd_new_line_above) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    // TODO(snd): Why does this not restore the cursor to the right position?
    snd_begin_history_group_for_buffer(app, buffer);

    seek_beginning_of_line(app);
    move_left(app);
    snd_write_text_and_auto_indent(app, string_u8_litexpr("\n"));

    snd_exit_command_mode(app);
}

CUSTOM_COMMAND_SIG(snd_join_line) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
    i64 line_end = get_line_end_pos(app, buffer, line);
    i64 next_line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line + 1);

    // @TODO: Do language agnostic comments
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
#endif

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

internal i64 snd_boundary_word(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
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

#if 0
internal Vim_Motion_Result snd_execute_text_motion(Application_Links* app, Buffer_ID buffer, i64 start_pos_init, Scan_Direction dir, Snd_Text_Action associated_action, i32 motion_count, Snd_Text_Motion motion) {
    Vim_Motion_Result outer_result = { Ii64_neg_inf, start_pos_init };

    for (i32 motion_index = 0; motion_index < motion_count; motion_index++) {
        i64 start_pos = outer_result.seek_pos;

        Vim_Motion_Result inner_result = vim_null_motion(start_pos);

        switch (motion) {
            // @TODO: Simplify this, figure out how it should _really_ work
            case SndTextMotion_Word: {
                Scratch_Block scratch(app);

                i64 end_pos = start_pos;

                b32 final_motion = (motion_index + 1 >= motion_count);
                b32 include_final_whitespace = !final_motion || (associated_action != SndTextAction_Change);

                if (dir == Scan_Forward) {
                    if (line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, end_pos))) {
                        i64 next_line = get_line_number_from_pos(app, buffer, end_pos) + 1;
                        i64 next_line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, next_line);

                        end_pos = next_line_start;
                    } else {
                        end_pos = scan(app, push_boundary_list(scratch, snd_boundary_word, boundary_line), buffer, Scan_Forward, end_pos);

                        u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
                        if (include_final_whitespace && !character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                            end_pos = scan(app, push_boundary_list(scratch, boundary_whitespace, boundary_line), buffer, Scan_Forward, end_pos);
                        }
                    }

                    if (final_motion && (associated_action != SndTextAction_None)) {
                        // @Note: To mimic vim behaviour, we'll have to slice off one character from the end when using word motion for text actions
                        end_pos--;
                    }
                } else {
                    if (line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, end_pos))) {
                        i64 next_line = get_line_number_from_pos(app, buffer, end_pos) - 1;
                        i64 next_line_end = get_line_end_pos(app, buffer, next_line);

                        end_pos = next_line_end;
                    } else {
                        end_pos = scan(app, push_boundary_list(scratch, snd_boundary_word, boundary_line), buffer, Scan_Backward, end_pos);

                        u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
                        if (!character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                            end_pos = scan(app, push_boundary_list(scratch, boundary_whitespace, boundary_line), buffer, Scan_Backward, end_pos);
                        }
                    }
                }

                inner_result.seek_pos = end_pos;
                inner_result.range = Ii64(start_pos, end_pos);
            } break;

            case SndTextMotion_WordEnd: {
                Scratch_Block scratch(app);
                // @TODO: Use consistent word definition
                inner_result.seek_pos = scan(app, push_boundary_list(scratch, boundary_token), buffer, Scan_Forward, start_pos + 1) - 1; // @Unicode
                inner_result.range = Ii64(start_pos, inner_result.seek_pos);
            } break;

            case SndTextMotion_LineSide: {
                inner_result.range = Ii64(start_pos, get_line_side_pos_from_pos(app, buffer, start_pos, dir == Scan_Forward ? Side_Max : Side_Min));
                inner_result.seek_pos = (dir == Scan_Forward) ? inner_result.range.max : inner_result.range.min;
            } break;

            case SndTextMotion_ScopeVimStyle: {
                Token_Array token_array = get_token_array_from_buffer(app, buffer);
                if (token_array.count > 0) {
                    Token_Base_Kind opening_token = TokenBaseKind_EOF;
                    Range_i64 line_range = get_line_pos_range(app, buffer, get_line_number_from_pos(app, buffer, start_pos));
                    Token_Iterator_Array it = token_iterator_pos(0, &token_array, start_pos);

                    b32 loop = true;
                    while (loop && opening_token == TokenBaseKind_EOF) {
                        Token* token = token_it_read(&it);
                        if (range_contains_inclusive(line_range, token->pos)) {
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

                                    inner_result.seek_pos = end_pos;
                                    inner_result.range = Ii64(start_pos, end_pos);

                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
            } break;

            case SndTextMotion_Scope4CoderStyle: {
                Find_Nest_Flag flags = FindNest_Scope;
                Range_i64 range = Ii64(start_pos);
                if (dir == Scan_Forward) {
                    if (find_nest_side(app, buffer, range.start + 1, flags, Scan_Forward, NestDelim_Open, &range) &&
                        find_nest_side(app, buffer, range.end, flags|FindNest_Balanced|FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &range.end)
                    ) {
                        inner_result.range = range;
                        inner_result.seek_pos = range.min;
                    }
                } else {
                    if (find_nest_side(app, buffer, start_pos - 1, flags, Scan_Backward, NestDelim_Open, &range) &&
                        find_nest_side(app, buffer, range.end, flags|FindNest_Balanced|FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &range.end)
                    ) {
                        inner_result.range = range;
                        inner_result.seek_pos = range.min;
                    }
                }
            } break;

            case SndTextMotion_SurroundingScope: {
                Range_i64 range = Ii64(start_pos);
                if (find_surrounding_nest(app, buffer, range.min, FindNest_Scope, &range)) {
                    for (;;) {
                        if (!find_surrounding_nest(app, buffer, range.min, FindNest_Scope, &range)) {
                            break;
                        }
                    }
                }
                inner_result.range = range;
                inner_result.seek_pos = range.min;
            } break;
        }

        outer_result = { range_union(outer_result.range, inner_result.range), inner_result.seek_pos };
    }

    return outer_result;
}
#endif

internal void snd_cut_range_inclusive(Application_Links* app, View_ID view, Buffer_ID buffer, Range_i64 range) {
    view_set_cursor(app, view, seek_pos(range.min));
    view_set_mark(app, view, seek_pos(range.max + 1));

    cut(app);
}

#if 0
internal void snd_execute_text_action(Application_Links* app, View_ID view, Buffer_ID buffer, Snd_Text_Action action, Range_i64 range) {
    switch (action) {
        case SndTextAction_Cut: {
            snd_cut_range_inclusive(app, view, buffer, range);
            vim_command_end(app);
        } break;

        case SndTextAction_Change: {
            snd_cut_range_inclusive(app, view, buffer, range);
            snd_exit_command_mode(app);
        } break;
    }
}

enum Snd_Command_Kind {
    SndCommandKind_None,

    SndCommandKind_Utiliy,
    SndCommandKind_Window,
    SndCommandKind_Text,
};

internal Snd_Text_Motion snd_get_text_motion_from_input(User_Input in, Scan_Direction* out_dir) {
    Snd_Text_Motion result = SndTextMotion_None;
    Scan_Direction dir = Scan_Forward;

    if (in.event.kind == InputEventKind_KeyStroke) {
        Key_Code kc = in.event.key.code;
        b32 ctrl = has_modifier(&in, KeyCode_Control);
        b32 shift = has_modifier(&in, KeyCode_Shift);

        switch (kc) {
            case KeyCode_W: { result = SndTextMotion_Word; } break;
            case KeyCode_E: { result = SndTextMotion_WordEnd; } break;
            case KeyCode_B: {
                result = SndTextMotion_Word;
                dir = Scan_Backward;
            } break;
            case KeyCode_0: {
                result = SndTextMotion_LineSide;
                dir = Scan_Backward;
            } break;
            case KeyCode_4: {
                if (shift) result = SndTextMotion_LineSide;
            } break;
            case KeyCode_5: {
                if (shift) result = SndTextMotion_ScopeVimStyle;
            } break;
            case KeyCode_LeftBracket: {
                if (ctrl && shift) {
                    result = SndTextMotion_SurroundingScope;
                } else if (shift) {
                    result = SndTextMotion_Scope4CoderStyle;
                    dir = Scan_Backward;
                }
            } break;
            case KeyCode_RightBracket: {
                if (shift) result = SndTextMotion_Scope4CoderStyle;
            } break;
        }
    }

    if (out_dir) *out_dir = dir;

    return result;
}

internal CUSTOM_COMMAND_SIG(snd_do_text_motion) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    User_Input in = get_current_input(app);
    Scan_Direction dir = Scan_Forward;
    Snd_Text_Motion motion = snd_get_text_motion_from_input(in, &dir);

    Vim_Motion_Result mr = snd_execute_text_motion(app, buffer, view_get_cursor_pos(app, view), dir, SndTextAction_None, 1, motion);
    view_set_cursor_and_preferred_x(app, view, seek_pos(mr.seek_pos));
}

internal CUSTOM_COMMAND_SIG(snd_handle_chordal_input) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (!buffer_exists(app, buffer)) {
        return;
    }

    if (!vim_most_recent_command.is_open && !get_current_input_is_virtual(app)) {
        Buffer_ID keyboard_log_buffer = get_keyboard_log_buffer(app);
        i64 end = buffer_get_size(app, keyboard_log_buffer);
        Buffer_Cursor cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_pos(end));
        Buffer_Cursor back_cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_line_col(cursor.line - 1, 1));
        vim_binds.recorded_command_macro_range.min = back_cursor.pos;
    }

    Snd_Command_Kind command_kind = SndCommandKind_None;

    Key_Code utility_kc = 0;

    Snd_Window_Action window_action = SndWindowAction_None;

    Snd_Text_Action text_action = SndTextAction_None;
    Snd_Text_Motion text_motion = SndTextMotion_None;
    Scan_Direction  text_motion_direction = Scan_Forward;
    i32             text_motion_count     = 0;

    User_Input in = get_current_input(app);

    b32 first_time_in_loop = true;
    for (;;) {
        if (first_time_in_loop) {
            first_time_in_loop = false;
        } else {
            in = get_next_input(app, (EventPropertyGroup_AnyKeyboardEvent & ~EventProperty_AnyKeyRelease), EventProperty_Escape|EventProperty_ViewActivation);
        }

        if (in.abort) {
            break;
        }

        Key_Code kc = in.event.key.code;
        b32 ctrl_down  = has_modifier(&in, KeyCode_Control);
        b32 shift_down = has_modifier(&in, KeyCode_Shift);
        b32 has_modifiers = is_modified(&in.event);

        if (kc == KeyCode_Control || kc == KeyCode_Shift) {
            continue;
        }

        if (in.event.kind == InputEventKind_KeyStroke) {
            //
            // Complete Actions
            //
            b32 hit_any = true;
            if (kc == KeyCode_G && shift_down) {
                // @Note: Inline command
                goto_end_of_file(app);
            } else {
                hit_any = false;
            } if (hit_any) break;

            //
            // Action Leaders
            //
            Snd_Text_Action entered_action = SndTextAction_None;
            if (kc == KeyCode_C) {
                entered_action = SndTextAction_Change;
            } else if (kc == KeyCode_D) {
                entered_action = SndTextAction_Cut;
            }

            if (entered_action) {
                if (!text_action) {
                    text_action = entered_action;
                    command_kind = SndCommandKind_Text;
                    continue;
                } else {
                    if (text_action == entered_action) {
                        // @Incomplete: Do the right behaviour
                        break;
                    } else {
                        break;
                    }
                }
            }

            if (!command_kind) {
                hit_any = true;
                if (kc == KeyCode_G || kc == KeyCode_Z) {
                    command_kind = SndCommandKind_Utiliy;
                    utility_kc = kc;
                } else if (kc == KeyCode_W && ctrl_down) {
                    if (command_kind == SndCommandKind_Window) {
                        window_action = SndWindowAction_Cycle;
                    }
                    command_kind = SndCommandKind_Window;
                } else {
                    hit_any = false;
                } if (hit_any) continue;
            }

            //
            // Action Tails
            //
            if (command_kind == SndCommandKind_Utiliy) {
                if (utility_kc == KeyCode_Z) {
                    if (kc == KeyCode_Z) {
                        center_view(app);
                        break;
                    }
                }

                if (utility_kc == KeyCode_G) {
                    if (kc == KeyCode_G) {
                        goto_beginning_of_file(app);
                        break;
                    }
                }
            }

            //
            // Motion Count
            //
            if (!has_modifiers && (kc >= KeyCode_0 && kc <= KeyCode_9)) {
                i32 count = cast(i32) (kc - KeyCode_0);
                if (!text_motion_count) {
                    if (kc != KeyCode_0) {
                        text_motion_count = count;
                        continue;
                    }
                } else {
                    text_motion_count = 10*text_motion_count + count;
                    continue;
                }
            }

            //
            // Action Tails / Motions
            //
            text_motion = snd_get_text_motion_from_input(in, &text_motion_direction);
            if (text_motion) {
                break;
            }
        }
    }

    if (!text_motion_count) {
        text_motion_count = 1;
    }

    if (command_kind == SndCommandKind_Window) {
        Panel_ID panel = view_get_panel(app, view);
        switch (window_action) {
            case SndWindowAction_Cycle: { change_active_panel(app); } break;
            case SndWindowAction_HSplit: { panel_split(app, panel, Dimension_Y); } break;
            case SndWindowAction_VSplit: { panel_split(app, panel, Dimension_X); } break;
            case SndWindowAction_Swap: { swap_panels(app); } break;
        }
    } else {
        if (text_motion) {
            Vim_Motion_Result mr = snd_execute_text_motion(app, buffer, view_get_cursor_pos(app, view), text_motion_direction, text_action, text_motion_count, text_motion);
            u32 buffer_flags = buffer_get_access_flags(app, buffer);
            if (text_action && (buffer_flags & Access_Write)) {
                vim_command_begin(app, buffer);
                snd_execute_text_action(app, view, buffer, text_action, mr.range);
            } else {
                view_set_cursor_and_preferred_x(app, view, seek_pos(mr.seek_pos));
            }
        }
    }
}
#endif

CUSTOM_COMMAND_SIG(snd_repeat_most_recent_command) {
    if (vim_binds.recording_command || get_current_input_is_virtual(app)) {
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
    String_Const_u8 macro = push_buffer_range(app, scratch, keyboard_buffer, vim_binds.recorded_command_macro_range);

    History_Group history = history_group_begin(app, buffer);
    keyboard_macro_play(app, macro);
    history_group_end(history);
}

CUSTOM_COMMAND_SIG(vim_handle_input) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    User_Input in = get_current_input(app);

    if (!vim_binds.recording_command && !get_current_input_is_virtual(app)) {
        Buffer_ID keyboard_log_buffer = get_keyboard_log_buffer(app);
        i64 end = buffer_get_size(app, keyboard_log_buffer);
        Buffer_Cursor cursor      = buffer_compute_cursor(app, keyboard_log_buffer, seek_pos(end));
        Buffer_Cursor back_cursor = buffer_compute_cursor(app, keyboard_log_buffer, seek_line_col(cursor.line - 1, 1));
        vim_binds.recorded_command_macro_range.min = back_cursor.pos;
    }

    i32 action_count = 1;
    if (input_to_digit(in) > 0) {
        i64 queried_number = vim_query_number(app, false, &in);
        action_count = cast(i32) clamp(1, queried_number, 256);
        Scratch_Block scratch(app);
        String_Const_u8 string = push_u8_stringf(scratch, "Action count: %d (query result: %lld)\n", action_count, queried_number);
        print_message(app, string);
    }

    Vim_Key key = get_vim_key_from_input(in);
    Vim_Key_Binding* bind = make_or_retrieve_vim_binding(&vim_binds, &vim_binds.keybind_table, key, false);

    if (!bind) {
        return;
    }

    if (bind->kind == VimBindingKind_Nested) {
        do {
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation);
            if (in.abort) return;
        } while (key_code_to_vim_modifier(in.event.key.code));
        key  = get_vim_key_from_input(in);
        bind = make_or_retrieve_vim_binding(&vim_binds, bind->nested_keybind_table, key, false);
    }

    if (!bind) {
        return;
    }

    if (bind->flags & VimBindingFlag_IsRepeatable) {
        vim_command_begin(app, buffer);
    }

    switch (bind->kind) {
        case VimBindingKind_Motion: {
            // print_message(app, string_u8_litexpr("It is a   M O T I O N\n"));
            Vim_Motion_Result mr = bind->motion(app, buffer, view_get_cursor_pos(app, view));
            view_set_cursor_and_preferred_x(app, view, seek_pos(mr.seek_pos));
        } break;

        case VimBindingKind_Action: {
            print_message(app, string_u8_litexpr("It is an   A C T I O N\n"));

            bind->action(app, view, buffer, action_count);
        } break;

        case VimBindingKind_4CoderCommand: {
            print_message(app, string_u8_litexpr("It is a   4 C O D E R   C O M M A N D\n"));

            bind->fcoder_command(app);
        } break;
    }

    if (bind->flags & VimBindingFlag_IsRepeatable) {
        Managed_Scope scope = buffer_get_managed_scope(app, buffer);
        Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
        if (*map_id_ptr == cast(i64) vim_mapid_normal) {
            vim_command_end(app);
        }
    }
}

internal Vim_Key_Binding* add_vim_binding(Vim_Binding_Handler* handler, Vim_Key key1, Vim_Key key2) {
    Vim_Key_Binding* result = 0;

    if (key1.kc) {
        Command_Binding binding;
        binding.custom = vim_handle_input;

        Input_Modifier_Set_Fixed fixed_modifiers = vim_modifiers_to_input_modifier_set_fixed(key1.mods);
        Input_Modifier_Set modifiers = { fixed_modifiers.mods, fixed_modifiers.count };
        map_set_binding_key(handler->mapping, handler->command_map, binding, key1.kc, &modifiers);

        Vim_Key_Binding* key1_binding = make_or_retrieve_vim_binding(handler, &handler->keybind_table, key1, true);

        if (key2.kc) {
            if (key1_binding->kind != VimBindingKind_Nested) {
                if (key1_binding->kind != VimBindingKind_None) {
                    Scratch_Block scratch(handler->app);
                    String_Const_u8 string = push_u8_stringf(scratch, "Warning: Stomped on prior vim keybind of type %u with key code %u and modifiers 0x%x\n",
                        key1_binding->kind, key1.kc, key1.mods);
                    print_message(handler->app, string);
                }

                key1_binding->kind = VimBindingKind_Nested;

                Table_u64_u64* inner_table = push_array(&handler->node_arena, Table_u64_u64, 1);
                *inner_table = make_table_u64_u64(&handler->allocator, 8);

                key1_binding->nested_keybind_table = inner_table;
            }

            result = make_or_retrieve_vim_binding(handler, key1_binding->nested_keybind_table, key2, true);
        } else {
            result = key1_binding;
        }
    }

    return result;
}

internal void add_vim_motion(Vim_Binding_Handler* handler, Vim_Motion* motion, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, key1, key2);
    bind->kind = VimBindingKind_Motion;
    bind->flags |= flags;
    bind->motion = motion;
}

internal void add_vim_action(Vim_Binding_Handler* handler, Vim_Action* action, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, key1, key2);
    bind->kind = VimBindingKind_Action;
    bind->flags |= flags|VimBindingFlag_IsRepeatable;
    bind->action = action;
}

internal void add_vim_4coder_command(Vim_Binding_Handler* handler, Custom_Command_Function* fcoder_command, Vim_Key key1, Vim_Key key2 = {}, u32 flags = 0) {
    Vim_Key_Binding* bind = add_vim_binding(handler, key1, key2);
    bind->kind = VimBindingKind_4CoderCommand;
    bind->flags |= flags;
    bind->fcoder_command = fcoder_command;
}

internal b32 snd_align_range(Application_Links* app, Buffer_ID buffer, Range_i64 col_range, Range_i64 line_range, String_Const_u8 align_target, b32 align_after_target) {
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
        did_align |= snd_align_range(app, buffer, Ii64(furthest_align_cursor.col + 1, col_range.max), line_range, align_target, align_after_target);
    }

    return did_align;
}

internal VIM_ACTION(vim_align) {
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos   = view_get_mark_pos(app, view);

    Range_i64 line_range = Ii64(get_line_number_from_pos(app, buffer, cursor_pos), get_line_number_from_pos(app, buffer, mark_pos));

    snd_align_range(app, buffer, Ii64(cast(i64) 0), line_range, string_u8_litexpr("="), false);
}

internal VIM_ACTION(vim_align_right) {
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos   = view_get_mark_pos(app, view);

    Range_i64 line_range = Ii64(get_line_number_from_pos(app, buffer, cursor_pos), get_line_number_from_pos(app, buffer, mark_pos));

    snd_align_range(app, buffer, Ii64(cast(i64) 0), line_range, string_u8_litexpr("="), true);
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
            end_pos = scan(app, push_boundary_list(scratch, snd_boundary_word, boundary_line), buffer, Scan_Forward, end_pos);

            u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
            if (!character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                end_pos = scan(app, push_boundary_list(scratch, boundary_whitespace, boundary_line), buffer, Scan_Forward, end_pos);
            }
        }
    } else {
        if (line_is_valid_and_blank(app, buffer, get_line_number_from_pos(app, buffer, end_pos))) {
            i64 next_line = get_line_number_from_pos(app, buffer, end_pos) - 1;
            i64 next_line_end = get_line_end_pos(app, buffer, next_line);

            end_pos = next_line_end;
        } else {
            end_pos = scan(app, push_boundary_list(scratch, snd_boundary_word, boundary_line), buffer, Scan_Backward, end_pos);

            u8 c_at_cursor = buffer_get_char(app, buffer, end_pos);
            if (!character_is_newline(c_at_cursor) && character_is_whitespace(c_at_cursor)) {
                end_pos = scan(app, push_boundary_list(scratch, boundary_whitespace, boundary_line), buffer, Scan_Backward, end_pos);
            }
        }
    }

    result.seek_pos = end_pos;
    result.range = Ii64(start_pos, end_pos);

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

internal VIM_MOTION(vim_motion_buffer_start) {
    Vim_Motion_Result result = vim_null_motion(start_pos);

    result.range = Ii64(0, start_pos);
    result.seek_pos = 0;

    return result;
}

internal VIM_MOTION(vim_motion_buffer_end) {
    Vim_Motion_Result result = vim_null_motion(start_pos);

    result.range = Ii64(start_pos, buffer_get_size(app, buffer) - 1);
    result.seek_pos = result.range.max;

    return result;
}

internal Vim_Motion_Result vim_motion_line_side(Application_Links* app, Buffer_ID buffer, Scan_Direction dir, i64 start_pos) {
    Vim_Motion_Result result = vim_null_motion(start_pos);
    result.range = Ii64(start_pos, get_line_side_pos_from_pos(app, buffer, start_pos, dir == Scan_Forward ? Side_Max : Side_Min));
    result.seek_pos = (dir == Scan_Forward) ? result.range.max : result.range.min;
    return result;
}

internal VIM_MOTION(vim_motion_line_start) {
    return vim_motion_line_side(app, buffer, Scan_Backward, start_pos);
}

internal VIM_MOTION(vim_motion_line_end) {
    return vim_motion_line_side(app, buffer, Scan_Forward, start_pos);
}

internal VIM_ACTION(vim_change) {
    Vim_Motion* motion = vim_query_motion(app);

    if (motion) {
        Vim_Motion_Result mr = motion(app, buffer, view_get_cursor_pos(app, view));
        snd_cut_range_inclusive(app, view, buffer, mr.range);
        vim_enter_insert_mode(app);
    }
}

//
// @Note: Hooks and such
//

function void vim_draw_cursor(Application_Links *app, View_ID view_id, b32 is_active_view, Buffer_ID buffer, Text_Layout_ID text_layout_id, f32 roundness, f32 outline_thickness, Vim_Mode mode) {
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);
    if (!has_highlight_range) {
        i32 cursor_sub_id = default_cursor_sub_id();

        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        i64 mark_pos = view_get_mark_pos(app, view_id);
        if (is_active_view) {
            if (mode == VimMode_Normal) {
                draw_character_block(app, text_layout_id, cursor_pos, roundness, fcolor_id(defcolor_cursor, cursor_sub_id));
                paint_text_color_pos(app, text_layout_id, cursor_pos, fcolor_id(defcolor_at_cursor));
                draw_character_wire_frame(app, text_layout_id, mark_pos, roundness, outline_thickness, fcolor_id(defcolor_mark));
            } else {
                draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(defcolor_cursor, cursor_sub_id));
                draw_character_wire_frame(app, text_layout_id, mark_pos, roundness, outline_thickness, fcolor_id(defcolor_mark));
            }
        } else {
            /* Draw nothing at all... */
        }
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
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
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
    if (global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
    }

    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (metrics.normal_advance*0.5f)*0.9f;
    f32 mark_thickness = 2.f;

    // NOTE(allen): Cursor
    // NOTE(snd): But do it my way!!!
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Vim_Buffer_Attachment* vim_buffer = scope_attachment(app, scope, vim_buffer_attachment, Vim_Buffer_Attachment);
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

    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));

#if 0
    switch (vim_buffer->mode) {
        case VimMode_Append:
        case VimMode_Insert: {
            draw_rectangle(app, bar, 0.f, pack_color(V4f32(1.0f, 1.0f, 0.f, 0.15f)));
        } break;
    }
#endif

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
    // NOTE(snd): but do it my way!!!
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
                    string_match(ext, string_u8_litexpr("cc"))
                ) {
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
    // TODO(snd): I don't know if scope attachments are zero initialised so better safe than sorry
    block_zero_struct(vim_buffer);
    if (treat_as_code) {
        vim_buffer->flags |= VimBufferFlag_TreatAsCode;
    }

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

function void snd_setup_mapping(Application_Links* app, Mapping *mapping, i64 global_id, i64 shared_id, i64 file_id, i64 code_id, i64 command_id) {
    MappingScope();
    SelectMapping(mapping);

    SelectMap(global_id);
    {
        BindCore(default_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
        Bind(keyboard_macro_start_recording ,   KeyCode_U, KeyCode_Control);
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

    SelectMap(command_id);
    {
        ParentMap(shared_id);
#if 0
        Bind(snd_do_text_motion,                            KeyCode_W);
        Bind(snd_do_text_motion,                            KeyCode_W, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_E);
        Bind(snd_do_text_motion,                            KeyCode_B);
        Bind(snd_do_text_motion,                            KeyCode_B, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_0);
        Bind(snd_do_text_motion,                            KeyCode_4, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_5, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_G, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_LeftBracket, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
        Bind(snd_do_text_motion,                            KeyCode_RightBracket, KeyCode_Shift);

        Bind(snd_handle_chordal_input,                      KeyCode_W, KeyCode_Control);
        Bind(snd_handle_chordal_input,                      KeyCode_D);
        Bind(snd_handle_chordal_input,                      KeyCode_C);
        Bind(snd_handle_chordal_input,                      KeyCode_Z);
        Bind(snd_handle_chordal_input,                      KeyCode_G);
#endif

        Bind(move_up,                                       KeyCode_K);
        Bind(move_up_to_blank_line_end,                     KeyCode_K, KeyCode_Control);
        Bind(snd_move_line_up,                              KeyCode_K, KeyCode_Alt);
        Bind(move_down,                                     KeyCode_J);
        Bind(move_down_to_blank_line_end,                   KeyCode_J, KeyCode_Control);
        Bind(snd_move_line_down,                            KeyCode_J, KeyCode_Alt);
#if 0
        Bind(snd_join_line,                                 KeyCode_J, KeyCode_Shift);
#endif
        Bind(move_left,                                     KeyCode_H);
        Bind(move_right,                                    KeyCode_L);
#if 0
        Bind(snd_new_line_below,                            KeyCode_O);
        Bind(snd_new_line_above,                            KeyCode_O, KeyCode_Shift);
#endif
        Bind(interactive_open_or_new,                       KeyCode_O, KeyCode_Control);
        Bind(move_right,                                    KeyCode_L);
        Bind(page_up,                                       KeyCode_B, KeyCode_Control);
        Bind(page_down,                                     KeyCode_F, KeyCode_Control);
        Bind(snd_repeat_most_recent_command,                KeyCode_Period);
        Bind(set_mark,                                      KeyCode_V);
        Bind(snd_select_line,                               KeyCode_V, KeyCode_Shift);
        Bind(delete_char,                                   KeyCode_X);
#if 0
        Bind(snd_inclusive_cut,                             KeyCode_D, KeyCode_Shift);
        Bind(snd_change,                                    KeyCode_C, KeyCode_Shift);
#endif
        Bind(copy,                                          KeyCode_Y);
        Bind(paste_and_indent,                              KeyCode_P);
        Bind(goto_next_jump,                                KeyCode_N, KeyCode_Control);
        Bind(goto_prev_jump,                                KeyCode_P, KeyCode_Control);
        Bind(search,                                        KeyCode_ForwardSlash);
        Bind(undo,                                          KeyCode_U);
        Bind(redo,                                          KeyCode_R, KeyCode_Control);
        Bind(command_lister,                                KeyCode_Semicolon, KeyCode_Shift);

        InitializeVimBindingHandler(&vim_binds, app);
        add_vim_motion(&vim_binds, vim_motion_word, vim_key(KeyCode_W));
        add_vim_motion(&vim_binds, vim_motion_word_backward, vim_key(KeyCode_B));
        add_vim_motion(&vim_binds, vim_motion_line_start,   vim_key(KeyCode_0));
        add_vim_motion(&vim_binds, vim_motion_line_end, vim_key(KeyCode_4, KeyCode_Shift));
        add_vim_motion(&vim_binds, vim_motion_buffer_start, vim_key(KeyCode_G), vim_key(KeyCode_G));
        add_vim_motion(&vim_binds, vim_motion_buffer_end,   vim_key(KeyCode_G, KeyCode_Shift));

        add_vim_action(&vim_binds, vim_change,      vim_key(KeyCode_C));
        add_vim_action(&vim_binds, vim_align,       vim_key(KeyCode_G), vim_key(KeyCode_L));
        add_vim_action(&vim_binds, vim_align_right, vim_key(KeyCode_G), vim_key(KeyCode_L, KeyCode_Shift));

        add_vim_4coder_command(&vim_binds, vim_enter_insert_mode,  vim_key(KeyCode_I), {}, VimBindingFlag_IsRepeatable);
        add_vim_4coder_command(&vim_binds, vim_enter_append_mode,  vim_key(KeyCode_A), {}, VimBindingFlag_IsRepeatable);
        add_vim_4coder_command(&vim_binds, vim_enter_append_eol_mode, vim_key(KeyCode_A, KeyCode_Shift), {}, VimBindingFlag_IsRepeatable);
        add_vim_4coder_command(&vim_binds, center_view,            vim_key(KeyCode_Z), vim_key(KeyCode_Z));
        add_vim_4coder_command(&vim_binds, swap_panels,            vim_key(KeyCode_W, KeyCode_Control), vim_key(KeyCode_W, KeyCode_Control));
#if 0
        add_vim_motion(&vim_binds, vim_motion_left,              { KeyCode_H });
        add_vim_motion(&vim_binds, vim_motion_down,              { KeyCode_J });
        add_vim_motion(&vim_binds, vim_motion_up,                { KeyCode_K });
        add_vim_motion(&vim_binds, vim_motion_down,              { KeyCode_L });
        add_vim_motion(&vim_binds, vim_motion_word_end,          { KeyCode_E });
        add_vim_motion(&vim_binds, vim_motion_scope,             { KeyCode_6, VimModifier_Shift });
        add_vim_motion(&vim_binds, vim_motion_buffer_end,        { KeyCode_G, VimModifier_Shift });
        add_vim_motion(&vim_binds, vim_motion_buffer_start,      { KeyCode_G }, { KeyCode_G });

        add_vim_action(&vim_binds, vim_align,       { KeyCode_L });
        add_vim_action(&vim_binds, vim_align_right, { KeyCode_L, VimModifier_Shift });

        add_vim_action(&vim_binds, vim_move_panel_left,  { KeyCode_W, KeyCode_Control }, { KeyCode_H });
        add_vim_action(&vim_binds, vim_move_panel_down,  { KeyCode_W, KeyCode_Control }, { KeyCode_J });
        add_vim_action(&vim_binds, vim_move_panel_up,    { KeyCode_W, KeyCode_Control }, { KeyCode_K });
        add_vim_action(&vim_binds, vim_move_panel_right, { KeyCode_W, KeyCode_Control }, { KeyCode_L });
#endif
    }
}

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
    // set_custom_hook(app, HookID_ViewEventHandler, snd_view_input_handler);

    mapping_init(tctx, &framework_mapping);
    // setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
    snd_setup_mapping(app, &framework_mapping, mapid_global, vim_mapid_shared, mapid_file, mapid_code, vim_mapid_normal);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

