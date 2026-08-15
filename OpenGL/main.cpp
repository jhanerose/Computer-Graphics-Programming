// =============================================================================
//  Computer Graphics Programming
//  Lab Activity: Drawing Your First Triangle
//  University of Perpetual Help System DALTA - College of Computer Studies
//
//  Goal: draw one red triangle using a VAO, a VBO, and a pair of GLSL shaders.
//  Build (Visual Studio Community, x64):
//    Linker -> Input:  opengl32.lib  glfw3.lib  glew32.lib
//    Copy glew32.dll into the same folder as the built .exe
//    Do NOT define GLEW_STATIC. The static glew32s.lib is built against a
//    different C runtime than glfw3.lib, which causes an MSVCRT/LIBCMT clash.
//
//  This project must contain exactly ONE .cpp file with a main() function.
// =============================================================================
#define GLEW_STATIC
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Window size
const GLint WIDTH = 800, HEIGHT = 600;

// OpenGL never hands you the object itself. It hands you an ID and keeps
// the real object in graphics memory. These three IDs are all we need today.
GLuint VAO, VBO, shader;

// -----------------------------------------------------------------------------
//  VERTEX SHADER  (Stage 2 of the pipeline - runs once per vertex)
//  >>> TASK 3 lives here: the two 0.4 values below are your scale. <<<
// -----------------------------------------------------------------------------
//  Stored as a plain string because the driver compiles GLSL at runtime.
//  Each line needs \n so GLSL sees separate lines, and a trailing backslash
//  so C++ continues the string onto the next line.
//
//  Note: your editor will NOT underline mistakes in here. Every error in this
//  string only shows up when the program runs.
// -----------------------------------------------------------------------------
static const char* vShader = "                                  \n\
#version 330                                                     \n\
                                                                 \n\
layout (location = 0) in vec3 pos;                               \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    gl_Position = vec4(0.75 * pos.x, 0.75 * pos.y, pos.z, 1.0);    \n\
}";

// -----------------------------------------------------------------------------
//  FRAGMENT SHADER  (Stage 8 of the pipeline - runs once per fragment)
//  >>> TASK 1 lives here: the vec4 below is your colour. <<<
// -----------------------------------------------------------------------------
//  Unlike gl_Position, this output is one you name yourself. A fragment shader
//  with a single output is assumed to be the pixel colour, whatever you call it.
// -----------------------------------------------------------------------------
static const char* fShader = "                                   \n\
#version 330                                                     \n\
                                                                 \n\
out vec4 colour;                                                 \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    colour = vec4(0.111, 0.222, 1.0, 1.0);                           \n\
}";

// -----------------------------------------------------------------------------
//  CreateTriangle - Stage 1: Vertex Specification
// -----------------------------------------------------------------------------
void CreateTriangle()
{
    // Nine floats, read three at a time as x, y, z.
    // The screen runs -1 to 1 on both axes, with 0,0 in the middle.
    // >>> TASK 2 lives here: replace these three corners with your own. <<<
    GLfloat vertices[] = {
         0.2f,  0.7f, 0.0f,    // bottom left
        -0.6f, -0.3f, 0.0f,    // bottom right
         0.8f, -0.8f, 0.0f,    // top middle 
    };

    // Step 1 & 2: make a VAO and bind it. Everything after this is
    // recorded against whichever VAO is currently bound.
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Step 3 & 4: make a VBO and bind it to the array-buffer target.
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Step 5: copy the data into graphics memory.
    // GL_STATIC_DRAW = set once, drawn many times.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Step 6: describe the layout.
    //   0         -> matches layout (location = 0) in the vertex shader
    //   3         -> three values per vertex (x, y, z)
    //   GL_FLOAT  -> the values are GLfloats
    //   GL_FALSE  -> do not normalise them
    //   0         -> stride: data is tightly packed, no gaps to skip
    //   0         -> offset: start at the very first value
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Step 7: switch attribute 0 on, so the shader actually receives it.
    glEnableVertexAttribArray(0);

    // Step 8: unbind, so the next object cannot write into this VAO.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------
//  AddShader - builds ONE shader and attaches it to the program
// -----------------------------------------------------------------------------
void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
    // Create an empty shader of the requested type.
    GLuint theShader = glCreateShader(shaderType);

    // glShaderSource expects arrays, so we build one-element arrays.
    const GLchar* theCode[1];
    theCode[0] = shaderCode;

    GLint codeLength[1];
    codeLength[0] = (GLint)strlen(shaderCode);   // this is why we need string.h

    // Hand the GLSL text to the shader object, then compile it.
    glShaderSource(theShader, 1, theCode, codeLength);
    glCompileShader(theShader);

    // --- Error check: did THIS ONE shader compile? ---
    // Shader functions (glGetShaderiv / glGetShaderInfoLog) report on a single
    // shader. Program functions report on the linked whole. Mixing them up is
    // the most common copy-paste bug in this lesson.
    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(theShader, sizeof(eLog), NULL, eLog);
        printf("Error compiling the %d shader: '%s'\n", shaderType, eLog);
        return;
    }

    // Only attach once it has compiled cleanly.
    glAttachShader(theProgram, theShader);
}

// -----------------------------------------------------------------------------
//  CompileShaders - builds the shader program and links both shaders into it
// -----------------------------------------------------------------------------
void CompileShaders()
{
    // The program object comes first, and it starts empty.
    shader = glCreateProgram();

    if (!shader)
    {
        // A failed program leaves you with ID 0, and every later call quietly
        // does nothing. Catching it here saves you a blank screen with no clue.
        printf("Error creating shader program!\n");
        return;
    }

    AddShader(shader, vShader, GL_VERTEX_SHADER);
    AddShader(shader, fShader, GL_FRAGMENT_SHADER);

    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    // Link: creates the executables on the graphics card and joins the shaders.
    // This is where mismatches show up, e.g. a vertex output with no matching
    // fragment input.
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
        printf("Error linking program: '%s'\n", eLog);
        return;
    }

    // Validate: is the linked program valid for the context we are running in?
    // Optional, but it catches problems that otherwise appear as a blank window.
    glValidateProgram(shader);
    glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
        printf("Error validating program: '%s'\n", eLog);
        return;
    }
}

// -----------------------------------------------------------------------------
//  main
// -----------------------------------------------------------------------------
int main()
{
    // --- Window setup (from the previous lesson) ---
    if (!glfwInit())
    {
        printf("GLFW initialisation failed!\n");
        glfwTerminate();
        return 1;
    }

    // Ask for OpenGL 3.3, core profile (no deprecated features).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "My First Triangle", NULL, NULL);
    if (!mainWindow)
    {
        printf("GLFW window creation failed!\n");
        glfwTerminate();
        return 1;
    }

    // Get the real framebuffer size (not the same as window size on hi-dpi screens).
    int bufferWidth, bufferHeight;
    glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

    glfwMakeContextCurrent(mainWindow);

    // GLEW must be initialised AFTER a context exists.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("GLEW initialisation failed!\n");
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, bufferWidth, bufferHeight);

    // --- Setup: happens ONCE, before the loop ---
    CreateTriangle();
    CompileShaders();

    // --- Render loop: happens EVERY FRAME ---
    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        // Black background, so the red triangle stands out.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        glBindVertexArray(VAO);

        // GL_TRIANGLES -> read the vertices in threes and fill them in
        // 0            -> start at the first vertex
        // 3            -> draw three vertices
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);

        glUseProgram(0);   // 0 means "no shader"

        glfwSwapBuffers(mainWindow);
    }

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
    return 0;
}

// =============================================================================
//  LAB ACTIVITY - MAKE IT YOURS
//  SADICON, JHANE ROSE U. -> 24-2038-129
// =============================================================================
//
//  TASK 1 - YOUR COLOUR                         [edit fShader, near the top]
//    Take the last three digits of your student number.
//    Divide each by 9 to get a value from 0.0 to 1.0. Those are R, G and B.
//    If a digit is 0, use 0.2 instead, or you will draw black on black.
//    Last three digits: 1, 2, 9
//    R = 1 / 9 = 0.111
//    G = 2 / 9 = 0.222
//    B = 9 / 9 = 1.0
//    Resulting colour vector: colour = vec4(0.111, 0.222, 1.0, 1.0);
//
//  TASK 2 - YOUR TRIANGLE                  [edit vertices[] in CreateTriangle]
//    Replace the three corners with your own. Rules:
//      - every value stays between -1.0 and 1.0
//      - no two corners may share the same x value
//      - the shape must not be symmetric
//    Sketch it on the -1..1 grid first, then check the render matches.
//    My three unique, asymmetrical vertices inside the -1.0 to 1.0 grid:
//    Vertex 1 (Top):           0.2f,  0.7f, 0.0f
//    Vertex 2 (Bottom Left):  -0.6f, -0.3f, 0.0f
//    Vertex 3 (Bottom Right):  0.8f, -0.8f, 0.0f
//
//  TASK 3 - YOUR SCALE                          [edit vShader, near the top]
//    Birth month: 9 (September)
//    Multiplier calculation: 9 / 12 = 0.75
//    Prediction: The triangle will take up 75% of the screen space, making 
//    it significantly larger than the default 0.4 size provided in the lab.
//
//  TASK 4 - YOUR BUG                                             [anywhere]
//    Pick one line to delete or mistype. Write down the error you expect,
//    then cause it and compare against what the console actually printed.
//    Line deleted: glAttachShader(theProgram, theShader); inside AddShader.
//    Expected error: The shaders will compile, but the screen will be black 
//    because they were never attached to the main shader program.
//    Actual console log: "Error validating program: Validation Failed: No 
//    vertex shader attached and no fragment shader attached."
//
//  STRETCH GOAL
//    Added second triangle mirrored across the y-axis.
//    How many floats does the array hold now, and what changes in
//    glDrawArrays? Do the shaders need to change at all?
//    Mirrored Triangle Array Size: 18 floats (6 total vertices).
//    glDrawArrays Changes: The vertex count parameter changed from 3 to 6 
//    -> glDrawArrays(GL_TRIANGLES, 0, 6);
//    Shader Changes: None. The shaders do not need to change because the 
//    vertex shader processes one vertex at a time regardless of the total 
//    count, and the fragment shader handles the resulting pixels.
// =============================================================================
