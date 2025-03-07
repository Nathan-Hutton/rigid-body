#pragma once

#include <GL/glew.h>

class PickingTexture
{
    public:
        PickingTexture(GLsizei width, GLsizei height)
        {
            m_width = width;
            m_height = height;

            glGenFramebuffers(1, &m_frameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

            // Make texture
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // Make depth buffer
            glGenRenderbuffers(1, &m_depthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);

            // Bind the framebuffer together
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Something went wrong making the framebuffer\n");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        void bind() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameBuffer); }
        GLuint getTextureID() { return m_texture; };

        ~PickingTexture(){}

    private:
        GLuint m_frameBuffer, m_texture, m_depthBuffer;
        GLsizei m_width, m_height;
};
