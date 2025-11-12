#version 330 core
 layout (location = 0) in vec3 
vPos; 
//--- 위치 변수:attribute position 0
 layout (location = 1) in vec3 
vColor; 
//--- 컬러 변수:attribute position 1
 out vec3 out_Color;
 uniform mat4 viewTransform;
 uniform mat4 modelTransform;
 uniform mat4 projectionTransform;
 uniform vec3 blockcolor;

 void main(void) 
{
 //--- 프래그먼트세이더에게전달
 gl_Position = projectionTransform * viewTransform * modelTransform * vec4(vPos, 1.0); 
 out_Color = blockcolor;
 }