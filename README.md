# ml_shadercomp
Shader compiler using the binary driver for Mali-400 GPUs.
This program takes a vertex and a fragment shader, compiles and links them to a program and
writes the program binary in a file. The dumping of the program binary is made with the 
extensions [OES_get_program_binary](https://www.khronos.org/registry/gles/extensions/OES/OES_get_program_binary.txt) and [ARM_mali_program_binary](https://www.khronos.org/registry/gles/extensions/ARM/ARM_mali_program_binary.txt). 

The generated binaries can be loaded again by a compatible driver, which should support the same two extensions as mentioned above. With this the startup time of an application should reduce because the driver doesn't need to compile and link the shaders. 

The program binary format resembles RIFF and part of it is documented by the Lima project (http://limadriver.org/MBS+File+Format/).
