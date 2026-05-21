gdebug.log_info("on_add_msg smoke test: PRELOAD ONLINE")

local mod = game.mod_runtime[game.current_mod]
mod.msg_count = 0

game.add_hook("on_add_msg", function(params)
  mod.msg_count = mod.msg_count + 1
  gdebug.log_info(string.format("on_add_msg #%d  type=%s  msg=%q",
    mod.msg_count, tostring(params.type), tostring(params.msg)))
end)
