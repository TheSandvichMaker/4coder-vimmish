#include "4coder_default_include.cpp"
#include "4coder_vimmish.cpp"

#include "generated/managed_id_metadata.cpp"

void custom_layer_init(Application_Links *app) {
    Thread_Context *tctx = get_thread_context(app);
	
    default_framework_init(app);
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
	
    vim_init(app);
    vim_set_default_hooks(app);
    vim_setup_default_mapping(app, &framework_mapping, mapid_global, vim_mapid_shared, mapid_file, mapid_code, vim_mapid_normal, vim_mapid_visual);
}
