

#include "GLTools.h"
#include "GLMatrixStack.h"
#include "GLFrame.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLGeometryTransform.h"


#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager shaderManager;
//模型视图矩阵
GLMatrixStack modelViewMatrix;
//投影矩阵
GLMatrixStack projectionMatrix;
GLFrame cameraFrame;
GLFrame objectFrame;
//视图椎体
GLFrustum viewFrustum;

//容器类
GLBatch pointBatch;
GLBatch lineBatch;
GLBatch lineStripBatch;
GLBatch lineLoopBatch;
GLBatch triangleBatch;
GLBatch triangleStripBatch;
GLBatch triangleFanBatch;

//几何变化管道
GLGeometryTransform transformPipeline;

GLfloat vBlue[] = {0.0f,0.0f,1.0f,1.0f};
GLfloat vBlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };

int nStep = 0;

//窗口大小改变时接受新的宽度和高度，其中0，0代表窗口中视图的左下角坐标，w，h代表像素
void ChangeSize(int w,int h)
{
    glViewport(0, 0, w, h);
    //视图椎体变量设置观察者位置
    viewFrustum.SetPerspective(35.0f, float(w) / float(h), 1.0f, 500.0f);
    //从视图椎体中加载投影矩阵
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    //模型视图矩阵设置为单元矩阵
    modelViewMatrix.LoadIdentity();
}

//为程序做一次性的设置
void SetupRC()
{
    //设置背景色
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f );
    //初始化着色管理器
    shaderManager.InitializeStockShaders();
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //设置变化管线的矩阵堆栈
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
    //设置camera位置
    cameraFrame.MoveForward(-15.0f);
    
    //定义三个点
    GLfloat vCoast[9] = {
        3,3,0,
        0,3,0,
        3,0,0
    };
    
    //用点的形式
    pointBatch.Begin(GL_POINTS, 3);
    pointBatch.CopyVertexData3f(vCoast);
    pointBatch.End();
    
    //通过线的形式
    lineBatch.Begin(GL_LINES, 3);
    lineBatch.CopyVertexData3f(vCoast);
    lineBatch.End();
    
    //通过线带的形式
    lineStripBatch.Begin(GL_LINE_STRIP, 3);
    lineStripBatch.CopyVertexData3f(vCoast);
    lineStripBatch.End();
    
    //通过线环的形式
    lineLoopBatch.Begin(GL_LINE_LOOP, 3);
    lineLoopBatch.CopyVertexData3f(vCoast);
    lineLoopBatch.End();
    
//    通过三角形创建金字塔
    GLfloat vPyramid[12][3] = {
        -2.0f, 0.0f, -2.0f,
        2.0f, 0.0f, -2.0f,
        0.0f, 4.0f, 0.0f,

        2.0f, 0.0f, -2.0f,
        2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,

        2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,

        -2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, -2.0f,
        0.0f, 4.0f, 0.0f
        
    };
    
    //GL_TRIANGLES 每3个顶点定义一个新的三角形
    triangleBatch.Begin(GL_TRIANGLES, 12);
    triangleBatch.CopyVertexData3f(vPyramid);
    triangleBatch.End();
    
    
    // 三角形扇形--六边形
    GLfloat vPoints[100][3];
    int nVerts = 0;
    //半径
    GLfloat r = 3.0f;
    //原点(x,y,z) = (0,0,0);
    vPoints[nVerts][0] = 0.0f;
    vPoints[nVerts][1] = 0.0f;
    vPoints[nVerts][2] = 0.0f;
    
    
    //M3D_2PI 就是2Pi 的意思，就一个圆的意思。 绘制圆形
    for(GLfloat angle = 0; angle < M3D_2PI; angle += M3D_2PI / 6.0f) {
        
        //数组下标自增（每自增1次就表示一个顶点）
        nVerts++;
        /*
         弧长=半径*角度,这里的角度是弧度制,不是平时的角度制
         既然知道了cos值,那么角度=arccos,求一个反三角函数就行了
         */
        //x点坐标 cos(angle) * 半径
        vPoints[nVerts][0] = float(cos(angle)) * r;
        //y点坐标 sin(angle) * 半径
        vPoints[nVerts][1] = float(sin(angle)) * r;
        //z点的坐标
        vPoints[nVerts][2] = -0.5f;
    }
    
    // 结束扇形 前面一共绘制7个顶点（包括圆心）
    //添加闭合的终点
    nVerts++;
    vPoints[nVerts][0] = r;
    vPoints[nVerts][1] = 0;
    vPoints[nVerts][2] = 0.0f;
    
    // 加载！
    //GL_TRIANGLE_FAN 以一个圆心为中心呈扇形排列，共用相邻顶点的一组三角形
    triangleFanBatch.Begin(GL_TRIANGLE_FAN, 8);
    triangleFanBatch.CopyVertexData3f(vPoints);
    triangleFanBatch.End();
    
    //三角形条带，一个小环或圆柱段
    //顶点下标
    int iCounter = 0;
    //半径
    GLfloat radius = 3.0f;
    //从0度~360度，以0.3弧度为步长
    for(GLfloat angle = 0.0f; angle <= (2.0f*M3D_PI); angle += 0.3f)
    {
        //或许圆形的顶点的X,Y
        GLfloat x = radius * sin(angle);
        GLfloat y = radius * cos(angle);
        
        //绘制2个三角形（他们的x,y顶点一样，只是z点不一样）
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = -0.5;
        iCounter++;
        
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = 0.5;
        iCounter++;
    }
    
    // 关闭循环
    printf("三角形带的顶点数：%d\n",iCounter);
    //结束循环，在循环位置生成2个三角形
    vPoints[iCounter][0] = vPoints[0][0];
    vPoints[iCounter][1] = vPoints[0][1];
    vPoints[iCounter][2] = -0.5;
    iCounter++;
    
    vPoints[iCounter][0] = vPoints[1][0];
    vPoints[iCounter][1] = vPoints[1][1];
    vPoints[iCounter][2] = 0.5;
    iCounter++;
    
    // GL_TRIANGLE_STRIP 共用一个条带（strip）上的顶点的一组三角形
    triangleStripBatch.Begin(GL_TRIANGLE_STRIP, iCounter);
    triangleStripBatch.CopyVertexData3f(vPoints);
    triangleStripBatch.End();
}


void DrawWireFramedBatch(GLBatch* pBatch)
{
    //1.上色
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vBlue);
    pBatch->Draw();
    
    //2.设置线相关内容
    
    // 偏移深度，在同一位置要绘制填充和边线，会产生z冲突，所以要偏移
    glPolygonOffset(-1.0f, -1.0f);
    //根据glPolygonOffset的设置，启用线的深度偏移
    glEnable(GL_POLYGON_OFFSET_LINE);
    
    //启用锯齿
    glEnable(GL_LINE_SMOOTH);
    //启用颜色混合
    glEnable(GL_BLEND);
    //混合方式，使用alpha，1-alpha方式混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //多边形模式，正反面都用画线模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.5f);
    
    //画线
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vBlack);
    pBatch->Draw();
    
    //3.复原状态
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

//开始渲染
void RenderScene(void) {
    //1.清除一个或一组特定的缓冲区
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    
    //2.记录当前模型视图矩阵，当前的值是单元矩阵，所以入栈的是单元矩阵。目的是本次改变后，调用出栈方法，将模型视图矩阵重新复制成单元矩阵，下面有对应PopMatrix函数调用
    modelViewMatrix.PushMatrix();
    
    //3.将camera frame转换成矩阵
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    //modelViewMatrix矩阵乘以camera矩阵栈，相乘的结果存放到modelViewMatrix
    modelViewMatrix.MultMatrix(mCamera);
    
    //4.将object frame转换成矩阵，objectFrame在键盘上下左右时设置的值
    M3DMatrix44f mObjectFrame;
    objectFrame.GetMatrix(mObjectFrame);
    //modelViewMatrix矩阵乘以object矩阵栈，相乘的结果存放到modelViewMatrix
    modelViewMatrix.MultMatrix(mObjectFrame);
    
    //5.使用平面着色器，参数2是从变换管线transformPipeline中获取MVP（ModelViewProjectionMatrix）矩阵
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vBlack);
    
    //6
    switch (nStep) {
        case 0:
            glPointSize(4.0f);
            pointBatch.Draw();
            glPointSize(1.0f);
            break;
        case 1:
            glLineWidth(2.0f);
            lineBatch.Draw();
            glLineWidth(1.0f);
            break;

        case 2:
            glLineWidth(2.0f);
            lineStripBatch.Draw();
            glLineWidth(1.0f);
            break;

        case 3:
            glLineWidth(2.0f);
            lineLoopBatch.Draw();
            glLineWidth(1.0f);
            break;

        case 4:
            DrawWireFramedBatch(&triangleBatch);
            break;

        case 5:
            DrawWireFramedBatch(&triangleStripBatch);
            break;

        case 6:
            DrawWireFramedBatch(&triangleFanBatch);
            break;
            
    }
    
    //出栈，将modelViewMatrix还原成单元矩阵
    modelViewMatrix.PopMatrix();
    
    //将在后台缓冲区进行渲染，然后在结束时交换到前台
    glutSwapBuffers();
}

void SpeciaKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) {
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 1.0f, 0, 0);
    }
    if (key == GLUT_KEY_DOWN) {
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 1.0f, 0, 0);
    }
    if (key == GLUT_KEY_LEFT) {
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 0, 1.0f, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 0, 1.0f, 0);
    }
    
    glutPostRedisplay();
}

void KeyPressFunc(unsigned char key, int x, int y)
{
    if (key == 32) {
        nStep++;
        if (nStep > 6) {
            nStep = 0;
        }
    }
    
    switch (nStep) {
        case 0:
            glutSetWindowTitle("GL_POINTS");
            break;
        
        case 1:
            glutSetWindowTitle("GL_LINES");
            break;

        case 2:
            glutSetWindowTitle("GL_LINE_STRIP");
            break;

        case 3:
            glutSetWindowTitle("GL_LINE_LOOP");
            break;

        case 4:
            glutSetWindowTitle("GL_TRIANGLES");
            break;

        case 5:
            glutSetWindowTitle("GL_TRIANGLE_STRIP");
            break;

        case 6:
            glutSetWindowTitle("GL_TRIANGLE_FAN");
            break;
            
    }
    glutPostRedisplay();
}

int main(int argc,char* argv[]) {

    //设置当前工作目录，针对MAC OS X
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库
    glutInit(&argc, argv);
    /*初始化双缓冲窗口，其中标志GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指
     双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区
     */
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    
    //GLUT窗口大小
    glutInitWindowSize(800,600);
    glutCreateWindow("GL_POINTS");
    
    //注册回调函数
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutKeyboardFunc(KeyPressFunc);
    glutSpecialFunc(SpeciaKeys);

    //驱动程序的初始化中没有出现任何问题
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }

    //调用SetupRC
    SetupRC();
    glutMainLoop();
    return 0;
}
