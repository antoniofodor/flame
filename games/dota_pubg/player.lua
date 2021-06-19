local player = {
	character = __character
}

local scene_receiver = scene.find_component("cReceiver")

scene_receiver.add_mouse_right_down_listener(function(mpos)
	local o = camera.node.get_global_pos()
	local d = normalize_3(camera.camera.screen_to_world(mpos) - o)
	local pe = malloc_pointer(1)
	local pos = s_physics.raycast(o, d, pe)
	local p = get_pointer(pe, 0)
	flame_free(pe)

	local e = { p=p }
	make_obj(e, "flame::Entity")
	local name = e.get_name()

	if name == "enemy" then
		player.character.target = __enemy.character
		player.character.state = "attack_target"
	elseif name == "terrain" then
		player.character.target_pos = vec2(pos.x, pos.z)
		player.character.state = "moving"
	end

end)

__player = player
