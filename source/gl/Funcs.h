GL_FUNC(void, glAttachShader, (GLuint, GLuint))
GL_FUNC(void, glClear, (GLbitfield))
GL_FUNC(void, glClearColor, (GLclampf, GLclampf, GLclampf, GLclampf))
GL_FUNC(void, glColor4f, (GLfloat, GLfloat, GLfloat, GLfloat))
GL_FUNC(void, glCompileShader, (GLuint))
GL_FUNC(GLuint, glCreateProgram, (void))
GL_FUNC(GLuint, glCreateShader, (GLenum))
GL_FUNC(void, glCullFace, (GLenum))
GL_FUNC(void, glDepthMask, (GLboolean))
GL_FUNC(void, glDetachShader, (GLuint, GLuint))
GL_FUNC(void, glDeleteProgram, (GLuint))
GL_FUNC(void, glDeleteShader, (GLuint))
GL_FUNC(void, glDisable, (GLenum))
GL_FUNC(void, glDrawElements, (GLenum, GLsizei, GLenum, const GLvoid*))
GL_FUNC(void, glEnable, (GLenum))
GL_FUNC(void, glEnableVertexAttribArray, (GLuint))
GL_FUNC(void, glFrontFace, (GLenum))
GL_FUNC(GLenum, glGetError, (void))
GL_FUNC(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params));
GL_FUNC(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei *, char *))
GL_FUNC(void, glGetShaderiv, (GLuint, GLenum, GLint *))
GL_FUNC(GLint, glGetUniformLocation, (GLuint, const char *))
GL_FUNC(void, glLinkProgram, (GLuint))
GL_FUNC(void, glShaderSource, (GLuint, GLsizei, const GLchar* const*, const GLint *))
GL_FUNC(void, glUniform3fv, (GLint, GLsizei, const GLfloat *))
GL_FUNC(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *))
GL_FUNC(void, glUseProgram, (GLuint))
GL_FUNC(void, glValidateProgram, (GLuint))
GL_FUNC(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *))
GL_FUNC(void, glViewport, (GLint, GLint, GLsizei, GLsizei))
GL_FUNC(GLint, glGetAttribLocation, (GLuint, const GLchar *))
GL_FUNC(void, glGenBuffers, (GLsizei, GLuint *))
GL_FUNC(void, glDeleteBuffers, (GLsizei, GLuint *))
GL_FUNC(void, glBindBuffer, (GLenum, GLuint))
GL_FUNC(void, glBufferData, (GLenum, GLsizeiptr, const GLvoid *, GLenum))
GL_FUNC(void, glDeleteVertexArrays, (GLsizei, const GLuint*))
GL_FUNC(void, glBindTexture, (GLenum, GLuint))
GL_FUNC(void, glGenTextures, (GLsizei, GLuint*))
GL_FUNC(void, glBindVertexArray, (GLuint))
GL_FUNC(void, glDeleteTextures, (GLsizei, const GLuint*))
GL_FUNC(void, glDrawArrays, (GLenum, GLint, GLsizei))
GL_FUNC(void, glGenVertexArrays, (GLsizei, GLuint*))
GL_FUNC(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*))
GL_FUNC(const GLubyte*, glGetString, (GLenum))
GL_FUNC(void, glTexImage2D, (GLenum target, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*))
GL_FUNC(void, glTexParameteri, (GLenum, GLenum, GLint));
