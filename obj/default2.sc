# MicroRenderer Scene File

[Camera]
center 0 0 3
size 600 600
fov 55
near_far 0.1 100
yaw_pitch_roll -90 0 0
end

[DIRECTIONAL_LIGHT]
color 1 1 1
position 2 2 2
intensity 50
center 2 2 2
size 600 600
fov 90
near_far 0.1 1000
yaw_pitch_roll 0 0 0
end

[Model]
path ..\obj\diablo3_pose\diablo3_pose.obj
name diablo3_pose
enable 0
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\diablo3_pose\diablo3_pose_diffuse.tga
normal_map ..\obj\diablo3_pose\diablo3_pose_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 2
phong 0.9 0.6 0.005 150
end

[Model]
path ..\obj\floor.obj
name floor
enable 0
translation 0 0 0
rotation 0 0 0
scale 2 1 2
texture ..\obj\floor_diffuse.tga
normal_map ..\obj\floor_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 2
phong 0.9 0.6 0.005 150
end
