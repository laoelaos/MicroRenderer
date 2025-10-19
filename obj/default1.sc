# MicroRenderer Scene File
# phong = diffuse specular ambient p

[Camera]
center 0 0 3
size 600 600
fov 55
near_far 0.1 100
yaw_pitch_roll -90 0 0
end

[POINT_LIGHT]
color 1 1 1
position 20 20 20
intensity 2000
end

[Model]
path ..\obj\african_head\african_head.obj
name african_head
enable 1
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\african_head\african_head_diffuse.tga
normal_map ..\obj\african_head\african_head_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 1
phong 0.8 0.6 0.02 150
end

[Model]
path ..\obj\african_head\african_head_eye_inner.obj
name african_head_eye_inner
enable 1
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\african_head\african_head_eye_inner_diffuse.tga
normal_map ..\obj\african_head\african_head_eye_inner_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 1
phong 0.8 0.6 0.02 150
end
