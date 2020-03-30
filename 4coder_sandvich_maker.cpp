#include "4coder_default_include.cpp"

#define VIM_USE_CUSTOM_COLORS 1
#define VIM_CURSOR_ROUNDNESS 0.0f

#include "4coder_vimmish.cpp"

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
}
