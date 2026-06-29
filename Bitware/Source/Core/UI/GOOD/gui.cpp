#include "headers/includes.h"
#include <Core/Settings/Settings.h>
#include <Core/Features/Cache/Cache.h>
#include <Miscellaneous/Config/ConfigManager.h>
#include <Version.h>
#include <Engine/Offsets/Offsets.h>
#include <Globals.hxx>

#include <algorithm>

#include <Windows.h>

// https://www.unknowncheats.me/forum/members/3936684.html
static bool license_key_matches(const char* key)
{
	return std::string(key) == "demo";
}

static void sync_layout_preferences()
{
	elements->padding = var->gui.compact_layout ? c_vec2(8, 8) : c_vec2(10, 10);
	elements->window.rounding = var->gui.window_rounding;
}



static float visual_feature_padding_x = 0.f;
static float visual_feature_padding_y = 0.f;
static float visual_outer_padding = 8.f;
static float visual_sidebar_width = 170.f;
static float visual_window_width = 680.f;
static float visual_window_height = 380.f;
static float visual_column_gap = 8.f;

extern c_vec4 g_sidebar_selected_rect;
extern c_vec4 g_pill_selected_rect;

struct visual_panel_density_state {};

static visual_panel_density_state begin_visual_panel_density() { return {}; }
static void end_visual_panel_density(visual_panel_density_state) {}

static float visual_section_height(int section_count)
{
	const float content_height = gui->content_avail().y;
	const float gaps = s_(visual_column_gap) * ImMax(section_count - 1, 0);
	return ImMax(s_(220.f), (content_height - gaps) / ImMax(section_count, 1));
}

static bool slider_int(const std::string& name, const std::string& desc, int* value, float vmin, float vmax, const std::string& fmt = "%.1f")
{
	float fval = static_cast<float>(*value);
	if (widgets->slider(name, desc, &fval, vmin, vmax, fmt))
	{
		*value = static_cast<int>(fval);
		return true;
	}
	return false;
}

static void draw_visual_sidebar_glass(const c_rect& bounds)
{
	c_draw_list* dl = gui->background_drawlist();
	const float sb_rnd = s_(12);

	{
		ImVec4 top = clr->child.Value; top.w = 1.0f;
		ImVec4 bot = clr->child.Value; bot.w = 1.0f;
		bot.x *= 0.80f; bot.y *= 0.80f; bot.z *= 0.80f;
		draw->fade_rect_filled(dl, bounds.Min, bounds.Max,
			draw->get_clr(top), draw->get_clr(bot),
			fade_direction::vertically, sb_rnd);
	}

	draw->rect_filled(dl, bounds.Min - s_(10, 10), bounds.Max + s_(10, 10),
		draw->get_clr(clr->accent, 0.04f), sb_rnd + s_(10));
	draw->rect_filled(dl, bounds.Min - s_(6, 6), bounds.Max + s_(6, 6),
		draw->get_clr(clr->accent, 0.10f), sb_rnd + s_(6));
	draw->rect_filled(dl, bounds.Min - s_(2, 2), bounds.Max + s_(2, 2),
		draw->get_clr(clr->accent, 0.18f), sb_rnd + s_(2));
}

static void begin_visual_section(const std::string& name, float height)
{
	gui->begin_def_child(name, c_vec2(elements->child_width, height), child_flags_none, window_flags_no_bring_to_front_on_focus | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_vertical_scrollbar);
	c_window* window = gui->get_window();
	if (window->SkipItems)
		return;

	const float sec_rnd = s_(12);

	{
		ImVec4 top = clr->child.Value; top.w = 1.0f;
		ImVec4 bot = clr->child.Value; bot.w = 1.0f;
		bot.x *= 0.80f; bot.y *= 0.80f; bot.z *= 0.80f;
		draw->fade_rect_filled(window->DrawList, window->Pos, window->Pos + window->Size,
			draw->get_clr(top), draw->get_clr(bot),
			fade_direction::vertically, sec_rnd);
	}

	draw->rect_filled(window->DrawList, window->Pos - s_(10, 10), window->Pos + window->Size + s_(10, 10),
		draw->get_clr(clr->accent, 0.04f), sec_rnd + s_(10));
	draw->rect_filled(window->DrawList, window->Pos - s_(6, 6), window->Pos + window->Size + s_(6, 6),
		draw->get_clr(clr->accent, 0.10f), sec_rnd + s_(6));
	draw->rect_filled(window->DrawList, window->Pos - s_(2, 2), window->Pos + window->Size + s_(2, 2),
		draw->get_clr(clr->accent, 0.18f), sec_rnd + s_(2));
	const float edge_inset = s_(12);
	const float edge_y = s_(1);
	draw->line(window->DrawList,
		window->Pos + c_vec2(edge_inset, edge_y),
		window->Pos + c_vec2(window->Size.x - edge_inset, edge_y),
		draw->get_clr(clr->white, 0.06f));
	draw->line(window->DrawList,
		window->Pos + c_vec2(edge_inset, window->Size.y - edge_y),
		window->Pos + c_vec2(window->Size.x - edge_inset, window->Size.y - edge_y),
		draw->get_clr(clr->black, 0.10f));
	ImFont* title_font = font->get(inter_semibold, 12);
	const float title_scroll_pct = window->ScrollMax.y > 0.f
		? ImClamp(window->Scroll.y / window->ScrollMax.y * 1.5f, 0.f, 1.f)
		: 0.f;
	draw->text(window->DrawList, title_font, title_font->FontSize,
		window->Pos + s_(10, 8), draw->get_clr(clr->text, 1.f - title_scroll_pct), name.data());
	window->DC.CursorPos = window->Pos + s_(22, 24) - window->Scroll;
	window->DC.CursorStartPos = window->DC.CursorPos;
	window->DC.Indent.x = s_(22);
}

static void end_visual_section()
{
	gui->end_def_child();
}

static void set_gui_scale_percent(float scale)
{
	const int next_dpi = static_cast<int>(ImClamp(scale, 100.f, 200.f) + 0.5f);
	if (next_dpi == var->gui.stored_dpi)
		return;

	var->gui.stored_dpi = next_dpi;
	var->gui.dpi_changed = true;
}

static void handle_gui_scale_wheel()
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl || io.MouseWheel == 0.f)
		return;

	set_gui_scale_percent(static_cast<float>(var->gui.stored_dpi) + io.MouseWheel * 5.f);
}

// Persistent keybind_t objects synced with SettingsStore
static keybind_t g_aimbot_key{ 0, false, false, false, false, keybind_mode_hold };
static keybind_t g_aimbot_fov_key{ 0, false, false, false, false, keybind_mode_toggle };
static keybind_t g_silent_key{ 0, false, false, false, false, keybind_mode_hold };
static keybind_t g_silent_fov_key{ 0, false, false, false, false, keybind_mode_toggle };
static keybind_t g_triggerbot_key{ 0, false, false, false, false, keybind_mode_hold };
static keybind_t g_visuals_key{ 0, false, false, false, false, keybind_mode_toggle };
static keybind_t g_misc_speed_key{ 0, false, false, false, false, keybind_mode_toggle };
static keybind_t g_misc_jump_key{ 0, false, false, false, false, keybind_mode_toggle };
static keybind_t g_menu_key{ (int)ImGuiKey_Insert, false, false, false, false, keybind_mode_toggle };

static void sync_keybind_from_store(keybind_t& kb, ImGuiKey store_key, ImKeyBindMode store_mode)
{
	if (!kb.capturing)
	{
		kb.key = (int)store_key;
		// Convert ImKeyBindMode → keybind_mode
		if (store_mode == ImKeyBindMode_Toggle)
			kb.mode = keybind_mode_toggle;
		else
			kb.mode = keybind_mode_hold;
	}
}

static void sync_keybind_to_store(const keybind_t& kb, ImGuiKey& store_key, ImKeyBindMode& store_mode)
{
	store_key = (ImGuiKey)kb.key;
	// Convert keybind_mode → ImKeyBindMode
	if (kb.mode == keybind_mode_toggle)
		store_mode = ImKeyBindMode_Toggle;
	else
		store_mode = ImKeyBindMode_Hold;
}

void c_gui::render()
{
	gui->initialize();
	handle_gui_scale_wheel();
	sync_layout_preferences();

	// Sync keybind wrappers from SettingsStore
	sync_keybind_from_store(g_aimbot_key, SettingsStore::Aimbot_Key, SettingsStore::Aimbot_Mode);
	sync_keybind_from_store(g_aimbot_fov_key, SettingsStore::Aimbot_FovToggleKey, SettingsStore::Aimbot_FovToggleMode);
	sync_keybind_from_store(g_silent_key, SettingsStore::Silent_Key, SettingsStore::Silent_Mode);
	sync_keybind_from_store(g_silent_fov_key, SettingsStore::Silent_FovToggleKey, SettingsStore::Silent_FovToggleMode);
	sync_keybind_from_store(g_triggerbot_key, SettingsStore::Triggerbot_Key, SettingsStore::Triggerbot_Mode);
	sync_keybind_from_store(g_visuals_key, SettingsStore::Visuals_ToggleKey, SettingsStore::Visuals_ToggleMode);
	sync_keybind_from_store(g_misc_speed_key, SettingsStore::Misc_Speed_Key, SettingsStore::Misc_Speed_Key_Mode);
	sync_keybind_from_store(g_misc_jump_key, SettingsStore::Misc_Jump_Key, SettingsStore::Misc_Jump_Key_Mode);
	// Menu key — sync key only, always toggle
	if (!g_menu_key.capturing) g_menu_key.key = (int)SettingsStore::Settings_Menu_Keybind;

	gui->easing(var->gui.tab_alpha, var->gui.tab != var->gui.tab_stored ? 0.f : 1.f, 7.f, static_easing);
	if (var->gui.tab_alpha == 0)
		var->gui.tab = var->gui.tab_stored;

	c_vec2 size = c_vec2(0, 0);

	if (!var->gui.menu_open_target)
	{
		size.x = s_(1);
		size.y = s_(1);
	}
	else if (var->gui.tab_stored == 0)
	{
		size.x = s_(380);
		size.y = s_(236);
	}
	else
	{
		size.x = s_(visual_window_width);
		size.y = s_(visual_window_height);
	}

	gui->easing(elements->window.size.x, size.x, 24.f, dynamic_easing);
	gui->easing(elements->window.size.y, size.y, 24.f, dynamic_easing);

	gui->easing(var->gui.menu_open_alpha, var->gui.menu_open_target ? 1.f : 0.f, 12.f, dynamic_easing);

	gui->set_next_window_size(elements->window.size);
	gui->set_next_window_pos(c_vec2(0, 0), gui_cond_first_use_ever);

	gui->begin(elements->window.name, nullptr, window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_bring_to_front_on_focus | window_flags_no_focus_on_appearing | window_flags_no_background | window_flags_no_decoration);
	{
		gui->set_style();
		gui->draw_decorations();

		// Simplified login: strict_license=false so skip directly to tab 1
		if (var->gui.tab == 0 && !var->gui.strict_license)
		{
			var->gui.tab = 1;
			var->gui.tab_stored = 1;
		}

		if (var->gui.tab == 0)
		{
			gui->set_pos(c_vec2(0, 0), pos_all);
			gui->begin_content("Login", size, s_(14, 14), s_(0, 0), 0, child_flags_none);
			{
				c_window* login_window = gui->get_window();
				draw->rect_filled(login_window->DrawList, login_window->Rect().Min, login_window->Rect().Max, draw->get_clr(clr->child), s_(12));
				widgets->brand_header("Bitware", "License access");
				gui->dummy(c_vec2(0, s_(30)));
				widgets->primary_button("Enter", "enter");
			}
			gui->end_content();
			gui->end();
			return;
		}

		// Reset sub-tab when tab changes
		if (var->gui.tab != var->gui.tab_stored)
			var->gui.sub_tab_stored = 1;

		const float top_row_y = visual_outer_padding;
		const float top_row_height = 50.f;
		const float top_row_gap = 2.f;
		const float bottom_row_y = top_row_y + top_row_height + top_row_gap;
		const float bottom_row_height = visual_window_height - bottom_row_y - visual_outer_padding;
		const float sidebar_tabs_y = top_row_y + top_row_height;
		const float sidebar_tabs_height = visual_window_height - sidebar_tabs_y - visual_outer_padding;

		{
			c_window* w = gui->get_window();
			const c_vec2 wp = w->Pos;
			draw_visual_sidebar_glass(c_rect(wp + c_vec2(s_(visual_outer_padding), s_(top_row_y)), wp + c_vec2(s_(visual_outer_padding + visual_sidebar_width), elements->window.size.y - s_(visual_outer_padding))));
		}

		gui->set_pos(c_vec2(s_(visual_outer_padding), s_(top_row_y)), pos_all);
		gui->begin_content("TopBrand", c_vec2(s_(visual_sidebar_width), s_(top_row_height)), s_(0, 0), s_(0, 0), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
		{
			widgets->brand_header("Bitware", var->gui.profile_name[0] != '\0' ? var->gui.profile_name : "Control panel");
		}
		gui->end_content();

		gui->set_pos(c_vec2(s_(visual_outer_padding), s_(sidebar_tabs_y)), pos_all);
		gui->begin_content("Tabs", c_vec2(s_(visual_sidebar_width), s_(sidebar_tabs_height)), s_(10, 10), s_(0, 4), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
		{
			var->gui.sidebar_glass = true;

			{
				c_window* tabs_inner_bg = gui->get_window();
				static c_vec4 sidebar_overlay = c_vec4(0, 0, 0, 0);
				sidebar_overlay.x = g_sidebar_selected_rect.x;
				sidebar_overlay.z = g_sidebar_selected_rect.z;
				gui->easing(sidebar_overlay.y, g_sidebar_selected_rect.y, 18.f, dynamic_easing);
				gui->easing(sidebar_overlay.w, g_sidebar_selected_rect.w, 18.f, dynamic_easing);
				if (sidebar_overlay.z > sidebar_overlay.x + 1.f)
				{
					draw->rect_filled(tabs_inner_bg->DrawList,
						c_vec2(sidebar_overlay.x, sidebar_overlay.y), c_vec2(sidebar_overlay.z, sidebar_overlay.w),
						draw->get_clr(clr->widget, 0.40f), s_(9.1f));
				}
			}

			widgets->tab_button("Aimbot", "aim", 1);
			widgets->tab_button("Visuals", "visuals", 2);
			widgets->tab_button("Misc", "movement", 3);
			widgets->tab_button("Players", "squad", 4);
			widgets->tab_button("Configs", "loadout", 5);
			widgets->tab_button("About", "polish", 6);

			{
				static c_vec4 sidebar_indicator = c_vec4(0, 0, 0, 0);
				sidebar_indicator.x = g_sidebar_selected_rect.x;
				sidebar_indicator.z = g_sidebar_selected_rect.z;
				gui->easing(sidebar_indicator.y, g_sidebar_selected_rect.y, 18.f, dynamic_easing);
				gui->easing(sidebar_indicator.w, g_sidebar_selected_rect.w, 18.f, dynamic_easing);
				if (sidebar_indicator.w > sidebar_indicator.y + 1.f)
				{
					c_window* tabs_inner = gui->get_window();
					const float bar_w = s_(2);
					const float bar_inset_y = s_(6);
					const float bar_x = sidebar_indicator.x + s_(2);
					const c_vec2 bar_min = c_vec2(bar_x, sidebar_indicator.y + bar_inset_y);
					const c_vec2 bar_max = c_vec2(bar_x + bar_w, sidebar_indicator.w - bar_inset_y);
					draw->shadow_rect(tabs_inner->DrawList, bar_min - s_(2, 4), bar_max + s_(2, 4),
						draw->get_clr(clr->accent, 0.25f), s_(10), c_vec2(0, 0), 0, s_(2));
					draw->rect_filled(tabs_inner->DrawList, bar_min, bar_max,
						draw->get_clr(clr->accent), bar_w * 0.5f);
				}
			}

			gui->easing(elements->tab_window_width, gui->get_window()->Size.x, 24.f, dynamic_easing);
			var->gui.sidebar_glass = false;
		}
		gui->end_content();
		
		gui->push_var(style_var_alpha, var->gui.menu_open_alpha * var->gui.tab_alpha);

		if (var->gui.tab != 0)
		{
			const float feature_x = visual_outer_padding + visual_sidebar_width + visual_outer_padding;
			const float feature_width = visual_window_width - feature_x - visual_outer_padding;
			const float feature_header_height = top_row_height * 0.8f;
			const float feature_header_y = top_row_y;

			gui->set_pos(c_vec2(s_(feature_x), s_(feature_header_y) + s_(10) * (1.f - var->gui.tab_alpha)), pos_all);
			{
				const c_vec2 pill_pos = gui->get_window()->DC.CursorPos;
				draw->rect_filled(gui->get_window()->DrawList, pill_pos, pill_pos + c_vec2(s_(feature_width), s_(feature_header_height)), draw->get_clr(clr->child), s_(14));

				gui->begin_content("FeatureHeader", c_vec2(s_(feature_width), s_(feature_header_height)), s_(8, 6), s_(8, 0), window_flags_no_scrollbar | window_flags_no_background, child_flags_none);
				{
					c_window* hdr_win = gui->get_window();

				static c_vec4 pill_overlay = c_vec4(0, 0, 0, 0);
				const bool has_subtabs = (var->gui.tab == 1 || var->gui.tab == 2 || var->gui.tab == 4);

				if (has_subtabs)
				{
					pill_overlay.y = g_pill_selected_rect.y;
					pill_overlay.w = g_pill_selected_rect.w;
					gui->easing(pill_overlay.x, g_pill_selected_rect.x, 18.f, dynamic_easing);
					gui->easing(pill_overlay.z, g_pill_selected_rect.z, 18.f, dynamic_easing);
					if (pill_overlay.z > pill_overlay.x + 1.f)
					{
						draw->rect_filled(hdr_win->DrawList,
							c_vec2(pill_overlay.x, pill_overlay.y), c_vec2(pill_overlay.z, pill_overlay.w),
							draw->get_clr(clr->widget), s_(9.1f));
					}
				}
				else
				{
					g_pill_selected_rect = c_vec4(0, 0, 0, 0);
					pill_overlay = c_vec4(0, 0, 0, 0);
				}

				ImFont* tab_font = font->get(inter_semibold, 11);
				const float tab_gap = s_(8.f);
				const float search_height = s_(28);
				const float avail_width = gui->content_avail().x;

				float subtabs_total_width = 0.f;
				if (has_subtabs)
				{
					if (var->gui.tab == 1)
					{
						const float a_width = s_(42.f) + gui->text_size(tab_font, "Aimbot").x;
						const float s_width = s_(42.f) + gui->text_size(tab_font, "Silent").x;
						const float t_width = s_(42.f) + gui->text_size(tab_font, "Trigger").x;
						subtabs_total_width = a_width + tab_gap + s_width + tab_gap + t_width;
					}
					else if (var->gui.tab == 2)
					{
						const float e_width = s_(42.f) + gui->text_size(tab_font, "ESP").x;
						const float c_width = s_(42.f) + gui->text_size(tab_font, "Chams").x;
						const float w_width = s_(42.f) + gui->text_size(tab_font, "World").x;
						subtabs_total_width = e_width + tab_gap + c_width + tab_gap + w_width;
					}
					else if (var->gui.tab == 4)
					{
						const float l_width = s_(42.f) + gui->text_size(tab_font, "List").x;
						const float w_width = s_(42.f) + gui->text_size(tab_font, "Whitelist").x;
						subtabs_total_width = l_width + tab_gap + w_width;
					}
				}

				const float search_width = avail_width - subtabs_total_width - (has_subtabs ? tab_gap : 0.f);
				widgets->search_field("Search...", var->gui.feature_search, sizeof(var->gui.feature_search), c_vec2(search_width, search_height));

				if (has_subtabs)
				{
					gui->sameline();

					if (var->gui.tab == 1) // Aimbot sub-tabs
					{
						const float a_width = s_(42.f) + gui->text_size(tab_font, "Aimbot").x;
						const float s_width = s_(42.f) + gui->text_size(tab_font, "Silent").x;
						const float t_width = s_(42.f) + gui->text_size(tab_font, "Trigger").x;
						widgets->sub_tab_button("Aimbot", "aim", 1, a_width);
						gui->sameline(0.f, tab_gap);
						widgets->sub_tab_button("Silent", "target", 2, s_width);
						gui->sameline(0.f, tab_gap);
						widgets->sub_tab_button("Trigger", "match", 3, t_width);
					}
					else if (var->gui.tab == 2) // Visuals sub-tabs
					{
						const float e_width = s_(42.f) + gui->text_size(tab_font, "ESP").x;
						const float c_width = s_(42.f) + gui->text_size(tab_font, "Chams").x;
						const float w_width = s_(42.f) + gui->text_size(tab_font, "World").x;
						widgets->sub_tab_button("ESP", "visuals", 1, e_width);
						gui->sameline(0.f, tab_gap);
						widgets->sub_tab_button("Chams", "shapes", 2, c_width);
						gui->sameline(0.f, tab_gap);
						widgets->sub_tab_button("World", "world", 3, w_width);
					}
					else if (var->gui.tab == 4) // Players sub-tabs
					{
						const float l_width = s_(42.f) + gui->text_size(tab_font, "List").x;
						const float w_width = s_(42.f) + gui->text_size(tab_font, "Whitelist").x;
						widgets->sub_tab_button("List", "squad", 1, l_width);
						gui->sameline(0.f, tab_gap);
						widgets->sub_tab_button("Whitelist", "profile", 2, w_width);
					}
				}
				}
				gui->end_content();
			}

			gui->set_pos(c_vec2(s_(feature_x), s_(bottom_row_y) + s_(10) * (1.f - var->gui.tab_alpha)), pos_all);
			gui->begin_content("Features", c_vec2(s_(feature_width), s_(bottom_row_height)),
				s_(visual_feature_padding_x, visual_feature_padding_y),
				c_vec2(s_(visual_column_gap), 0.f), window_flags_no_scroll_with_mouse);
			{
				const visual_panel_density_state density_state = begin_visual_panel_density();

				gui->easing(elements->child_width, (gui->content_avail().x - s_(visual_column_gap)) / 2, 24.f, dynamic_easing);
				const float half_section_height = visual_section_height(1);

				// ── TAB 1: AIMBOT ──
				if (var->gui.tab == 1)
				{
					if (var->gui.sub_tab_stored == 1) // Aimbot
					{
						gui->begin_group();
						{
							begin_visual_section("Aimbot", half_section_height);
							widgets->checkbox("Enabled", "Master aimbot switch", &SettingsStore::Aimbot_Enabled);
							widgets->dropdown("Type", "Mouse or Camera", &SettingsStore::Aimbot_type, { "Mouse", "Camera" });
							widgets->keybind("Activation key", "Key to activate aimbot", &g_aimbot_key);
							widgets->dropdown("Hitbox", "Target hitbox", &SettingsStore::Aimbot_HitPart, { "Head", "Torso", "Lower Torso", "Random" });
							widgets->checkbox("Sticky aim", "Persist target lock", &SettingsStore::Aimbot_AimbotSticky);
							widgets->checkbox("Wall check", "Require line of sight", &SettingsStore::Aimbot_WallCheck);
							widgets->checkbox("Knocked check", "Ignore knocked players", &SettingsStore::Aimbot_KnockedCheck);
							widgets->checkbox("Shake", "Add aim perturbation", &SettingsStore::Aimbot_Shake);
							if (SettingsStore::Aimbot_Shake)
							{
								widgets->slider("Shake X", "", &SettingsStore::Aimbot_ShakeX, 0.f, 10.f);
								widgets->slider("Shake Y", "", &SettingsStore::Aimbot_ShakeY, 0.f, 10.f);
								widgets->slider("Shake Z", "", &SettingsStore::Aimbot_ShakeZ, 0.f, 10.f);
							}
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("Smoothing & FOV", half_section_height);
							if (SettingsStore::Aimbot_type == 0) // Mouse
							{
								widgets->slider("Mouse smooth X", "", &SettingsStore::Aimbot_Mouse_SmoothingX, 0.f, 12.f);
								widgets->slider("Mouse smooth Y", "", &SettingsStore::Aimbot_Mouse_SmoothingY, 0.f, 12.f);
								widgets->slider("Mouse sensitivity", "", &SettingsStore::Aimbot_Mouse_Sensitivity, 0.f, 5.f);
							}
							else // Camera
							{
								widgets->slider("Camera smooth X", "", &SettingsStore::Aimbot_Camera_SmoothingX, 0.f, 12.f);
								widgets->slider("Camera smooth Y", "", &SettingsStore::Aimbot_Camera_SmoothingY, 0.f, 12.f);
							}
							widgets->checkbox("Use FOV", "Clamp aim to FOV circle", &SettingsStore::Aimbot_useFov);
							if (SettingsStore::Aimbot_useFov)
							{
								widgets->slider("FOV size", "", &SettingsStore::Aimbot_FovSize, 1.f, 360.f, "%.0f");
								widgets->checkbox("Draw FOV", "Show FOV circle", &SettingsStore::Aimbot_DrawFov);
								widgets->checkbox("Fill FOV", "Fill FOV circle", &SettingsStore::Aimbot_FillFov);
								widgets->keybind("FOV toggle key", "Key to toggle FOV circle", &g_aimbot_fov_key);
								widgets->checkbox("FOV spin", "Rotate FOV ring", &SettingsStore::Aimbot_FovSpin);
								if (SettingsStore::Aimbot_FovSpin)
								{
									widgets->dropdown("Spin dir", "", &SettingsStore::Aimbot_FovSpinDirection, { "CW", "CCW" });
									slider_int("Spin speed", "", &SettingsStore::Aimbot_FovSpinSpeed, 1.f, 5.f, "%.0f");
								}
								widgets->color_picker("FOV color", (c_vec4*)SettingsStore::Aimbot_FovColor);
							}
							end_visual_section();
						}
						gui->end_group();
					}
					else if (var->gui.sub_tab_stored == 2) // Silent Aim
					{
						gui->begin_group();
						{
							begin_visual_section("Silent Aim", half_section_height);
							widgets->checkbox("Enabled", "Master silent aim switch", &SettingsStore::Silent_Enabled);
							widgets->dropdown("Type", "", &SettingsStore::Silent_type, { "Viewport" });
							widgets->dropdown("Hitbox", "Target hitbox", &SettingsStore::Silent_HitPart, { "Head", "Torso", "Lower Torso" });
							widgets->checkbox("Team check", "Ignore teammates", &SettingsStore::Silent_TeamCheck);
							widgets->checkbox("Wall check", "Require line of sight", &SettingsStore::Silent_WallCheck);
							widgets->checkbox("Knocked check", "Ignore knocked players", &SettingsStore::Silent_KnockedCheck);
							widgets->keybind("Activation key", "Key to activate silent aim", &g_silent_key);
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("FOV & Prediction", half_section_height);
							widgets->checkbox("Use FOV", "Clamp silent to FOV", &SettingsStore::Silent_useFov);
							if (SettingsStore::Silent_useFov)
							{
								widgets->slider("FOV size", "", &SettingsStore::Silent_FovSize, 1.f, 360.f, "%.0f");
								widgets->checkbox("Draw FOV", "", &SettingsStore::Silent_DrawFov);
								widgets->checkbox("Fill FOV", "", &SettingsStore::Silent_FillFov);
								widgets->keybind("FOV toggle key", "Key to toggle FOV circle", &g_silent_fov_key);
								widgets->checkbox("FOV spin", "Rotate FOV ring", &SettingsStore::Silent_FovSpin);
								if (SettingsStore::Silent_FovSpin)
								{
									widgets->dropdown("Spin dir", "", &SettingsStore::Silent_FovSpinDirection, { "CW", "CCW" });
									slider_int("Spin speed", "", &SettingsStore::Silent_FovSpinSpeed, 1.f, 5.f, "%.0f");
								}
								widgets->color_picker("FOV color", (c_vec4*)SettingsStore::Silent_FovColor);
							}
							widgets->checkbox("Prediction", "Velocity-based prediction", &SettingsStore::Silent_Prediction_Enabled);
							if (SettingsStore::Silent_Prediction_Enabled)
								widgets->slider("Prediction value", "", &SettingsStore::Silent_Prediction_Value, 0.f, 1.f);
							end_visual_section();
						}
						gui->end_group();
					}
					else if (var->gui.sub_tab_stored == 3) // Triggerbot
					{
						gui->begin_group();
						{
							begin_visual_section("Triggerbot", half_section_height);
							widgets->checkbox("Enabled", "Master triggerbot switch", &SettingsStore::Triggerbot_Enabled);
							widgets->dropdown("Hitbox", "Target hitbox", &SettingsStore::Triggerbot_HitPart, { "Head", "Torso", "Lower Torso", "All" });
							widgets->dropdown("Fire mode", "Tap or full auto", &SettingsStore::Triggerbot_FireMode, { "Tap", "Full Auto" });
							slider_int("Delay (ms)", "Delay before firing", &SettingsStore::Triggerbot_Delay, 0.f, 500.f, "%.0f");
							slider_int("Randomize (ms)", "Random delay variation", &SettingsStore::Triggerbot_Randomize, 0.f, 250.f, "%.0f");
							widgets->checkbox("Wall check", "Require line of sight", &SettingsStore::Triggerbot_WallCheck);
							widgets->checkbox("Knocked check", "Ignore knocked players", &SettingsStore::Triggerbot_KnockedCheck);
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("Keybind", half_section_height);
							widgets->keybind("Activation key", "Key to activate triggerbot", &g_triggerbot_key);
							end_visual_section();
						}
						gui->end_group();
					}
				}

				// ── TAB 2: VISUALS ──
				if (var->gui.tab == 2)
				{
					if (var->gui.sub_tab_stored == 1) // ESP
					{
						gui->begin_group();
						{
							begin_visual_section("ESP", half_section_height);
							widgets->checkbox("Enabled", "Master ESP switch", &SettingsStore::Visuals_Enabled);
							widgets->keybind("Toggle key", "Key to toggle ESP", &g_visuals_key);
							widgets->checkbox("Wall check", "Show occluded color", &SettingsStore::Visuals_WallCheck);
							widgets->color_picker("Occluded color", (c_vec4*)SettingsStore::Visuals_OccludedColor);
							widgets->slider("Render distance", "Max distance to render", &SettingsStore::Visuals_Render_Distance, 10.f, 500.f, "%.0f");
							widgets->checkbox("Box", "2D bounding box", &SettingsStore::Visuals_Box);
							if (SettingsStore::Visuals_Box)
							{
								widgets->dropdown("Box type", "", &SettingsStore::Visuals_Box_Type, { "Bounding", "Corner" });
								widgets->checkbox("Box fill", "", &SettingsStore::Visuals_Box_Fill);
								if (SettingsStore::Visuals_Box_Fill)
								{
									widgets->checkbox("Gradient", "", &SettingsStore::Visuals_Box_Fill_Gradient);
									if (SettingsStore::Visuals_Box_Fill_Gradient)
									{
										widgets->checkbox("Rotate", "", &SettingsStore::Visuals_Box_Fill_Gradient_Rotate);
										widgets->dropdown("Fill type", "", &SettingsStore::Visuals_Box_Fill_Type, { "Side", "Bottom", "Spin" });
										slider_int("Speed", "", &SettingsStore::Visuals_BoxFillSpeed, 1.f, 5.f, "%.0f");
									}
									widgets->color_picker("Fill top", (c_vec4*)SettingsStore::Visuals_BoxFillTop);
									widgets->color_picker("Fill bottom", (c_vec4*)SettingsStore::Visuals_BoxFillBottom);
								}
								widgets->color_picker("Box color", (c_vec4*)SettingsStore::Visuals_BoxColor);
							}
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("Player Info", half_section_height);
							widgets->checkbox("Healthbar", "Vertical health bar", &SettingsStore::Visuals_Healthbar);
							if (SettingsStore::Visuals_Healthbar)
							{
								widgets->dropdown("Healthbar type", "", &SettingsStore::Visuals_Healthbar_Type, { "Static", "Gradient" });
								slider_int("Gap", "Bar distance from box", &SettingsStore::Visuals_Gap, 1.f, 5.f, "%.0f");
								slider_int("Thickness", "Bar width", &SettingsStore::Visuals_Thickness, 1.f, 5.f, "%.0f");
								widgets->color_picker("Healthbar color", (c_vec4*)SettingsStore::Visuals_HealthbarColor);
								widgets->color_picker("Healthbar top", (c_vec4*)SettingsStore::Visuals_HealthbarTop);
								widgets->color_picker("Healthbar mid", (c_vec4*)SettingsStore::Visuals_HealthbarMiddle);
								widgets->color_picker("Healthbar bot", (c_vec4*)SettingsStore::Visuals_HealthbarBottom);
							}
							widgets->checkbox("Health text", "Show HP value", &SettingsStore::Visuals_Health);
							if (SettingsStore::Visuals_Health)
								widgets->color_picker("Health color", (c_vec4*)SettingsStore::Visuals_HealthColor);
							widgets->checkbox("Name", "Show player name", &SettingsStore::Visuals_Name);
							if (SettingsStore::Visuals_Name)
							{
								widgets->dropdown("Name type", "", &SettingsStore::Visuals_Name_Type, { "Name", "DisplayName", "Both" });
								widgets->color_picker("Name color", (c_vec4*)SettingsStore::Visuals_NameColor);
							}
							widgets->checkbox("Distance", "Show distance", &SettingsStore::Visuals_Distance);
							if (SettingsStore::Visuals_Distance)
								widgets->color_picker("Distance color", (c_vec4*)SettingsStore::Visuals_DistanceColor);
							widgets->checkbox("Rig type", "Show R6/R15 tag", &SettingsStore::Visuals_Rig_Type);
							if (SettingsStore::Visuals_Rig_Type)
								widgets->color_picker("Rig color", (c_vec4*)SettingsStore::Visuals_RigTypeColor);
							widgets->checkbox("Tool", "Show equipped tool", &SettingsStore::Visuals_Tool);
							if (SettingsStore::Visuals_Tool)
								widgets->color_picker("Tool color", (c_vec4*)SettingsStore::Visuals_ToolColor);
							widgets->checkbox("Skeleton", "Show bone chain", &SettingsStore::Visuals_Skeleton);
							if (SettingsStore::Visuals_Skeleton)
								widgets->color_picker("Skeleton color", (c_vec4*)SettingsStore::Visuals_SkeletonColor);
							end_visual_section();
						}
						gui->end_group();
					}
					else if (var->gui.sub_tab_stored == 2) // Chams
					{
						gui->begin_group();
						{
							begin_visual_section("Chams", half_section_height);
							widgets->checkbox("Enabled", "Master chams switch", &SettingsStore::Visuals_Chams);
							widgets->dropdown("Type", "", &SettingsStore::Visuals_Chams_Type, { "GPU Mesh", "Cube", "Highlight" });
							widgets->checkbox("Fade", "Pulsing alpha effect", &SettingsStore::Visuals_ChamsFade);
							if (SettingsStore::Visuals_ChamsFade)
								slider_int("Fade speed", "", &SettingsStore::Visuals_ChamsFadeSpeed, 1.f, 5.f, "%.0f");
							widgets->color_picker("Chams color", (c_vec4*)SettingsStore::Visuals_ChamsColor);
							widgets->color_picker("Outline color", (c_vec4*)SettingsStore::Visuals_ChamsOutline);
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("GPU Mesh", half_section_height);
							widgets->checkbox("Fill", "Fill mesh body", &SettingsStore::Visuals_Chams_Fill_Enabled);
							if (SettingsStore::Visuals_Chams_Fill_Enabled)
								widgets->slider("Fill alpha", "", &SettingsStore::Visuals_Chams_Fill_Transparency, 0.f, 1.f);
							widgets->checkbox("Outline", "Outline mesh body", &SettingsStore::Visuals_Chams_Outline_Enabled);
							if (SettingsStore::Visuals_Chams_Outline_Enabled)
								widgets->slider("Outline thickness", "", &SettingsStore::Visuals_Chams_Outline_Thickness, 0.f, 3.f);
							widgets->checkbox("Occlusion", "Separate visible/occluded", &SettingsStore::Visuals_Chams_Occlusion);
							if (SettingsStore::Visuals_Chams_Occlusion)
							{
								widgets->color_picker("Visible color", (c_vec4*)SettingsStore::Visuals_Chams_Visible_Color);
								widgets->color_picker("Occluded color", (c_vec4*)SettingsStore::Visuals_Chams_Occluded_Color);
							}
							widgets->dropdown("Quality", "", &SettingsStore::Visuals_Chams_Quality, { "Low", "Medium", "High" });
							widgets->checkbox("Use shaders", "Enable shader effects", &SettingsStore::Visuals_Chams_Use_Shaders);
							if (SettingsStore::Visuals_Chams_Use_Shaders)
							{
								widgets->dropdown("Shader", "", &SettingsStore::Visuals_Chams_Shader, {
									"Default", "Metallic", "Dissolve", "Wireframe", "Grayscale",
									"Caustic", "Chrome", "Liquid", "Hologram", "Sliced",
									"Rainbow", "Glass", "Lava", "Glitch", "Ice",
									"Neon Pulse", "Gold", "Depth Fog", "Splatter", "Rim Light",
									"Wood", "Plastic" });
								int shader = SettingsStore::Visuals_Chams_Shader;
								if (shader == 0) {}
								else if (shader == 1) widgets->slider("Roughness", "", &SettingsStore::Visuals_Chams_Metallic_Roughness, 0.f, 1.f);
								else if (shader == 2) { widgets->slider("Dissolve", "", &SettingsStore::Visuals_Chams_Dissolve_Amount, 0.f, 1.f); widgets->slider("Edge", "", &SettingsStore::Visuals_Chams_Dissolve_Edge, 0.f, 0.2f); }
								else if (shader == 5) { widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Caustic_Speed, 0.f, 5.f); widgets->slider("Scale", "", &SettingsStore::Visuals_Chams_Caustic_Scale, 0.f, 5.f); }
								else if (shader == 6) widgets->slider("Sharpness", "", &SettingsStore::Visuals_Chams_Chrome_Sharpness, 0.f, 5.f);
								else if (shader == 7) { widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Liquid_Speed, 0.f, 5.f); widgets->slider("Wave", "", &SettingsStore::Visuals_Chams_Liquid_Wave, 0.f, 1.f); }
								else if (shader == 8) { widgets->slider("Scan speed", "", &SettingsStore::Visuals_Chams_Hologram_Scan_Speed, 0.f, 5.f); widgets->slider("Opacity", "", &SettingsStore::Visuals_Chams_Hologram_Opacity, 0.f, 1.f); }
								else if (shader == 9) { widgets->slider("Gap", "", &SettingsStore::Visuals_Chams_Sliced_Gap, 0.f, 1.f); widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Sliced_Speed, 0.f, 5.f); }
								else if (shader == 10) widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Cycle_Speed, 0.f, 5.f);
								else if (shader == 12) { widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Lava_Speed, 0.f, 5.f); widgets->slider("Scale", "", &SettingsStore::Visuals_Chams_Lava_Scale, 0.f, 5.f); }
								else if (shader == 13) { widgets->slider("Intensity", "", &SettingsStore::Visuals_Chams_Glitch_Intensity, 0.f, 1.f); widgets->slider("Speed", "", &SettingsStore::Visuals_Chams_Glitch_Speed, 0.f, 5.f); }
								else if (shader == 14) widgets->slider("Roughness", "", &SettingsStore::Visuals_Chams_Ice_Roughness, 0.f, 1.f);
								else if (shader == 15) { widgets->slider("Pulse speed", "", &SettingsStore::Visuals_Chams_Neon_Pulse_Speed, 0.f, 5.f); widgets->slider("Pulse width", "", &SettingsStore::Visuals_Chams_Neon_Pulse_Width, 0.f, 1.f); }
								else if (shader == 17) widgets->slider("Fog density", "", &SettingsStore::Visuals_Chams_Depth_Fog_Density, 0.f, 5.f);
								else if (shader == 18) widgets->slider("Scale", "", &SettingsStore::Visuals_Chams_Splatter_Scale, 0.f, 5.f);
								else if (shader == 19) widgets->slider("Rim power", "", &SettingsStore::Visuals_Chams_Rim_Power, 0.f, 5.f);
								else if (shader == 20) widgets->slider("Scale", "", &SettingsStore::Visuals_Chams_Wood_Scale, 0.f, 5.f);
								else if (shader == 21) widgets->slider("Shininess", "", &SettingsStore::Visuals_Chams_Plastic_Shininess, 0.f, 10.f);
							}
							end_visual_section();
						}
						gui->end_group();
					}
					else if (var->gui.sub_tab_stored == 3) // World
					{
						gui->begin_group();
						{
							begin_visual_section("Skybox", half_section_height);
							widgets->checkbox("Skybox", "Enable skybox changer", &SettingsStore::World_Skybox);
							if (SettingsStore::World_Skybox)
							{
								widgets->dropdown("Skybox type", "", &SettingsStore::World_Skybox_Type, {
									"Bitware.fun", "Space", "Pink Sky", "Minecraft", "Night Cloudy",
									"Sparkling Night", "Winterness", "Dark Crimson", "Nebula", "Tropical", "Green Sky" });
								widgets->checkbox("Rotation", "Rotate skybox", &SettingsStore::World_Rotate);
								if (SettingsStore::World_Rotate)
									widgets->slider("Rotate speed", "", &SettingsStore::World_Rotate_Speed, 0.f, 5.f);
							}
							widgets->checkbox("Atmosphere", "Set ambient color", &SettingsStore::World_Ambience);
							if (SettingsStore::World_Ambience)
								widgets->color_picker("Ambient color", (c_vec4*)SettingsStore::World_AmbienceColor);
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("Environment", half_section_height);
							widgets->checkbox("Fog", "Enable fog control", &SettingsStore::World_Fog);
							if (SettingsStore::World_Fog)
							{
								widgets->slider("Fog distance", "", &SettingsStore::World_Fog_Distance, 0.f, 1000.f, "%.0f");
								widgets->color_picker("Fog color", (c_vec4*)SettingsStore::World_FogColor);
							}
							widgets->checkbox("Brightness", "Override brightness", &SettingsStore::World_Brightness);
							if (SettingsStore::World_Brightness)
								widgets->slider("Brightness", "", &SettingsStore::World_BrightnessI, 0.f, 10.f);
							widgets->checkbox("Exposure", "Override exposure", &SettingsStore::World_Exposure);
							if (SettingsStore::World_Exposure)
								widgets->slider("Exposure", "", &SettingsStore::World_ExposureI, -3.f, 3.f);
							widgets->checkbox("FOV changer", "Override camera FOV", &SettingsStore::World_FOV);
							if (SettingsStore::World_FOV)
								widgets->slider("FOV value", "", &SettingsStore::World_FOV_Distance, 70.f, 120.f, "%.0f");
							end_visual_section();
						}
						gui->end_group();
					}
				}

				// ── TAB 3: MISC ──
				if (var->gui.tab == 3)
				{
					gui->begin_group();
					{
						begin_visual_section("Movement", half_section_height);
						widgets->checkbox("Speedhack", "Override walk speed", &SettingsStore::Misc_Speed);
						if (SettingsStore::Misc_Speed)
						{
							widgets->slider("Speed value", "Walkspeed override", &SettingsStore::Misc_Speed_Value, 10.f, 500.f, "%.0f");
							widgets->keybind("Speed key", "Key to toggle speedhack", &g_misc_speed_key);
						}
						widgets->checkbox("Jump power", "Override jump power", &SettingsStore::Misc_Jump);
						if (SettingsStore::Misc_Jump)
						{
							widgets->slider("Jump value", "JumpPower override", &SettingsStore::Misc_Jump_Value, 10.f, 200.f, "%.0f");
							widgets->keybind("Jump key", "Key to toggle jump power", &g_misc_jump_key);
						}
						end_visual_section();
					}
					gui->end_group();
					gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
					gui->begin_group();
					{
						begin_visual_section("Settings", half_section_height);
						widgets->dropdown("Performance mode", "Frame rate target", &SettingsStore::Settings_Performance_Mode, { "Low (60)", "Medium (144)", "High (240)" });
						widgets->checkbox("Streamproof", "Hide from screen capture", &SettingsStore::Settings_Streamproof);
						widgets->checkbox("VSync", "Enable vertical sync", &SettingsStore::Settings_VSync);
						widgets->checkbox("Team check", "Global team filter", &SettingsStore::Settings_Team_Check);
						widgets->checkbox("Client check", "Exclude local player", &SettingsStore::Settings_Client_Check);
						widgets->dropdown("Wallcheck method", "", &SettingsStore::WallCheck_Method, { "OBB", "Raycast" });
						widgets->keybind("Menu key", "Key to open/close menu", &g_menu_key);
						end_visual_section();
					}
					gui->end_group();
				}

				// ── TAB 4: PLAYERS ──
				if (var->gui.tab == 4)
				{
					if (var->gui.sub_tab_stored == 1) // Player List
					{
						gui->begin_group();
						{
							begin_visual_section("Player List", half_section_height);
							{
								std::shared_ptr<const std::vector<SDK::Player>> snapshot;
								{
									std::lock_guard<std::mutex> lock(Cache::Cache_Mutex);
									snapshot = Globals::Player_Cache;
								}
								if (snapshot && !snapshot->empty())
								{
									std::vector<SDK::Player> sorted = *snapshot;
									std::sort(sorted.begin(), sorted.end(),
										[](const SDK::Player& a, const SDK::Player& b) {
											return a.UserID < b.UserID;
										});
									for (auto& player : sorted)
									{
										bool whitelisted = Globals::Whitelist::UserIDs.count(player.UserID) > 0;
										char label[192];
										snprintf(label, sizeof(label), "%s [%.0fm]", player.Name.c_str(), player.Distance);
										char checkbox_id[192];
										snprintf(checkbox_id, sizeof(checkbox_id), "%s##%llu", label, player.UserID);
										if (widgets->checkbox(checkbox_id, "", &whitelisted))
										{
											if (whitelisted)
												Globals::Whitelist::UserIDs.insert(player.UserID);
											else
												Globals::Whitelist::UserIDs.erase(player.UserID);
										}
									}
								}
								else
								{
									c_window* pw = gui->get_window();
									draw->text(pw->DrawList, font->get(inter_semibold, 11), 11.f,
										pw->DC.CursorPos + s_(4, 4), draw->get_clr(clr->text), "No players found");
								}
							}
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("Whitelist", half_section_height);
							widgets->checkbox("Enabled", "Master whitelist switch", &SettingsStore::Whitelist_Enabled);
							widgets->color_picker("Color", (c_vec4*)SettingsStore::Whitelist_Color);
							{
								char buf[64];
								snprintf(buf, sizeof(buf), "%zu whitelisted user(s)", Globals::Whitelist::UserIDs.size());
								c_window* ww = gui->get_window();
								draw->text(ww->DrawList, font->get(inter_semibold, 11), 11.f,
									ww->DC.CursorPos + s_(4, 4), draw->get_clr(clr->text), buf);
							}
							end_visual_section();
						}
						gui->end_group();
					}
					else if (var->gui.sub_tab_stored == 2) // Whitelist
					{
						gui->begin_group();
						{
							begin_visual_section("Whitelist", half_section_height);
							widgets->checkbox("Enabled", "Master whitelist switch", &SettingsStore::Whitelist_Enabled);
							widgets->color_picker("Color", (c_vec4*)SettingsStore::Whitelist_Color);
							end_visual_section();
						}
						gui->end_group();
						gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
						gui->begin_group();
						{
							begin_visual_section("User IDs", half_section_height);
							{
								char buf[64];
								snprintf(buf, sizeof(buf), "%zu whitelisted user(s)", Globals::Whitelist::UserIDs.size());
								c_window* ww = gui->get_window();
								draw->text(ww->DrawList, font->get(inter_semibold, 11), 11.f,
									ww->DC.CursorPos + s_(4, 4), draw->get_clr(clr->text), buf);
							}
							end_visual_section();
						}
						gui->end_group();
					}
				}

			// ── TAB 5: CONFIGS ──
				if (var->gui.tab == 5)
				{
					// ── LEFT: Config Cards ──
					gui->begin_group();
					{
						begin_visual_section("Configs", half_section_height);
						{
							auto configs = ConfigManager::Get().ListNames();
							c_window* w = gui->get_window();

							if (configs.empty())
							{
								draw->text(w->DrawList, font->get(inter_semibold, 11), 11.f,
									w->DC.CursorPos, draw->get_clr(clr->text), "No saved configs");
							}
							else
							{
								float pad = s_(12);
								float card_h = s_(86);
								float btn_h = s_(34);
								float btn_gap = s_(8);
								float rounding = s_(8);

								for (const auto& name : configs)
								{
									c_vec2 card_pos = w->DC.CursorPos;
									float card_w = gui->content_avail().x - s_(22);

									// Card background
									c_rect card_rect(card_pos, card_pos + c_vec2(card_w, card_h));
									draw->rect_filled(w->DrawList, card_rect.Min, card_rect.Max,
										draw->get_clr(clr->widget), rounding);

									// Config name
									draw->text(w->DrawList, font->get(inter_semibold, 12), 12.f,
										card_pos + c_vec2(pad, pad), draw->get_clr(clr->text), name.c_str());

									// Three buttons at bottom
									float btn_area = card_w - pad * 2;
									float btn_w = (btn_area - btn_gap * 2) / 3;
									float btn_y = card_pos.y + card_h - pad - btn_h;

									const char* labels[] = { "Load", "Save", "Delete" };

									for (int i = 0; i < 3; i++)
									{
										float bx = card_pos.x + pad + (btn_w + btn_gap) * i;
										c_rect btn_rect(bx, btn_y, bx + btn_w, btn_y + btn_h);

										bool hovered = gui->is_rect_visible(btn_rect) && ImGui::IsMouseHoveringRect(btn_rect.Min, btn_rect.Max);
										bool clicked = hovered && ImGui::IsMouseClicked(0);

										c_vec4 btn_col = hovered
											? c_vec4(clr->accent.Value.x * 0.6f + 0.4f,
												clr->accent.Value.y * 0.6f + 0.4f,
												clr->accent.Value.z * 0.6f + 0.4f,
												1.f)
											: c_vec4(0.2f, 0.2f, 0.25f, 1.f);

										draw->rect_filled(w->DrawList, btn_rect.Min, btn_rect.Max,
											draw->get_clr(btn_col), s_(6));
draw->text_clipped(w->DrawList, font->get(inter_semibold, 11),
    btn_rect.Min, btn_rect.Max, draw->get_clr(clr->text),
    labels[i], NULL, NULL, ImVec2(0.5f, 0.5f));

										if (clicked)
										{
											if (i == 0)
											{
												ConfigManager::Get().Load(name.c_str());
												notify->add_notify("Config", ("Loaded: " + name).c_str(), success);
											}
											else if (i == 1)
											{
												ConfigManager::Get().Save(name.c_str());
												notify->add_notify("Config", ("Saved: " + name).c_str(), success);
											}
											else
											{
												ConfigManager::Get().Delete(name.c_str());
												notify->add_notify("Config", ("Deleted: " + name).c_str(), success);
											}
										}
									}

									// Advance cursor to next card
									w->DC.CursorPos = c_vec2(card_pos.x, card_pos.y + card_h + s_(8));
									w->DC.CursorMaxPos = c_vec2(ImMax(w->DC.CursorMaxPos.x, w->DC.CursorPos.x),
										ImMax(w->DC.CursorMaxPos.y, w->DC.CursorPos.y));
								}
							}
						}
						end_visual_section();
					}
					gui->end_group();
					gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
					gui->begin_group();
					{
						begin_visual_section("Actions", half_section_height);
						{
							static char config_name[128] = "";
							c_window* aw = gui->get_window();
							draw->text(aw->DrawList, font->get(inter_semibold, 11), 11.f,
								aw->DC.CursorPos + s_(4, 4), draw->get_clr(clr->text), "Save current settings:");
								aw->DC.CursorPos += c_vec2(0, s_(20));
							const float input_width = elements->child_width - s_(44);
							aw->DC.CursorPos.x = aw->Pos.x + (elements->child_width - input_width) * 0.5f;
							ImGui::SetNextItemWidth(input_width);
							ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, s_(8));
							ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.14f, 0.16f, 1.f));
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.8f, 1.f));
							ImGui::InputText("##config_input", config_name, sizeof(config_name));
							ImGui::PopStyleColor(2);
							ImGui::PopStyleVar();
							aw->DC.CursorPos += c_vec2(0, ImGui::GetItemRectSize().y + s_(8));
							const float btn_w = elements->child_width - s_(88);
							aw->DC.CursorPos.x = aw->Pos.x + (elements->child_width - btn_w) * 0.5f;
							ImGui::PushStyleColor(ImGuiCol_Button, draw->get_clr(clr->accent, 1.f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, draw->get_clr(c_vec4(clr->accent.Value.x * 1.15f, clr->accent.Value.y * 1.15f, clr->accent.Value.z * 1.15f, 1.f), 1.f));
							ImGui::PushStyleColor(ImGuiCol_Text, draw->get_clr(clr->black, 1.f));
							ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, s_(8));
							if (ImGui::Button("Save Config", ImVec2(btn_w, s_(34))))
							{
								if (config_name[0] != '\0')
								{
									ConfigManager::Get().Save(config_name);
									notify->add_notify("Config", ("Saved: " + std::string(config_name)).c_str(), success);
									config_name[0] = '\0';
								}
							}
							ImGui::PopStyleVar();
							ImGui::PopStyleColor(3);
							gui->dummy(c_vec2(0, s_(24)));
							{
								int old_dpi = var->gui.stored_dpi;
								slider_int("UI Size", "Scale the interface 100%–200%", &var->gui.stored_dpi, 100, 200, "%.0f%%");
								if (var->gui.stored_dpi != old_dpi)
									var->gui.dpi_changed = true;
							}
						}
						end_visual_section();
					}
					gui->end_group();
				}

				// ── TAB 6: ABOUT ──
				if (var->gui.tab == 6)
				{
					gui->begin_group();
					{
						begin_visual_section("About", half_section_height);
						{
							c_window* aw = gui->get_window();
							c_vec2 p = aw->DC.CursorPos;
							draw->text(aw->DrawList, font->get(inter_semibold, 14), 14.f,
								p + s_(4, 4), draw->get_clr(clr->text), "Bitware v" BITWARE_VERSION_STRING);
							draw->text(aw->DrawList, font->get(inter_medium, 11), 11.f,
								p + s_(4, 28), draw->get_clr(clr->text), ("Roblox: " + Offsets::ClientVersion).c_str());
							draw->text(aw->DrawList, font->get(inter_medium, 11), 11.f,
								p + s_(4, 44), draw->get_clr(clr->text), "Release | 64-bit");
						}
						end_visual_section();
					}
					gui->end_group();
					gui->sameline(elements->child_width + s_(visual_column_gap), 0.f);
					gui->begin_group();
					{
						begin_visual_section("Credits", half_section_height);
						{
							c_window* cw = gui->get_window();
							c_vec2 p = cw->DC.CursorPos;
							draw->text(cw->DrawList, font->get(inter_semibold, 12), 12.f,
								p + s_(4, 4), draw->get_clr(clr->text), "Developer: Borked");
							draw->text(cw->DrawList, font->get(inter_medium, 11), 11.f,
								p + s_(4, 24), draw->get_clr(clr->text), "Owner: Retro");
						}
						end_visual_section();
					}
					gui->end_group();
				}

				end_visual_panel_density(density_state);
			}
			gui->end_content();
		}
		gui->pop_var();
	}

	// Sync keybind wrappers back to SettingsStore
	sync_keybind_to_store(g_aimbot_key, SettingsStore::Aimbot_Key, SettingsStore::Aimbot_Mode);
	sync_keybind_to_store(g_aimbot_fov_key, SettingsStore::Aimbot_FovToggleKey, SettingsStore::Aimbot_FovToggleMode);
	sync_keybind_to_store(g_silent_key, SettingsStore::Silent_Key, SettingsStore::Silent_Mode);
	sync_keybind_to_store(g_silent_fov_key, SettingsStore::Silent_FovToggleKey, SettingsStore::Silent_FovToggleMode);
	sync_keybind_to_store(g_triggerbot_key, SettingsStore::Triggerbot_Key, SettingsStore::Triggerbot_Mode);
	sync_keybind_to_store(g_visuals_key, SettingsStore::Visuals_ToggleKey, SettingsStore::Visuals_ToggleMode);
	sync_keybind_to_store(g_misc_speed_key, SettingsStore::Misc_Speed_Key, SettingsStore::Misc_Speed_Key_Mode);
	sync_keybind_to_store(g_misc_jump_key, SettingsStore::Misc_Jump_Key, SettingsStore::Misc_Jump_Key_Mode);
	// Menu key — sync key only
	if (!g_menu_key.capturing) SettingsStore::Settings_Menu_Keybind = (ImGuiKey)g_menu_key.key;

	gui->end();
}
