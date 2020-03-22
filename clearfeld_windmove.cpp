enum windmove_directions {
    windmove_up,
    windmove_left,
    windmove_down,
    windmove_right
};

void
windmove_to_panel(Application_Links* app, u8 direction, b32 swap_on_move)
{
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID cur_buffer = view_get_buffer(app, view_id, Access_Always);

    Panel_ID original_panel = view_get_panel(app, view_id);
    Rect_f32 original_rect  = view_get_screen_rect(app, view_id);

    Panel_ID current_panel = original_panel;

    Side preferred_side        = (direction == windmove_up   || direction == windmove_left ? Side_Min : Side_Max);
    b32 should_move_horizontal = (direction == windmove_left || direction == windmove_right);

    Panel_ID move_top_node = 0;
    if(panel_get_parent(app, original_panel) != 0) {
        while(move_top_node == 0) {
            Panel_ID parent = panel_get_parent(app, current_panel);
            if(!parent) {
                break;
            }

            Panel_ID min_panel = panel_get_child(app, parent, Side_Min);
            Panel_ID max_panel = panel_get_child(app, parent, Side_Max);

            b32 is_on_wrong_side_of_split = ((current_panel == min_panel && preferred_side == Side_Min) ||
                                             (current_panel == max_panel && preferred_side == Side_Max));

            b32 is_wrong_split = false;

            if(!is_on_wrong_side_of_split) {
                Panel_ID min_search = min_panel;
                while(!panel_is_leaf(app, min_search)) {
                    min_search = panel_get_child(app, min_search, Side_Min);
                }

                Panel_ID max_search = max_panel;
                while(!panel_is_leaf(app, max_search)) {
                    max_search = panel_get_child(app, max_search, Side_Min);
                }

                View_ID min_search_view_id = panel_get_view(app, min_search, Access_Always);
                View_ID max_search_view_id = panel_get_view(app, max_search, Access_Always);

                Rect_f32 min_origin_rect = view_get_screen_rect(app, min_search_view_id);
                Rect_f32 max_origin_rect = view_get_screen_rect(app, max_search_view_id);

                b32 is_vertical   = (min_origin_rect.x0 != max_origin_rect.x0 && min_origin_rect.y0 == max_origin_rect.y0);
                b32 is_horizontal = (min_origin_rect.y0 != max_origin_rect.y0 && min_origin_rect.x0 == max_origin_rect.x0);

                if(should_move_horizontal && is_horizontal || !should_move_horizontal && is_vertical) {
                    is_wrong_split = true;
                }
            }

            if(is_on_wrong_side_of_split || is_wrong_split) {
                current_panel = parent;
            } else {
                move_top_node = parent;
            }
        }
    }

    if(move_top_node != 0) {
        current_panel = panel_get_child(app, move_top_node, preferred_side);

        while(!panel_is_leaf(app, current_panel)) {
            Panel_ID min_panel = panel_get_child(app, current_panel, Side_Min);
            Panel_ID max_panel = panel_get_child(app, current_panel, Side_Max);

            Panel_ID min_search = min_panel;
            while(!panel_is_leaf(app, min_search)) {
                min_search = panel_get_child(app, min_search, Side_Min);
            }

            Panel_ID max_search = max_panel;
            while(!panel_is_leaf(app, max_search)) {
                max_search = panel_get_child(app, max_search, Side_Min);
            }

            Rect_f32 min_origin_rect = view_get_screen_rect(app, panel_get_view(app, min_search, Access_Always));
            Rect_f32 max_origin_rect = view_get_screen_rect(app, panel_get_view(app, max_search, Access_Always));

            b32 is_vertical   = (min_origin_rect.x0 != max_origin_rect.x0 && min_origin_rect.y0 == max_origin_rect.y0);
            b32 is_horizontal = (min_origin_rect.y0 != max_origin_rect.y0 && min_origin_rect.x0 == max_origin_rect.x0);

            if(!should_move_horizontal && is_horizontal || should_move_horizontal && is_vertical) {
                if(preferred_side == Side_Min) {
                    current_panel = max_panel;
                } else {
                    current_panel = min_panel;
                }
            } else {
                f32 dist_from_min = 0;
                f32 dist_from_max = 0;

                if(should_move_horizontal) {
                    dist_from_min = original_rect.y0 - min_origin_rect.y0;
                    dist_from_max = original_rect.y0 - max_origin_rect.y0;
                } else {
                    dist_from_min = original_rect.x0 - min_origin_rect.x0;
                    dist_from_max = original_rect.x0 - max_origin_rect.x0;
                }

                if(dist_from_max < 0 || dist_from_min < dist_from_max) {
                    current_panel = min_panel;
                } else {
                    current_panel = max_panel;
                }
            }
        }

        View_ID target_view = panel_get_view(app, current_panel, Access_Always);
        view_set_active(app, target_view);

        if(swap_on_move) {
            Buffer_ID target_buffer = view_get_buffer(app, target_view, Access_Always);
            view_set_buffer(app, target_view, cur_buffer, Access_Always);
            view_set_buffer(app, view_id, target_buffer, Access_Always);
        }
    }
}

CUSTOM_COMMAND_SIG(windmove_panel_up)
CUSTOM_DOC("Move up from the active view.")
{
    windmove_to_panel(app, windmove_up, false);
}

CUSTOM_COMMAND_SIG(windmove_panel_down)
CUSTOM_DOC("Move down from the active view.")
{
    windmove_to_panel(app, windmove_down, false);
}

CUSTOM_COMMAND_SIG(windmove_panel_left)
CUSTOM_DOC("Move left from the active view.")
{
    windmove_to_panel(app, windmove_left, false);
}

CUSTOM_COMMAND_SIG(windmove_panel_right)
CUSTOM_DOC("Move right from the active view.")
{
    windmove_to_panel(app, windmove_right, false);
}

CUSTOM_COMMAND_SIG(windmove_panel_swap_up)
CUSTOM_DOC("Swap buffer up from the active view.")
{
    windmove_to_panel(app, windmove_up, true);
}

CUSTOM_COMMAND_SIG(windmove_panel_swap_down)
CUSTOM_DOC("Swap buffer down from the active view.")
{
    windmove_to_panel(app, windmove_down, true);
}

CUSTOM_COMMAND_SIG(windmove_panel_swap_left)
CUSTOM_DOC("Swap buffer left from the active view.")
{
    windmove_to_panel(app, windmove_left, true);
}

CUSTOM_COMMAND_SIG(windmove_panel_swap_right)
CUSTOM_DOC("Swap buffer right from the active view.")
{
    windmove_to_panel(app, windmove_right, true);
}
