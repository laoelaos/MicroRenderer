.PHONY : d1 d2 d3

d1:
	cmake --build build -j
	build/tinyrenderer obj/diablo3_pose/diablo3_pose obj/floor

d2:
	cmake --build build -j
	build/tinyrenderer obj/african_head/african_head obj/african_head/african_head_eye_inner

d3:
	cmake --build build -j
	build/tinyrenderer obj/boggie/body obj/boggie/eyes obj/boggie/head.obj obj/floor