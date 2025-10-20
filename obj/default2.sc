# MicroRenderer Scene File
# phong = diffuse specular ambient p

[SkyBox]
..\obj\Hotel.jpg

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
intensity 40
center 2 2 2
size 600 600
fov 90
near_far 0.1 1000
yaw_pitch_roll -135 -45 0
end

[DIRECTIONAL_LIGHT]
color 1 1 1
position -2 2 0
intensity 20
center -2 2 0
size 600 600
fov 90
near_far 0.1 1000
yaw_pitch_roll 0 -45 0
end

[Model]
path ..\obj\diablo3_pose\diablo3_pose.obj
name diablo3_pose
enable 1
translation 0 0 0
rotation 0 0 0
scale 1 1 1
texture ..\obj\diablo3_pose\diablo3_pose_diffuse.tga
normal_map ..\obj\diablo3_pose\diablo3_pose_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 1
phong 0.8 0.6 0.02 150
end

[Model]
path ..\obj\floor.obj
name floor
enable 1
translation 0 0 0
rotation 0 0 0
scale 2 1 2
texture ..\obj\floor_diffuse.tga
normal_map ..\obj\floor_nm_tangent.tga
diffuse_mapping 1
normal_type 1
shade_frequency 1
phong 0.5 0.7 0.005 80
end
