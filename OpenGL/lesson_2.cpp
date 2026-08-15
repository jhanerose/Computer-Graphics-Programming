// =============================================================================
//  Computer Graphics Programming
//  Lesson: Uniforms and Transformations
//  University of Perpetual Help System DALTA - College of Computer Studies
//
//  This is the finished lesson code and the STARTING POINT for the lab
//  activity at the bottom of this file. Build it, run it, make sure you get a
//  red triangle sliding, spinning and pulsing. Only then start the activity.
//
//  Build (Visual Studio Community, x64):
//    Linker -> Input:  opengl32.lib  glfw3.lib  glew32.lib
//    Copy glew32.dll into the same folder as the built .exe
//    GLM 1.0.3 is header-only: download it from github.com/g-truc/glm and
//    drop the inner glm folder into your include directory. Nothing to link.
//    Do NOT define GLEW_STATIC.
//
//  This project must contain exactly ONE .cpp file with a main() function.
// =============================================================================

#include <stdio.h>
#include <string.h>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Window size
const GLint WIDTH = 800, HEIGHT = 600;

// glm::rotate wants RADIANS. We think in degrees, so we convert.
// Write your angle in degrees, then multiply by this.
const float toRadians = 3.14159265f / 180.0f;

// OpenGL hands back IDs, not objects. These four are all we need.
GLuint VAO, VBO, shader, uniformModel;

// --- Animation state: plain C++ bookkeeping, never touches OpenGL directly ---

// Sliding left and right
bool  direction = true;      // true = moving right
float triOffset = 0.0f;      // current position
float triMaxOffset = 0.7f;      // turn around at +/- this
float triIncrement = 0.0005f;   // distance added each frame

// Spinning
float curAngle = 0.0f;          // degrees

// Pulsing
bool  sizeDirection = true;     // true = growing
float curSize = 0.4f;
float maxSize = 0.8f;
float minSize = 0.1f;

// -----------------------------------------------------------------------------
//  VERTEX SHADER  (Stage 2 - runs once per vertex)
// -----------------------------------------------------------------------------
//  Notice how little is here. There are no hard-coded numbers left. Everything
//  about position, angle and size arrives inside the model matrix, so this
//  shader never has to change again.
//
//  >>> ACTIVITY TASK 2 lives here: add your uniform float yShift. <<<
// -----------------------------------------------------------------------------
static const char* vShader = "                                  \n\
#version 460                                                     \n\
                                                                 \n\
layout (location = 0) in vec3 pos;                               \n\
                                                                 \n\
uniform mat4 model;                                              \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    gl_Position = model * vec4(pos, 1.0);                        \n\
}";

// -----------------------------------------------------------------------------
//  FRAGMENT SHADER  (Stage 8 - runs once per fragment)
// -----------------------------------------------------------------------------
static const char* fShader = "                                   \n\
#version 460                                                     \n\
                                                                 \n\
out vec4 colour;                                                 \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    colour = vec4(1.0, 0.0, 0.0, 1.0);                           \n\
}";

// -----------------------------------------------------------------------------
//  CreateTriangle - Stage 1: Vertex Specification
// -----------------------------------------------------------------------------
void CreateTriangle()
{
    // Three points, read three floats at a time as x, y, z.
    // The screen runs -1 to 1 on both axes, with 0,0 in the middle.
    //
    // >>> ACTIVITY TASK 1 lives here: replace these with your polygon. <<<
    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f,   // bottom left
         1.0f, -1.0f, 0.0f,   // bottom right
         0.0f,  1.0f, 0.0f    // top middle
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //   0        -> matches layout (location = 0) in the vertex shader
    //   3        -> three values per vertex (x, y, z)
    //   GL_FLOAT -> the values are GLfloats
    //   GL_FALSE -> do not normalise them
    //   0        -> stride: tightly packed, no gaps to skip
    //   0        -> offset: start at the very first value
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------
//  AddShader - builds ONE shader and attaches it to the program
// -----------------------------------------------------------------------------
void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
    GLuint theShader = glCreateShader(shaderType);

    const GLchar* theCode[1];
    theCode[0] = shaderCode;

    GLint codeLength[1];
    codeLength[0] = (GLint)strlen(shaderCode);

    glShaderSource(theShader, 1, theCode, codeLength);
    glCompileShader(theShader);

    // Shader functions report on ONE shader. Program functions report on the
    // linked whole. Mixing them up is the classic copy-paste bug.
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

// -----------------------------------------------------------------------------
//  CompileShaders - builds the program, links it, and finds the uniform
// -----------------------------------------------------------------------------
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

    // Find the uniform by NAME. This must happen AFTER linking, because before
    // that the variable does not have a location yet.
    // A typo here returns -1 and then fails silently. Spell it exactly.
    uniformModel = glGetUniformLocation(shader, "model");
}

// -----------------------------------------------------------------------------
//  main
// -----------------------------------------------------------------------------
int main()
{
    if (!glfwInit())
    {
        printf("GLFW initialisation failed!\n");
        glfwTerminate();
        return 1;
    }

    // Ask for OpenGL 4.6 core profile, the latest version of the spec.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Uniforms and Transformations", NULL, NULL);
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

    // --- Setup: happens ONCE, before the loop ---
    CreateTriangle();
    CompileShaders();

    // --- Render loop: happens EVERY FRAME ---
    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        // ---- Update the animation values ----

        // Slide left and right, turning around at the limits.
        // abs() collapses both bounds into a single test.
        if (direction) { triOffset += triIncrement; }
        else { triOffset -= triIncrement; }

        if (std::abs(triOffset) >= triMaxOffset)
        {
            direction = !direction;   // flip the flag in one line
        }

        // Spin. The wrap at 360 is not required, it just stops the
        // number growing without limit if the program runs for hours.
        curAngle += 0.05f;
        if (curAngle >= 360.0f)
        {
            curAngle -= 360.0f;
        }

        // Pulse. Either bound flips the direction, so one if handles both.
        if (sizeDirection) { curSize += 0.0001f; }
        else { curSize -= 0.0001f; }

        if (curSize >= maxSize || curSize <= minSize)
        {
            sizeDirection = !sizeDirection;
        }

        // ---- Draw ----
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // Start from the identity matrix: the do-nothing transform.
        // NOTE the 1.0f. Since GLM 0.9.9, including the current 1.0.3,
        // a bare glm::mat4 model; is UNINITIALISED and full of garbage.
        // Your shape will vanish or render as nonsense, with no error.
        glm::mat4 model = glm::mat4(1.0f);

        // ORDER MATTERS. Written top to bottom, these apply in reverse,
        // so scale is written LAST and therefore happens first.
        // Swap any two lines and the motion changes completely.
        model = glm::translate(model, glm::vec3(triOffset, 0.0f, 0.0f));
        model = glm::rotate(model, curAngle * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(curSize, curSize, 1.0f));

        // Hand the matrix to the shader.
        //   uniformModel -> the location we looked up earlier
        //   1            -> we are sending one matrix
        //   GL_FALSE     -> do not transpose it
        //   value_ptr    -> a pointer to the raw floats inside the GLM object
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
//  LAB ACTIVITY - YOUR OWN SHAPE, MOVING
// =============================================================================
//  The program above is your STARTING POINT. Get it running first.
//  Every value below comes from the digits of YOUR student number, so no two
//  submissions should look alike. Write your six numbers down before you code.
//
//  -------------------------------------------------------------------------
//  STEP 0 - NUMBER YOUR DIGITS
//  -------------------------------------------------------------------------
//    Ignore the dashes and number every digit left to right.
//
//        2  0  6  7  5  9  4  7  6          <- example: 20-6759-476
//       d1 d2 d3 d4 d5 d6 d7 d8 d9
//
//    Now read off your six values:
//
//       sides    = 4 + (d9 mod 5)        example -> 5
//       travel   = 0.3 + d8 * 0.05       example -> 0.65
//       spin     = (d7 + 1) / 1000       example -> 0.005
//       minSize  = 0.1 + d1 * 0.05       example -> 0.20
//       maxSize  = minSize + 0.3         example -> 0.50
//       yShift   = (d2 - 5) / 20         example -> -0.25
//
//    Every digit 0-9 gives a safe value. There are no special cases.
//
//  -------------------------------------------------------------------------
//  TASK 1 - YOUR SHAPE                     [edit vertices[] in CreateTriangle]
//  -------------------------------------------------------------------------
//       4 -> square     12 vertices        7 -> heptagon   21 vertices
//       5 -> pentagon   15 vertices        8 -> octagon    24 vertices
//       6 -> hexagon    18 vertices
//
//    Build it as a triangle fan: one triangle per side, every triangle
//    sharing the centre point (0, 0).
//
//       float step = 360.0f / sides;
//
//       // rim point i
//       x = 0.5f * cos(i * step * toRadians);
//       y = 0.5f * sin(i * step * toRadians);
//
//       // triangle i is: (0,0), point i, point i+1
//       // the LAST triangle wraps back to point 0
//
//    Sketch it on the -1 to 1 grid on paper first.
//    Remember to update the count in glDrawArrays to sides * 3.
//
//  -------------------------------------------------------------------------
//  TASK 2 - YOUR UNIFORM                          [edit vShader, then main]
//  -------------------------------------------------------------------------
//    Add uniform float yShift to the vertex shader and apply it to pos.y.
//    Do all three steps yourself:
//       1. declare it in the shader
//       2. look it up with glGetUniformLocation, at the end of CompileShaders
//       3. set it with glUniform1f, after glUseProgram
//
//    If nothing moves, check your spelling. A name mismatch returns -1 and
//    then fails silently with no error message at all.
//
//  -------------------------------------------------------------------------
//  TASK 3 - YOUR MOTION
//  -------------------------------------------------------------------------
//    Set triMaxOffset to your travel value.
//    Set the curAngle increment to your spin value.
//
//  -------------------------------------------------------------------------
//  TASK 4 - YOUR PULSE
//  -------------------------------------------------------------------------
//    Set minSize and maxSize to your two values.
//    Make sure curSize starts somewhere between them.
//
//  -------------------------------------------------------------------------
//  PART 2 - PROVE YOU UNDERSTAND ORDER
//  -------------------------------------------------------------------------
//    Reorder the three glm lines in the render loop and run it four times.
//    Write down what you EXPECT before each run, then what happened.
//
//       A   translate, rotate, scale
//       B   rotate, translate, scale
//       C   scale, translate, rotate
//       D   your own choice
//
//    Then answer in writing: which order gave the motion you actually
//    wanted, and why did the other three fail?
//
//    Tip: a hexagon looks similar at many angles, so the spin can be hard
//    to see. Scale one axis differently, or watch a single vertex.
//
//  HAND IN
//    Your student number, your six seed values, four screenshots,
//    your predictions, and your written answer.
// =============================================================================