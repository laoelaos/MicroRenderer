.PHONY : d1 d2 d3

d1:
	cmake --build build -j
	build/tinyrenderer obj/diablo3_pose/diablo3_pose.obj obj/floor.obj

d2:
	cmake --build build -j
	build/tinyrenderer obj/african_head/african_head.obj obj/african_head/african_head_eye_inner.obj obj/african_head/african_head_eye_outer.obj

d3:
	cmake --build build -j
	build/tinyrenderer obj/boggie/body.obj obj/boggie/eyes.obj obj/boggie/head.obj obj/floor.obj