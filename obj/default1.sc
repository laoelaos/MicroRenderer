# MicroRenderer Scene File

[Camera]
eye 0 0 0
center 0 0 2
up 0 1 0
size 600 600
fov 55
near_far 2 3
yaw_pitch_roll -90 0 0

[Light]
color 1 1 1
position 20 20 20
intensity 2000
end

[Model]
path ..\obj\african_head\african_head.obj
name newObj
enable 0
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\african_head\african_head_diffuse.tga
normal_map ..\obj\african_head\african_head_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 2
phong 0.9 0.6 0.005 150

[Model]
path ..\obj\african_head\african_head_eye_inner.obj
name newObj
enable 0
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\african_head\african_head_eye_inner_diffuse.tga
normal_map ..\obj\african_head\african_head_eye_inner_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 2
phong 0.9 0.6 0.005 150

