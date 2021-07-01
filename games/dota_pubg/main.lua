obj_root = scene.find_child("obj_root")
obj_root_n = obj_root.find_component("cNode")

alt_pressing = false

scene_receiver = scene.find_component("cReceiver")
local e_key_alt = find_enum("KeyboardKey")["Alt"]
scene_receiver.add_key_down_listener(function(key)
	if key == e_key_alt then
		alt_pressing = true
	end
end)
scene_receiver.add_key_up_listener(function(key)
	if key == e_key_alt then
		alt_pressing = false
	end
end)

local e_grasses = {}
local e = create_entity("D:\\assets\\vegetation\\grass1.prefab")
table.insert(e_grasses, e)

local e_trees = {}
local e = create_entity("D:\\assets\\vegetation\\tree1.prefab")
table.insert(e_trees, e)

local e_terrain = scene.find_child("terrain")
local terrain = e_terrain.find_component("cTerrain")
local vegetation_root = e_terrain.find_child("vegetation")

terrain_scatter(terrain, vegetation_root, vec4(190, 190, 20, 20), 0.2, e_grasses, 0.03, 0.8)
terrain_scatter(terrain, vegetation_root, vec4(190, 190, 20, 20), 2.5, e_trees, 0.1, 0.8)
--[[
local e_plants = {}
table.insert(e_plants, create_entity("D:\\assets\\vegetation\\plant1.prefab"))

scatter(vec4(0.0, 0.0, 400.0, 400.0), 0.2, e_grasses, 0.05, 2.5)
scatter(vec4(0.0, 0.0, 400.0, 400.0), 0.5, e_plants, 0.0025, 1.0)
]]

local character_panel = scene.find_child("character_panel")
local hp_bar = character_panel.find_child("hp_bar").find_component("cElement")
local hp_text = character_panel.find_child("hp_text").find_component("cText")
local mp_bar = character_panel.find_child("mp_bar").find_component("cElement")
local mp_text = character_panel.find_child("mp_text").find_component("cText")
local exp_bar = character_panel.find_child("exp_bar").find_component("cElement")
local exp_text = character_panel.find_child("exp_text").find_component("cText")
local exp_text = character_panel.find_child("exp_text").find_component("cText")
obj_root.add_event(function()
	hp_bar.set_scalex(main_player.HP / main_player.HP_MAX)
	hp_text.set_text(string.format("%d/%d +%.1f", math.floor(main_player.HP / 10.0), math.floor(main_player.HP_MAX / 10.0), main_player.HP_RECOVER / 10.0))
	mp_bar.set_scalex(main_player.MP / main_player.MP_MAX)
	mp_text.set_text(string.format("%d/%d +%.1f", math.floor(main_player.MP / 10.0), math.floor(main_player.MP_MAX / 10.0), main_player.MP_RECOVER / 10.0))
	exp_bar.set_scalex(main_player.EXP / main_player.EXP_NEXT)
	exp_text.set_text("LV "..main_player.LV..":  "..main_player.EXP.."/"..main_player.EXP_NEXT)
end, 0.0)

ui_equipment_slots = {}
for i=1, EQUIPMENT_SLOTS_COUNT, 1 do
	local ui = scene.find_child("equipment_slot"..i)
	ui_equipment_slots[i] = ui
	local icon = ui.find_child("icon")
	ui.receiver = icon.find_component("cReceiver")
	ui.image = icon.find_component("cImage")

	ui.receiver.add_mouse_right_down_listener(function(mpos)
		
	end)
end

ui_item_slots = {}
for i=1, ITEM_SLOTS_COUNT, 1 do
	local ui = scene.find_child("item_slot"..i)
	ui_item_slots[i] = ui
	local icon = ui.find_child("icon")
	ui.receiver = icon.find_component("cReceiver")
	ui.image = icon.find_component("cImage")

	ui.receiver.add_mouse_right_down_listener(function(mpos)
		main_player.use_bag_item(i)
	end)
end

local attributes_btn = scene.find_child("attributes_btn")
attributes_btn.wnd = nil
attributes_btn.find_component("cReceiver").add_mouse_click_listener(function()
	if not attributes_btn.wnd_openning then
		attributes_btn.wnd = create_entity("attributes")
		attributes_btn.wnd.find_driver("dWindow").add_close_listener(function()
			__ui.remove_child(attributes_btn.wnd)
			attributes_btn.wnd = nil
		end)

		local hp_max_text = attributes_btn.wnd.find_child("hp_max_text").find_component("cText")
		local mp_max_text = attributes_btn.wnd.find_child("mp_max_text").find_component("cText")
		local lv_text = attributes_btn.wnd.find_child("lv_text").find_component("cText")
		local exp_text = attributes_btn.wnd.find_child("exp_text").find_component("cText")
		local phy_dmg_text = attributes_btn.wnd.find_child("phy_dmg_text").find_component("cText")
		local mag_dmg_text = attributes_btn.wnd.find_child("mag_dmg_text").find_component("cText")
		local atk_dmg_text = attributes_btn.wnd.find_child("atk_dmg_text").find_component("cText")
		local sta_text = attributes_btn.wnd.find_child("sta_text").find_component("cText")
		local spi_text = attributes_btn.wnd.find_child("spi_text").find_component("cText")
		local luk_text = attributes_btn.wnd.find_child("luk_text").find_component("cText")
		local str_text = attributes_btn.wnd.find_child("str_text").find_component("cText")
		local agi_text = attributes_btn.wnd.find_child("agi_text").find_component("cText")
		local int_text = attributes_btn.wnd.find_child("int_text").find_component("cText")
		local points_text = attributes_btn.wnd.find_child("points_text").find_component("cText")

		local add_sta_btn = attributes_btn.wnd.find_child("add_sta_btn")
		local add_spi_btn = attributes_btn.wnd.find_child("add_spi_btn")
		local add_luk_btn = attributes_btn.wnd.find_child("add_luk_btn")
		local add_str_btn = attributes_btn.wnd.find_child("add_str_btn")
		local add_agi_btn = attributes_btn.wnd.find_child("add_agi_btn")
		local add_int_btn = attributes_btn.wnd.find_child("add_int_btn")
		if main_player.attribute_points > 0 then
			add_sta_btn.set_visible(true)
			add_spi_btn.set_visible(true)
			add_luk_btn.set_visible(true)
			add_str_btn.set_visible(true)
			add_agi_btn.set_visible(true)
			add_int_btn.set_visible(true)
		end

		function update()
			hp_max_text.set_text(string.format("HP MAX: %d", math.floor(main_player.HP_MAX / 10.0)))
			mp_max_text.set_text(string.format("MP MAX: %d", math.floor(main_player.MP_MAX / 10.0)))
			lv_text.set_text(string.format("LV: %d", main_player.LV))
			exp_text.set_text(string.format("EXP: %d/%d", main_player.EXP, main_player.EXP_NEXT))
			phy_dmg_text.set_text(string.format("PHY DMG: %d", main_player.PHY_DMG))
			mag_dmg_text.set_text(string.format("MAG DMG: %d", main_player.MAG_DMG))
			atk_dmg_text.set_text(string.format("ATK DMG: %d (%s)", math.floor(main_player.ATK_DMG / 10.0), main_player.ATK_TYPE))
		
			sta_text.set_text(string.format("STA: %d", main_player.STA))
			spi_text.set_text(string.format("SPI: %d", main_player.SPI))
			luk_text.set_text(string.format("LUK: %d", main_player.LUK))
			str_text.set_text(string.format("STR: %d", main_player.STR))
			agi_text.set_text(string.format("AGI: %d", main_player.AGI))
			int_text.set_text(string.format("INT: %d", main_player.INT))
			points_text.set_text(string.format("Points: %d", main_player.attribute_points))
		end

		function add_attribute(attr)
			if main_player.attribute_points > 0 then
				main_player[attr] = main_player[attr] + 1
				main_player.calc_stats()
				main_player.attribute_points = main_player.attribute_points - 1
				update()
				if main_player.attribute_points == 0 then
					add_sta_btn.set_visible(false)
					add_spi_btn.set_visible(false)
					add_luk_btn.set_visible(false)
					add_str_btn.set_visible(false)
					add_agi_btn.set_visible(false)
					add_int_btn.set_visible(false)
				end
			end
		end
		add_sta_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("STA")
		end)
		add_spi_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("SPI")
		end)
		add_luk_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("LUK")
		end)
		add_str_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("STR")
		end)
		add_agi_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("AGI")
		end)
		add_int_btn.find_component("cReceiver").add_mouse_click_listener(function()
			add_attribute("INT")
		end)

		update()

		__ui.add_child(attributes_btn.wnd)
	end
end)


for i=1, 10, 1 do
	local e = create_entity("remore")
	e.set_name("enemy_"..tostring(math.floor(math.random() * 10000)))
	--e.find_component("cNode").set_pos(vec3(math.random() * 400, 200, math.random() * 400))
	e.find_component("cNode").set_pos(vec3(190 + math.random() * 20, 200, 190 + math.random() * 20))
	local npc = make_npc(e, 1)
	obj_root.add_child(e)
end

local e = create_entity("player")
e.set_name("player")
e.find_component("cNode").set_pos(vec3(200, 65, 200))
main_player = make_player(e)
main_player.receive_item(1, 1)
main_player.awake()
obj_root.add_child(e)

obj_root.add_event(function()
	for n, char in pairs(characters[2]) do
		if obj_root_n.is_any_within_circle(char.pos.to_flat(), 50, 1) then
			char.awake()
		else
			char.sleep()
		end
	end
end, 1.0)
