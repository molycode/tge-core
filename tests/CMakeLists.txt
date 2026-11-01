# Core's own proof. Nothing here may name a module: core sits below every one of them, so a test that needs
# a renderer, a scene or a window belongs to whoever composes those.

add_subdirectory(unit)
add_subdirectory(threading)
add_subdirectory(io)

if(TARGET TgeNet)
	add_subdirectory(net)
endif()

if(TARGET TgeProcess)
	add_subdirectory(process)
endif()
