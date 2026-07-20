import os
import csv
from collections import defaultdict

app_names_abb = {
    "all_in_one":           "allinone",
    "all_in_one_adas":      "allinone_ad",
    "colordetect":          "colordetect",
    "cornertracker":        "cornertrack",
    "defect_detection":     "defct_detct",
    "gaussiandifference":   "gaussidiff",
    "isp_24bit_decompand":  "isp_decomp",
    "isp_multistream":      "isp_mltstrm",
    "isppipeline":          "isp_pipe",
    "isppipeline_autogain": "isp_pipe_ag",
    "isppipeline_gtm":      "isp_pipe_gt",
    "isppipeline_hdr":      "isp_pipe_hd",
    "isppipeline_mono":     "isp_pipe_mn",
    "isppipeline_rgbir":    "isp_pipe_rg",
    "letterbox":            "letterbox",
    "stereopipeline":       "stereo_pipe",
    "tvl1":                 "tvl1",
}

def parse_function_file():
    folder_funcs = defaultdict(set)
    file_counts = defaultdict(int)
    folder_counts = defaultdict(int)
    
    with open('file_functions.txt', 'r') as f:
        current_folder = None
        for line in f:
            line = line.strip()
            if line.startswith('Folder: '):
                current_folder = line[8:]
            elif line.startswith('Number of Functions: '):
                if current_folder:
                    count = int(line.split(': ')[1])
                    folder_counts[current_folder] = count
            elif line.startswith('xf::cv::'):
                func = line.split('::')[-1]
                if current_folder:
                    folder_funcs[current_folder].add(func)
                file_counts[func] += 1
                
    return {k: sorted(v) for k, v in folder_funcs.items()}, file_counts, folder_counts

def calculate_overlap_matrices(folder_funcs):
    folders = sorted(folder_funcs.keys())
    n = len(folders)
    similarity_matrix = [[0.0]*n for _ in range(n)]
    overlap_matrix = [[0]*n for _ in range(n)]
    
    for i in range(n):
        set_i = set(folder_funcs[folders[i]])
        for j in range(n):  # Process all pairs for absolute counts
            set_j = set(folder_funcs[folders[j]])
            intersection = len(set_i & set_j)
            union = len(set_i | set_j) or 1
            
            # Store both similarity and absolute overlap
            similarity_matrix[i][j] = round(intersection/union, 4)
            overlap_matrix[i][j] = intersection
    
    return folders, similarity_matrix, overlap_matrix

def main():
    folder_funcs, file_counts, folder_counts = parse_function_file()
    
    # Generate top 10 functions
    top_functions = sorted(file_counts.items(), key=lambda x: (-x[1], x[0]))[:10]
    with open('top_10_functions.txt', 'w') as f:
        f.write("Top 10 Most Used Functions Across All Files:\n")
        for func, count in top_functions:
            f.write(f"{func}: {count}\n")
    
    # Generate matrices
    folders, similarity_matrix, overlap_matrix = calculate_overlap_matrices(folder_funcs)
    
    # Create display names with function counts
    display_names = []
    for folder in folders:
        basename = os.path.basename(folder)
        count = folder_counts.get(folder, 0)
        display_names.append(f"{app_names_abb[basename]} ({count})")
    
    # Write similarity matrix (Jaccard index)
    with open('similarity_matrix.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Folder'] + display_names)
        for i, folder in enumerate(folders):
            basename = os.path.basename(folder)
            count = folder_counts.get(folder, 0)
            writer.writerow([f"{app_names_abb[basename]} ({count})"] + similarity_matrix[i])
    
    # Write overlap matrix (absolute counts)
    with open('overlap_matrix.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Folder'] + display_names)
        for i, folder in enumerate(folders):
            basename = os.path.basename(folder)
            count = folder_counts.get(folder, 0)
            writer.writerow([f"{app_names_abb[basename]} ({count})"] + overlap_matrix[i])
    
    print("Generated two matrices:")
    print("- similarity_matrix.csv (Jaccard similarity)")
    print("- overlap_matrix.csv (absolute function overlap counts)")

if __name__ == "__main__":
    main()

# python3 analyze_functions.py
