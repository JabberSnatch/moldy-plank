macro(hotreload_setup target)

  if (WIN32)
	target_link_options(${target} PRIVATE "/MANIFEST:NO" "-PDB:${target}_%random%.pdb")

	add_custom_command(
	  TARGET ${target} PRE_BUILD
      COMMAND echo building > build.lock)
	add_custom_command(
	  TARGET ${target} PRE_BUILD
      COMMAND if exist ${target}_*.pdb del ${target}_*.pdb)
	add_custom_command(
	  TARGET ${target} PRE_BUILD
      COMMAND if exist ${target}.dl_* del ${target}.dl_*)
	add_custom_command(
	  TARGET ${target} POST_BUILD
      COMMAND del build.lock)
  endif()

  if (UNIX)
	add_custom_command(
	  TARGET ${target} PRE_BUILD
	  COMMAND echo building > build.lock)
	add_custom_command(
	  TARGET ${target} POST_BUILD
	  COMMAND rm build.lock)
  endif()

endmacro()
