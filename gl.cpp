const int glWindowWidth = 512 + 256;
const int glWindowHeight = 256 + 128;

GLFWwindow* setupGLWindow() {
  // Initialize GLFW
  if (!glfwInit()) {
    return NULL;
  }

  // Create a windowed mode window and its OpenGL context
  GLFWwindow* window = glfwCreateWindow(glWindowWidth, glWindowHeight, "", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return NULL;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  if (glewInit() != GLEW_OK) {
    return NULL;
  }

  // Check if OpenGL ES 3.0 is supported
  if (!GLEW_VERSION_3_0) {
    // OpenGL ES 3.0 is not supported
    return NULL;
  }

  return window;
}

const char* gVertexShader = R"(#version 300 es
  layout(location = 0) in vec2 vPosition;
  layout(location = 1) in vec2 vTexCoord;
  out vec2 texCoord;
  void main() {
    gl_Position = vec4(vPosition, 0.0, 1.0);
    texCoord = vTexCoord;
  }
)";

const char* gFragmentShader = R"(#version 300 es
  precision mediump float;
  in vec2 texCoord;
  uniform sampler2D uTexture;
  out vec4 fragColor;
  void main() {
    fragColor = texture(uTexture, texCoord);
  }
)";

GLuint loadShader(GLenum type, const char *shaderSrc) {
  GLuint shader;
  GLint compiled;
  shader = glCreateShader(type);
  GLenum error = glGetError();

  if (shader == 0) {
    return 0;
  }
  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      char *infoLog = (char *)malloc(sizeof(char) * infoLen);
      glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
      free(infoLog);
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

GLuint createProgram(const char *vertexSource, const char *fragmentSource) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
  if (!vertexShader) {
    return 0;
  }
  GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (!fragmentShader) {
    return 0;
  }
  GLuint program = glCreateProgram();
  if (program == 0) {
    return 0;
  }
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram  (program);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    GLint bufLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
    if (bufLength) {
      char *buf = (char *)malloc(bufLength);
      if (buf) {
        glGetProgramInfoLog(program, bufLength, NULL, buf);
        free(buf);
      }
    }
    glDeleteProgram(program);
    program = 0;
  }
  return program;
}

GLuint gvPositionHandle;
GLuint gIndexBuffer;
GLint gvTexCoordHandle;
GLint guTextureHandle;
GLuint gTexture;

const GLushort textureIndices[] = {
  0, 1, 2,
  2, 3, 0,
};

const GLfloat textureVertices[] = {
  0.5f, 0.5f, -0.5f, 0.0f, 0.0f,
  0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
  -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
};

bool setupGLProgram() {
  GLuint gProgram = createProgram(gVertexShader, gFragmentShader);
  if (!gProgram) {
    return false;
  }

  gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
  gvTexCoordHandle = glGetAttribLocation(gProgram, "vTexCoord");
  guTextureHandle = glGetUniformLocation(gProgram, "uTexture");

  glUseProgram(gProgram);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glGenBuffers(1, &gIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

  // Apply the texture indices attribute array
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(textureIndices), textureIndices, GL_STATIC_DRAW);

  // Update the texture coordinate attribute array
  glVertexAttribPointer(gvTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), textureVertices + 3);

  // Enable attribute array for texture coordinates
  glEnableVertexAttribArray(gvTexCoordHandle);

  // Set the active texture unit
  glActiveTexture(GL_TEXTURE0);

  // Generate texture
  glGenTextures(1, &gTexture);
 
  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, gTexture);

  // Set texture wrap mode
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Set the texture uniform
  glUniform1i(guTextureHandle, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

  glEnableVertexAttribArray(gvPositionHandle);

  return true;
}

void drawTexture(unsigned char *rgb, float x, float y, int width, int height) {
  // Texture vertices moved by X and Y
  const GLfloat vertices[] = {
    x, y + 1.0f, -0.5f, 0.0f, 0.0f,
    x, y - 1.0f, -0.5f, 1.0f, 0.0f,
    x - 1.0f, y - 1.0f, -0.5f, 1.0f, 1.0f,
    x - 1.0f, y + 1.0f, -0.5f, 0.0f, 1.0f,
  };

  // Update the vertex attribute array
  glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices);
  
  // Draw texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}
