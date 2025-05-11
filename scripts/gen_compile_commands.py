import os
Import("env") # platformio specific stuff

# include toolchain paths (i.e. to have stuff like the Arduino framework headers present in the compile commands)
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)


env.Replace(COMPILATIONDB_PATH=os.path.join("$PROJECT_DIR", "compile_commands.json"))
