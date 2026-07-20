import re
import os
import sys

def extract_xf_cv_functions(code):
    pattern = r'xf::cv::(\w+)\s*<[^>]+>\s*\('
    return sorted(set(re.findall(pattern, code)))

def process_repo(repo_dir):
    cpp_files = []
    for root, _, files in os.walk(repo_dir):
        for file in files:
            if file.endswith('.cpp') and '_tb' not in file:
                cpp_files.append(os.path.join(root, file))
    
    # Sort by directory then filename
    cpp_files.sort(key=lambda x: (os.path.dirname(x), x))
    
    with open('file_functions.txt', 'w') as out_file:
        for cpp_file in cpp_files:
            try:
                with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
                    code = f.read()
                funcs = extract_xf_cv_functions(code)
                if funcs:
                    folder_path = os.path.dirname(cpp_file)
                    out_file.write(f"Folder: {folder_path}\nFile: {cpp_file}\nNumber of Functions: {len(funcs)}\nFunctions:\n")
                    for func in funcs:
                        out_file.write(f"  xf::cv::{func}\n")
                    out_file.write("\n")
            except Exception as e:
                print(f"Error processing {cpp_file}: {e}", file=sys.stderr)

    print("Generated file_functions.txt")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python process_files.py <repository_directory>")
        sys.exit(1)
    process_repo(sys.argv[1])

# python3 process_cv_files.py vision/L3/examples/
