// System Headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Standard Headers
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <GLTools.h>    // OpenGL toolkit
#include <GLMatrixStack.h>
#include <GLFrame.h>
#include <GLFrustum.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#include <math.h>
#include <stdlib.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);//回调函数原型声明
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

GLFrame             viewFrame;
GLFrustum           viewFrustum;
GLTriangleBatch     sphereBatch;
GLBatch             cubeBatch;
GLMatrixStack       modelViewMatrix;
GLMatrixStack       projectionMatrix;
GLGeometryTransform transformPipeline;
GLuint              cubeTexture;
GLint               reflectionShader;
GLint               skyBoxShader;

GLint               locMVPReflect, locMVReflect, locNormalReflect, locInvertedCamera;
GLint                locMVPSkyBox;

// Six sides of a cube map
const char *szCubeFaces[6] = { "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/pos_x.tga",
    "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/neg_x.tga",
    "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/pos_y.tga",
    "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/neg_y.tga",
    "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/pos_z.tga",
    "/Users/amon/workspace/opengl/SB5/Src/Chapter07/CubeMapped/neg_z.tga" };

GLenum  cube[6] = {  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

//////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering
// context.
void SetupRC() {
    GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFormat;
    int i;

    // Cull backs of polygons
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);

    // Set up texture maps
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load Cube Map images
    for(i = 0; i < 6; i++) {
        // Load this texture map
        pBytes = gltReadTGABits(szCubeFaces[i], &iWidth, &iHeight, &iComponents, &eFormat);
        glTexImage2D(cube[i], 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
        free(pBytes);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    viewFrame.MoveForward(-4.0f);
    gltMakeSphere(sphereBatch, 1.0f, 52, 26);
    gltMakeCube(cubeBatch, 20.0f);

    reflectionShader = gltLoadShaderPairWithAttributes("Reflection.vp", "Reflection.fp", 2,
                                                       GLT_ATTRIBUTE_VERTEX, "vVertex",
                                                       GLT_ATTRIBUTE_NORMAL, "vNormal");

    locMVPReflect = glGetUniformLocation(reflectionShader, "mvpMatrix");
    locMVReflect = glGetUniformLocation(reflectionShader, "mvMatrix");
    locNormalReflect = glGetUniformLocation(reflectionShader, "normalMatrix");
    locInvertedCamera = glGetUniformLocation(reflectionShader, "mInverseCamera");
    
    skyBoxShader = gltLoadShaderPairWithAttributes("SkyBox.vp", "SkyBox.fp", 2,
                                                   GLT_ATTRIBUTE_VERTEX, "vVertex",
                                                   GLT_ATTRIBUTE_NORMAL, "vNormal");
    locMVPSkyBox = glGetUniformLocation(skyBoxShader, "mvpMatrix");
}

void ShutdownRC(void) {
    glDeleteTextures(1, &cubeTexture);
}

// Called to draw scene
void RenderScene(void) {
    // Clear the window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    M3DMatrix44f mCamera;
    M3DMatrix44f mCameraRotOnly;
    M3DMatrix44f mInverseCamera;

    viewFrame.GetCameraMatrix(mCamera, false);
    viewFrame.GetCameraMatrix(mCameraRotOnly, true);
    m3dInvertMatrix44(mInverseCamera, mCameraRotOnly);

    modelViewMatrix.PushMatrix();
    // Draw the sphere
    modelViewMatrix.MultMatrix(mCamera);
    glUseProgram(reflectionShader);
    glUniformMatrix4fv(locMVPReflect, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
    glUniformMatrix4fv(locMVReflect, 1, GL_FALSE, transformPipeline.GetModelViewMatrix());
    glUniformMatrix3fv(locNormalReflect, 1, GL_FALSE, transformPipeline.GetNormalMatrix());
    glUniformMatrix4fv(locInvertedCamera, 1, GL_FALSE, mInverseCamera);

    glEnable(GL_CULL_FACE);
    sphereBatch.Draw();
    glDisable(GL_CULL_FACE);
    modelViewMatrix.PopMatrix();

    modelViewMatrix.PushMatrix();
    modelViewMatrix.MultMatrix(mCameraRotOnly);
    glUseProgram(skyBoxShader);
    glUniformMatrix4fv(locMVPSkyBox, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
    cubeBatch.Draw();
    modelViewMatrix.PopMatrix();

    // Do the buffer Swap
//    glutSwapBuffers();
}


int main(int argc, char * argv[]) {
    //初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif
    //创建一个窗口对象
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "FirstGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    //通知GLFW将我们窗口的上下文设置为当前线程的主上下文
    glfwMakeContextCurrent(window);
    //对窗口注册一个回调函数,每当窗口改变大小，GLFW会调用这个函数并填充相应的参数供你处理
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //初始化GLAD用来管理OpenGL的函数指针

    glewInit();
    SetupRC();
    //渲染循环
    while(!glfwWindowShouldClose(window)) {
        // 输入
        processInput(window);
        
        // 检查并调用事件，交换缓冲
        glfwSwapBuffers(window);//检查触发事件
        glfwPollEvents();    //交换颜色缓冲
    }
    ShutdownRC();
    //释放/删除之前的分配的所有资源
    glfwTerminate();
    return EXIT_SUCCESS;
}

//输入控制，检查用户是否按下了返回键(Esc)
void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// 当用户改变窗口的大小的时候，视口也应该被调整
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // 注意：对于视网膜(Retina)显示屏，width和height都会明显比原输入值更高一点。
    glViewport(0, 0, width, height);
}
