//TF
#include <math.h>
#include <vector>
#include <time.h>

#include <glutil.h>
#include <figures.h>
#include <camera.h>

#include <files.hpp>
#include <model.hpp>

i32 n = 64;
std::vector<float> perlinNoise1D(int nOctaves, float fScalingBias) {
	std::vector<float> pNoise1D(n);

	std::vector<float> noise1D(n);
	for (int i = 0; i < n; i++) noise1D[i] = (float)rand() / (float)RAND_MAX;

	for (int i = 0; i < n; i++)
	{
		float iNoise = 0.0f;
		float fScale = 1.0f;
		float fScaleAcc = 0.0f;
		for (int o = 0; o < nOctaves; o++)
		{
			int nPitch = n >> o;
			int nSample1 = (i / nPitch) * nPitch;
			int nSample2 = (nSample1 + nPitch) % n;

			float fBlend = (float)(i - nSample1) / (float)nPitch;
			float fInter = (1.0f - fBlend) * noise1D[nSample1] + fBlend * noise1D[nSample2];

			iNoise += fInter * fScale;
			fScaleAcc += fScale;
			fScale /= fScalingBias;
		}
		pNoise1D[i] = iNoise / fScaleAcc;
	}
	return pNoise1D;
}

const u32 FSIZE = sizeof(f32);
const u32 ISIZE = sizeof(u32);
const u32 SCRWIDTH = 1280;
const u32 SCRHEIGHT = 720;
const f32 ASPECT = (f32)SCRWIDTH / (f32)SCRHEIGHT;

glm::vec3 lightPos(50.0f, 20.0f, -100.0f); //posicion de la fuente de luz

Cam* cam;

f32 lastx;
f32 lasty;
f32 deltaTime = 0.0f;
f32 lastFrame = 0.0f;
bool firstmouse = true;
bool wireframe = false;

/**
 * keyboard input processing
 **/
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam->processKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam->processKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam->processKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam->processKeyboard(RIGHT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		wireframe = true;
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		wireframe = false;
	}
}

void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos) {
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (firstmouse) {
			lastx = xpos;
			lasty = ypos;
			firstmouse = false;
			return;
		}
		cam->processMouse((f32)(xpos - lastx), (f32)(lasty - ypos));
		lastx = xpos;
		lasty = ypos;
	}
	else {
		firstmouse = true;
	}
}

void scroll_callback(GLFWwindow* window, f64 xoffset, f64 yoffset) {
	cam->processScroll((f32)yoffset);
}

i32 main() {
	srand(time(NULL));
	GLFWwindow* window = glutilInit(3, 3, SCRWIDTH, SCRHEIGHT, "Hola");
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	Shader* lightingShader = new Shader("lightingmaps.vert", "lightingmaps.frag");
	Shader* lightCubeShader = new Shader("lightcube.vert", "lightcube.frag");

	cam = new Cam();
	Cube* cubex = new Cube();

	Files* files = new Files("resources/shaders", "resources/textures", "resources/objects");
	ShaderO* shader = new ShaderO(files, "shaderO.vert", "shaderO.frag");
	Model*  obj1 = new Model(files, "eva/eva.obj");
	/* https://www.renderhub.com/rip-van-winkle/evangelion-unit-01-from-evangelion-breaking-dawn# */
	Model*  obj2 = new Model(files, "frog/frog.obj");
	/* https://free3d.com/3d-model/banjofrog-v1--699349.html */
	Model*  obj3 = new Model(files, "Tree/Tree.obj");
	/* https://free3d.com/3d-model/tree-74556.html */

	glm::vec3 lightColor = glm::vec3(1.0f);

	f32 aux_h;
	std::vector<float> pNoise1D = perlinNoise1D(7, 3.20);
	std::vector<glm::vec3> positions(n*n);
	for (u32 i = 0; i < n; ++i) {
		for (u32 j = 0; j < n; ++j) {
			f32 x = i - n / 2.0f + 0.01 * 0;
			f32 z = j - n / 2.0f + 0.01 * 0;
			f32 y;
			if (i == 0 && j == 0) {
				aux_h = int(pNoise1D[i] * 120);
				y = 0.0;
			}
			else {
				y = int(pNoise1D[i] * 120) - aux_h ;
			}
			positions[i*n + j] = glm::vec3(x, y, z);
		}
	}

	u32 n_frogs = n / 4;
	std::vector<glm::vec3> frogPositions(n_frogs*n_frogs);
	for (u32 i = 0; i < n_frogs; ++i) {
		for (u32 j = 0; j < n_frogs; ++j) {
			f32 x = (i - 8.0) * 2 ;
			f32 z = (j - 8.0) * 2  - 20.0;
			f32 y = rand() % 80 + 100; ;
			frogPositions[i*n_frogs + j] = glm::vec3(x, y, z);
		}
	}

	u32 cubeVao, lightCubeVao, vbo, ebo;
	glGenVertexArrays(1, &cubeVao);
	glGenVertexArrays(1, &lightCubeVao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(cubeVao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glBufferData(GL_ARRAY_BUFFER, cubex->getVSize()*FSIZE,
		cubex->getVertices(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubex->getISize()*ISIZE,
		cubex->getIndices(), GL_STATIC_DRAW);

	// posiciones
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(0));
	glEnableVertexAttribArray(0);
	// normales: ojo que es el 3er comp, por eso offset es 6
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(6));
	glEnableVertexAttribArray(1);
	// textures
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(9));
	glEnableVertexAttribArray(2);

	glBindVertexArray(lightCubeVao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, cubex->len(), cubex->skip(0));
	glEnableVertexAttribArray(0);

	glEnable(GL_DEPTH_TEST);

	unsigned int texture1 = lightingShader->loadTexture("texture1.jpg");
	unsigned int texture2 = lightingShader->loadTexture("texture2.jpg");
	unsigned int texture3 = lightingShader->loadTexture("texture3.jpg");

	f32 evaScale = 10.0f;
	f32 evaY = -50.0f;	

	while (!glfwWindowShouldClose(window)) {
		f32 currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 proj = glm::perspective(cam->getZoom(), ASPECT, 0.1f, 100.0f);
		lightingShader->useProgram();
		glm::mat4 model = glm::mat4(1.0f);
		glBindVertexArray(cubeVao);

		for (u32 i = 0; i < positions.size(); ++i) {

			if (positions[i].y >= -2) {
				glBindTexture(GL_TEXTURE_2D, texture3);
				lightingShader->setVec3("xyzMat.specular", 0.5f, 0.5f, 0.5f);
				lightingShader->setF32("xyzMat.shininess", 64.0f);
			}
			else if (positions[i].y > -7) {
				glBindTexture(GL_TEXTURE_2D, texture2);
				lightingShader->setVec3("xyzMat.specular", 0.12f, 0.12f, 0.12f);
				lightingShader->setF32("xyzMat.shininess", 2.0f);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, texture1);
				lightingShader->setVec3("xyzMat.specular", 0.19f, 0.19f, 0.19f);
				lightingShader->setF32("xyzMat.shininess", 4.0f);
			}

			lightingShader->setVec3("xyzLht.position", lightPos);
			lightingShader->setVec3("xyz", cam->getPos());

			lightingShader->setVec3("xyzLht.ambient", 0.2f, 0.2f, 0.2f);
			lightingShader->setVec3("xyzLht.diffuse", 0.5f, 0.5f, 0.5f);
			lightingShader->setVec3("xyzLht.specular", 1.0f, 1.0f, 1.0f);



			lightingShader->setMat4("proj", proj);
			lightingShader->setMat4("view", cam->getViewM4());

			model = glm::mat4(1.0f);
			model = glm::translate(model, positions[i]);
			lightingShader->setMat4("model", model);
			glDrawElements(GL_TRIANGLES, cubex->getISize(), GL_UNSIGNED_INT, 0);
						
		}

		//modelos
		shader->use();
		shader->setVec3("xyz", lightPos);
		shader->setVec3("xyzColor", lightColor);
		shader->setVec3("xyzView", cam->getPos());

		shader->setMat4("proj", proj);
		shader->setMat4("view", cam->getViewM4());

		evaScale += 0.015;
		evaY -= 0.005;
		model = glm::mat4(1.0f);
		model = translate(model, glm::vec3(0.0f, evaY, -40.0f));	
		model = glm::scale(model, glm::vec3(evaScale));
		shader->setMat4("model", model);
		obj1->Draw(shader);


		for (u32 i = 0; i < frogPositions.size(); ++i) {
			frogPositions[i].y -= 0.40;
			if (frogPositions[i].y <= -10.0) {
				frogPositions[i].y = 30.0;
			}

			model = glm::mat4(1.0f);
			model = translate(model, frogPositions[i]);
			model = rotate(model, currentFrame*8, glm::vec3(0.0f, 0.50f, 0.0f));
			model = glm::scale(model, glm::vec3(0.15));
			shader->setMat4("model", model);
			obj2->Draw(shader);

		}		

		model = glm::mat4(1.0f);
		model = translate(model, glm::vec3(-30.0f, -10.0f, -30.0f));
		model = glm::scale(model, glm::vec3(30.0));
		shader->setMat4("model", model);
		obj3->Draw(shader);

		//luz
		lightCubeShader->useProgram();
		lightCubeShader->setMat4("proj", proj);
		lightCubeShader->setMat4("view", cam->getViewM4());
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.05));
		lightCubeShader->setMat4("model", model);

		glBindVertexArray(lightCubeVao);
		glDrawElements(GL_TRIANGLES, cubex->getISize(), GL_UNSIGNED_INT, 0);		

		glfwSwapBuffers(window);
		glfwPollEvents();
	};

	glDeleteVertexArrays(1, &cubeVao);
	glDeleteVertexArrays(1, &lightCubeVao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	delete lightingShader;
	delete lightCubeShader;
	delete cubex;
	delete cam;
	delete shader;
	delete obj1;
	delete obj3;
	delete obj2;

	return 0;
}