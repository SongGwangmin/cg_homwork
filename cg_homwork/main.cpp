#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <limits>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <random>
#include <list>
#include <algorithm>
#include <cmath>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

#define pi 3.14159265358979323846
#define worldmapsize 50.0f


std::random_device rd;

// random_device 를 통해 난수 생성 엔진을 초기화 한다.
std::mt19937 gen(rd());

std::uniform_int_distribution<int> dis(0, 256);
std::uniform_int_distribution<int> polyrandom(0, 24);
std::uniform_int_distribution<int> longnessRandom(5, 20); // longness용 랜덤 (5~20)
//std::uniform_int_distribution<int> numdis(0, windowWidth - rectspace);

//--- 아래 5개 함수는 사용자 정의 함수 임
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
void setupBuffers();
void TimerFunction(int value);
void settingtimerfunc(int value);

//--- 필요한 변수 선언
GLint width = 1200, height = 800;
GLuint shaderProgramID; //--- 세이더 프로그램 이름
GLuint vertexShader; //--- 버텍스 세이더 객체
GLuint fragmentShader; //--- 프래그먼트 세이더 객체
GLuint VAO, VBO; //--- 버텍스 배열 객체, 버텍스 버퍼 객체

int projectionToggle = 1; // 투영 토글 (1: perspective, 0: orthographic)
int updowntoggle = 0; // 상하 움직임 토글 (0: 정지, 1: 움직임)
float cameraAngleY = 0.0f; // 카메라 Y축 회전 각도

glm::mat4 dir;
// 카메라 변수
glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 100.0f);      // 카메라 위치
glm::vec3 cameraTarget = glm::vec3(0.0f, 10.0f, 0.0f);      // 카메라 타겟


// AABB 구조체 정의
struct AABB {
	glm::vec3 min;  // 최소 좌표 (왼쪽 아래 뒤)
	glm::vec3 max;  // 최대 좌표 (오른쪽 위 앞)
};

// 전체 3D AABB 충돌 체크
bool checkAABBCollision(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// X, Z축만 충돌 체크 (Y축 제외)
bool checkAABBCollisionXZ(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
		(a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Y축만 충돌 체크
bool checkAABBCollisionY(const AABB& a, const AABB& b) {
	return (a.min.y <= b.max.y && a.max.y >= b.min.y);
}


// 3개의 큐브 AABB 정보 저장
AABB cubeAABB[3];

// Player 구조체 정의
struct Player {
	glm::vec3 centerPos;     // 중심 좌표
	glm::vec3 size;          // 크기 (폭, 높이, 깊이)
	glm::vec3 velocity;      // 속도 (물리 시스템용)
	bool isOnGround;         // 바닥에 있는지 여부

	// AABB 계산 함수
	AABB getAABB() const {
		AABB box;
		box.min = centerPos - size / 2.0f;  // 중심에서 반 크기만큼 뺌
		box.max = centerPos + size / 2.0f;  // 중심에서 반 크기만큼 더함
		return box;
	}
};

// Player 전역 변수
Player player;

// BlockData 구조체 정의
struct BlockData {
	glm::vec3 pos;      // 위치
	glm::vec3 color;    // 색상
	float longness;     // 길이
	float blockwidth;       // 너비
	float blockheight;      // 높이
	float nowheight;      // 현재 높이
	float deltaHeight;  // 높이 변화량
	int velocity;       // 속도 (1 또는 -1)
};

// Forward declaration
class polygon;
std::vector<float> allVertices;

// 육면체 클래스
class Cube {
private:
	glm::vec3 vertices[8]; // 8개의 꼭지점
	glm::vec3 color;       // 색상

public:
	// 생성자: 8개의 점을 받아서 육면체 생성
	Cube(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
		glm::vec3 v4, glm::vec3 v5, glm::vec3 v6, glm::vec3 v7,
		glm::vec3 col = glm::vec3(1.0f, 1.0f, 1.0f))
		: color(col)
	{
		vertices[0] = v0;
		vertices[1] = v1;
		vertices[2] = v2;
	vertices[3] = v3;
		vertices[4] = v4;
		vertices[5] = v5;
		vertices[6] = v6;
		vertices[7] = v7;
	}

	// 육면체의 정점 데이터를 VBO에 추가하는 함수
	void sendVertexData(std::vector<float>& vbo) {
		// 육면체는 6개의 면으로 구성되며, 각 면은 2개의 삼각형으로 구성
		// 면의 인덱스 순서 (반시계방향)
		int faceIndices[6][4] = {
			{0, 1, 2, 3}, // 앞면
			{4, 7, 6, 5}, // 뒷면
			{0, 4, 5, 1}, // 아랫면      
			{2, 6, 7, 3}, // 윗면
			{0, 3, 7, 4}, // 왼쪽면
			{1, 5, 6, 2}  // 오른쪽면
		};

		// 각 면을 2개의 삼각형으로 분할하여 VBO에 추가
		for (int face = 0; face < 6; ++face) {
			int* indices = faceIndices[face];

			// 첫 번째 삼각형 (0, 1, 2)
			addTriangle(vbo,
				vertices[indices[0]],

				vertices[indices[1]],

				vertices[indices[2]]);

			// 두 번째 삼각형 (0, 2, 3)
			addTriangle(vbo,
				vertices[indices[0]],

				vertices[indices[2]],

				vertices[indices[3]]);
		}
	}

	// 색상 설정
	void setColor(glm::vec3 col) {
		color = col;
	}

	// 특정 꼭지점 가져오기
	glm::vec3 getVertex(int index) const {
		if (index >= 0 && index < 8) {
			return vertices[index];
		}
		return glm::vec3(0.0f);
	}

	// 특정 꼭지점 설정
	void setVertex(int index, glm::vec3 v) {
		if (index >= 0 && index < 8) {
			vertices[index] = v;
		}
	}

private:
	// 삼각형 데이터를 VBO에 추가하는 헬퍼 함수
	void addTriangle(std::vector<float>& vbo,
		const glm::vec3& v0,
		const glm::vec3& v1,
		const glm::vec3& v2)
	{
		// 정점 0
		vbo.insert(vbo.end(), {
			v0.x, v0.y, v0.z,
			color.r, color.g, color.b
			});

		// 정점 1
		vbo.insert(vbo.end(), {
			v1.x, v1.y, v1.z,
			color.r, color.g, color.b
			});

		// 정점 2
		vbo.insert(vbo.end(), {
			v2.x, v2.y, v2.z,
			color.r, color.g, color.b
			});
	}
};


// Cube 전역 변수
Cube* whiteCube = nullptr;


void Keyboard(unsigned char key, int x, int y);
void rotatetimer(int value);
void SpecialKeys(int key, int x, int y); // 특수 키(화살표 키) 콜백 함수 선언
void Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			// 마우스 버튼을 누를 때 현재 위치 저장
	
		}
	}
}
void Motion(int x, int y) { // 마우스 모션 콜백 함수 선언


}
void Mousemove(int x, int y) {
	
}

char* fileutub(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb"); // Open file for reading
	if (!fptr) // Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file
	length = ftell(fptr); // Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
	fclose(fptr); // Close the file
	buf[length] = 0; // Null terminator
	return buf; // Return the buffer
}

void setupBuffers() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// 정점 속성 설정: 위치 (3개) + 색상 (3개) = 총 6개 float
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

// 가로, 세로 개수 전역 변수
int gridWidth = 0;
int gridHeight = 0;

// BlockData 2D 배열을 1D 벡터로 저장
std::vector<BlockData> blockGrid;

// 2D 인덱스를 1D 인덱스로 변환하는 함수
inline int getBlockIndex(int x, int z) {
	if (x < 0 || x >= gridWidth || z < 0 || z >= gridHeight) {
		return -1; // 유효하지 않은 인덱스
	}
	return z * gridWidth + x;
}

// 특정 위치의 BlockData 가져오기 (참조 반환으로 직접 수정 가능)
inline BlockData& getBlock(int x, int z) {
	return blockGrid[getBlockIndex(x, z)];
}

// 특정 위치의 BlockData 가져오기 (const 버전)
inline const BlockData& getBlockConst(int x, int z) {
	return blockGrid[getBlockIndex(x, z)];
}

// 숫자 입력 받기 함수
bool getValidInput(const char* prompt, int& value) {
	std::cout << prompt;
	
	if (!(std::cin >> value)) {
		// 입력이 숫자가 아닌 경우
		std::cin.clear(); // 에러 플래그 초기화
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 입력 버퍼 비우기
		return false;
	}
	
	// 숫자는 맞지만 범위를 벗어난 경우
	if (value < 5 || value > 25) {
		return false;
	}
	
	return true;
}

// 가로, 세로 개수 입력받기
void getGridDimensions() {
	std::cout << "=================================\n";
	std::cout << "그리드 크기를 입력하세요 (5~25)\n";
	std::cout << "=================================\n";
	
	// 가로 개수 입력
	while (!getValidInput("가로 개수를 입력하세요 (5~25): ", gridWidth)) {
		std::cout << "잘못된 입력입니다. 5 이상 25 이하의 정수를 입력하세요.\n";
	}
	
	// 세로 개수 입력
	while (!getValidInput("세로 개수를 입력하세요 (5~25): ", gridHeight)) {
		std::cout << "잘못된 입력입니다. 5 이상 25 이하의 정수를 입력하세요.\n";
	}
	
	std::cout << "\n입력 완료!\n";
	std::cout << "가로: " << gridWidth << ", 세로: " << gridHeight << "\n";
	std::cout << "=================================\n\n";
}

int main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
	// 가로, 세로 개수 입력받기
	getGridDimensions();

	// BlockData 그리드 초기화
	blockGrid.resize(gridWidth * gridHeight);
	
	// 블록 크기 계산
	float blockWidth = worldmapsize / gridWidth;
	float blockHeight = worldmapsize / gridHeight;
	
	// 각 블록 초기화 (예시: 위치와 색상 설정)
	for (int z = 0; z < gridHeight; ++z) {
		for (int x = 0; x < gridWidth; ++x) {
			int index = getBlockIndex(x, z);
			
			// 블록 위치 계산 (그리드 중앙 정렬)
			blockGrid[index].pos = glm::vec3(
				x * blockWidth - worldmapsize / 2.0f + blockWidth / 2.0f,
				0.0f,
				z * blockHeight - worldmapsize / 2.0f + blockHeight / 2.0f
			);
			
			blockGrid[index].color = glm::vec3(
				static_cast<float>(dis(gen)) / 256.0f,
				static_cast<float>(dis(gen)) / 256.0f,
				static_cast<float>(dis(gen)) / 256.0f
			); // 랜덤 색상
			
			int randomLength = longnessRandom(gen); // 5~20 랜덤 정수
			blockGrid[index].longness = static_cast<float>(randomLength) * 1.5f;
			randomLength = longnessRandom(gen); // 5~20 랜덤 정수
			blockGrid[index].deltaHeight = static_cast<float>(randomLength) / 20.0f;
			blockGrid[index].blockwidth = blockWidth;
			blockGrid[index].blockheight = blockHeight;
			blockGrid[index].nowheight = 0.1f; // 초기 높이 0.1f
			blockGrid[index].velocity = (dis(gen) % 2 == 0) ? 1 : -1; // 랜덤 속도 1 또는 -1
		}
	}

	//width = 800;
	//height = 800;

	//--- 윈도우 생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Rectangle Rendering");
	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glewInit();
	//--- 세이더 읽어와서 세이더 프로그램 만들기: 사용자 정의함수 호출
	make_vertexShaders(); //--- 버텍스 세이더 만들기
	make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
	shaderProgramID = make_shaderProgram();
	glEnable(GL_CULL_FACE);
	// 버퍼 설정
	setupBuffers();
	glEnable(GL_DEPTH_TEST);

	// 하얀색 큐브 생성: (-0.5, 0, -0.5)부터 (0.5, 1, 0.5)까지
	// 8개의 정점 정의
	glm::vec3 v0(-0.5f, 0.0f, -0.5f);  // 아래 앞 왼쪽
	glm::vec3 v1(0.5f, 0.0f, -0.5f);   // 아래 앞 오른쪽
	glm::vec3 v2(0.5f, 1.0f, -0.5f);   // 위 앞 오른쪽
	glm::vec3 v3(-0.5f, 1.0f, -0.5f);  // 위 앞 왼쪽
	glm::vec3 v4(-0.5f, 0.0f, 0.5f);   // 아래 뒤 왼쪽
	glm::vec3 v5(0.5f, 0.0f, 0.5f);    // 아래 뒤 오른쪽
	glm::vec3 v6(0.5f, 1.0f, 0.5f);    // 위 뒤 오른쪽
	glm::vec3 v7(-0.5f, 1.0f, 0.5f);   // 위 뒤 왼쪽

	whiteCube = new Cube(v0, v1, v2, v3, v4, v5, v6, v7, glm::vec3(1.0f, 1.0f, 1.0f));

	// 큐브 정점 데이터를 allVertices에 추가
	whiteCube->sendVertexData(allVertices);

	

	//--- 세이더 프로그램 만들기

	glutDisplayFunc(drawScene); //--- 출력 콜백 함수
	glutReshapeFunc(Reshape);

	glutTimerFunc(25, settingtimerfunc, 2); // 초기화 타이머 시작

	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKeys); // 특수 키 콜백 함수 등록
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion); // 마우스 모션 콜백 함수 등록
	glutPassiveMotionFunc(Mousemove); // 마우스 이동 콜백 함수 등록 (버튼 누르지 않고 이동)

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glutMainLoop();
	return 0;
}



void make_vertexShaders()
{
	GLchar* vertexSource;
	//--- 버텍스 세이더 읽어 저장하고 컴파일 하기
	//--- filebuf: 사용자정의 함수로 텍스트를 읽어서 문자열에 저장하는 함수
	vertexSource = fileutub("vertex_projection.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	GLchar* fragmentSource;
	//--- 프래그먼트 세이더 읽어 저장하고 컴파일하기
	fragmentSource = fileutub("fragment_matrix.glsl"); // 프래그세이더 읽어오기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

GLuint make_shaderProgram()
{
	GLint result;
	GLchar* errorLog = NULL;
	GLuint shaderID;
	shaderID = glCreateProgram(); //--- 세이더 프로그램 만들기
	glAttachShader(shaderID, vertexShader); //--- 세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(shaderID, fragmentShader); //--- 세이더 프로그램에 프래그먼트 세이더 붙이기
	glLinkProgram(shaderID); //--- 세이더 프로그램 링크하기
	glDeleteShader(vertexShader); //--- 세이더 객체를 세이더 프로그램에 링크했음으로, 세이더 객체 자체는 삭제 가능
	glDeleteShader(fragmentShader);
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result); // ---세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return false;
	}
	glUseProgram(shaderID); //--- 만들어진 세이더 프로그램 사용하기
	return shaderID;
}

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수
{
	GLfloat rColor, gColor, bColor;
	rColor = gColor = bColor = 0.0; //--- 배경색을 검은색으로 설정
	glClearColor(rColor, gColor, bColor, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 카메라 설정
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// 셰이더 사용
	glUseProgram(shaderProgramID);

	// 모델 행렬과 색상 uniform 위치
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "modelTransform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "blockcolor");
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	unsigned int viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");

	// VBO 데이터 바인딩
	if (!allVertices.empty()) {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(float),
			allVertices.data(), GL_STATIC_DRAW);

		// ===== 첫 번째 뷰포트: 전체 화면 (0, 0, 1200, 800) =====
		glViewport(0, 0, 1200, 800);

		// 카메라 위치 변경
		


		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(cameraAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 tcameraPos = glm::vec3(rotationMatrix * glm::vec4(cameraPos, 1.0f));
		glm::vec3 tcameraTarget = glm::vec3(rotationMatrix * glm::vec4(cameraTarget, 1.0f));
		cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

		// 투영 행렬 설정
		glm::mat4 projection;
		if (projectionToggle == 1) {
			// 원근 투영 (Perspective)
			projection = glm::perspective(
				glm::radians(30.0f),
				1200.0f / 800.0f,
				0.1f,
				300.0f
			);
		}
		else {
			// 직교 투영 (Orthographic)
			float aspectRatio = 1200.0f / 800.0f;
			float orthoSize = 50.0f;
			
			projection = glm::ortho(
				-orthoSize * aspectRatio,
				orthoSize * aspectRatio,
				-orthoSize,
				orthoSize,
				0.1f,
				300.0f
			);
		}
		glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

		// 뷰 행렬 설정
		glm::mat4 view = glm::lookAt(tcameraPos, cameraTarget, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

		// 모든 블록 그리기
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				const BlockData& block = getBlockConst(x, z);
				
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, block.pos);
				model = glm::scale(model, glm::vec3(block.blockwidth, block.nowheight, block.blockheight));
				
				glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(colorLocation, 1, glm::value_ptr(block.color));
				
				glDrawArrays(GL_TRIANGLES, 0, allVertices.size() / 6);
			}
		}

		// ===== 두 번째 뷰포트: 작은 화면 (900, 600, 300, 200) =====
		glViewport(900, 600, 300, 200);
		glm::vec3 upcameraPos = glm::vec3(0.0f, 150.0f, 0.0f); // 카메라 위치 변경
		glm::vec3 upcameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraUp = glm::vec3(0.0f, 0.0f, -1.0f); // 카메라 업 벡터 변경
		// 두 번째 뷰포트용 투영 행렬 (다른 투영 방식 또는 동일)
		if (projectionToggle == 1) {
			projection = glm::perspective(
				glm::radians(30.0f),
				300.0f / 200.0f,  // 작은 뷰포트의 종횡비
				0.1f,
				300.0f
			);
		}
		else {
			float aspectRatio = 300.0f / 200.0f;
			float orthoSize = 50.0f;
			
			projection = glm::ortho(
				-orthoSize * aspectRatio,
				orthoSize * aspectRatio,
				-orthoSize,
				orthoSize,
				0.1f,
				300.0f
			);
		}
		glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

		// 두 번째 뷰포트용 뷰 행렬 (같은 카메라 또는 다른 각도)
		view = glm::lookAt(upcameraPos, upcameraTarget, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

		// 모든 블록 다시 그리기
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				const BlockData& block = getBlockConst(x, z);
				
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, block.pos);
				model = glm::scale(model, glm::vec3(block.blockwidth, block.nowheight, block.blockheight));
				
				glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(colorLocation, 1, glm::value_ptr(block.color));
				
				glDrawArrays(GL_TRIANGLES, 0, allVertices.size() / 6);
			}
		}

		glBindVertexArray(0);
	}

	glutSwapBuffers(); // 화면에 출력하기
}

//--- 다시그리기 콜백 함수
GLvoid Reshape(int w, int h) //--- 콜백 함수: 다시 그리기 콜백 함수
{
	glViewport(0, 0, w, h);
}

void Keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'q': // 프로그램 종료
		glutLeaveMainLoop();
		break;
	case 'Q': // 프로그램 종료
		glutLeaveMainLoop();
		break;
	case 'p': // Perspective 투영
	case 'P':
		projectionToggle = 1;
		std::cout << "Perspective 투영 모드\n";
		break;
	case 'o': // Orthographic 투영
	case 'O':
		projectionToggle = 0;
		std::cout << "Orthographic 투영 모드\n";
		break;
	case 'm': // 상하 움직임 시작
		updowntoggle = 1;
		std::cout << "상하 움직임 시작\n";
		break;
	case 'M': // 상하 움직임 정지
		updowntoggle = 0;
		std::cout << "상하 움직임 정지\n";
		break;
	case '+': // deltaHeight 증가
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				BlockData& block = getBlock(x, z);
				block.deltaHeight += 0.1f;
			}
		}
		std::cout << "deltaHeight 0.1f 증가\n";
		break;
	case '-': // deltaHeight 감소
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				BlockData& block = getBlock(x, z);
				block.deltaHeight -= 0.1f;
				
				// 음수가 되면 0.0f로 설정
				if (block.deltaHeight < 0.0f) {
					block.deltaHeight = 0.0f;
				}
			}
		}
		std::cout << "deltaHeight 0.1f 감소\n";
		break;
	case 'v': // nowheight 리셋
	case 'V':
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				BlockData& block = getBlock(x, z);
				block.nowheight = 0.1f;
			}
		}
		std::cout << "모든 블록의 nowheight를 0.1f로 리셋\n";
		break;
	case 'y': // Y축 양의 방향으로 5도 회전
		{
		cameraAngleY += 5.0f;
		std::cout << "카메라 Y축 +5도 회전\n";
		}
		break;
	case 'Y': // Y축 음의 방향으로 5도 회전
		{
		cameraAngleY -= 5.0f;
		std::cout << "카메라 Y축 -5도 회전\n";
		}
		break;
	case 'z':
	{
		cameraPos.z += 5.0f;
		//cameraTarget.z += 5.0f;
	}
	break;
	case 'Z':
		{
		cameraPos.z -= 5.0f;
		//cameraTarget.z -= 5.0f;
	}
		break;
	case 'c': // 초기화
	case 'C':
		{
			// 카메라 초기화
			cameraAngleY = 0.0f;
			cameraPos = glm::vec3(0.0f, 100.0f, 100.0f);
			
			// 모든 블록의 nowheight를 longness로 설정
			for (int z = 0; z < gridHeight; ++z) {
				for (int x = 0; x < gridWidth; ++x) {
					BlockData& block = getBlock(x, z);
					block.nowheight = block.longness;
				}
			}
			
			// updowntoggle 끄기
			updowntoggle = 0;
			
			std::cout << "초기화 완료: 카메라, 블록 높이, 움직임 정지\n";
		}
		break;


	default:
		break;
	}

	glutPostRedisplay();
}

void settingtimerfunc(int value)
{
	bool allBlocksFinished = true;
	
	// 모든 블록의 nowheight를 증가시키기
	for (int z = 0; z < gridHeight; ++z) {
		for (int x = 0; x < gridWidth; ++x) {
			BlockData& block = getBlock(x, z);
			
			// nowheight가 longness보다 작으면 deltaHeight만큼 증가
			if (block.nowheight < block.longness) {
				block.nowheight += block.deltaHeight;
				
				// 오버슈팅 방지 (nowheight가 longness를 초과하지 않도록)
				if (block.nowheight > block.longness) {
					block.nowheight = block.longness;
				}
				
				// 아직 완료되지 않은 블록이 있음
				if (block.nowheight < block.longness) {
					allBlocksFinished = false;
				}
			}
		}
	}

	glutPostRedisplay();
	
	// 모든 블록이 완료되지 않았으면 계속 실행
	if (!allBlocksFinished) {
		glutTimerFunc(25, settingtimerfunc, 2);
	}
	else {
		// 모든 블록이 완료되면 TimerFunction 시작
		glutTimerFunc(25, TimerFunction, 1);
	}
}

void TimerFunction(int value)
{
	// updowntoggle이 켜져있으면 블록들을 상하로 움직임
	if (updowntoggle == 1) {
		for (int z = 0; z < gridHeight; ++z) {
			for (int x = 0; x < gridWidth; ++x) {
				BlockData& block = getBlock(x, z);
				
				// nowheight에 deltaHeight * velocity를 더함
				block.nowheight += block.deltaHeight * block.velocity;
				
				// 상한선 체크: 30.0f를 넘으면
				if (block.nowheight > 30.0f) {
					block.nowheight = 30.0f;
					block.velocity = -1; // 방향 반전
				}
				
				// 하한선 체크: 0.0f 밑으로 가면
				if (block.nowheight < 0.0f) {
					block.nowheight = 0.0f;
					block.velocity = 1; // 방향 반전
				}
			}
		}
	}

	glutPostRedisplay();
	glutTimerFunc(25, TimerFunction, 1);
}


void SpecialKeys(int key, int x, int y) // 특수 키(화살표 키) 콜백 함수
{
	const float MOVE_SPEED = 1.0f;

	switch (key) {
	case GLUT_KEY_UP: // 위쪽 화살표 - z축 음의 방향으로 이동
		player.centerPos.z -= MOVE_SPEED;
		break;

	case GLUT_KEY_DOWN: // 아래쪽 화살표 - z축 양의 방향으로 이동
		player.centerPos.z += MOVE_SPEED;
		break;

	case GLUT_KEY_LEFT: // 왼쪽 화살표 - x축 음의 방향으로 이동
		player.centerPos.x -= MOVE_SPEED;
		break;

	case GLUT_KEY_RIGHT: // 오른쪽 화살표 - x축 양의 방향으로 이동
		player.centerPos.x += MOVE_SPEED;
		break;
	}

	glutPostRedisplay();
}
