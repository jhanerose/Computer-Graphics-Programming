// =============================================================================
//  Computer Graphics Programming
//  SADICON, JHANE ROSE U. BSIT-GD III 24-2038-129
//  LAB ACTIVITY B - INDEXED DRAWS
//  University of Perpetual Help System DALTA - College of Computer Studies
//
//  THIS FILE ALREADY BUILDS AND RUNS. Compile it first. You should get a
//  3-sided pyramid spinning like a spinning top. Fix any build problem before
//  you change anything.
//
//  THERE ARE NO TODO MARKERS AND NO CONSTANTS TO SWAP. Every value you need to
//  change is a plain number sitting inside a function. Working out WHICH
//  number does WHAT, and where it lives, is part of the task.
//
//  The brief and the hand-in list are at the BOTTOM of this file.
//
//  ---------------------------------------------------------------------------
//  WHAT THIS PROGRAM DOES RIGHT NOW
//    - A FOURTH vertex, pushed back along z. First use of the third dimension.
//    - An INDEX BUFFER (IBO), so the four corners are defined once and then
//      referred to by number.
//    - glDrawElements instead of glDrawArrays.
//    - DEPTH TESTING, without which the wrong face draws on top.
//    - Rotation restored, around the Y axis so the depth is visible.
//
//  WHAT YOU SHOULD SEE
//    A pyramid spinning like a spinning top, with faces correctly hiding each
//    other. It will still look oddly flat and distorted. That is expected:
//    there is no projection matrix yet. Lesson 5 fixes it.
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

GLuint VAO, VBO, IBO, shader, uniformModel;

float curAngle = 0.0f;

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
    vCol = clamp(pos, 0.0, 1.0);                                 \n\
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

void CreateObject()
{
    // TASK 1: Dynamic 4-sided pyramid based on seed values (sides = 4, apexHeight = 0.7f)
    const int sides = 4;
    const float apexHeight = 0.7f;

    // 4 base vertices + 1 apex vertex = 5 vertices * 3 components = 15 floats
    GLfloat vertices[15];
    for (int i = 0; i < sides; ++i)
    {
        float a = i * (360.0f / sides) * toRadians;
        vertices[i * 3 + 0] = 0.5f * cosf(a);
        vertices[i * 3 + 1] = -1.0f;
        vertices[i * 3 + 2] = 0.5f * sinf(a);
    }
    // Apex goes last
    vertices[sides * 3 + 0] = 0.0f;
    vertices[sides * 3 + 1] = apexHeight;
    vertices[sides * 3 + 2] = 0.0f;

    // 4 side triangles (12 indices) + 2 base fan triangles (6 indices) = 18 indices
    unsigned int indices[18];
    int indexCount = 0;
    int apexIndex = sides;

    // SIDES: for each i: i, apex, (i + 1) % sides
    for (int i = 0; i < sides; ++i)
    {
        indices[indexCount++] = i;
        indices[indexCount++] = apexIndex;
        indices[indexCount++] = (i + 1) % sides;
    }

    // BASE FAN: for i from 1 to sides-2: 0, i + 1, i
    for (int i = 1; i <= sides - 2; ++i)
    {
        indices[indexCount++] = 0;
        indices[indexCount++] = i + 1;
        indices[indexCount++] = i;
    }

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // The index buffer. Same three steps as the VBO, only the TARGET
    // changes: GL_ELEMENT_ARRAY_BUFFER instead of GL_ARRAY_BUFFER.
    // Because it is bound while the VAO is bound, the VAO remembers it.
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ORDER MATTERS HERE.
    // Unbind the VAO FIRST, then the element array buffer. Do it the other way
    // round and the VAO records that the index buffer was detached, so the
    // draw call finds no indices and nothing appears, with no error message.
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Activity B: Indexed Draws | SADICON_24-2038-129", NULL, NULL);
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

    // Without this, OpenGL draws triangles in the order you listed them and
    // has no idea which are nearer. The wrong face wins. See also the extra
    // GL_DEPTH_BUFFER_BIT in glClear below: you need BOTH lines, not one.
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, bufferWidth, bufferHeight);

    CreateObject();
    CompileShaders();

    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        curAngle += 0.1f;
        if (curAngle >= 360.0f)
        {
            curAngle -= 360.0f;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // The vertical bar is bitwise OR. Each buffer flag is a different
        // power of two, so OR-ing them clears both in one call.
        // Enable the depth test but forget this and last frame's depth values
        // survive, which makes the object flicker or vanish. 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 model = glm::mat4(1.0f);

        // Rotate around the Y AXIS, not Z. Picture a pole running straight
        // up the screen with the pyramid spinning around it, like a
        // spinning top. Rotating around z would show you nothing new.
        model = glm::rotate(model, curAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // GL_TRIANGLES     same mode as before
        // 18               the count of INDICES for 4-sided pyramid
        // GL_UNSIGNED_INT  must match the type of indices[]
        // 0                offset into the index buffer
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
//  STEP 0 - YOUR SEED VALUES
//  ---------------------------------------------------------------------------
//    Ignore the dashes in your student number and number every digit left to
//    right.
//
//        2  4  2  0  3  8  1  2  9          <- 24-2038-129
//       d1 d2 d3 d4 d5 d6 d7 d8 d9
//       
//       base sides   = 3 + (d9 mod 4)        -> 4
//       apex height  = 0.5 + (d1 x 0.1)      -> 0.7
//
//       
//  TASK 1 - YOUR PYRAMID
//  ---------------------------------------------------------------------------
//    The shape is currently a 3-sided pyramid with its four corners typed out
//    by hand. Replace it with one whose base has YOUR number of sides, built
//    in a loop rather than typed out.
//
//    Work out for yourself which parts of the program have to change. There
//    are THREE separate places. Miss any one of them and the shape will be
//    wrong or invisible, with no error message. Finding all three is the task.
//
//    Ring point i, for i from 0 to sides-1:
//        float a = i * (360.0f / sides) * toRadians;
//        x = 0.5f * cosf(a);   y = -1.0f;   z = 0.5f * sinf(a);
//
//    The apex goes LAST in the array, at (0.0f, YOUR apex height, 0.0f), so
//    its index is the number of sides.
//
//    Indices come in two groups:
//        SIDES     for each i:  i, apex, (i + 1) % sides
//        BASE FAN  for i from 1 to sides-2:  0, i + 1, i
//
//    Check your numbers against this table before you build:
//
//        base sides   vertices   triangles   indices
//             3           4          4          12
//             4           5          6          18
//             5           6          8          24
//             6           7         10          30
//
//
//  TASK 2 - BREAK THE DEPTH BUFFER
//  ---------------------------------------------------------------------------
//    Two separate lines in this program make faces hide each other correctly.
//    Find them both.
//      - Disable the first one, run it, note what happens, put it back.
//      - Now disable the second one instead, run it, note what happens.
//    The two failures look nothing alike. Put both back when done.
//
//
//  ANSWER IN A COMMENT IN THIS FILE
//  ---------------------------------------------------------------------------
//   a) Name the three places you had to change in Task 1, and say what each
//      one does.
//      -> 1. Vertex & apex parameters (`sides`, `apexHeight`) define the
//         pyramid's number of sides and apex height.
//         2. Loop-based vertex/index generation inside `CreateObject()`
//         generates the pyramid's vertices and triangle indices dynamically.
//         3. The draw index count (`18`) inside `glDrawElements()` tells OpenGL
//         how many indices to use when drawing the pyramid.
//
//   b) How many vertices would you have written out WITHOUT indexing?
//      -> 18 vertices (6 triangles * 3 vertices per triangle).
//
//   c) In a closed solid every edge is shared by exactly two faces. Check two
//      of your edges by hand and show your working.
//      -> Apex edge (Index 4 to 0) is shared by side triangles (0, 4, 1)
//         and (3, 4, 0). Base edge (Vertex 0 to 1) is shared by side triangle
//         (0, 4, 1) and base fan triangle (0, 1, 2).
//
//   d) Describe both failures from Task 2 and explain why they look
//      different.
//      -> Disabling depth testing prevents OpenGL from correctly determining
//         which surface is closer to the camera, causing incorrect face
//         overlap. Omitting `GL_DEPTH_BUFFER_BIT` leaves stale depth values
//         from previous frames, which can cause incorrect depth comparisons
//         and visual artifacts.
//
//   e) Change the rotation axis to (0, 0, 1). Why can you no longer tell the
//      object is 3D? Put it back.
//      -> Rotating around (0, 0, 1) only rotates the object within the XY
//         plane because the Z-values remain unchanged. This removes the
//         rotation-based depth cues that normally make the object appear 3D.
//
//   f) The pyramid still looks oddly flat from the side. What is missing?
//      -> A projection matrix is missing to correct for the window's aspect
//         ratio (e.g., 800x600). Coordinates currently map directly to the
//         window borders, causing the shape to stretch, squash, and distort
//         as it rotates because 1.0 extends all the way to any edge.
// =============================================================================
//  WHAT TO HAND IN
// =============================================================================
//    1. Your student number and your two seed values.
//    2. This file, finished.
//    3. Three screenshots: your pyramid from an angle where the extra base
//       sides are visible, and one of each failure from Task 2.
//    4. Your written answers to a through f.
//
// =============================================================================
//  STRETCH GOAL
// =============================================================================
//    Turn your shape into a closed BIPYRAMID: add a second apex below the ring
//    at (0.0f, -2.0f, 0.0f) and the triangles joining it to the ring. The base
//    fan is no longer needed, because the base is no longer visible.
//    How many vertices, triangles and indices now? Prove it is still closed.
//
// =============================================================================
//  IF SOMETHING BREAKS
// =============================================================================
//    Nothing on screen at all
//      - Did you update the draw count? It is one of the three places.
//      - Did you unbind the VAO BEFORE the element array buffer?
//
//    Only part of the shape appears
//      - The draw count is still the old number.
//
//    Shape has a hole or a missing face
//      - Recount your index triples. Every edge needs exactly two faces.
//      - Base fan loop runs i from 1 to sides-2, not 0 to sides.
//
//    Crash, or a stray face going nowhere
//      - You left the % off (i + 1) % sides in the ring loop.
// =============================================================================