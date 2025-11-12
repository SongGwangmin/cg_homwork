#version 330 core
 //--- out_Color: 버텍스 세이더에서입력받는색상값
//--- FragColor: 출력할 색상의 값으로프레임버퍼로전달됨.  
in  vec3 out_Color;
 out vec4 FragColor;
 void main(void) 
{
 //--- 버텍스세이더에게서전달받음
//--- 색상 출력
FragColor = vec4 (out_Color, 1.0);
 }