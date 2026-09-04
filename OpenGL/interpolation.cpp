// =============================================================================
//  Computer Graphics Programming
//  SADICON, JHANE ROSE U. BSIT-GD III 24-2038-129
//  LAB ACTIVITY A - INTERPOLATION
//  University of Perpetual Help System DALTA - College of Computer Studies
//
//  THIS FILE ALREADY BUILDS AND RUNS. Compile it first. You should get a still
//  triangle with colours blending across it. Fix any build problem before you
//  change anything.
//
//  THERE ARE NO TODO MARKERS AND NO CONSTANTS TO SWAP. The value you need to
//  change is a plain line of code inside one of the shaders. Working out which
//  line, and what it does, is part of the task. Read before you edit.
//
//  The brief and the hand-in list are at the BOTTOM of this file.
//
//  ---------------------------------------------------------------------------
//  WHAT THIS PROGRAM DOES RIGHT NOW
//    - The translate call is REMOVED and the scale is FIXED at 0.4f, so the
//      triangle sits still. You have not lost your animation, we are just
//      taking the movement out of the way to look at colour.
//    - The vertex shader now OUTPUTS a value.
//    - The fragment shader now takes that value IN and uses it.
//
//  WHAT YOU SHOULD SEE
//    A still triangle with colours blending smoothly across it, including an
//    orange band that you never wrote anywhere.
//
//  Build (Visual Studio Community, x64):
//    Linker -> Input:  opengl32.lib  glfw3.lib  glew32.lib
//    Copy glew32.dll next to the built .exe
//    GLM 1.0.3 is header-only. Only ONE .cpp may define main().
// =============================================================================

#define GLEW_STATIC
#include <stdio.h>
#include <string.h>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const GLint WIDTH = 800, HEIGHT = 600;
const float toRadians = 3.14159265f / 180.0f;

GLuint VAO, VBO, shader, uniformModel;

// -----------------------------------------------------------------------------
//  VERTEX SHADER
// -----------------------------------------------------------------------------
//  The new line is the "out". Anything you write to an out here is passed on
//  to the fragment shader, and it arrives INTERPOLATED. You do not switch that
//  on, it is simply what happens on the way.
//
//  WHY THE clamp
//    Positions run -1.0 to 1.0.  Colour channels run 0.0 to 1.0.
//    The bottom left corner is (-1.0, -1.0, 0.0), which as a colour is minus
//    red, minus green, no blue. That renders as a dead black area.
//    clamp pushes anything below 0.0 up to 0.0, so the shape stays colourful.
//
//    TRY IT: delete the clamp, run it, look at the bottom left, then put it
//    back. The reason is much clearer once you have seen the black.
// -----------------------------------------------------------------------------
static const char* vShader = "                                  \n\
#version 460                                                     \n\
                                                                 \n\
layout (location = 0) in vec3 pos;                               \n\
                                                                 \n\
out vec3 vCol;                                                   \n\
                                                                 \n\
uniform mat4 model;                                              \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    gl_Position = model * vec4(pos, 1.0);                        \n\
    vCol = clamp(pos.yzx, 0.0, 1.0);                             \n\
}";

// -----------------------------------------------------------------------------
//  FRAGMENT SHADER
// -----------------------------------------------------------------------------
//  The "in" must match the vertex shader's "out" in BOTH name and type.
//  A mismatch is a link error, which your link status check will report.
//
//  vCol here is NOT the value the vertex shader wrote. It is the blend
//  calculated for this one fragment, weighted by how close it sits to each
//  of the three corners.
// -----------------------------------------------------------------------------
static const char* fShader = "                                   \n\
#version 460                                                     \n\
                                                                 \n\
in vec3 vCol;                                                    \n\
                                                                 \n\
out vec4 colour;                                                 \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    colour = vec4(vCol, 1.0);                                    \n\
}";

void CreateTriangle()
{
    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
    GLuint theShader = glCreateShader(shaderType);

    const GLchar* theCode[1];
    theCode[0] = shaderCode;

    GLint codeLength[1];
    codeLength[0] = (GLint)strlen(shaderCode);

    glShaderSource(theShader, 1, theCode, codeLength);
    glCompileShader(theShader);

    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(theShader, sizeof(eLog), NULL, eLog);
        printf("Error compiling the %d shader: '%s'\n", shaderType, eLog);
        return;
    }

    glAttachShader(theProgram, theShader);
}

void CompileShaders()
{
    shader = glCreateProgram();

    if (!shader)
    {
        printf("Error creating shader program!\n");
        return;
    }

    AddShader(shader, vShader, GL_VERTEX_SHADER);
    AddShader(shader, fShader, GL_FRAGMENT_SHADER);

    GLint result = 0;
    GLchar eLog[1024] = { 0 };

    // A name or type mismatch between out vCol and in vCol shows up HERE.
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
        printf("Error linking program: '%s'\n", eLog);
        return;
    }

    glValidateProgram(shader);
    glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
        printf("Error validating program: '%s'\n", eLog);
        return;
    }

    uniformModel = glGetUniformLocation(shader, "model");
}

int main()
{
    if (!glfwInit())
    {
        printf("GLFW initialisation failed!\n");
        glfwTerminate();
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Activity A: Interpolation | SADICON_24-2038-129", NULL, NULL);
    if (!mainWindow)
    {
        printf("GLFW window creation failed!\n");
        glfwTerminate();
        return 1;
    }

    int bufferWidth, bufferHeight;
    glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

    glfwMakeContextCurrent(mainWindow);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("GLEW initialisation failed!\n");
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, bufferWidth, bufferHeight);

    CreateTriangle();
    CompileShaders();

    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // Still, at a fixed size. No translate, no rotate, no pulse.
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glUseProgram(0);

        glfwSwapBuffers(mainWindow);
    }

    glfwDestroyWindow(mainWindow);
    glfwTerminate();
    return 0;
}

// =============================================================================
//  THE BRIEF
// =============================================================================
//
//  STEP 0 - YOUR SEED VALUE
//  ---------------------------------------------------------------------------
//    Ignore the dashes in your student number and number every digit left to
//    right.
//
//        2  4  2  0  3  8  1  2  9          <- 24-2038-129
//       d1 d2 d3 d4 d5 d6 d7 d8 d9
//
//       colour order = d2 mod 3             -> 1
//
//
//  TASK 1 - YOUR COLOUR SCHEME
//  ---------------------------------------------------------------------------
//    Somewhere in this file a single line decides what colour each vertex
//    gets, by reusing its position. Find it. Then change which position axis
//    feeds which colour channel, to match your colour order:
//
//       0  ->  x to red,  y to green,  z to blue
//       1  ->  y to red,  z to green,  x to blue     (Mode 1 swizzle order .yzx)
//       2  ->  z to red,  x to green,  y to blue
//
//
//  TASK 2 - BREAK IT ON PURPOSE
//  ---------------------------------------------------------------------------
//    Remove the clamp from that line and run it. Look carefully at the shape.
//    Then put the clamp back.
//    a. vCol = clamp(pos, 0.0, 1.0); default, the original line
//    b. vCol = pos.yzx; remove the clamp, and swizzle to the colour order mode 1
//    c. vCol = clamp(pos.yzx, 0.0, 1.0);  put the clamp back, swizzled to the colour order
//
//
//  TASK 3 - BREAK THE LINK
//  ---------------------------------------------------------------------------
//    Rename the variable in the VERTEX shader only, leaving the fragment
//    shader untouched. Build it. Read the console. Then put it back.
//    a. Renamed vCol to vColBroken in the vertex shader, leaving the
//       fragment shader unchanged, then restored vCol after the error.
//
//  ANSWER IN A COMMENT IN THIS FILE
//  ---------------------------------------------------------------------------
//
// a) Which corner of your triangle is brightest, and why that one?
//    The top corner (0.0, 1.0, 0.0) and the bottom-right corner (1.0, -1.0, 0.0)
//    are equally the brightest. Under Mode 1 (.yzx), the top corner's y-value
//    of 1.0 feeds the Red channel, while the bottom-right corner's x-value of
//    1.0 feeds the Blue channel. Both reach the maximum colour channel intensity
//    value of 1.0.
//
// b) Pick a point midway along one edge. Nobody wrote a colour for it.
//    Where did its colour come from?
//    Its colour was determined by linear interpolation (barycentric interpolation)
//    performed automatically by the GPU rasterizer. The hardware smoothly blends
//    the output colour values from the surrounding vertices based on the
//    fragment's position between them.
//
// c) Which part went black without the clamp, and why that part?
//    The bottom-left corner and its surrounding region went dead black.
//    Its geometry position coordinates are (-1.0, -1.0, 0.0). Without the
//    clamp function, these negative values pass directly into the colour
//    values, producing no visible intensity for those channels.
//
// d) What error did Task 3 produce, and which of the checks already in this
//    program reported it?
//    It produced a shader Program Linking Error due to an interface mismatch.
//    This was caught and reported by the glLinkProgram() check inside the
//    CompileShaders() function, which verifies that vertex shader 'out' variables
//    match fragment shader 'in' variables by name and type.
//
// e) The vertex shader runs 3 times. The fragment shader runs thousands of
//    times. Explain in your own words how one feeds the other.
//    The vertex shader runs 3 times to calculate positions and custom colors for
//    the triangle's corners. The GPU then breaks the triangle down into thousands
//    of pixel-sized pieces (fragments). During this stage, the GPU smoothly blends
//    the corner colour data together and feeds a unique, blended colour value into
//    the fragment shader for every single pixel it draws on your screen.
//
// =============================================================================
//  WHAT TO HAND IN
// =============================================================================
//    1. Your student number and your colour order value.
//    2. This file, finished.
//    3. Two screenshots: your colour scheme, and the black-corner version
//       from Task 2.
//    4. Your written answers to a through e.
//
// =============================================================================
//  IF SOMETHING BREAKS
// =============================================================================
//    Shape is entirely black
//      - Your channel order may map an axis that is negative everywhere.
//        Check the clamp is still there.
//
//    Build fails with a linking error mentioning a variable name
//      - The vertex shader out and the fragment shader in must match in BOTH
//        name and type. That is Task 3 working as intended.
//
//    Colours look the same as before you started
//      - Check you edited the vertex shader, not the fragment shader.
// =============================================================================