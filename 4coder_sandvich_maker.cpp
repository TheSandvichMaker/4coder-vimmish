#include "4coder_default_include.cpp"

#define VIM_USE_CUSTOM_COLORS 1
#define VIM_CURSOR_ROUNDNESS 0.0f

#include "4coder_vimmish.cpp"

CUSTOM_COMMAND_SIG(print_key_codes)
CUSTOM_DOC("Get the Key_Code for any given key")
{
    print_message(app, string_u8_litexpr("Type any key to see its Key_Code, ESC to exit.\n"));
    for (;;) {
        User_Input in = get_next_input(app, EventProperty_AnyKey, EventProperty_Escape|EventProperty_ViewActivation|EventProperty_Exit);
        if (in.abort) {
            break;
        }
        print_message(app, string_u8_litexpr("Key_Code: "));
        print_message(app, SCu8(key_code_name[in.event.key.code]));
        print_message(app, string_u8_litexpr("\n"));
    }
    print_message(app, string_u8_litexpr("Exited print_key_codes.\n"));
}

#include "generated/managed_id_metadata.cpp"

void custom_layer_init(Application_Links *app) {
    Thread_Context* tctx = get_thread_context(app);

    //
    // Base 4coder Initialization
    //

    default_framework_init(app);
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);

    //
    // Vim Layer Initialization
    //

    vim_init(app);
    vim_set_default_hooks(app);
    vim_setup_default_mapping(app, &framework_mapping, vim_key(KeyCode_Space));

    vim_add_abbreviation("breka", "break");
}
