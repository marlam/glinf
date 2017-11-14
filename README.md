# glinfo

This is a simple tool that prints information about OpenGL or OpenGLES
contexts.

It is roughly comparable to `glewinfo` or `glxinfo`, with the following differences:
- it uses Qt for platform abstraction
- it prints implementation limits such as `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`
- it omits information about GLX and visuals

Currently only the few implementation limits that I am interested in are
printed, but more can be added easily.
