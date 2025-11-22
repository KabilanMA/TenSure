import os
import sys

TEMPLATE_HEADER = """#pragma once
#include "backends/backend_interface.hpp"
#include "{module_name}_wrapper/generator.hpp"
#include "{module_name}_wrapper/executor.hpp"
#include "{module_name}_wrapper/comparator.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

struct {module_backend_name}Backend : public FuzzBackend {{
    bool generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) override;

    int execute_kernel(const fs::path& kernelPath, const fs::path& outputDir) override;

    bool compare_results(const string& refDir,
                         const string& testDir) override;
}};

// Plugin entry points
extern "C" FuzzBackend* create_backend();
extern "C" void destroy_backend(FuzzBackend* backend);
"""

TEMPLATE_SOURCE = """#include "{module_name}_wrapper/{module_name}_backend.hpp"

bool {module_backend_name}Backend::generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) {{
    cout << "GENERATE {module_backend_name} KERNELS" << endl;
    
    return true;
}}

int {module_backend_name}Backend::execute_kernel(const fs::path& kernelPath, const fs::path& outputDir) {{
    cout << "EXECUTE {module_backend_name} KERNEL" << endl;
    return 0;
}}

bool {module_backend_name}Backend::compare_results(const string& refDir, const string& testDir) {{
    cout << "COMPARE {module_backend_name} RESULTS" << endl;
    return true;
}}

// Plugin entry points
extern "C" FuzzBackend* create_backend() {{
    return new {module_backend_name}Backend();
}}

extern "C" void destroy_backend(FuzzBackend* backend) {{
    delete backend;
}}

"""

def append_to_cmake(module_name: str, cmake_path: str ="CMakeLists.txt"):
    module_name = module_name.lower()
    if not os.path.exists(cmake_path):
        print("CMakeLists.txt not found. Exiting.")
        sys.exit(1)
    '''
    # ------------------------------
    # Finch backend
    # ------------------------------
    if(BUILD_FINCH)
        file(GLOB_RECURSE FINCH_SRC
            ${CMAKE_SOURCE_DIR}/src/finch_wrapper/*.cpp
        )
        add_library(finch_wrapper SHARED ${FINCH_SRC})
        target_include_directories(finch_wrapper PUBLIC ${CMAKE_SOURCE_DIR}/include)
    endif()
    '''
    lines_to_add = f"""

# ==== Auto-generated for module {module_name.capitalize()} ====
option(BUILD_{module_name.upper()} "Build {module_name.capitalize()} backend" OFF)
if (BUILD_{module_name.upper()})
    file(GLOB_RECURSE {module_name.upper()}_SRC
        ${{CMAKE_SOURCE_DIR}}/src/{module_name.lower()}_wrapper/*.cpp
    )
    add_library({module_name.lower()}_wrapper SHARED ${{{module_name.upper()}_SRC}})
    target_include_directories({module_name}_wrapper PUBLIC ${{CMAKE_SOURCE_DIR}}/include)
endif()
"""

    with open(cmake_path, "a") as f:
        f.write(lines_to_add)

    print(f"Updated {cmake_path}")


def create_files(base_dir, module_name, file_names, extensions):
    module_path = os.path.join(base_dir, module_name)
    os.makedirs(module_path, exist_ok=True)

    generated_files = []
    for file_name in file_names:
        filename = f"{file_name}.{extensions}"
        full_path = os.path.join(module_path, filename)
        with open(full_path, "w") as f:
            f.write("")  # empty file
        generated_files.append(full_path)

    return generated_files


def write_one_header(include_directory, module_name: str):
    path = os.path.join("include", include_directory, f"{module_name.lower()}_backend.hpp")
    with open(path, "w") as f:
        f.write(TEMPLATE_HEADER.format(module_name=module_name.lower(), module_backend_name=module_name.capitalize()))


def write_one_source(src_directory, module_name: str):
    path = os.path.join("src", src_directory, f"{module_name.lower()}_backend.cpp")
    with open(path, "w") as f:
        f.write(TEMPLATE_SOURCE.format(module_name=module_name.lower(), module_backend_name=module_name.capitalize()))


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate_module.py <module_name>")
        sys.exit(1)
    

    module_name = sys.argv[1]

    support_files = ["comparator", "executor", "generator", f"{module_name.lower()}_backend"]

    # 1. Update CMakeLists.txt
    append_to_cmake(module_name)

    # 2. Create include/module_name directory + 4 .hpp
    hpp_files = create_files("include", f"{module_name.lower()}_wrapper", support_files, "hpp")

    # 3. Create src/module_name directory + 4 .cpp
    cpp_files = create_files("src", f"{module_name.lower()}_wrapper", support_files, "cpp")

    # 4. Write only one header and one source file
    write_one_header(f"{module_name.lower()}_wrapper", module_name)
    write_one_source(f"{module_name.lower()}_wrapper", module_name)

    print("Done. Created:")
    print(" - include files:", hpp_files)
    print(" - src files:", cpp_files)


if __name__ == "__main__":
    main()
