#include "4coder_default_include.cpp"

#define VIM_USE_CUSTOM_COLORS 0
#define VIM_CURSOR_ROUNDNESS 0.0f
#define VIM_USE_ECHO_BAR 1
#define VIM_DRAW_PANEL_MARGINS 0
#define VIM_FILE_BAR_ON_BOTTOM 0
#define VIM_KEYBOARD_LAYOUT VIM_KEYBOARD_LAYOUT_US_INTERNATIONAL

#include "4coder_vimmish.cpp"

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

CUSTOM_COMMAND_SIG(sandvich_startup)
{
    ProfileScope(app, "sandvich startup");
    User_Input input = get_current_input(app);
    if (match_core_code(&input, CoreCode_Startup)){
        String_Const_u8_Array file_names = input.event.core.file_names;
        load_themes_default_folder(app);
        default_4coder_initialize(app, file_names);
        default_4coder_side_by_side_panels(app, file_names);
        
        b32 automatically_load_project = def_get_config_b32(vars_save_string_lit("automatically_load_project"));
        if (automatically_load_project){
            load_project(app);
        }
        toggle_fullscreen(app);
    }
    
    {
        def_enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
        clear_all_layouts(app);
    }
}

void custom_layer_init(Application_Links *app) {
    Thread_Context* tctx = get_thread_context(app);
    
    //
    // Base 4coder Initialization
    //
    
    default_framework_init(app);
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
    
#if OS_MAC
    setup_mac_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#else
    setup_default_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
#endif
	setup_essential_mapping(&framework_mapping, global_map_id, file_map_id, code_map_id);
    
    //
    // Vim Layer Initialization
    //
    
    vim_init(app);
    vim_set_default_hooks(app);
    vim_setup_default_mapping(app, &framework_mapping, vim_key(KeyCode_Space));
    
    vim_add_abbreviation("breka", "break");
    vim_add_abbreviation("ture", "true");
    
    {
        MappingScope();
        SelectMapping(&framework_mapping);
        SelectMap(global_map_id);
        BindCore(sandvich_startup, CoreCode_Startup);
    }
}
