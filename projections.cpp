// =============================================================================
//  Computer Graphics Programming
//  SADICON, JHANE ROSE U. BSIT-GD III 24-2038-129
//  LAB ACTIVITY C - PROJECTIONS
//  University of Perpetual Help System DALTA - College of Computer Studies
//
//  THIS FILE ALREADY BUILDS AND RUNS. Compile it first. You should get a
//  pyramid with real depth, no longer distorted. Fix any build problem before
//  you change anything.
//
//  THERE ARE NO TODO MARKERS AND NO CONSTANTS TO SWAP. Every value you need to
//  change is a plain number sitting inside a function.
//
//  IF YOU HAVE ALREADY FINISHED ACTIVITY B, you may start from your own
//  finished file instead of this one and keep your own pyramid.
//
//  The brief and the hand-in list are at the BOTTOM of this file.
//
//  ---------------------------------------------------------------------------
//  WHAT THIS PROGRAM DOES RIGHT NOW
//    - A PROJECTION MATRIX, built once with glm::perspective.
//    - A second uniform, so the shader receives it.
//    - The shader now combines THREE matrices:
//          gl_Position = projection * view * model * vec4(pos, 1.0);
//      Read that RIGHT TO LEFT: local -> world -> view -> clip.
//    - The pyramid is pushed back along -z so it sits inside the frustum.
//
//  WHAT YOU SHOULD SEE
//    A pyramid with real depth. The distortion from the last two lessons is
//    gone. Push it further away and it now gets SMALLER, which it never did
//    before.
//
//  NOTE ON THE VIEW MATRIX
//    We have no camera yet, so view stays as the identity, which is the same
//    as saying the camera sits at the origin looking down -z. It is included
//    now so the shader does not have to change again when the camera arrives.
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

GLuint VAO, VBO, IBO, shader, uniformModel, uniformProjection, uniformView;

float curAngle = 0.0f;

// -----------------------------------------------------------------------------
//  VERTEX SHADER
// -----------------------------------------------------------------------------
//  THE ORDER OF THE MATRICES IS NOT A STYLE CHOICE.
//  Matrix multiplication is not commutative. Written as
//      projection * view * model * vec4(pos, 1.0)
//  the RIGHTMOST operation happens first, so a vertex travels:
//      local space -> (model) -> world -> (view) -> view space
//                  -> (projection) -> clip space
//  Write it in any other order and you get nonsense.
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
uniform mat4 view;                                               \n\
uniform mat4 projection;                                         \n\
                                                                 \n\
void main()                                                      \n\
{                                                                \n\
    gl_Position = projection * view * model * vec4(pos, 1.0);    \n\
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
    // Four corners, each written ONCE.
    // Vertex 1 is the new one: same height as the other two base corners, but
    // pushed back along z. This is the first time we use the third dimension.
    GLfloat vertices[] = {
        -1.0f, -1.0f,  0.0f,    // 0  base, back left
         0.0f, -1.0f,  1.0f,    // 1  base, forward     <-- NEW
         1.0f, -1.0f,  0.0f,    // 2  base, back right
         0.0f,  1.0f,  0.0f     // 3  apex
    };

    // Four triangles built from those four corners.
    // 12 index values, but only 4 vertices stored. Without indexing you would
    // have to write out 12 full vertices, repeating most corners three times.
    //
    // Sanity check for any closed solid: every edge is shared by exactly two
    // faces. These four triples satisfy that. If your shape has a hole, this
    // is the check to run.
    unsigned int indices[] = {
        0, 3, 1,    // side
        1, 3, 2,    // side
        2, 3, 0,    // side
        0, 1, 2     // base   (all three y values are -1)
    };

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
    uniformView = glGetUniformLocation(shader, "view");
    uniformProjection = glGetUniformLocation(shader, "projection");
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

    GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Activity C: Projections | SADICON_24-2038-129", NULL, NULL);
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

    // Cap the frame rate to the monitor refresh, so the rotation runs at the
    // same speed on every machine. Without it the loop is uncapped and the
    // spin can be far faster on one computer than another.
    // glfwSwapInterval(1);

    glViewport(0, 0, bufferWidth, bufferHeight);

    CreateObject();
    CompileShaders();

    // Built ONCE, outside the loop. Field of view and window size rarely
    // change, unlike the model matrix which is rebuilt every frame.
    //
    // TWO SILENT FAILURES TO AVOID:
    //  1. fov must be in RADIANS. GLM has required this since 0.9.6. Passing
    //     45.0f directly is a valid number, so nothing errors, you just get a
    //     bizarre view. Reuse toRadians.
    //  2. aspect must use the FRAMEBUFFER size, not the window size. On a
    //     hi-dpi screen they differ. Note the casts: integer division would
    //     give 1 and the scene would come out subtly squashed.
    glm::mat4 projection = glm::perspective(40.0f * toRadians,
        (GLfloat)bufferWidth / (GLfloat)bufferHeight,
        1.0f,
        6.0f);

    // No camera yet, so the view matrix does nothing. Same as a camera parked
    // at the origin looking down -z.
    glm::mat4 view = glm::mat4(1.0f);

    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwPollEvents();

        // Rotation speed. Lower is slower.
        // If it still spins too fast, the cause is usually that vsync is off,
        // so the loop runs as fast as the machine allows and the speed differs
        // from computer to computer. Uncomment the glfwSwapInterval line near
        // glewInit to cap it to the monitor refresh rate.
        curAngle += 0.02f;
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

        // Push it AWAY from the camera. With a perspective projection the
        // object must sit beyond the near plane, or it is clipped and you
        // see nothing. Try changing -2.5f to -6.0f: it should now get
        // visibly SMALLER, which it never did in the earlier lessons.
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -6.0f));
        model = glm::rotate(model, curAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // GL_TRIANGLES     same mode as before
        // 12               the count of INDICES, not vertices
        // GL_UNSIGNED_INT  must match the type of indices[]
        // 0                offset into the index buffer
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

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
//       fov        = 30 + (d8 x 5) degrees    -> 40 degrees
//       far plane  = 5 + d7                   -> 6.0f
//
//
//  TASK 1 - YOUR VIEW
//  ---------------------------------------------------------------------------
//    One call in this program builds the projection matrix. Find it, and set
//    it to use your field of view and your far plane.
//
//    Leave the near plane and the aspect ratio alone. But look carefully at
//    how the field of view is written before you change it. Something is
//    applied to it that you must not remove.
//
//
//  TASK 2 - PROVE DISTANCE NOW MATTERS
//  ---------------------------------------------------------------------------
//    Find the line that pushes the shape away from the camera. Change that
//    distance from -2.5f to -6.0f. Screenshot both.
//
//
//  TASK 3 - CLIP IT ON PURPOSE
//  ---------------------------------------------------------------------------
//    Set your far plane to 2.0f while the shape sits at -2.5f. Run it.
//    Then put the far plane back and set the NEAR plane to 3.0f instead.
//    Run it again. Put everything back when done.
//
//
//  TASK 4 - FIELD OF VIEW EXTREMES
//  ---------------------------------------------------------------------------
//    Try a fov of 20, then 100. Look at the edges of the shape in each.
//
//
//  ANSWER IN A COMMENT IN THIS FILE
//  ---------------------------------------------------------------------------
//    a) What happened in Task 2, and why did the same change do nothing back
//       in the indexed draws activity? Use the word frustum.
//       -> Changing the translation from -2.5f to -6.0f pushed the pyramid
//          farther into the viewing frustum, making it appear smaller because
//          perspective projection scales objects down as their distance from
//          the camera increases. In the earlier indexed draws activity, there
//          was no projection applied, so changing the Z position did not affect
//          the object's screen-space size.
//
//    b) Task 3 made the shape vanish twice. Explain each one separately.
//       -> The first disappearance happened when the far plane was set to 2.0f
//          while the object was at -2.5f, placing it beyond the far clipping
//          boundary. The second happened when the near plane was raised to
//          3.0f while the object remained at -2.5f, placing it outside the
//          near clipping boundary.
//
//    c) Describe a fov of 20 next to one of 100. Which would you pick for a
//       first-person game, and why?
//       -> A FOV of 20° gives a narrow view, making the scene appear more
//          zoomed in. A FOV of 100° gives a much wider view, showing more of
//          the surroundings and making the scene appear more zoomed out. For
//          a first-person game, I would choose around 90° to 100° because it
//          provides better spatial awareness and a wider view of the scene.
//
//    d) Remove the conversion from the field of view. It still compiles and
//       runs. What do you see, and what does that tell you about the kinds of
//       bug you should expect in graphics work?
//       -> The program still compiles and runs, but the view becomes extremely
//          distorted because 40.0f is interpreted as radians instead of degrees.
//          This shows that graphics programs can have logical or unit-related
//          bugs that do not produce compiler errors but still cause incorrect
//          visual results.
//
//    e) The shader line reads projection * view * model * vec4(pos, 1.0).
//       Explain what each matrix does, reading right to left.
//       -> `model`: Transforms local vertex coordinates into world space through
//          translation, rotation, and scaling.
//          `view`: Transforms world-space coordinates into camera/view space
//          relative to the observer.
//          `projection`: Transforms view-space coordinates into clip space,
//          applying perspective and establishing the viewing frustum.
//
//    f) The view matrix in this program does nothing at all. Why is it here?
//       -> The view matrix is initialized as an identity matrix (`glm::mat4(1.0f)`)
//          so it currently does not change the object's coordinates. It is included
//          to provide the view transformation stage of the graphics pipeline and
//          can later be replaced with a camera transformation when a movable
//          camera is introduced.
// 
// =============================================================================
//  WHAT TO HAND IN
// =============================================================================
//    1. Your student number and your two seed values.
//    2. This file, finished.
//    3. Four screenshots: your view at -2.5f, the same at -6.0f, and your two
//       field-of-view extremes from Task 4.
//    4. Your written answers to a through f.
//
// =============================================================================
//  IF SOMETHING BREAKS
// =============================================================================
//    Nothing on screen at all
//      - Is your object between the near and far planes? Both clip.
//      - Did you set the far plane smaller than the object distance?
//
//    Wildly distorted view
//      - Look again at how the field of view is written. Something is applied
//        to it that you must not remove.
//
//    Scene looks subtly squashed
//      - The aspect ratio must use the framebuffer size, and both values must
//        be cast, or integer division gives 1.
// =============================================================================