/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VBO_H
#define VBO_H

#include "Rendering/GL/myGL.h"

/**
 * @brief VBO
 *
 * Vertex buffer Object class (ARB_vertex_buffer_object).
 */
class VBO
{
public:
	VBO(GLenum defTarget = GL_ARRAY_BUFFER, const bool storage = false);
	VBO(const VBO& other) = delete;
	VBO(VBO&& other) { *this = std::move(other); }
	virtual ~VBO();

	VBO& operator=(const VBO& other) = delete;
	VBO& operator=(VBO&& other);

	/**
	 * @param target can be either GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER or GL_UNIFORM_BUFFER_EXT
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBindBuffer.xml
	 */
	void Bind() const { Bind(defTarget); }
	void Bind(GLenum target) const;
	void Unbind() const;

	/**
	 * @param usage can be either GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, or GL_DYNAMIC_COPY
	 * @param data (optional) initialize the VBO with the data (the array must have minimum `size` length!)
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glBufferData.xml
	 */
	void Resize(GLsizeiptr newSize, GLenum newUsage = GL_STREAM_DRAW);
	void New(GLsizeiptr newSize, GLenum newUsage = GL_STREAM_DRAW, const void* newData = nullptr);
	void Invalidate(); //< discards all current data (frees the memory w/o resizing)

	/**
	 * @see http://www.opengl.org/sdk/docs/man/xhtml/glMapBufferRange.xml
	 */
	GLubyte* MapBuffer(GLbitfield access = GL_WRITE_ONLY);
	GLubyte* MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access = GL_WRITE_ONLY);
	void UnmapBuffer();

	GLuint GetId() const {
		if (vboId == 0)
			glGenBuffers(1, &vboId);
		return vboId;
	}

	size_t GetSize() const { return bufSize; }
	const GLvoid* GetPtr(GLintptr offset = 0) const;

public:
	mutable GLuint vboId;
	size_t bufSize; // can be smaller than memSize
	size_t memSize; // actual length of <data>; only set when !isSupported

	mutable GLenum curBoundTarget;
	GLenum defTarget;
	GLenum usage;

public:
	mutable bool bound;
	bool mapped;
	bool nullSizeMapped; // Nvidia workaround
	bool immutableStorage;
};

#endif /* VBO_H */
