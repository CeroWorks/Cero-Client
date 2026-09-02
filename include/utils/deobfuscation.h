#ifndef DEOBFUSCATION_H
#define DEOBFUSCATION_H

int version_needs_deobfuscation(const char* version);
int deobfuscate_client_jar(const char* client_dir, const char* version,
                            const char* input_jar, const char* output_jar,
                            const char* json_path, const char* java_exe,
                            const char* cero_jar_path);

#endif