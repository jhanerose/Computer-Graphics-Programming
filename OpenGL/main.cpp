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
//    define GLEW_STATIC.
//
//  This project must contain exactly ONE .cpp file with a main() function.
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

// Window size
const GLint WIDTH = 800, HEIGHT = 600;

// glm::rotate wants RADIANS. We think in degrees, so we convert.
const float toRadians = 3.14159265f / 180.0f;

// OpenGL hands back IDs, not objects. These four are all we need.
GLuint VAO, VBO, shader, uniformModel, uniformYShift;

// --- Animation state: plain C++ bookkeeping, never touches OpenGL directly ---

// Sliding left and right
bool  direction = true;      // true = moving right
float triOffset = 0.0f;      // current position
float triMaxOffset = 0.40f;   // Adapted to reference animation value
float triIncrement = 0.0005f;   // distance added each frame

// Spinning
float curAngle = 0.000f;     // Starting degrees
float spinIncrement = 0.05f; // Adapted to reference animation value

// Pulsing
bool  sizeDirection = true;     // true = growing
float curSize = 0.4f;           // Adapted to reference animation value
float maxSize = 0.8f;           // Adapted to reference animation value
float minSize = 0.1f;           // Adapted to reference animation value

// -----------------------------------------------------------------------------
//  VERTEX SHADER  (Stage 2 - runs once per vertex)
// -----------------------------------------------------------------------------
static const char* vShader = "                                   \n\
#version 460                                                     \n\
                                                                 \n\
layout (location = 0) in vec3 pos;                               \n\
                                                                 \n\
uniform mat4 model;                                              \n\
uniform float yShift;                                            \n\
                                                                 \n\
out vec3 vertexColor;                                            \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    gl_Position = model * vec4(pos.x, pos.y + yShift, pos.z, 1.0); \n\
    // Map local coordinates (-0.5 to 0.5) to RGB colors (0.0 to 1.0) \n\
    vertexColor = vec3(pos.x + 0.5, pos.y + 0.5, 0.8);           \n\
}";

// -----------------------------------------------------------------------------
//  FRAGMENT SHADER  (Stage 8 - runs once per fragment)
// -----------------------------------------------------------------------------
static const char* fShader = "                                   \n\
#version 460                                                     \n\
                                                                 \n\
in vec3 vertexColor;                                             \n\
out vec4 colour;                                                 \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    // Apply the interpolated gradient color instead of solid red  \n\
    colour = vec4(vertexColor, 1.0);                             \n\
}";

// -----------------------------------------------------------------------------
//  CreateTriangle - Stage 1: Vertex Specification
// -----------------------------------------------------------------------------
void CreateTriangle()
{
    // >>> ACTIVITY TASK 1 lives here: replace these with your polygon. <<<
    const int sides = 8; // octagon
    GLfloat vertices[sides * 3 * 3];

    // one triangle per side, all from the centre
    float step = 360.0f / sides;
    int idx = 0;

    for (int i = 0; i < sides; ++i)
    {
        // Calculate trigonometry once per loop iteration
        float currentX = 0.5f * cos(i * step * toRadians);
        float currentY = 0.5f * sin(i * step * toRadians);
        float nextX = 0.5f * cos((i + 1) * step * toRadians);
        float nextY = 0.5f * sin((i + 1) * step * toRadians);

        // 1. Centre point
        vertices[idx++] = 0.0f;
        vertices[idx++] = 0.0f;
        vertices[idx++] = 0.0f;

        // 2. Rim point i
        vertices[idx++] = currentX;
        vertices[idx++] = currentY;
        vertices[idx++] = 0.0f;

        // 3. Rim point i+1 
        vertices[idx++] = nextX;
        vertices[idx++] = nextY;
        vertices[idx++] = 0.0f;
    }

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

    // Find the uniform by NAME. This must happen AFTER linking.
    uniformModel = glGetUniformLocation(shader, "model");
    uniformYShift = glGetUniformLocation(shader, "yShift");
}

// -----------------------------------------------------------------------------
// Callback function for dynamic window resizing
// -----------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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

    // Set the resize callback right after creating the window
    glfwSetFramebufferSizeCallback(mainWindow, framebuffer_size_callback);

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
        if (direction) { triOffset += triIncrement; }
        else { triOffset -= triIncrement; }

        if (std::abs(triOffset) >= triMaxOffset)
        {
            direction = !direction;   // flip the flag in one line
        }

        // Spin using your specific ID increment
        curAngle += spinIncrement;
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

        // TASK 2: Send your yShift value across (-0.05f) every frame
        glUniform1f(uniformYShift, -0.05f);

        // Start from the identity matrix: the do-nothing transform.
        glm::mat4 model = glm::mat4(1.0f);

        // ORDER MATTERS. Written top to bottom, these apply in reverse.
        model = glm::translate(model, glm::vec3(triOffset, 0.0f, 0.0f));
        model = glm::rotate(model, curAngle * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(curSize, curSize, 1.0f));

        // Hand the matrix to the shader.
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 24);
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
//        2  4  2  0  3  8  1  2  9          <- SADICON, JHANE ROSE U.: 24-2038-129
//       d1 d2 d3 d4 d5 d6 d7 d8 d9
//
//    Now read off your six values:
//
//       sides    = 4 + (9 mod 5)          -> 8
//       travel   = 0.3 + 2 * 0.05         -> 0.40
//       spin     = (d7 + 1) / 1000        -> 0.002
//       minSize  = 0.1 + d1 * 0.05        -> 0.20
//       maxSize  = minSize + 0.3          -> 0.50
//       yShift   = (d2 - 5) / 20          -0.05
//
//    Every digit 0-9 gives a safe value. There are no special cases.
